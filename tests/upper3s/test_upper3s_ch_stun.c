/*
 * Functional tests for the Third Strike Upper stun items.
 *
 * The design document asks for the stun of the Fukiage of Makoto to be
 * lowered, so that the 100% stun combo it could be part of is not
 * guaranteed:
 *
 *   "Stun damage of L/M/H Fukiage reduced from 29/31/31 to 27/28/31."
 *
 * The stun of a move is the index `UNK_7.piyo` of the attack data into the
 * per character table `bin2obj/gauge.c:_add_piyo_gauge[20][16]`, whose values
 * run 0..50.  The row of Makoto (the character index 16) is
 *
 *   { 0, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 24, 25, 29, 31 }
 *
 * so the index 14 is a stun of 29 and the index 15 a stun of 31 - the values
 * the document quotes for the Fukiage.  The attack table of Makoto holds
 * exactly three entries with those two indexes (the entries 38, 61 and 62,
 * with the damage of 24, 28 and 32), which are the light, the medium and the
 * heavy of the move.  The EX is the entry 63 with the index 10, a stun of 21,
 * which the document does not quote, so it keeps the value of the disc and
 * has no entry in the table of the patch.
 *
 * The override is an absolute value: `HITCHECK.c` writes it to the stun of
 * the defender instead of the value the index of the disc would give, so
 * that 27 and 28 can be used even though the row of the character does not
 * hold them.
 *
 * The per character move data is loaded from the disc image at runtime, so
 * the only way to test the override layer is to build a synthetic character
 * and check that the framework rewrites exactly the numbers it is supposed
 * to rewrite - and nothing else.
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

/* The stun the framework would use for the move (char_id, koc, index). */
static s16 applied_stun(s16 char_id, s16 koc, s16 index) {
    PLW wk;

    setup_character(&wk, char_id);
    wk.wu.now_koc = koc;
    wk.wu.char_index = index;
    upper3s_apply_character(&wk);

    return upper3s_stun_value(char_id, &wk.wu);
}

/* The scale the framework would use for the move (char_id, koc, index). */
static s16 applied_stun_permille(s16 char_id, s16 koc, s16 index) {
    PLW wk;

    setup_character(&wk, char_id);
    wk.wu.now_koc = koc;
    wk.wu.char_index = index;
    upper3s_apply_character(&wk);

    return upper3s_stun_permille(char_id, &wk.wu);
}

int main(void) {
    printf("the stun items of Third Strike Upper\n");

    /* The Fukiage of Makoto: the command 28 of cmd_data.c, the 623+P special,
     * whose four strengths are the char_ix 36, 37, 38 and 39 of the
     * assignment table of the character. */
    check(applied_stun(U3S_MAKOTO, U3S_KOC_ATTACK, 36) == 27,
          "Makoto: the light Fukiage adds 27 stun");
    check(applied_stun(U3S_MAKOTO, U3S_KOC_ATTACK, 37) == 28,
          "Makoto: the medium Fukiage adds 28 stun");
    check(applied_stun(U3S_MAKOTO, U3S_KOC_ATTACK, 38) == 31,
          "Makoto: the heavy Fukiage adds 31 stun");

    /* The document only names the three strengths: the EX keeps the stun of
     * the disc. */
    check(applied_stun(U3S_MAKOTO, U3S_KOC_ATTACK, 39) == -1,
          "Makoto: the EX Fukiage keeps the stun of the disc");

    /* The stun of the Fukiage is an absolute value, so it is not scaled on
     * top of the value of the disc. */
    check(applied_stun_permille(U3S_MAKOTO, U3S_KOC_ATTACK, 36) == 1000,
          "Makoto: the stun of the light Fukiage is not scaled");

    /* A move the document does not touch keeps the stun of the disc. */
    check(applied_stun(U3S_MAKOTO, U3S_KOC_ATTACK, 35) == -1,
          "Makoto: the Karakusa keeps the stun of the disc");
    check(applied_stun(U3S_MAKOTO, U3S_KOC_NORMAL, 6) == -1,
          "Makoto: the close Fierce keeps the stun of the disc");
    check(applied_stun(U3S_KEN, U3S_KOC_ATTACK, 28) == -1,
          "Ken: the jab Dragon Punch keeps the stun of the disc");

    /* The stun of the Abare Tosanami is halved relative to the value of the
     * disc, so it is scaled and not replaced. */
    check(applied_stun(U3S_MAKOTO, U3S_KOC_SUPER, 43) == -1,
          "Makoto: the stun of the Abare Tosanami is scaled, not replaced");
    check(applied_stun_permille(U3S_MAKOTO, U3S_KOC_SUPER, 43) == 500,
          "Makoto: the stun of the Abare Tosanami is halved");

    printf("the stun items: %d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
