/*
 * A dump of the character data of the disc.
 *
 * The frame data of the moves of Third Strike - the startup, active and
 * recovery frames, the hitboxes, the damage, the stun, the hitstun and the
 * hit advantage - is not part of this repository: it is read from the disc
 * image at runtime.  This file dumps that data, so that the balance
 * changes of the design document can be derived from the retail values
 * instead of being guessed.
 *
 * The character data is read from the disc on demand, so the dump follows
 * the loader: texgroup.c calls upper3s_dump_character() every time the data
 * of a character has been read, and the dump asks the loader for the next
 * character, until all of them have been dumped.
 *
 * The output is a text file with one line per move:
 *
 *   MOVE koc=<slot> idx=<index> att=<attack index> frames=<n>
 *        codes=<the command code of every frame>
 *        pow=<damage index> dmg=<damage> impact=<hitstun> piyo=<stun index>
 *        hs_me=<hit advantage of the attacker> hs_you=<hit advantage of
 *        the defender> dipsw=<parry flags> guard=<guard flags> dir=<dir>
 *
 * The file holds the attack table of the character as well:
 *
 *   ATT <character index> <name> <number of entries>
 *        <the entries, 16 bytes each, as hex>
 *
 * and the layout of the block of the character:
 *
 *   BLOCK <the offset of every table of the block, 25 numbers>
 */

/*
 * The index of the character in the order of the game (Gill 0, Alex 1, ...).
 * The number of the character in the order of My_char[].
 */
#include "common.h"
#include "sf33rd/Source/Game/CHARID.h"
#include "sf33rd/Source/Game/GD3rd.h"
#include "sf33rd/Source/Game/PLCNT.h"
#include "sf33rd/Source/Game/POW_DATA.h"
#include "sf33rd/Source/Game/upper3s.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(UPPER3S_DUMP)

static s32 upper3s_dump_resume(void);

/* The version of the layout of the dump file. */
#define UPPER3S_DUMP_VERSION "DUMP 7"

/* The names of the characters, in the order of the game. */
static const char* const upper3s_dump_names[U3S_CHAR_COUNT] = {
    "gill",  "alex",   "ryu",    "yun",    "dudley", "necro", "hugo",  "ibuki", "elena", "oro",
    "yang",  "ken",    "sean",   "urien",  "akuma",  "chunli", "makoto", "q",    "twelve", "remy"
};

static FILE* upper3s_dump_file;
static u8 upper3s_dump_done[U3S_CHAR_COUNT];

/*
 * The number of entries of the attack table of the character that is being
 * dumped.  The moves that point outside of the table are not written: the
 * scan of the frames of a move does not follow the jumps of the chain, so it
 * can read an index that is not an index at all.
 */
static s16 upper3s_dump_att_size;

static void upper3s_dump_do(s16 char_index, const CharInitData* cit);

/* The character that has been asked for and has not been dumped yet. */
static s16 upper3s_dump_pending = -1;

/* The character that will be asked for next. */
static s16 upper3s_dump_next = -1;

/* The character the loader was asked for before the dump took over. */
static s16 upper3s_dump_saved_plt_req = 0;

/* How many frames the dump waited for the character it asked for. */
static s16 upper3s_dump_pending_frames = 0;

/*
 * The number of entries of a slot of the character data is not stored
 * anywhere, but the entries are sorted by their offset, so the slot ends
 * when the offsets stop growing.
 *
 * That is not right: the move table of a slot holds the offset of every move
 * of the slot, and the moves that share their chain (the variants of a
 * normal) repeat their offset, so the list of the offsets stops growing long
 * before the end of the table.
 *
 * The table of the slot is the head of the data of the slot: the first word
 * of it is the offset of the data of the first move, and the data follows the
 * table and a trailer of three words.  The number of moves of the slot is
 * therefore the offset of the data divided by four, less the three words of
 * the trailer.
 */
static s16 upper3s_dump_slot_length(u32* table) {
    s32 length;

    if (table == NULL) {
        return 0;
    }

    length = (s32)(table[0] / 4) - 3;

    if (length < 0) {
        return 0;
    }

    if (length > 256) {
        return 256;
    }

    return (s16)length;
}

/*
 * The frame chain of a move is a list of blocks of `cgd_type` u32. Every
 * block starts with a command code; the chain ends when the command code is
 * out of range.
 *
 * The block holds the hit and the attack data of the frame:
 *
 *   [0] cg_type, cg_ctr
 *   [1] cg_olc_ix, cg_number
 *   [2] cg_hit_ix, cg_att_ix
 *
 * A frame that carries a hit box is a frame that hits.
 */
static s16 upper3s_dump_frames(u32* header, s16 cgd_type, s32 limit, s16* hit_ix, s16* att_ix) {
    s16 frames;

    *hit_ix = 0;
    *att_ix = -1;

    for (frames = 0; frames < 512; frames++) {
        u32* block = &header[frames * cgd_type];
        u16 code = (u16)(block[0] & 0xFFFF);
        s16 hit;
        s16 att;

        /* The chain of a move that does not end would walk out of the slot. */
        if (((s32)(block - header) + cgd_type) > limit) {
            break;
        }

        if (code >= 0x100) {
            break;
        }

        if (cgd_type < 3) {
            continue;
        }

        hit = (s16)(block[2] & 0xFFFF);
        att = (s16)(block[2] >> 16);

        if (att >= 0) {
            *att_ix = att;
        }

        if (hit != 0) {
            *hit_ix = hit;
        }
    }

    return frames;
}

/*
 * What the dump reads from the chain of a move.
 */
typedef struct {
    u32 offset;
    s16 cgd_type;
    s16 hit_ix;
    s16 att_ix;
    s16 frames;
} Upper3sDumpMove;

/*
 * Walks the chain of a move and reads what the dump needs from it.
 *
 * The scan of the blocks that follow each other is enough to find the index
 * of the attack data and the index of the hit box of a move.
 */
static s32 upper3s_dump_walk(s16 koc, s16 index, WORK* wk, s32 limit, Upper3sDumpMove* move) {
    u32* table = wk->char_table[koc];
    u32* header;
    u32 offset;

    move->offset = 0;
    move->cgd_type = 0;
    move->hit_ix = 0;
    move->att_ix = -1;
    move->frames = 0;

    if (table == NULL) {
        return 0;
    }

    offset = table[index];
    move->offset = offset;

    /* The offset of a move is the place of its chain in the slot.  The table
     * of the slot is followed by the data of the moves, and the entries that
     * are not offsets of a move are the data itself. */
    if ((offset == 0) || ((offset & 3) != 0)) {
        return 0;
    }

    if (((s32)(offset / 4) + 2) >= limit) {
        return 0;
    }

    header = &table[offset / 4];

    /* The two u32 in front of the frames hold the size of a frame and the
     * pattern of the move. */
    move->cgd_type = (s16)(header[-2] & 0xFFFF);

    if ((move->cgd_type < 2) || (move->cgd_type > 32)) {
        return 0;
    }

    move->frames = upper3s_dump_frames(header, move->cgd_type,
                                        limit - (s32)(offset / 4),
                                        &move->hit_ix, &move->att_ix);

    return 1;
}

static void upper3s_dump_move(FILE* out, s16 koc, s16 index, WORK* wk, s32 limit) {
    u32* table = wk->char_table[koc];
    Upper3sDumpMove move;
    u32* header;
    s16 frame;
    s16 pow = -1;
    s16 dmg = -1;
    s16 impact = -1;
    s16 piyo = -1;
    s16 hs_me = -1;
    s16 hs_you = -1;
    s16 dipsw = -1;
    s16 guard = -1;
    s16 dir = -1;

    if (!upper3s_dump_walk(koc, index, wk, limit, &move)) {
        return;
    }

    /* The attack data is optional: a move whose chain the scan cannot follow
     * can point at an index that is not an index at all. */
    if ((move.att_ix >= 0) && (move.att_ix < upper3s_dump_att_size) && (wk->att_ix_table != NULL)) {
        UNK_7* att = &wk->att_ix_table[move.att_ix];

        pow = att->pow;
        dmg = Power_Data[att->pow];
        impact = att->impact;
        piyo = att->piyo;
        hs_me = att->hs_me;
        hs_you = att->hs_you;
        dipsw = att->dipsw;
        guard = att->guard;
        dir = att->dir;
    }

    header = &table[move.offset / 4];

    fprintf(out, "MOVE koc=%d idx=%d att=%d hit=%d frames=%d pow=%d dmg=%d impact=%d piyo=%d"
                 " hs_me=%d hs_you=%d dipsw=%d guard=%d dir=%d cgd=%d off=%d raw=%08x,%08x,%08x,%08x codes=",
            koc, index, move.att_ix, move.hit_ix, move.frames, pow, dmg, impact, piyo,
            hs_me, hs_you, dipsw, guard, dir, move.cgd_type, move.offset,
            header[0], header[1], header[2], header[3]);

    for (frame = 0; frame < move.frames; frame++) {
        fprintf(out, "%s%d", (frame == 0) ? "" : ",", (u16)(header[frame * move.cgd_type] & 0xFFFF));
    }

    fprintf(out, "\n");
}

/*
 * Dumps the whole data of a slot.
 *
 * The frame chain of a move is a little program: the frames are walked with
 * jumps, so the data of the whole slot is needed to read it. The data is
 * written as a list of u32, in the order the engine addresses it.
 */
static void upper3s_dump_slot(FILE* out, s16 koc, WORK* wk, s32 words) {
    u32* table = wk->char_table[koc];
    s16 length;
    s32 index;

    if (table == NULL) {
        return;
    }

    length = upper3s_dump_slot_length(table);

    fprintf(out, "SLOT koc=%d len=%d size=%u data=", koc, length, (u32)(words * 4));

    for (index = 0; index < words; index++) {
        fprintf(out, "%08x", table[index]);
    }

    fprintf(out, "\n");
}
/*
 * The member of CharInitData that holds a slot of character data
 * (CHARID.c:set_char_base_data).
 */
static const u8 upper3s_dump_slot_member[U3S_KOC_COUNT] = {
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

/*
 * The size of a table of the block of a character, in words.
 *
 * The block of the character is one piece of memory and the 25 members of
 * CharInitData are the offsets of the tables of that block, so the size of a
 * table is the distance to the next table of the block.
 */
static s32 upper3s_dump_block_words(const CharInitData* cit, s16 member) {
    uintptr_t addrs[25];
    uintptr_t mine;
    s32 words = 0;
    s16 i;
    s16 j;

    if ((member < 0) || (member > 24)) {
        return 0;
    }

    mine = (uintptr_t)((uintptr_t*)cit)[member];

    if (mine == 0) {
        return 0;
    }

    for (i = 0; i < 25; i++) {
        addrs[i] = (uintptr_t)((uintptr_t*)cit)[i];
    }

    /* The tables are not in the order of the struct, so they are sorted. */
    for (i = 1; i < 25; i++) {
        uintptr_t key = addrs[i];

        for (j = i - 1; (j >= 0) && (addrs[j] > key); j--) {
            addrs[j + 1] = addrs[j];
        }

        addrs[j + 1] = key;
    }

    for (i = 0; i < 25; i++) {
        if (addrs[i] == mine) {
            if (i + 1 < 25) {
                words = (s32)((addrs[i + 1] - mine) / 4);
            }

            break;
        }
    }

    return words;
}

/*
 * The size of a slot of character data, in words.
 */
static s32 upper3s_dump_slot_words(const CharInitData* cit, s16 koc) {
    if ((koc < 0) || (koc >= U3S_KOC_COUNT)) {
        return 0;
    }

    return upper3s_dump_block_words(cit, upper3s_dump_slot_member[koc]);
}

/*
 * How many entries the attack table may have more than the distance to the
 * next table of the block.
 *
 * The tables of the block are not always packed one behind the other, so the
 * table of the attack data can be longer than the distance that tells its
 * size.  The margin covers that, and it keeps the dump from reading far
 * past the table when the scan of a move reads an index that is not an index
 * at all.
 */
#define UPPER3S_DUMP_ATT_MARGIN 16

/*
 * The number of entries of the attack table of a character.
 *
 * The attack table is the table `wk->att_ix_table` points at; it holds the
 * damage, the stun and the hitstun of every hit of the moves.  Its size is
 * the distance to the next table of the block of the character.
 *
 * The distance is counted in u32, so it has to be turned into bytes before
 * it is divided by the size of an entry.  The table can also be longer than
 * the distance to the next table - the tables of the block are not always
 * packed one behind the other - so the table has to hold every index that a
 * move of the character uses.
 */
static s16 upper3s_dump_att_count(const CharInitData* cit, s16 needed) {
    s32 words = upper3s_dump_block_words(cit, 23);
    s32 count = 0;
    s32 limit;

    if (words > 0) {
        count = (words * 4) / (s32)sizeof(UNK_7);
    }

    limit = count + UPPER3S_DUMP_ATT_MARGIN;

    if (count < needed) {
        count = needed;
    }

    if (count > limit) {
        count = limit;
    }

    /* The table is never that big; the cap is here so that a block whose
     * tables are not in one piece cannot make the dump read past the end. */
    if (count > 256) {
        count = 256;
    }

    return (s16)count;
}

/*
 * The biggest index of the attack data that the moves of the character use.
 *
 * The scan of the blocks of a move does not follow the jumps of the chain, so
 * it can read an index that is not an index at all.  An index that is bigger
 * than the table can hold is not an index of the table, so it is ignored.
 */
static s16 upper3s_dump_max_att(WORK* wk, const CharInitData* cit, s16 limit) {
    s16 max = -1;
    s16 koc;

    for (koc = 0; koc < U3S_KOC_COUNT; koc++) {
        s16 length = upper3s_dump_slot_length(wk->char_table[koc]);
        s32 words = upper3s_dump_slot_words(cit, koc);
        s16 index;

        for (index = 0; index < length; index++) {
            Upper3sDumpMove move;

            if (!upper3s_dump_walk(koc, index, wk, words, &move)) {
                continue;
            }

            if ((move.att_ix < 0) || (move.att_ix > limit)) {
                continue;
            }

            if (move.att_ix > max) {
                max = move.att_ix;
            }
        }
    }

    return max;
}

/*
 * The layout of the block of the character.
 *
 * The 25 members of CharInitData are the addresses of the tables of one block
 * of memory.  They are written as the offsets from the first table of the
 * block, so that the line is the same from one run to the next.
 */
static void upper3s_dump_block(FILE* out, const CharInitData* cit) {
    uintptr_t addrs[25];
    uintptr_t min;
    s16 i;

    min = (uintptr_t)-1;

    for (i = 0; i < 25; i++) {
        addrs[i] = (uintptr_t)((uintptr_t*)cit)[i];

        if ((addrs[i] != 0) && (addrs[i] < min)) {
            min = addrs[i];
        }
    }

    fprintf(out, "BLOCK");

    for (i = 0; i < 25; i++) {
        fprintf(out, " %d", (addrs[i] == 0) ? -1 : (s32)(addrs[i] - min));
    }

    fprintf(out, "\n");
}

/*
 * Dumps the attack table of a character.
 *
 * Every entry is 16 bytes: the reaction, the level, the index of the move
 * that hits, the index of the button, the pushback, the guard, the direction,
 * a free byte, the index of the damage, the impact, the index of the stun, the
 * type of the guard, the hitstun of the attacker and of the defender, and the
 * two marks of the hit.
 *
 * The entries are written as they are in memory: the two hitstun fields are
 * signed, so they are cast to make every entry the same width.
 */
static void upper3s_dump_att(FILE* out, s16 char_index, const CharInitData* cit) {
    const UNK_7* table = cit->atit;
    s16 count = upper3s_dump_att_size;
    s16 i;

    if ((table == NULL) || (count <= 0)) {
        return;
    }

    fprintf(out, "ATT %d %s %d\n", char_index, upper3s_dump_names[char_index], count);

    for (i = 0; i < count; i++) {
        const UNK_7* att = &table[i];

        fprintf(out, "%02x%02x%02x%02x%02x%02x%02x%02x"
                    "%02x%02x%02x%02x%02x%02x%02x%02x\n",
                (u8)att->reaction, (u8)att->level, (u8)att->mkh_ix, (u8)att->but_ix,
                (u8)att->dipsw, (u8)att->guard, (u8)att->dir, (u8)att->free,
                (u8)att->pow, (u8)att->impact, (u8)att->piyo, (u8)att->ng_type,
                (u8)att->hs_me, (u8)att->hs_you, (u8)att->hit_mark, (u8)att->dmg_mark);
    }
}

/*
 * Dumps the character that has just been read from the disc.
 *
 * `cit` is the data of the character, as it has just been read from the
 * disc; the loader has not copied it into its own table yet, so that the
 * dump does not depend on the state of the loader.
 */
void upper3s_dump_character(s16 char_index, const CharInitData* cit) {
    if ((char_index < 0) || (char_index >= U3S_CHAR_COUNT) || (cit == NULL)) {
        return;
    }

    upper3s_dump_do(char_index, cit);
}

/*
 * Dumps a character whose data the loader has read before: the data is then
 * already in the table of the loader.
 */
void upper3s_dump_character_loaded(s16 char_index) {
    if ((char_index < 0) || (char_index >= U3S_CHAR_COUNT)) {
        return;
    }

    upper3s_dump_do(char_index, &char_init_data[plid_data[char_index]]);
}

static void upper3s_dump_do(s16 char_index, const CharInitData* cit) {
    static WORK scratch;
    CharInitData saved;
    s16 koc;

    if (upper3s_dump_file == NULL) {
        upper3s_dump_file = fopen("upper3s_dump.txt", "w");

        if (upper3s_dump_file == NULL) {
            return;
        }
    }

    if (upper3s_dump_done[char_index]) {
        return;
    }

    upper3s_dump_done[char_index] = 1;

    /*
     * The dump asks the loader for a character with the player of the game,
     * which changes the character the loader thinks it is loading. The
     * character of the game is put back as soon as the dump is done with it.
     */
    if (plt_req[0] == char_index) {
        plt_req[0] = upper3s_dump_saved_plt_req;
    }

    /* set_char_base_data() reads the data of the character out of the table
     * of the loader, so the data of the disc is put in place for the time
     * it takes to fill the work structure. */
    saved = char_init_data[plid_data[char_index]];
    char_init_data[plid_data[char_index]] = *cit;

    scratch.charset_id = plid_data[char_index];
    set_char_base_data(&scratch);

    char_init_data[plid_data[char_index]] = saved;

    fprintf(upper3s_dump_file, "CHAR %d %s charset=%d\n", char_index,
            upper3s_dump_names[char_index], scratch.charset_id);

    /* The layout of the block of the character: the tables of the block are
     * addressed by the members of CharInitData, and their size is the
     * distance to the next one. */
    upper3s_dump_block(upper3s_dump_file, cit);

    /* The attack table holds the damage, the stun and the hitstun of the
     * hits; the moves of the character are addressed with its index.
     *
     * The table has to hold every index that a move of the character uses, so
     * the size of the table is only known once the moves have been walked. */
    upper3s_dump_att_size = upper3s_dump_att_count(cit, upper3s_dump_max_att(&scratch, cit, 255) + 1);
    upper3s_dump_att(upper3s_dump_file, char_index, cit);

    for (koc = 0; koc < U3S_KOC_COUNT; koc++) {
        s16 length = upper3s_dump_slot_length(scratch.char_table[koc]);
        s32 words = upper3s_dump_slot_words(cit, koc);
        s16 index;

        for (index = 0; index < length; index++) {
            upper3s_dump_move(upper3s_dump_file, koc, index, &scratch, words);
        }

        upper3s_dump_slot(upper3s_dump_file, koc, &scratch, words);
    }

    /*
     * The marker of the end of the character.  The run of the game can die
     * in the middle of a character, and the line that starts it is written
     * before its data, so only the characters that have this marker are
     * complete and only they are skipped by the next run.
     */
    fprintf(upper3s_dump_file, "END %d %s\n", char_index, upper3s_dump_names[char_index]);

    fflush(upper3s_dump_file);
}

/*
 * Starts the dump. Called once the disc has been mounted.
 */
void upper3s_dump_start(void) {
    if (upper3s_dump_file == NULL) {
        s32 resume = upper3s_dump_resume();

        if (resume == 1) {
            upper3s_dump_file = fopen("upper3s_dump.txt", "a");
        } else {
            upper3s_dump_file = fopen("upper3s_dump.txt", "w");
        }

        if (upper3s_dump_file == NULL) {
            return;
        }

        fprintf(upper3s_dump_file, "%s\n", UPPER3S_DUMP_VERSION);
        fflush(upper3s_dump_file);
    }

    if (upper3s_dump_next < 0) {
        upper3s_dump_next = 0;
    }
}

/*
 * Cuts the dump file back to the given length.
 *
 * The file is written again with the part that is kept, because the C
 * library cannot cut a file short.
 */
static void upper3s_dump_cut(long length) {
    FILE* file = fopen("upper3s_dump.txt", "rb");
    FILE* cut;
    long copied = 0;
    char buffer[4096];

    if (file == NULL) {
        return;
    }

    cut = fopen("upper3s_dump_cut.tmp", "wb");

    if (cut == NULL) {
        fclose(file);
        return;
    }

    while (copied < length) {
        size_t want = (size_t)(length - copied);
        size_t got;

        if (want > sizeof(buffer)) {
            want = sizeof(buffer);
        }

        got = fread(buffer, 1, want, file);

        if (got == 0) {
            break;
        }

        fwrite(buffer, 1, got, cut);
        copied += (long)got;
    }

    fclose(cut);
    fclose(file);

    remove("upper3s_dump.txt");

    if (rename("upper3s_dump_cut.tmp", "upper3s_dump.txt") != 0) {
        fprintf(stderr, "dump: the file could not be cut back to %ld\n", length);
    }
}

/*
 * Picks up where the last run of the game stopped.
 *
 * The data of a character takes a few megabytes of the memory of the game,
 * and the memory of the game cannot hold all twenty characters at once, so
 * the dump is done over several runs of the game. The characters that have
 * been dumped are read back from the file, and the dump continues with the
 * first character that is missing.
 *
 * Returns 1 when the dump continues, 0 when there is nothing to continue.
 */
static s32 upper3s_dump_resume(void) {
    FILE* file = fopen("upper3s_dump.txt", "r");
    char line[256];
    long last_end = -1;
    s16 index;

    if (file == NULL) {
        return 0;
    }

    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return 0;
    }

    /* A dump of an older version is thrown away: its layout is different. */
    if (strncmp(line, UPPER3S_DUMP_VERSION, strlen(UPPER3S_DUMP_VERSION)) != 0) {
        fclose(file);
        fprintf(stderr, "dump: the file is of an older version, starting over\n");
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "END ", 4) == 0) {
            index = atoi(line + 4);

            if ((index >= 0) && (index < U3S_CHAR_COUNT)) {
                upper3s_dump_done[index] = 1;
            }

            last_end = ftell(file);
        }
    }

    fclose(file);

    /*
     * The data of the character that the last run did not finish is thrown
     * away: the file is cut back to the end of the last character that is
     * complete, so that the file holds every character exactly once.
     */
    if (last_end >= 0) {
        upper3s_dump_cut(last_end);
    }

    for (index = 0; index < U3S_CHAR_COUNT; index++) {
        if (!upper3s_dump_done[index]) {
            break;
        }
    }

    fprintf(stderr, "dump: continuing with character %d\n", index);
    upper3s_dump_next = index;
    return 1;
}

/*
 * Asks the loader for the next character. Called every frame: the loader
 * only reads one request per frame, and the request queue of the game can
 * be full, so the request is retried until it goes through.
 */
void upper3s_dump_pump(void) {
    if (upper3s_dump_file == NULL) {
        return;
    }

    if (upper3s_dump_next < 0) {
        upper3s_dump_next = 0;
    }

    /* The character that has been asked for has been dumped. */
    if ((upper3s_dump_pending >= 0) && upper3s_dump_done[upper3s_dump_pending]) {
        upper3s_dump_pending = -1;
        upper3s_dump_pending_frames = 0;
    }

    if (upper3s_dump_pending >= 0) {
        upper3s_dump_pending_frames++;

        /*
         * The game empties the request queue of the loader when it changes
         * the screen, which drops the request of the dump. The request is
         * therefore made again when the character has not turned up for a
         * while.
         */
        if (upper3s_dump_pending_frames > 180) {
            upper3s_dump_pending_frames = 0;

            if (ldreq_queue_space() >= 6) {
                upper3s_dump_saved_plt_req = plt_req[0];
                Push_LDREQ_Queue_Char(upper3s_dump_pending, 1);
                fprintf(stderr, "dump: asked for character %d again\n", upper3s_dump_pending);
            }
        }

        return;
    }

    if (upper3s_dump_next >= U3S_CHAR_COUNT) {
        return;
    }

    /* The loader needs a few slots in its request queue for one character. */
    if (ldreq_queue_space() < 6) {
        return;
    }

    upper3s_dump_pending = upper3s_dump_next++;

    /*
     * The request has to use the same player as the one that has read the
     * texture group before: the loader drops a request of the second player
     * for a texture group that the first player has already read.
     */
    upper3s_dump_saved_plt_req = plt_req[0];
    Push_LDREQ_Queue_Char(upper3s_dump_pending, 1);
    fprintf(stderr, "dump: asked for character %d\n", upper3s_dump_pending);
}

#endif
