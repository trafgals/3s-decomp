#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"
#include "sf33rd/Source/Game/upper3s_chain.h"

/*
 * Third Strike Upper - the frame chain of the disc.
 *
 * See include/sf33rd/Source/Game/upper3s_chain.h for the description of
 * the chain and of the numbers that are read out of it.
 *
 * The data of a character is one block of memory and the tables of that
 * block follow each other, so the size of a table is the distance to the
 * next table of the block.  The 24 tables that the engine keeps in the
 * work structure are the tables of that block, which is what makes it
 * possible to find out where a slot of the character data ends.
 */

/*
 * The member of CharInitData that holds a slot of the character data
 * (CHARID.c:set_char_base_data).
 */
static const u8 upper3s_chain_slot_member[U3S_KOC_COUNT] = {
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
 * The tables of the block of the character, in the order of CharInitData.
 *
 * The 25th member of CharInitData (prot) is not in the work structure,
 * so the block is described by 24 of its 25 tables.  That is enough to
 * find the end of every table but the last one of the block, and the
 * slot that is the last one is not patched.
 */
static void upper3s_chain_block(WORK* wk, uintptr_t* addrs) {
    addrs[0] = (uintptr_t)wk->char_table[0];  /* nmca */
    addrs[1] = (uintptr_t)wk->char_table[1];  /* dmca */
    addrs[2] = (uintptr_t)wk->char_table[6];  /* btca */
    addrs[3] = (uintptr_t)wk->char_table[2];  /* caca */
    addrs[4] = (uintptr_t)wk->char_table[3];  /* cuca */
    addrs[5] = (uintptr_t)wk->char_table[4];  /* atca */
    addrs[6] = (uintptr_t)wk->char_table[5];  /* saca */
    addrs[7] = (uintptr_t)wk->char_table[7];  /* exca */
    addrs[8] = (uintptr_t)wk->char_table[8];  /* cbca */
    addrs[9] = (uintptr_t)wk->char_table[9];  /* yuca */
    addrs[10] = (uintptr_t)wk->step_xy_table;
    addrs[11] = (uintptr_t)wk->move_xy_table;
    addrs[12] = (uintptr_t)wk->se_random_table;
    addrs[13] = (uintptr_t)wk->overlap_char_tbl;
    addrs[14] = (uintptr_t)wk->olc_ix_table;
    addrs[15] = (uintptr_t)wk->rival_catch_tbl;
    addrs[16] = (uintptr_t)wk->hit_ix_table;
    addrs[17] = (uintptr_t)wk->body_adrs;
    addrs[18] = (uintptr_t)wk->hand_adrs;
    addrs[19] = (uintptr_t)wk->catch_adrs;
    addrs[20] = (uintptr_t)wk->caught_adrs;
    addrs[21] = (uintptr_t)wk->attack_adrs;
    addrs[22] = (uintptr_t)wk->hosei_adrs;
    addrs[23] = (uintptr_t)wk->att_ix_table;
}

/*
 * The number of the words of the slot `koc` of the character of `wk`.
 *
 * The tables of the block are not in the order of the struct, so they are
 * sorted before their sizes are read.
 */
static s32 upper3s_chain_slot_words(WORK* wk, s16 koc) {
    uintptr_t addrs[24];
    uintptr_t mine;
    s32 words = 0;
    s16 i;
    s16 j;

    if ((koc < 0) || (koc >= U3S_KOC_COUNT)) {
        return 0;
    }

    upper3s_chain_block(wk, addrs);
    mine = addrs[upper3s_chain_slot_member[koc]];

    if (mine == 0) {
        return 0;
    }

    for (i = 1; i < 24; i++) {
        uintptr_t key = addrs[i];

        for (j = i - 1; (j >= 0) && (addrs[j] > key); j--) {
            addrs[j + 1] = addrs[j];
        }

        addrs[j + 1] = key;
    }

    for (i = 0; i < 24; i++) {
        if (addrs[i] == mine) {
            if (i + 1 < 24) {
                words = (s32)((addrs[i + 1] - mine) / 4);
            }

            break;
        }
    }

    return words;
}

/*
 * Opens the chain of the move (koc, index) of the character of `wk`.
 */
int upper3s_chain_open(WORK* wk, s16 koc, s16 index, Upper3sChain* chain) {
    u32* slot;
    s32 count;
    s32 base;
    s32 cgd;
    u32 off;

    if ((wk == NULL) || (chain == NULL)) {
        return 0;
    }

    if ((koc < 0) || (koc >= U3S_KOC_COUNT) || (index < 0)) {
        return 0;
    }

    slot = wk->char_table[koc];

    if (slot == NULL) {
        return 0;
    }

    count = upper3s_chain_slot_words(wk, koc);

    if (count <= 2) {
        return 0;
    }

    /*
     * The index of the move is the index of its entry in the table of the
     * slot; the table holds the byte offset of the chain of every move.
     */
    if (index >= count) {
        return 0;
    }

    off = slot[index];

    if ((off == 0) || (off & 3)) {
        return 0;
    }

    base = off / 4;

    if ((base < 2) || (base >= count)) {
        return 0;
    }

    /* The size of a block is written in front of the chain. */
    cgd = slot[base - 2] & 0xFFFF;

    if ((cgd < 3) || (cgd > 32)) {
        return 0;
    }

    chain->words = slot;
    chain->count = count;
    chain->base = base;
    chain->cgd = cgd;

    return 1;
}

/*
 * The command codes of the chain that the walk of the machinery has to
 * follow (CHARSET.c:decode_chcmd).
 */
#define U3S_CMD_IXFW 49 /* comm_ixfw - jump forward */
#define U3S_CMD_IXBW 50 /* comm_ixbw - jump backward */

/*
 * The frames of the chain.
 *
 * A block whose first word is below 0x100 has a hold of zero: it is a
 * *command* block and not a frame.  The command blocks are skipped and
 * the frames are taken in the order of the chain, which is the order the
 * move plays them in as long as the chain does not call another move.
 *
 * The two commands that jump over the blocks of the chain are followed,
 * because they are part of the straight line of the chain:
 * `comm_ixfw` jumps over `pat` blocks and `comm_ixbw` jumps back over
 * `pat` of them (CHARSET.c:comm_ixfw() and comm_ixbw()).
 *
 * A command block that calls another move moves the animation into the
 * chain of that move, and the frames that follow it in this chain are not
 * the frames of the move.  The caller therefore checks the startup it
 * reads against the startup of the retail data before it changes
 * anything.
 *
 * `stopped` tells why the walk ended: 0 when the slot ended and 1 when
 * the walk ran out of room.
 */
static s32 upper3s_chain_frames(const Upper3sChain* chain, Upper3sFrame* frames, s32 max, s32* stopped) {
    s32 n;
    s32 found = 0;
    s32 steps = 0;

    for (n = 0; (n < max) && (found < max); n++) {
        s32 word = chain->base + n * chain->cgd;
        u32 w0;
        s32 att;
        s32 hold;

        if (++steps > max * 4) {
            /* the chain jumps back and forth: it is not walked */
            *stopped = 2;
            return found;
        }

        if (word + 2 >= chain->count) {
            *stopped = 0;
            return found;
        }

        w0 = chain->words[word];

        if ((w0 & 0xFFFF) < 0x100) {
            /* a command block: it is not a frame */
            s32 code = (s32)(w0 & 0xFF);
            s32 pat = (s16)(chain->words[word + 1] >> 16);

            if (code == U3S_CMD_IXFW) {
                n += pat - 1;
            } else if (code == U3S_CMD_IXBW) {
                n -= pat + 1;
            }

            if (n < 0) {
                /* the jump went behind the start of the chain */
                *stopped = 0;
                return found;
            }

            continue;
        }

        /*
         * The low byte of the first word is the type of the frame.  The
         * type 0xFF is the end of the animation: the game reads it in
         * appear.c and in the character state machines to know that the
         * chain is over.  It is a sentinel and not a frame of the move, so
         * the walk stops here and the marker is not counted.
         */
        if ((w0 & 0xFF) == U3S_CHAIN_END_CODE) {
            *stopped = 0;
            return found;
        }

        /* The attack index is the high half of the third word, shifted. */
        att = (s16)(chain->words[word + 2] >> 16) >> 6;

        if (att < 0) {
            att = -att;
        }

        hold = (s32)((w0 >> 8) & 0xFF);

        if (hold >= U3S_CHAIN_WAIT_HOLD) {
            /*
             * A frame that is held for 250 frames is the marker of the end
             * of the animation: the character state machine decides when
             * the move ends and the chain only waits there.  The marker is
             * a sentinel and not a frame of the move, so the walk stops
             * here and the marker is not counted.
             */
            *stopped = 0;
            return found;
        }

        frames[found].word = word;
        frames[found].hold = hold;
        frames[found].att = att;
        found++;
    }

    *stopped = 1;

    return found;
}

/*
 * Reads the startup and the recovery of the chain out of its data.
 */
int upper3s_chain_info(const Upper3sChain* chain, Upper3sChainInfo* info) {
    Upper3sFrame frames[U3S_CHAIN_MAX_FRAMES];
    s32 n;
    s32 i;
    s32 first;
    s32 last;
    s32 stopped;

    info->startup = -1;
    info->recovery = -1;

    n = upper3s_chain_frames(chain, frames, U3S_CHAIN_MAX_FRAMES, &stopped);

    first = -1;
    last = -1;

    for (i = 0; i < n; i++) {
        if (frames[i].att != 0) {
            if (first < 0) {
                first = i;
            }

            last = i;
        }
    }

    if (first < 0) {
        return 0;
    }

    info->startup = 0;

    for (i = 0; i < first; i++) {
        info->startup = (s16)(info->startup + frames[i].hold);
    }

    /*
     * The recovery is the number of the game frames that follow the last
     * frame that carries the attack data.  It is the recovery of the
     * straight line of the chain: the walk skips the command blocks and
     * follows the two commands that jump over the blocks, so a chain that
     * branches in its recovery has more frames than that.
     *
     * The number is only known when the walk reached the end of the chain.
     * A walk that was cut short does not hold the frames that follow the
     * last frame that hits, so the number it would give is not the
     * recovery of the move and it is left at -1.
     */
    if (stopped != 0) {
        return 1;
    }

    info->recovery = 0;

    for (i = last + 1; i < n; i++) {
        info->recovery = (s16)(info->recovery + frames[i].hold);
    }

    return 1;
}

/*
 * Shortens the frames of `from` to `until` so that they add up to
 * `frames` game frames.
 *
 * The longest frame is shortened first, so that the shape of the
 * animation is kept as well as possible, and no frame is shortened below
 * a single game frame: a block with a hold of zero is a command block and
 * not a frame.
 *
 * Returns 0 when the frames cannot be brought to `frames` game frames.
 */
static int upper3s_chain_shorten(Upper3sChain* chain, Upper3sFrame* frames, s32 from, s32 until, s16 target) {
    s32 hold[U3S_CHAIN_MAX_FRAMES];
    s32 total = 0;
    s32 i;

    for (i = from; i < until; i++) {
        hold[i] = frames[i].hold;
        total += hold[i];
    }

    if (total == target) {
        return 1;
    }

    if (total < target) {
        /*
         * The startup can also be made longer: the last frame of the
         * startup is the one that is held the longest before the hit.
         */
        if (until - from < 1) {
            return 0;
        }

        hold[until - 1] += target - total;

        if (hold[until - 1] > 0xFF) {
            return 0;
        }
    } else {
        for (;;) {
            s32 best = -1;

            for (i = from; i < until; i++) {
                if (hold[i] > 1) {
                    if ((best < 0) || (hold[i] > hold[best])) {
                        best = i;
                    }
                }
            }

            if (best < 0) {
                /*
                 * Every frame is already held for a single game frame, so
                 * the target is out of reach.  The chain is left alone.
                 */
                return 0;
            }

            hold[best]--;
            total--;

            if (total == target) {
                break;
            }
        }
    }

    /* The change is possible: the frames are written back. */
    for (i = from; i < until; i++) {
        if (hold[i] != frames[i].hold) {
            chain->words[frames[i].word] =
                (chain->words[frames[i].word] & 0xFFFF00FF) | ((u32)hold[i] << 8);
        }
    }

    return 1;
}

/*
 * Sets the startup of the move to `frames` game frames.
 *
 * The change is only made when the chain still has the startup `from`:
 * the walk of the machinery does not follow the commands that call
 * another move, so the startup it reads is the startup of the retail data
 * only when the chain is the one that is expected.  When it is not, the
 * chain is left alone.
 *
 * A chain that already has the startup `frames` is left alone as well and
 * the change is reported as done: the patch is applied again every time
 * the character is loaded, and several moves of the patch can share one
 * chain - the light, the medium and the heavy version of a move of the
 * disc, for example.
 */
int upper3s_chain_set_startup(Upper3sChain* chain, s16 from, s16 frames) {
    Upper3sFrame chain_frames[U3S_CHAIN_MAX_FRAMES];
    s32 n;
    s32 i;
    s32 first;
    s32 total;
    s32 stopped;

    if ((chain == NULL) || (frames < 0) || (from < 0)) {
        return 0;
    }

    n = upper3s_chain_frames(chain, chain_frames, U3S_CHAIN_MAX_FRAMES, &stopped);

    first = -1;

    for (i = 0; i < n; i++) {
        if (chain_frames[i].att != 0) {
            first = i;
            break;
        }
    }

    if (first < 0) {
        return 0;
    }

    total = 0;

    for (i = 0; i < first; i++) {
        total += chain_frames[i].hold;
    }

    if (total != from) {
        /*
         * The chain does not hold the startup of the retail data, so it is
         * not the chain that is expected.  It is left alone - unless it
         * already holds the target, which means that the change is already
         * there: the patch is applied again every time the character is
         * loaded, and several moves of the patch can share one chain.
         */
        return (total == frames);
    }

    return upper3s_chain_shorten(chain, chain_frames, 0, first, frames);
}

/*
 * Sets the recovery of the move to `frames` game frames.
 */
int upper3s_chain_set_recovery(Upper3sChain* chain, s16 from, s16 frames) {
    Upper3sFrame chain_frames[U3S_CHAIN_MAX_FRAMES];
    s32 n;
    s32 i;
    s32 last;
    s32 total;
    s32 stopped;

    if ((chain == NULL) || (frames < 0) || (from < 0)) {
        return 0;
    }

    n = upper3s_chain_frames(chain, chain_frames, U3S_CHAIN_MAX_FRAMES, &stopped);

    last = -1;

    for (i = 0; i < n; i++) {
        if (chain_frames[i].att != 0) {
            last = i;
        }
    }

    if (last < 0) {
        return 0;
    }

    total = 0;

    for (i = last + 1; i < n; i++) {
        total += chain_frames[i].hold;
    }

    /*
     * The frames that follow the last frame that hits are only the frames
     * of the move when the walk reached the end of the chain.
     */
    if (stopped != 0) {
        return 0;
    }

    if (total != from) {
        /*
         * The chain does not hold the recovery of the retail data, so it is
         * not the chain that is expected.  It is left alone - unless it
         * already holds the target, which means that the change is already
         * there: the patch is applied again every time the character is
         * loaded, and several moves of the patch can share one chain.
         */
        return (total == frames);
    }

    return upper3s_chain_shorten(chain, chain_frames, last + 1, n, frames);
}

/*
 * The startup of the move (koc, index) of the character `wk`.
 */
s16 upper3s_chain_startup(WORK* wk, s16 koc, s16 index) {
    Upper3sChain chain;
    Upper3sChainInfo info;

    if (!upper3s_chain_open(wk, koc, index, &chain)) {
        return -1;
    }

    if (!upper3s_chain_info(&chain, &info)) {
        return -1;
    }

    return info.startup;
}
