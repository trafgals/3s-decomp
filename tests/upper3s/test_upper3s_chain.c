/*
 * Functional tests for the Third Strike Upper frame chain machinery.
 *
 * The frame data of the moves is not part of the repository: it is read from
 * the disc image at runtime.  The data of a move is a chain of blocks of
 * `cgd_type` u32 words that the engine walks once per game frame, and the
 * number of the game frames a frame is held for is the second byte of the
 * first word of the block (`cg_ctr`).
 *
 * The **startup** of a move is the sum of the holds of the frames before
 * the first frame that carries the attack data.  The design document changes
 * the startup of a few moves, and the changes are made in the data of the
 * disc: the holds of the frames of the startup are shortened.
 *
 * The per character move data is loaded from the disc image at runtime, so
 * the only way to test the override layer is to build a synthetic character
 * and check that the framework rewrites exactly the numbers it is supposed
 * to rewrite - and nothing else.
 *
 * The synthetic chains below are the chains of the disc, with the holds of
 * the frames of the startup of the real moves, so that the numbers of the
 * design document are the numbers of the test.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"
#include "sf33rd/Source/Game/upper3s_chain.h"

static int failures;
static int checks;

static void check(int condition, const char* what) {
    checks++;

    if (!condition) {
        failures++;
        printf("  FAIL  %s\n", what);
    } else {
        printf("  ok    %s\n", what);
    }
}

/*
 * The block of the synthetic character.
 *
 * The tables of the block of a character follow each other in memory, so
 * the size of a table is the distance to the next table.  The layout below
 * reproduces that: the data of a slot is followed by the next slot, and
 * the tables of the character come after the slots.
 *
 * Every move is given a room of MOVE_WORDS words, which is more than the
 * longest chain of the test needs, so that the chain of one move cannot
 * run into the chain of the next one.  The special moves of the disc go up
 * to the index 67, so the slot has to hold that many of them.  A chain of
 * thirty frames is 120 words long, so a move needs more than that.
 */
#define MOVE_WORDS 128
#define MOVE_COUNT 68
/*
 * The chains of the moves start after the table of the offsets of the slot,
 * so that the chain of a move cannot overwrite the offset of another one.
 */
#define MOVE_BASE MOVE_COUNT
#define SLOT_WORDS 9216
#define SLOT4_AT 0
#define SLOT5_AT (SLOT4_AT + SLOT_WORDS)
#define TABLES_AT (SLOT5_AT + SLOT_WORDS)
/*
 * The block has to hold the tables of the character as well.  The attack
 * table is the biggest of them: one entry of UNK_7 is 16 bytes, so the
 * MOVE_COUNT entries of the test need MOVE_COUNT * 4 words.  A block that
 * is too small overflows into whatever the linker puts next, and the test
 * then corrupts the data of another module without saying so.
 */
#define ATT_TABLE_AT (TABLES_AT + 16)
#define BLOCK_WORDS (ATT_TABLE_AT + MOVE_COUNT * 4 + 16)

/*
 * The block is aligned so that the table of the hitstun, which is 16 bytes
 * per entry, fits into it.
 */
static union {
    u32 words[BLOCK_WORDS];
    u8 alignment[16];
} block_storage;

#define BLOCK block_storage.words

/* The hold of every frame of the chain of every move of the slot. */
static u8 move_hold[U3S_KOC_COUNT][MOVE_COUNT][MOVE_WORDS];
/* The number of the frames of the chain of every move of the slot. */
static u8 move_frames[U3S_KOC_COUNT][MOVE_COUNT];
/* The number of the frames before the first frame that hits. */
static u8 move_active[U3S_KOC_COUNT][MOVE_COUNT];
/* The number of the frames that carry the attack data. */
static u8 move_hits[U3S_KOC_COUNT][MOVE_COUNT];
/*
 * The move whose chain a move shares, or 0 when it has a chain of its own.
 *
 * The disc puts some of the moves of a character in one chain: the light,
 * the medium and the heavy version of the Ryuubi Kyaku of Sean share one
 * chain, and so do the 6LP and the 4LP of Q.  A change of such a chain is
 * a change of every move that shares it, so the tests have to see that.
 */
static u16 move_share[U3S_KOC_COUNT][MOVE_COUNT];

/* Writes the chain of a move into the slot. */
static void build_move(u32* slot, s16 koc, s16 index, s16 active, s16 hits) {
    s32 base = MOVE_BASE + index * MOVE_WORDS;
    s32 n;

    if (move_share[koc][index] != 0) {
        /*
         * The move shares the chain of another one: the slot holds the same
         * offset for the two of them, and the chain itself is written once.
         */
        slot[index] = (u32)((MOVE_BASE + move_share[koc][index] * MOVE_WORDS) * 4);
        return;
    }

    slot[index] = (u32)(base * 4);
    slot[base - 2] = 4; /* the size of a block, in words */
    slot[base - 1] = 0;

    for (n = 0; n < move_frames[koc][index]; n++) {
        u32* frame = slot + base + n * 4;
        u32 att = ((n >= active) && (n < active + hits)) ? (u32)(index + 1) : 0;

        frame[0] = (u32)(move_hold[koc][index][n] << 8);
        frame[1] = 0;
        frame[2] = att << 22;
        frame[3] = 0;
    }

    /*
     * The chain of a move ends with the marker of the end of the
     * animation: a frame that is held for 250 frames.  The character state
     * machine decides when the move ends and the chain only waits there,
     * so the walk of the chain stops at the marker.
     */
    {
        u32* end = slot + base + move_frames[koc][index] * 4;

        end[0] = (u32)(U3S_CHAIN_WAIT_HOLD << 8);
        end[1] = 0;
        end[2] = 0;
        end[3] = 0;
    }
}

/* Fills the chain of a move with the holds of the frames of the disc. */
static void set_chain(s16 koc, s16 index, const s8* holds, s16 count, s16 active, s16 hits) {
    s16 i;

    for (i = 0; i < count; i++) {
        move_hold[koc][index][i] = (u8)holds[i];
    }

    move_frames[koc][index] = (u8)count;
    move_active[koc][index] = (u8)active;
    move_hits[koc][index] = (u8)hits;
}

/* Builds the synthetic character. */
static void setup_character(PLW* wk) {
    s16 i;

    memset(wk, 0, sizeof(PLW));
    memset(BLOCK, 0, sizeof(block_storage.words));

    /* The two slots that the test uses, and the table that follows each. */
    for (i = 0; i < MOVE_COUNT; i++) {
        build_move(BLOCK + SLOT4_AT, U3S_KOC_NORMAL, i, move_active[U3S_KOC_NORMAL][i],
                   move_hits[U3S_KOC_NORMAL][i]);
        build_move(BLOCK + SLOT5_AT, U3S_KOC_ATTACK, i, move_active[U3S_KOC_ATTACK][i],
                   move_hits[U3S_KOC_ATTACK][i]);
    }

    /* The tables of the character, which come after the slots. */
    wk->wu.body_adrs = (UNK_1*)(BLOCK + TABLES_AT + 0);
    wk->wu.hand_adrs = (UNK_2*)(BLOCK + TABLES_AT + 1);
    wk->wu.catch_adrs = (UNK_3*)(BLOCK + TABLES_AT + 2);
    wk->wu.caught_adrs = (UNK_4*)(BLOCK + TABLES_AT + 3);
    wk->wu.attack_adrs = (UNK_5*)(BLOCK + TABLES_AT + 4);
    wk->wu.hosei_adrs = (UNK_6*)(BLOCK + TABLES_AT + 5);
    wk->wu.hit_ix_table = (UNK_0*)(BLOCK + TABLES_AT + 6);
    wk->wu.step_xy_table = (s16*)(BLOCK + TABLES_AT + 7);
    wk->wu.move_xy_table = (s16*)(BLOCK + TABLES_AT + 8);
    wk->wu.se_random_table = BLOCK + TABLES_AT + 9;
    wk->wu.overlap_char_tbl = (UNK_8*)(BLOCK + TABLES_AT + 10);
    wk->wu.olc_ix_table = (UNK_9*)(BLOCK + TABLES_AT + 11);
    wk->wu.rival_catch_tbl = (CatchTable*)(BLOCK + TABLES_AT + 12);
    wk->wu.att_ix_table = (UNK_7*)(BLOCK + ATT_TABLE_AT);

    for (i = 0; i < MOVE_COUNT; i++) {
        wk->wu.att_ix_table[i].pow = (u8)(i + 1);
        wk->wu.att_ix_table[i].hs_you = (s8)(-(i + 1));
    }

    /* The slot of the special moves and the slot of the normal attacks. */
    wk->wu.char_table[U3S_KOC_ATTACK] = BLOCK + SLOT5_AT;
    wk->wu.char_table[U3S_KOC_NORMAL] = BLOCK + SLOT4_AT;

    /* The slots that the test does not use. */
    for (i = 0; i < U3S_KOC_COUNT; i++) {
        if ((i != U3S_KOC_NORMAL) && (i != U3S_KOC_ATTACK)) {
            wk->wu.char_table[i] = BLOCK + TABLES_AT + 200 + i;
        }
    }

    wk->player_number = U3S_SEAN;
    wk->wu.now_koc = U3S_KOC_NORMAL;
}

/* The startup of the move (koc, index) of the synthetic character. */
static s16 startup_of(PLW* wk, s16 koc, s16 index) {
    return upper3s_chain_startup(&wk->wu, koc, index);
}

/* The overrides of one character that the design document asks for. */
typedef struct {
    s16 char_id;
    s16 koc;
    s16 index;
    s16 startup;
    const char* what;
} StartupItem;

static const StartupItem items[] = {
    { U3S_SEAN, U3S_KOC_ATTACK, 32, 19, "Sean: the light Ryuubi Kyaku starts in 19 frames" },
    { U3S_SEAN, U3S_KOC_ATTACK, 33, 19, "Sean: the medium Ryuubi Kyaku starts in 19 frames" },
    { U3S_SEAN, U3S_KOC_ATTACK, 34, 19, "Sean: the heavy Ryuubi Kyaku starts in 19 frames" },
    { U3S_HUGO, U3S_KOC_ATTACK, 27, 17, "Hugo: the EX Giant Palm Bomber starts in 17 frames" },
    { U3S_HUGO, U3S_KOC_ATTACK, 64, 18, "Hugo: the light Meat Squasher starts in 18 frames" },
    { U3S_HUGO, U3S_KOC_ATTACK, 65, 24, "Hugo: the medium Meat Squasher starts in 24 frames" },
    { U3S_HUGO, U3S_KOC_ATTACK, 66, 30, "Hugo: the heavy Meat Squasher starts in 30 frames" },
    { U3S_SEAN, U3S_KOC_ATTACK, 64, 3, "Sean: the light Dragon Smash starts in 3 frames" },
    { U3S_SEAN, U3S_KOC_ATTACK, 65, 4, "Sean: the medium Dragon Smash starts in 4 frames" },
    { U3S_SEAN, U3S_KOC_ATTACK, 66, 5, "Sean: the heavy Dragon Smash starts in 5 frames" },
    { U3S_SEAN, U3S_KOC_ATTACK, 67, 5, "Sean: the EX Dragon Smash starts in 5 frames" },
    { U3S_Q, U3S_KOC_NORMAL, 1, 4, "Q: the dash standing Jab starts in 4 frames" },
    { U3S_Q, U3S_KOC_NORMAL, 2, 4, "Q: the back dash standing Jab starts in 4 frames" },
    { U3S_CHUNLI, U3S_KOC_NORMAL, 18, 3, "Chun-Li: the crouching Jab starts in 3 frames" },
};

/*
 * The chains of the disc, with the holds of the frames of the real moves.
 *
 * The chains of the characters that are not in the list are not built: the
 * move of a character is only looked up in the data of that character, so
 * the chains of two characters that use the same index do not collide.
 *
 *   Q: the dash standing Jab, three frames of two game frames and then
 *      the frame that hits.
 *   Sean: the Ryuubi Kyaku, the wind up of the spin.
 *   Sean: the Dragon Smash.
 *   Hugo: the EX Giant Palm Bomber.
 *   Hugo: the Meat Squasher.
 *   Chun-Li: the crouching Jab, the only two frame normal of the cast.
 */
static void setup_chains(s16 char_id) {
    static const s8 q_jab[] = { 2, 2, 2, 2, 2, 2, 2, 2 };
    static const s8 ryuubi[] = { 3, 3, 2, 2, 2, 2, 8, 3, 2, 2, 2, 2 };
    static const s8 ryuubi_ex[] = { 1, 1, 2, 1, 5, 12, 3, 2, 2, 2, 2 };
    static const s8 palm[] = { 2, 3, 5, 6, 1, 2, 3, 1, 1, 1, 1, 1, 6, 1, 1, 1, 1, 3, 3 };
    static const s8 chunli_jab[] = { 1, 1, 2 };

    /* The Dragon Smash of Sean: the entries 64 to 67 of the special moves. */
    static const s8 smash_light[] = { 3, 2, 3, 1, 2, 3, 4, 4, 5 };
    static const s8 smash_medium[] = { 3, 2, 2, 2, 1, 2, 3, 4, 5, 5 };
    static const s8 smash_heavy[] = { 1, 2, 2, 2, 2, 2, 1, 2, 4, 5, 5, 6 };
    static const s8 smash_ex[] = { 2, 2, 2, 1, 1, 1, 2, 4, 5, 5, 5, 6 };

    /* The Meat Squasher of Hugo: the entries 64 to 66 of the special moves. */
    static const s8 squasher_light[] = { 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 3 };
    static const s8 squasher_medium[] = { 5, 5, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4,
                                           2, 2, 3, 3, 3, 4, 4, 4, 4, 4, 3 };
    static const s8 squasher_heavy[] = { 6, 6, 5, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4,
                                         2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 3 };

    memset(move_hold, 0, sizeof(move_hold));
    memset(move_frames, 0, sizeof(move_frames));
    memset(move_active, 0, sizeof(move_active));
    memset(move_hits, 0, sizeof(move_hits));
    memset(move_share, 0, sizeof(move_share));

    switch (char_id) {
    case U3S_SEAN:
        /*
         * The three strengths of the Ryuubi Kyaku share one chain in the
         * data of the disc, so they share one chain here.  The EX version
         * has a chain of its own.
         */
        move_share[U3S_KOC_ATTACK][33] = 32;
        move_share[U3S_KOC_ATTACK][34] = 32;
        set_chain(U3S_KOC_ATTACK, 32, ryuubi, 12, 8, 3);
        set_chain(U3S_KOC_ATTACK, 35, ryuubi_ex, 11, 8, 3);
        set_chain(U3S_KOC_ATTACK, 64, smash_light, 9, 2, 2);
        set_chain(U3S_KOC_ATTACK, 65, smash_medium, 10, 3, 2);
        set_chain(U3S_KOC_ATTACK, 66, smash_heavy, 12, 5, 2);
        set_chain(U3S_KOC_ATTACK, 67, smash_ex, 12, 5, 3);
        break;

    case U3S_HUGO:
        set_chain(U3S_KOC_ATTACK, 27, palm, 19, 7, 5);
        set_chain(U3S_KOC_ATTACK, 64, squasher_light, 19, 9, 1);
        set_chain(U3S_KOC_ATTACK, 65, squasher_medium, 28, 18, 1);
        set_chain(U3S_KOC_ATTACK, 66, squasher_heavy, 30, 20, 1);
        break;

    case U3S_Q:
        /* The 6LP and the 4LP share one chain in the data of the disc. */
        move_share[U3S_KOC_NORMAL][2] = 1;
        set_chain(U3S_KOC_NORMAL, 1, q_jab, 8, 3, 2);
        set_chain(U3S_KOC_NORMAL, 3, q_jab, 8, 3, 2);
        break;

    case U3S_CHUNLI:
        set_chain(U3S_KOC_NORMAL, 18, chunli_jab, 3, 2, 1);
        break;
    }
}

int main(void) {
    PLW wk;
    Upper3sChain chain;
    Upper3sChainInfo info;
    size_t i;
    s16 value;

    setup_chains(U3S_SEAN);
    setup_character(&wk);

    /* The startup of a move is read out of the chain. */
    check(startup_of(&wk, U3S_KOC_ATTACK, 32) == 25,
          "Sean: the Ryuubi Kyaku starts in 25 frames before the patch");

    setup_chains(U3S_HUGO);
    setup_character(&wk);
    check(startup_of(&wk, U3S_KOC_ATTACK, 27) == 22,
          "Hugo: the EX Giant Palm Bomber starts in 22 frames before the patch");

    setup_chains(U3S_Q);
    setup_character(&wk);
    check(startup_of(&wk, U3S_KOC_NORMAL, 1) == 6,
          "Q: the dash standing Jab starts in 6 frames before the patch");

    setup_chains(U3S_CHUNLI);
    setup_character(&wk);
    check(startup_of(&wk, U3S_KOC_NORMAL, 18) == 2,
          "Chun-Li: the crouching Jab starts in 2 frames before the patch");

    /* The chain of a move that does not exist cannot be opened. */
    check(upper3s_chain_open(&wk.wu, U3S_KOC_NORMAL, MOVE_COUNT, &chain) == 0,
          "a move that the slot does not have has no chain");
    check(upper3s_chain_open(&wk.wu, U3S_KOC_NORMAL, SLOT_WORDS + 1, &chain) == 0,
          "a move outside of the slot has no chain");
    check(upper3s_chain_open(&wk.wu, U3S_KOC_COUNT, 0, &chain) == 0,
          "a slot that does not exist has no chain");

    /* The startup of a move is set to the number of the design document. */
    for (i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        setup_chains(items[i].char_id);
        setup_character(&wk);
        wk.player_number = items[i].char_id;
        wk.wu.char_index = items[i].index;
        wk.wu.now_koc = items[i].koc;
        upper3s_apply_character(&wk);

        check(startup_of(&wk, items[i].koc, items[i].index) == items[i].startup, items[i].what);
    }

    /* The patch is applied again when the other player of the character
     * loads it, and it does not shorten the move a second time. */
    setup_chains(U3S_SEAN);
    setup_character(&wk);
    wk.player_number = U3S_SEAN;
    upper3s_apply_character(&wk);
    check(startup_of(&wk, U3S_KOC_ATTACK, 32) == 19,
          "Sean: the light Ryuubi Kyaku starts in 19 frames");
    upper3s_apply_character(&wk);
    check(startup_of(&wk, U3S_KOC_ATTACK, 32) == 19,
          "Sean: the light Ryuubi Kyaku still starts in 19 frames after a second patch");

    /*
     * The three strengths of the Ryuubi Kyaku share one chain in the data
     * of the disc, so the change of the chain changes the three of them.
     */
    setup_chains(U3S_SEAN);
    setup_character(&wk);
    wk.player_number = U3S_SEAN;
    upper3s_apply_character(&wk);
    check(startup_of(&wk, U3S_KOC_ATTACK, 32) == startup_of(&wk, U3S_KOC_ATTACK, 33),
          "Sean: the light and the medium Ryuubi Kyaku share their chain");
    check(startup_of(&wk, U3S_KOC_ATTACK, 33) == startup_of(&wk, U3S_KOC_ATTACK, 34),
          "Sean: the medium and the heavy Ryuubi Kyaku share their chain");
    check(startup_of(&wk, U3S_KOC_ATTACK, 34) == 19,
          "Sean: the heavy Ryuubi Kyaku starts in 19 frames as well");

    /* A move that the design document does not touch keeps the startup of
     * the disc. */
    setup_chains(U3S_SEAN);
    setup_character(&wk);
    wk.player_number = U3S_SEAN;
    upper3s_apply_character(&wk);
    check(startup_of(&wk, U3S_KOC_ATTACK, 35) == 27,
          "Sean: the EX Ryuubi Kyaku keeps the startup of the disc");

    /* The frame that hits is not shortened: only the startup is. */
    setup_chains(U3S_SEAN);
    setup_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_ATTACK, 32, &chain) == 1,
          "the chain of the Ryuubi Kyaku opens");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read");
    check(info.startup == 25, "the startup of the Ryuubi Kyaku is 25 frames");
    check(upper3s_chain_set_startup(&chain, 25, 19) == 1, "the startup of the Ryuubi Kyaku is set");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read again");
    check(info.startup == 19, "the startup of the Ryuubi Kyaku is 19 frames");
    check(upper3s_chain_set_startup(&chain, 19, 19) == 1, "setting the same startup again works");
    check(info.startup == 19, "the startup of the Ryuubi Kyaku is still 19 frames");

    /*
     * A chain that already holds the target is left alone and the change is
     * reported as done: the patch is applied again every time the character
     * is loaded, and the moves that share a chain are changed by the entry
     * of the first one of them.
     */
    check(upper3s_chain_set_startup(&chain, 25, 19) == 1,
          "a chain that already holds the target is not changed again");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read again");
    check(info.startup == 19, "the startup of the Ryuubi Kyaku is still 19 frames");

    /*
     * A target that the chain does not hold and that is not the one of the
     * retail data is refused: two moves that share a chain cannot have two
     * different startups.
     */
    check(upper3s_chain_set_startup(&chain, 25, 17) == 0,
          "a second target for a chain that is already changed is refused");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read again");
    check(info.startup == 19, "the startup of the Ryuubi Kyaku is still 19 frames");

    /* The change is only made when the chain still has the startup of the
     * retail data. */
    setup_chains(U3S_SEAN);
    setup_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_ATTACK, 32, &chain) == 1,
          "the chain of the Ryuubi Kyaku opens for the check");
    check(upper3s_chain_set_startup(&chain, 24, 19) == 0,
          "the startup is not changed when the data is not the one that is expected");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read again");
    check(info.startup == 25, "the startup of the Ryuubi Kyaku is still 25 frames");

    /* A startup that the chain of the move cannot reach leaves the chain
     * alone. */
    setup_chains(U3S_SEAN);
    setup_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_ATTACK, 32, &chain) == 1,
          "the chain of the Ryuubi Kyaku opens again");
    check(upper3s_chain_set_startup(&chain, 25, 2) == 0,
          "the startup of the Ryuubi Kyaku cannot be brought to 2 frames");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read again");
    check(info.startup == 25, "the startup of the Ryuubi Kyaku is still 25 frames");

    /* A startup that is longer than the retail one is made longer. */
    setup_chains(U3S_CHUNLI);
    setup_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_NORMAL, 18, &chain) == 1,
          "the chain of the crouching Jab of Chun-Li opens");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the crouching Jab is read");
    check(info.startup == 2, "the crouching Jab of Chun-Li starts in 2 frames");
    check(upper3s_chain_set_startup(&chain, 2, 3) == 1, "the crouching Jab is made one frame slower");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the crouching Jab is read again");
    check(info.startup == 3, "the crouching Jab of Chun-Li starts in 3 frames");

    /* The recovery is the number of the frames after the last hit. */
    setup_chains(U3S_SEAN);
    setup_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_ATTACK, 32, &chain) == 1,
          "the chain of the Ryuubi Kyaku opens for the recovery");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read");
    check(info.recovery == 2, "the Ryuubi Kyaku has 2 frames after the hit");
    check(upper3s_chain_set_recovery(&chain, 2, 1) == 1, "the recovery of the Ryuubi Kyaku is set");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read again");
    check(info.recovery == 1, "the Ryuubi Kyaku has 1 frame after the hit");
    check(upper3s_chain_set_recovery(&chain, 1, 0) == 0,
          "the recovery of the Ryuubi Kyaku cannot be brought to 0 frames");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the Ryuubi Kyaku is read again");
    check(info.recovery == 1, "the Ryuubi Kyaku still has 1 frame after the hit");

    setup_chains(U3S_Q);
    setup_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_NORMAL, 3, &chain) == 1,
          "the chain of the dash Jab of Q opens for the recovery");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the dash Jab of Q is read");
    check(info.recovery == 6, "the dash Jab of Q has 6 frames after the hit");
    check(upper3s_chain_set_recovery(&chain, 6, 3) == 1, "the recovery of the dash Jab of Q is set");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the dash Jab of Q is read again");
    check(info.recovery == 3, "the dash Jab of Q has 3 frames after the hit");
    check(info.startup == 6, "the startup of the dash Jab of Q is untouched");

    /*
     * The 6LP and the 4LP of Q share one chain, so the patch has one entry
     * for each of them and the second one finds the chain that already
     * holds the startup of the target.  Neither of them is a dropped change.
     */
    {
        s32 before = upper3s_failure()->count;

        setup_chains(U3S_Q);
        setup_character(&wk);
        wk.player_number = U3S_Q;
        upper3s_apply_character(&wk);

        check(startup_of(&wk, U3S_KOC_NORMAL, 1) == 4, "Q: the dash standing Jab starts in 4 frames");
        check(startup_of(&wk, U3S_KOC_NORMAL, 2) == 4,
              "Q: the back dash standing Jab starts in 4 frames as well");
        check(upper3s_failure()->count == before,
              "Q: the change of the chain that two moves share is not a dropped change");
    }

    /*
     * The Meat Squasher of Hugo: the startup and the recovery of the move
     * are shortened, and the frames that hit are not touched.
     */
    setup_chains(U3S_HUGO);
    setup_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_ATTACK, 64, &chain) == 1,
          "the chain of the light Meat Squasher opens");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the light Meat Squasher is read");
    check(info.startup == 24, "the light Meat Squasher starts in 24 frames before the patch");
    check(info.recovery == 29, "the light Meat Squasher has 29 frames after the hit");
    wk.player_number = U3S_HUGO;
    upper3s_apply_character(&wk);
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the light Meat Squasher is read again");
    check(info.startup == 18, "the light Meat Squasher starts in 18 frames after the patch");
    check(info.recovery == 25, "the light Meat Squasher has 25 frames after the hit");

    /* The medium and the heavy Meat Squasher are shortened as well. */
    setup_chains(U3S_HUGO);
    setup_character(&wk);
    wk.player_number = U3S_HUGO;
    upper3s_apply_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_ATTACK, 65, &chain) == 1,
          "the chain of the medium Meat Squasher opens");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the medium Meat Squasher is read");
    check(info.startup == 24, "the medium Meat Squasher starts in 24 frames");
    check(info.recovery == 28, "the medium Meat Squasher has 28 frames after the hit");
    check(upper3s_chain_open(&wk.wu, U3S_KOC_ATTACK, 66, &chain) == 1,
          "the chain of the heavy Meat Squasher opens");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the heavy Meat Squasher is read");
    check(info.startup == 30, "the heavy Meat Squasher starts in 30 frames");
    check(info.recovery == 31, "the heavy Meat Squasher has 31 frames after the hit");

    /* The Dragon Smash of Sean: only the startup is shortened. */
    setup_chains(U3S_SEAN);
    setup_character(&wk);
    wk.player_number = U3S_SEAN;
    upper3s_apply_character(&wk);
    check(upper3s_chain_open(&wk.wu, U3S_KOC_ATTACK, 67, &chain) == 1,
          "the chain of the EX Dragon Smash opens");
    check(upper3s_chain_info(&chain, &info) == 1, "the chain of the EX Dragon Smash is read");
    check(info.startup == 5, "the EX Dragon Smash starts in 5 frames");
    check(info.recovery == 21, "the EX Dragon Smash keeps the recovery of the disc");

    /* A move whose chain holds no frame that hits has no startup. */
    {
        static const s8 empty[] = { 4, 4 };

        set_chain(U3S_KOC_NORMAL, 4, empty, 2, 9, 1);
        setup_character(&wk);
    }

    value = startup_of(&wk, U3S_KOC_NORMAL, 4);
    check(value == -1, "a move whose chain holds no hit has no startup");

    /*
     * A change that is dropped is counted, so that it is not silent: the
     * chain of the move does not hold the value of the retail data, so the
     * change is not made.
     */
    {
        Upper3sMove move;
        s32 before = upper3s_failure()->count;

        setup_chains(U3S_SEAN);
        setup_character(&wk);

        memset(&move, 0, sizeof(move));
        move.koc = U3S_KOC_ATTACK;
        move.index = 32;
        move.flags = U3S_MV_STARTUP;
        move.startup_from = 99; /* the chain of the move holds 25 */
        move.startup = 17;

        upper3s_apply_move(&wk.wu, &move);

        check(upper3s_failure()->count == before + 1, "a dropped change is counted");
        check(upper3s_failure()->koc == U3S_KOC_ATTACK, "the failure names the slot");
        check(upper3s_failure()->index == 32, "the failure names the move");
        check(upper3s_failure()->flags == U3S_MV_STARTUP, "the failure names the change");
    }

    /* A change that is made is not counted. */
    {
        Upper3sMove move;
        s32 before = upper3s_failure()->count;

        setup_chains(U3S_SEAN);
        setup_character(&wk);

        memset(&move, 0, sizeof(move));
        move.koc = U3S_KOC_ATTACK;
        move.index = 32;
        move.flags = U3S_MV_STARTUP;
        move.startup_from = 25;
        move.startup = 17;

        upper3s_apply_move(&wk.wu, &move);

        check(upper3s_failure()->count == before, "a change that is made is not counted");
        check(startup_of(&wk, U3S_KOC_ATTACK, 32) == 17, "the change was made");
    }

    /* A change whose chain cannot be opened is dropped as well. */
    {
        Upper3sMove move;
        s32 before = upper3s_failure()->count;

        setup_chains(U3S_SEAN);
        setup_character(&wk);

        memset(&move, 0, sizeof(move));
        move.koc = U3S_KOC_ATTACK;
        move.index = 200; /* there is no such move */
        move.flags = U3S_MV_STARTUP;
        move.startup_from = 25;
        move.startup = 17;

        upper3s_apply_move(&wk.wu, &move);

        check(upper3s_failure()->count == before + 1, "a change without a chain is counted");
        check(upper3s_failure()->index == 200, "the failure names the move");
    }

    /* The same change is only counted once, however many times it is applied. */
    {
        Upper3sMove move;
        s32 before = upper3s_failure()->count;

        setup_chains(U3S_SEAN);
        setup_character(&wk);

        memset(&move, 0, sizeof(move));
        move.koc = U3S_KOC_ATTACK;
        move.index = 201; /* there is no such move */
        move.flags = U3S_MV_STARTUP;
        move.startup_from = 25;
        move.startup = 17;

        upper3s_apply_move(&wk.wu, &move);
        check(upper3s_failure()->count == before + 1, "the first application is counted");

        upper3s_apply_move(&wk.wu, &move);
        check(upper3s_failure()->count == before + 1, "the change is only counted once");
    }

    printf("the frame chain machinery: %d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
