#ifndef WEB_CD_H
#define WEB_CD_H

// Web build disc access.
//
// The retail disc image is far too large to ship inside the wasm module, so
// the browser build streams it over HTTP. The CRI file system layer asks for
// 2-sector (4 KiB) reads at a time, which would be hopeless over the network
// if every read were a separate request, so reads are served out of a cache
// of larger blocks that are pulled with HTTP range requests.

void web_cd_init(void);
void web_cd_shutdown(void);

#endif // WEB_CD_H
