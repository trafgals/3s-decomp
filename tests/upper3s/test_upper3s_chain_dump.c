/*
 * Reads the dump of the character data of the disc and prints the startup
 * of every move of every character, as the runtime machinery reads it.
 *
 * The dump is written by the disc dump tool
 * (src/anniversary/sf33rd/Source/Game/upper3s/upper3s_dump.c) and holds
 * the tables of the block of every character.  The layout of the block is
 * the one of the disc: the tables are at the offsets of the BLOCK line and
 * the size of a table is the distance to the next table.
 *
 * The tool exists to check the runtime machinery against the offline
 * decoder of the frame chain (tools/upper3s/decode_chain.py), which is the
 * reference: it walks the chain the way the retail game does, with the
 * branches.  The runtime machinery only walks the straight line of a chain,
 * so the two agree on every move whose chain has no command block before
 * its first hit.
 *
 * The tool also checks the changes of the frame data of the patch against
 * the data of the disc, with the setters of the runtime machinery itself:
 * a change whose chain is not there, a change whose chain does not hold
 * the value of the retail data and a change the chain cannot be brought
 * to are all changes of the patch that are not in the game.
 *
 * Usage: test_upper3s_chain_dump [dump]
 *
 * The dump is not part of the repository, so the tool says so and fails
 * when the file is not there: a run that checked nothing is not a run that
 * passed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"
#include "sf33rd/Source/Game/upper3s_chain.h"

/*
 * The member of CharInitData that holds a slot of the character data
 * (CHARID.c:set_char_base_data).
 */
static const u8 slot_member[U3S_KOC_COUNT] = {
    0, /* nmca - the reactions */
    1, /* dmca */
    3, /* caca */
    4, /* cuma */
    5, /* atca - the normal attacks */
    6, /* saca - the special moves and the super arts */
    2, /* btca */
    7, /* exca */
    8, /* cbca */
    9  /* yuca */
};

#define MAX_OFFSETS 25
#define MAX_SLOTS 10
#define MAX_WORDS 65536

static u32 block[MAX_WORDS];
static u32 slot_data[MAX_SLOTS][MAX_WORDS];
static s32 slot_words[MAX_SLOTS];
static s32 offset[MAX_OFFSETS];
static s16 att_count;

static const char* skip_spaces(const char* p) {
    while (*p == ' ') {
        p++;
    }

    return p;
}

/* Reads a decimal number. */
static s32 read_number(const char** p) {
    const char* s = skip_spaces(*p);
    s32 value = 0;

    while ((*s >= '0') && (*s <= '9')) {
        value = value * 10 + (*s - '0');
        s++;
    }

    *p = s;

    return value;
}

/* Reads a hexadecimal number. */
static u32 read_hex(const char** p, s32 digits) {
    const char* s = skip_spaces(*p);
    u32 value = 0;
    s32 i;

    for (i = 0; i < digits; i++) {
        u8 digit = (u8)s[i];
        u32 nibble;

        if ((digit >= '0') && (digit <= '9')) {
            nibble = digit - '0';
        } else if ((digit >= 'a') && (digit <= 'f')) {
            nibble = digit - 'a' + 10;
        } else if ((digit >= 'A') && (digit <= 'F')) {
            nibble = digit - 'A' + 10;
        } else {
            break;
        }

        value = (value << 4) | nibble;
    }

    *p = s + i;

    return value;
}

/* Finds the text in the line. */
static const char* find(const char* line, const char* text) {
    const char* found = strstr(line, text);

    if (found == NULL) {
        return NULL;
    }

    return found + strlen(text);
}

/* Puts the tables of the block in place and returns the work structure. */
static void build_character(WORK* wk) {
    s32 base = 0;
    s32 i;
    s32 total = 0;
    u32* member[25];

    for (i = 0; i < MAX_OFFSETS; i++) {
        if (offset[i] < base) {
            base = offset[i];
        }
    }

    /*
     * The offsets of the block are in bytes and the block of this tool is
     * an array of u32, so everything is counted in words from here on.
     */
    for (i = 0; i < MAX_OFFSETS; i++) {
        s32 size = 0;
        s32 j;

        for (j = 0; j < MAX_OFFSETS; j++) {
            if ((offset[j] > offset[i]) && ((size == 0) || (offset[j] - offset[i] < size))) {
                size = offset[j] - offset[i];
            }
        }

        if (size == 0) {
            size = 0x100;
        }

        if (((offset[i] - base) / 4) + (size / 4) > total) {
            total = ((offset[i] - base) / 4) + (size / 4);
        }
    }

    if (total > MAX_WORDS) {
        total = MAX_WORDS;
    }

    memset(block, 0, sizeof(u32) * (u32)total);
    memset(wk, 0, sizeof(WORK));

    for (i = 0; i < MAX_SLOTS; i++) {
        s32 at = (offset[slot_member[i]] - base) / 4;

        if ((at >= 0) && (at + slot_words[i] <= total)) {
            memcpy(block + at, slot_data[i], sizeof(u32) * (u32)slot_words[i]);
        }
    }

    for (i = 0; i < MAX_OFFSETS; i++) {
        member[i] = block + ((offset[i] - base) / 4);
    }

    wk->char_table[0] = member[0];
    wk->char_table[1] = member[1];
    wk->char_table[2] = member[3];
    wk->char_table[3] = member[4];
    wk->char_table[4] = member[5];
    wk->char_table[5] = member[6];
    wk->char_table[6] = member[2];
    wk->char_table[7] = member[7];
    wk->char_table[8] = member[8];
    wk->char_table[9] = member[9];
    wk->step_xy_table = (s16*)member[10];
    wk->move_xy_table = (s16*)member[11];
    wk->se_random_table = member[12];
    wk->overlap_char_tbl = (UNK_8*)member[13];
    wk->olc_ix_table = (UNK_9*)member[14];
    wk->rival_catch_tbl = (CatchTable*)member[15];
    wk->hit_ix_table = (UNK_0*)member[16];
    wk->body_adrs = (UNK_1*)member[17];
    wk->hand_adrs = (UNK_2*)member[18];
    wk->catch_adrs = (UNK_3*)member[19];
    wk->caught_adrs = (UNK_4*)member[20];
    wk->attack_adrs = (UNK_5*)member[21];
    wk->hosei_adrs = (UNK_6*)member[22];
    wk->att_ix_table = (UNK_7*)member[23];
}

/*
 * Checks the changes of the frame data of the patch against the data of
 * the disc, with the setters of the runtime machinery.  It returns the
 * number of the changes that are not in the game.
 */
static s32 check_frame_data(WORK* wk, const char* name, s16 char_index) {
    const Upper3sMove* moves;
    s32 failures = 0;
    s16 i;

    if ((char_index < 0) || (char_index >= U3S_CHAR_COUNT)) {
        return 0;
    }

    moves = upper3s_moves[char_index];

    if (moves == NULL) {
        return 0;
    }

    for (i = 0; moves[i].flags != 0; i++) {
        Upper3sChain chain;

        if (!(moves[i].flags & (U3S_MV_STARTUP | U3S_MV_RECOVERY))) {
            continue;
        }

        if (!upper3s_chain_open(wk, moves[i].koc, moves[i].index, &chain)) {
            printf("FAILED %s: the chain of the move %d of the slot %d is not there\n",
                   name, moves[i].index, moves[i].koc);
            failures++;
            continue;
        }

        if ((moves[i].flags & U3S_MV_STARTUP) &&
            !upper3s_chain_set_startup(&chain, moves[i].startup_from, moves[i].startup)) {
            printf("FAILED %s: the startup of the move %d of the slot %d is not brought to %d\n",
                   name, moves[i].index, moves[i].koc, moves[i].startup);
            failures++;
        }

        if ((moves[i].flags & U3S_MV_RECOVERY) &&
            !upper3s_chain_set_recovery(&chain, moves[i].recovery_from, moves[i].recovery)) {
            printf("FAILED %s: the recovery of the move %d of the slot %d is not brought to %d\n",
                   name, moves[i].index, moves[i].koc, moves[i].recovery);
            failures++;
        }
    }

    return failures;
}

int main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "upper3s_dump.txt";
    FILE* handle = fopen(path, "r");
    static char line[1 << 20];
    s16 char_index = -1;
    char name[64];
    WORK wk;
    s32 failures = 0;

    if (handle == NULL) {
        printf("the dump of the character data is not there: %s\n", path);
        return 1;
    }

    name[0] = '\0';

    while (fgets(line, sizeof(line), handle) != NULL) {
        if (strncmp(line, "CHAR ", 5) == 0) {
            const char* p = line + 5;
            char_index = (s16)read_number(&p);
            p = skip_spaces(p);
            sscanf(p, "%63s", name);
            memset(offset, 0, sizeof(offset));
            /*
             * The slot of a character that the dump does not hold is not
             * the slot of the character before it.
             */
            memset(slot_words, 0, sizeof(slot_words));
            att_count = 0;
        } else if (strncmp(line, "BLOCK ", 6) == 0) {
            const char* p = line + 6;
            s32 i;

            for (i = 0; i < MAX_OFFSETS; i++) {
                offset[i] = read_number(&p);
            }
        } else if (strncmp(line, "ATT ", 4) == 0) {
            const char* p = line + 4;
            const char* q;

            read_number(&p);
            q = skip_spaces(p);
            while ((*q != '\0') && (*q != ' ')) {
                q++;
            }

            att_count = (s16)read_number(&q);
        } else if (strncmp(line, "SLOT ", 5) == 0) {
            const char* q = find(line, "koc=");
            s32 koc = read_number(&q);
            const char* data;
            s32 size;
            s32 words;
            s32 i;

            q = find(line, "size=");
            size = read_number(&q);

            data = find(line, "data=");
            words = size / 4;

            if ((koc < 0) || (koc >= MAX_SLOTS) || (words > MAX_WORDS) || (data == NULL)) {
                continue;
            }

            for (i = 0; i < words; i++) {
                slot_data[koc][i] = read_hex(&data, 8);
            }

            slot_words[koc] = words;
        } else if (strncmp(line, "END ", 4) == 0) {
            s16 koc;

            build_character(&wk);

            if (getenv("UPPER3S_CHAIN_DEBUG")) {
                for (koc = 0; koc < U3S_KOC_COUNT; koc++) {
                    fprintf(stderr, "DEBUG %s koc=%d words=%d\n", name, koc, slot_words[koc]);
                }
            }

            for (koc = 0; koc < U3S_KOC_COUNT; koc++) {
                s32 index;

                for (index = 0; index < slot_words[koc]; index++) {
                    Upper3sChain chain;
                    Upper3sChainInfo info;

                    if (!upper3s_chain_open(&wk, koc, (s16)index, &chain)) {
                        continue;
                    }

                    if (!upper3s_chain_info(&chain, &info)) {
                        continue;
                    }

                    printf("STARTUP %s %d %d %d %d\n", name, koc, index, info.startup, info.recovery);
                }
            }

            /*
             * The changes of the frame data of the patch are checked with
             * the setters of the runtime machinery, against the data of the
             * disc: a change whose chain is not there, a change whose chain
             * does not hold the value of the retail data and a change the
             * chain cannot be brought to are changes of the patch that are
             * not in the game.
             *
             * The values above are printed first, because the check changes
             * the chain of the move.
             */
            failures += check_frame_data(&wk, name, char_index);
        }
    }

    fclose(handle);

    if (failures != 0) {
        printf("FAILED: %d changes of the patch are not in the data of the disc\n", failures);
        return 1;
    }

    return 0;
}
