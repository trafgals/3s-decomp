// Web build disc access.
//
// The PS2 disc image is far too large to ship inside the wasm module (the
// SF33RD.AFS archive alone is ~612 MB), so the browser build streams it over
// HTTP instead.
//
// The CRI file system layer asks for 2-sector (4 KiB) reads at a time, which
// would be hopeless over the network if every read became its own request, so
// reads are served out of a cache of larger blocks that are pulled with HTTP
// range requests.

#include "common.h"
#include "port/port_trace.h"
#include "port/web/web_cd.h"
#include "port/web/web_trace.h"

#include <libcdvd.h>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AFS_LENGTH 642492416
#define AFS_START_LSN 0x100
#define AFS_SECTOR_COUNT ((AFS_LENGTH + 2047) / 2048)
#define AFS_END_LSN (AFS_START_LSN + AFS_SECTOR_COUNT)

#define SECTOR_SIZE 2048

// Cache geometry. 4 MiB blocks keep the request count low while still
// bounding how much of the disc image is held in memory.
#define WEB_CD_BLOCK_SIZE (4 * 1024 * 1024)
#define WEB_CD_CACHE_BLOCKS 16

static const char flist_path[] = "\\THIRD\\0FLIST.DIR;1";
static const char afs_path[] = "\\THIRD\\SF33RD.AFS;1";

typedef struct {
    u32 index; // block index, or WEB_CD_BLOCK_EMPTY when the slot is free
    u32 age;   // used for the least-recently-used eviction
    u8* data;
} WebCdBlock;

#define WEB_CD_BLOCK_EMPTY 0xFFFFFFFF

static WebCdBlock cache[WEB_CD_CACHE_BLOCKS];
static u32 clock_counter = 0;
static bool cache_ready = false;
static const char* afs_url = "rom/THIRD/SF33RD.AFS";
static int afs_url_is_dynamic = 0;
// The location of the disc image can be overridden with a query parameter.
// The address of the override has to outlive the function that reads it, so
// it is kept here and not on the stack of that function.
static char afs_url_override[512];

// Read a query parameter from the page URL. Emscripten 6 no longer exposes
// emscripten_get_query_parameter(), so this goes through JS directly.
//
// A pointer arrives in JS as a plain number, so the memory is reached
// through the heap view: "out[0] = 0" would assign a property to a number,
// which JavaScript silently ignores.
EM_JS(void, web_read_query_param, (const char* name, char* out, int out_len), {
    var params = new URLSearchParams(window.location.search);
    var value = params.get(UTF8ToString(name));

    if (value === null) {
        HEAPU8[out] = 0;
        return;
    }

    var bytes = new TextEncoder().encode(value);
    var n = Math.min(bytes.length, out_len - 1);

    for (var i = 0; i < n; i++) {
        HEAPU8[out + i] = bytes[i];
    }

    HEAPU8[out + n] = 0;
});

// Fetch a byte range of the disc image into the given buffer.
//
// The emscripten fetch library is deliberately not used here: it registers a
// run dependency on opening an IndexedDB, which can leave the module waiting
// forever before main() is ever called. A plain synchronous XHR keeps the
// disc reader self-contained.
//
// Returns the HTTP status, or a negative value on failure.
EM_JS(int, web_fetch_range, (const char* url, int offset, int length, unsigned char* out), {
    var xhr = new XMLHttpRequest();

    console.log('web_fetch_range: open ' + UTF8ToString(url) + ' ' + offset + '+' + length);

    try {
        xhr.open('GET', UTF8ToString(url), false);
        // A synchronous request may not use responseType, so the body is
        // pulled out as a binary string instead.
        xhr.overrideMimeType('text/plain; charset=x-user-defined');
        xhr.setRequestHeader('Range', 'bytes=' + offset + '-' + (offset + length - 1));
        xhr.send(null);
    } catch (e) {
        console.log('web_fetch_range: threw ' + e);
        return -1;
    }

    console.log('web_fetch_range: status ' + xhr.status);

    if (xhr.status !== 206 && xhr.status !== 200) {
        return -xhr.status;
    }

    var text = xhr.responseText;

    if (text.length < length) {
        return -1000 - text.length;
    }

    // The buffer is reached through the heap view: a pointer arrives in JS
    // as a plain number, and "out[i] = ..." would be ignored.
    for (var i = 0; i < length; i++) {
        HEAPU8[out + i] = text.charCodeAt(i) & 0xFF;
    }

    return xhr.status;
});

// Fetch a byte range of the disc image into a freshly allocated buffer.
static u8* fetch_range(u32 offset, u32 length) {
    u8* data;
    int status;

    PORT_TRACE("web: fetch_range: begin at %u, %u bytes", offset, length);

    data = (u8*)malloc(length);
    if (data == NULL) {
        fatal_error("web: out of memory while caching %s", afs_url);
    }

    // The request is the last thing that can block the page for a long
    // time, so it is reported with a beacon as well: a beacon does not wait
    // for the answer and is sent even when the stack is stuck.
    web_note("web: fetch_range: the request goes out");

    status = web_fetch_range(afs_url, (int)offset, (int)length, data);
    if (status < 0) {
        free(data);
        fatal_error("web: range request for %s failed (status %d)", afs_url, status);
    }

    PORT_TRACE("web: fetch_range: the request is done (%d)", status);

    return data;
}

// Return the cache block holding the given block index, loading it if needed.
static u8* get_block(u32 index) {
    u32 i;
    u32 oldest = 0;
    u32 oldest_age = 0xFFFFFFFF;
    u32 offset;

    // The game never calls sceCdInit, so the cache has to be set up when the
    // first read comes in.
    if (!cache_ready) {
        web_cd_init();
        cache_ready = true;
        PORT_TRACE("web: get_block: the cache is ready");
    }

    for (i = 0; i < WEB_CD_CACHE_BLOCKS; i++) {
        // A slot without a buffer is a free slot, even when it still holds
        // the index of a block: the game never calls sceCdInit, so the cache
        // starts out empty and the indices of its slots are all zero.
        if ((cache[i].data != NULL) && (cache[i].index == index)) {
            cache[i].age = ++clock_counter;
            return cache[i].data;
        }
    }

    for (i = 0; i < WEB_CD_CACHE_BLOCKS; i++) {
        if (cache[i].data == NULL) {
            oldest = i;
            oldest_age = 0;
            break;
        }

        if (cache[i].age < oldest_age) {
            oldest_age = cache[i].age;
            oldest = i;
        }
    }

    if (cache[oldest].data != NULL) {
        free(cache[oldest].data);
        cache[oldest].data = NULL;
    }

    offset = index * WEB_CD_BLOCK_SIZE;
    PORT_TRACE("web: get_block: loading block %u from %u", index, offset);
    cache[oldest].data = fetch_range(offset, WEB_CD_BLOCK_SIZE);
    cache[oldest].index = index;
    cache[oldest].age = ++clock_counter;

    return cache[oldest].data;
}

void web_cd_init(void) {
    u32 i;
    char* url;

    for (i = 0; i < WEB_CD_CACHE_BLOCKS; i++) {
        cache[i].index = WEB_CD_BLOCK_EMPTY;
        cache[i].age = 0;
        cache[i].data = NULL;
    }

    // Allow the disc image location to be overridden, e.g. ?afs=/assets/game.afs
    char query_value[512];
    web_read_query_param("afs", query_value, sizeof(query_value));
    if (query_value[0] != '\0') {
        size_t len = strlen(query_value);

        if (len >= sizeof(afs_url_override)) {
            len = sizeof(afs_url_override) - 1;
        }

        memcpy(afs_url_override, query_value, len);
        afs_url_override[len] = '\0';
        afs_url = afs_url_override;
        afs_url_is_dynamic = 1;
    }
}

void web_cd_shutdown(void) {
    u32 i;

    for (i = 0; i < WEB_CD_CACHE_BLOCKS; i++) {
        if (cache[i].data != NULL) {
            free(cache[i].data);
            cache[i].data = NULL;
        }

        cache[i].index = WEB_CD_BLOCK_EMPTY;
    }

    if (afs_url_is_dynamic) {
        afs_url = "rom/THIRD/SF33RD.AFS";
        afs_url_is_dynamic = 0;
    }
}

int sceCdBreak(void) {
    not_implemented(__func__);
    return 0;
}

int sceCdGetDiskType(void) {
    return SCECdPS2DVD;
}

int sceCdGetError(void) {
    return SCECdErNO;
}

int sceCdInit(int init_mode) {
    web_cd_init();
    return 1;
}

int sceCdMmode(int media) {
    not_implemented(__func__);
    return 0;
}

int sceCdRead(u_int lsn, u_int sectors, void* buf, sceCdRMode* mode) {
    u32 offset;
    u32 length;
    u32 done = 0;

    PORT_TRACE("web: sceCdRead at lsn %u, %u sectors", lsn, sectors);

    if (lsn == -1) {
        // The file list of the disc holds the name of the archive and
        // nothing else. The read has to fill the buffer, the file system of
        // the game reads the name out of it.
        static const char flist[] = "SF33RD.AFS\r\n";
        u32 count = sectors * SECTOR_SIZE;
        u32 size = sizeof(flist) - 1;

        memset(buf, 0, count);

        if (count > size) {
            count = size;
        }

        memcpy(buf, flist, count);
        return 1;
    }

    if ((lsn < AFS_START_LSN) || (lsn >= AFS_END_LSN)) {
        fatal_error("web: can't handle lsn %u", lsn);
        return 0;
    }

    offset = (lsn - AFS_START_LSN) * SECTOR_SIZE;
    length = sectors * SECTOR_SIZE;

    while (done < length) {
        u32 block_index = (offset + done) / WEB_CD_BLOCK_SIZE;
        u32 block_offset = (offset + done) % WEB_CD_BLOCK_SIZE;
        u32 chunk = WEB_CD_BLOCK_SIZE - block_offset;
        u8* block;

        if (chunk > length - done) {
            chunk = length - done;
        }

        block = get_block(block_index);

        memcpy((u8*)buf + done, block + block_offset, chunk);
        done += chunk;
    }

    return 1;
}

int sceCdSync(int mode) {
    return 0;
}

int sceCdLayerSearchFile(sceCdlFILE* fp, const char* name, int layer) {
    if (strncmp(name, flist_path, strlen(flist_path)) == 0) {
        fp->size = 12;
        fp->lsn = -1; // A wacky lsn on purpose, to recognise it in sceCdRead
    } else if (strncmp(name, afs_path, strlen(afs_path)) == 0) {
        fp->size = AFS_LENGTH;
        fp->lsn = AFS_START_LSN; // A wacky lsn on purpose, to recognise it in sceCdRead
    } else {
        fatal_error("web: can't handle filename %s", name);
    }

    return 1;
}
