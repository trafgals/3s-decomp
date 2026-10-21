/*
 * Functional tests for the Third Strike Upper hitstun items.
 *
 * The design document asks for a number of normals of five characters to
 * have more hitstun, so that the hit of the move is not negative on hit.
 *
 * The hitstun of a move is the hold of the defender: the field `hs_you` of
 * the attack data (`UNK_7.hs_you`).  `HITCHECK.c:dm_status_copy()` puts the
 * value into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into
 * `hit_stop`, and `PLMAIN.c:check_hit_stop()` holds the defender for it, so
 * the more frames it holds, the more the attacker is punished for the hit.
 *
 * The values are not guessed: `tools/upper3s/hit_advantage.py` computes the
 * advantage of the hit of every normal of every character out of the data of
 * the disc, and the values below are the ones that make the advantage of the
 * hit zero.  The model reproduces the number of the design document for the
 * standing Roundhouse of q (-10 on hit).
 *
 * The per character move data is loaded from the disc image at runtime, so
 * the only way to test the override layer is to build a synthetic character
 * and check that the framework rewrites exactly the numbers it is supposed to
 * rewrite - and nothing else.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

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

/* Every move of the synthetic character has a chain of its own, so that the
 * framework can tell the moves apart. */
#define MOVE_COUNT 40
#define HEADER_WORDS 4
#define BLOCK_POS 64
#define ATT_COUNT MOVE_COUNT

static u32 char_table[U3S_KOC_COUNT][BLOCK_POS + MOVE_COUNT * HEADER_WORDS];
static UNK_7 att_ix_table[ATT_COUNT];

/* The hitstun of the retail game, so that the test can tell a rewritten
 * value from one that was left alone. */
static s8 retail_hs_you[ATT_COUNT];

static void setup_character(PLW* wk, s16 char_id) {
    int i;

    memset(wk, 0, sizeof(PLW));
    memset(char_table, 0, sizeof(char_table));
    memset(att_ix_table, 0, sizeof(att_ix_table));

    for (i = 0; i < ATT_COUNT; i++) {
        att_ix_table[i].pow = (u8)(i + 1);
        att_ix_table[i].impact = (u8)(10 + i);
        att_ix_table[i].piyo = (u8)i;
        att_ix_table[i].hs_me = (s8)(i + 1);
        att_ix_table[i].hs_you = (s8)(-(i + 1));
        retail_hs_you[i] = att_ix_table[i].hs_you;
    }

    /* The chain of the move `i` starts at the header `i`, and the header
     * carries the index of the attack data of the move. */
    for (i = 0; i < MOVE_COUNT; i++) {
        u32 word = BLOCK_POS + i * HEADER_WORDS;

        char_table[U3S_KOC_NORMAL][i] = word * 4;
        char_table[U3S_KOC_NORMAL][word + 2] = (u32)((i & 0xFFFF) << 16);
        char_table[U3S_KOC_ATTACK][i] = word * 4;
        char_table[U3S_KOC_ATTACK][word + 2] = (u32)((i & 0xFFFF) << 16);
    }

    for (i = 0; i < U3S_KOC_COUNT; i++) {
        wk->wu.char_table[i] = char_table[i];
    }

    wk->wu.att_ix_table = att_ix_table;
    wk->player_number = char_id;
    wk->wu.now_koc = U3S_KOC_NORMAL;
}

/* Applies the overrides of a character and returns the hitstun that the
 * framework wrote into the attack data of the move. */
static s16 applied_hs_you(s16 char_id, s16 koc, s16 index) {
    PLW wk;

    setup_character(&wk, char_id);
    wk.wu.now_koc = koc;
    wk.wu.char_index = index;
    upper3s_apply_character(&wk);

    return (s16)att_ix_table[index].hs_you;
}

/* The overrides of one character that the design document asks for. */
typedef struct {
    s16 char_id;
    s16 koc;
    s16 index;
    s16 hs_you;
    const char* what;
} HitstunItem;

static const HitstunItem items[] = {
    { U3S_RYU, U3S_KOC_NORMAL, 6, -12, "Ryu: the close Fierce holds the defender 12 frames" },
    { U3S_RYU, U3S_KOC_NORMAL, 24, -12, "Ryu: the crouching Fierce holds the defender 12 frames" },
    { U3S_RYU, U3S_KOC_NORMAL, 30, -11, "Ryu: the crouching Forward holds the defender 11 frames" },
    { U3S_HUGO, U3S_KOC_NORMAL, 18, -11, "Hugo: the crouching Jab holds the defender 11 frames" },
    { U3S_HUGO, U3S_KOC_NORMAL, 27, -13, "Hugo: the crouching Short holds the defender 13 frames" },
    { U3S_SEAN, U3S_KOC_NORMAL, 7, -24, "Sean: the close Fierce holds the defender 24 frames" },
    { U3S_SEAN, U3S_KOC_NORMAL, 8, -26, "Sean: the far Fierce holds the defender 26 frames" },
    { U3S_SEAN, U3S_KOC_NORMAL, 24, -10, "Sean: the crouching Fierce holds the defender 10 frames" },
    { U3S_SEAN, U3S_KOC_NORMAL, 30, -12, "Sean: the crouching Forward holds the defender 12 frames" },
    { U3S_Q, U3S_KOC_NORMAL, 27, 10, "Q: the crouching Short holds the defender 10 frames" },
    { U3S_Q, U3S_KOC_NORMAL, 15, -27, "Q: the standing Roundhouse holds the defender 27 frames" },
    { U3S_TWELVE, U3S_KOC_NORMAL, 3, -10, "Twelve: the standing Strong holds the defender 10 frames" },
    { U3S_TWELVE, U3S_KOC_NORMAL, 24, -13, "Twelve: the crouching Fierce holds the defender 13 frames" },
    { U3S_TWELVE, U3S_KOC_NORMAL, 12, -11, "Twelve: the standing Forward holds the defender 11 frames" },
    { U3S_TWELVE, U3S_KOC_NORMAL, 15, -16, "Twelve: the standing Roundhouse holds the defender 16 frames" },
    { U3S_TWELVE, U3S_KOC_NORMAL, 30, -10, "Twelve: the crouching Forward holds the defender 10 frames" },
    { U3S_TWELVE, U3S_KOC_NORMAL, 14, -15, "Twelve: the Back+Forward holds the defender 15 frames" },
    { U3S_REMY, U3S_KOC_NORMAL, 14, -24, "Remy: the far Forward holds the defender 24 frames" },
    { U3S_REMY, U3S_KOC_NORMAL, 17, -28, "Remy: the far Roundhouse holds the defender 28 frames" },
    { U3S_REMY, U3S_KOC_NORMAL, 30, -33, "Remy: the crouching Forward holds the defender 33 frames" },
    { U3S_REMY, U3S_KOC_NORMAL, 13, -30, "Remy: the Towards+Forward holds the defender 30 frames" },
    { U3S_CHUNLI, U3S_KOC_ATTACK, 28, -17, "Chun-Li: the light Cold Blue Kick holds the defender 17 frames" },
    { U3S_CHUNLI, U3S_KOC_ATTACK, 29, -15, "Chun-Li: the medium Cold Blue Kick holds the defender 15 frames" },
};

int main(void) {
    s16 value;
    size_t i;

    printf("the hitstun items of Third Strike Upper\n");

    for (i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        value = applied_hs_you(items[i].char_id, items[i].koc, items[i].index);
        check(value == items[i].hs_you, items[i].what);
    }

    /* The crouching Strong of Remy does six points more damage. */
    {
        PLW wk;

        setup_character(&wk, U3S_REMY);
        wk.wu.char_index = 21;
        upper3s_apply_character(&wk);

        check(upper3s_damage_permille(U3S_REMY, &wk.wu) == 1110,
              "Remy: the crouching Strong does six points more damage");
    }

    /* A move that the design document does not touch keeps the hitstun of the
     * disc. */
    value = applied_hs_you(U3S_RYU, U3S_KOC_NORMAL, 0);
    check(value == retail_hs_you[0], "Ryu: the standing Jab keeps the hitstun of the disc");

    value = applied_hs_you(U3S_KEN, U3S_KOC_NORMAL, 6);
    check(value == retail_hs_you[6], "Ken: the close Fierce keeps the hitstun of the disc");

    printf("the hitstun items: %d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
