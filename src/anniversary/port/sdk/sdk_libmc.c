#include "common.h"

#include <libmc.h>

#include <stdbool.h>
#include <string.h>

typedef void (*OperationFinalizer)(int* result);

typedef struct GetInfoOperation {
    int* type;
    int* free;
    int* format;
} GetInfoOperation;

/*
 * A virtual PlayStation 2 memory card.
 *
 * The game asks the memory card for its type, its format and its free space
 * while it boots. When there is no card, the game stops and asks whether it
 * should be started without saving, which is a wall in the middle of the
 * boot. The card of the port therefore reports a formatted card with enough
 * free space, and with no save file on it: the saves of the game are not
 * persistent.
 */
#define CARD_SIZE (8 * 1024 * 1024)
#define CARD_BLOCK_SIZE (64 * 1024)
#define CARD_BLOCKS (CARD_SIZE / CARD_BLOCK_SIZE)

/* The free space of the card, in KB. */
#define CARD_FREE_KB 0x1F03

static GetInfoOperation get_info_operation = { 0 };
static int registered_operation = 0;

static unsigned char card_data[CARD_SIZE];
static unsigned char card_block_used[CARD_BLOCKS];
static int card_file_open = 0;
static int card_file_offset = 0;

static void finalize_success(int* result) {
    *result = sceMcResSucceed;
}

static void finalize_get_info(int* result) {
    *get_info_operation.type = sceMcTypePS2;
    *get_info_operation.free = CARD_FREE_KB;
    *get_info_operation.format = 1;
    *result = sceMcResSucceed;
}

static void finalize_get_dir(int* result) {
    // There is no save file on the card.
    *result = 0;
}

static OperationFinalizer finalizers[28] = {
    finalize_success, finalize_get_info,  finalize_success, finalize_success,  finalize_success,
    finalize_success, finalize_success,  finalize_success, finalize_success,  finalize_success,
    finalize_success, finalize_success,  finalize_success, finalize_get_dir,  finalize_success,
    finalize_success, finalize_success,  finalize_success, finalize_success,  finalize_success,
    finalize_success, finalize_success,  finalize_success, finalize_success,  finalize_success,
    finalize_success, finalize_success,  finalize_success};

int sceMcInit(void) {
    memset(card_data, 0, sizeof(card_data));
    memset(card_block_used, 0, sizeof(card_block_used));
    return sceMcIniSucceed;
}

int sceMcSync(int mode, int* cmd, int* result) {
    if ((registered_operation > 0) && (registered_operation < 28)) {
        *cmd = registered_operation;
        finalizers[registered_operation](result);
        registered_operation = 0;
        return sceMcExecFinish;
    } else {
        registered_operation = 0;
        return sceMcExecIdle;
    }
}

int sceMcGetInfo(int port, int slot, int* type, int* free, int* format) {
    registered_operation = sceMcFuncNoCardInfo;
    get_info_operation.type = type;
    get_info_operation.free = free;
    get_info_operation.format = format;
    return 0;
}

int sceMcOpen(int, int, const char*, int) {
    registered_operation = sceMcFuncNoOpen;
    card_file_open = 1;
    card_file_offset = 0;
    return 0;
}

int sceMcClose(int) {
    registered_operation = sceMcFuncNoClose;
    card_file_open = 0;
    card_file_offset = 0;
    return 0;
}

int sceMcRead(int, void* buf, int size) {
    registered_operation = sceMcFuncNoRead;

    if ((size < 0) || (card_file_offset + size > (int)sizeof(card_data))) {
        return -1;
    }

    memcpy(buf, &card_data[card_file_offset], size);
    card_file_offset += size;
    return size;
}

int sceMcWrite(int, const void* buf, int size) {
    registered_operation = sceMcFuncNoWrite;

    if ((size < 0) || (card_file_offset + size > (int)sizeof(card_data))) {
        return -1;
    }

    memcpy(&card_data[card_file_offset], buf, size);
    card_file_offset += size;
    return size;
}

int sceMcMkdir(int, int, const char*) {
    registered_operation = sceMcFuncNoMkdir;
    return 0;
}

int sceMcDelete(int, int, const char*) {
    registered_operation = sceMcFuncNoDelete;
    return 0;
}

int sceMcFormat(int, int) {
    registered_operation = sceMcFuncNoFormat;
    memset(card_data, 0, sizeof(card_data));
    memset(card_block_used, 0, sizeof(card_block_used));
    return 0;
}

int sceMcUnformat(int, int) {
    registered_operation = sceMcFuncNoUnformat;
    return 0;
}

int sceMcGetDir(int port, int slot, const char* name, unsigned int mode, int maxent, sceMcTblGetDir* table) {
    registered_operation = sceMcFuncNoGetDir;

    // The card holds no file, so the table stays empty.
    if (table != NULL && maxent > 0) {
        memset(table, 0, sizeof(*table));
    }

    return 0;
}

int sceMcGetEntSpace(int, int, const char*) {
    registered_operation = sceMcFuncNoEntSpace;
    return CARD_FREE_KB;
}
