/*
 * Functional tests for the Third Strike Upper changes of Dudley.
 *
 * Dudley - "Reduce stun damage on Rocket Uppercut (SAI)" and
 *          "Increase stun damage on Rolling Thunder (SAII)".
 *
 * The two supers of the retail table (PLCNT.c:super_arts_data[4]) are the
 * command sets 20 and 21, and the command sets of Dudley
 * (cmd_data.c:p4_cmd_20 and p4_cmd_21) ask for the routine numbers 19 and
 * 20, which PLPAT04.c:pl04_exatt_table maps to Att_SHOURYUUREPPA (the
 * 623+P super, the Rocket Uppercut) and to Att_HADOUKEN2 (the 236+P+P
 * super, the Rolling Thunder).  The four strengths of a super are the four
 * consecutive entries of the group 0 of the assignment table of Dudley
 * (bin2obj/asstbl.c:_assadr_lv_9900[4][0]):
 *
 *   SA I  (Rocket Uppercut)   char_ix 48 (light), 49 (medium), 50 (heavy)
 *   SA II (Rolling Thunder)   char_ix 32 (light), 33 (medium), 34 (heavy)
 *
 * The retail stun of a move is an index into a per player table
 * (UNK_7.piyo into _add_piyo_gauge[player][16]), so it cannot be read from
 * the sources: the stun is changed relative to the retail value.
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

/* The scale of the stun of a move, in permille. */
static s16 stun_permille(s16 char_id, s16 koc, s16 index) {
    PLW wk;

    memset(&wk, 0, sizeof(wk));
    wk.wu.now_koc = koc;
    wk.wu.char_index = index;

    return upper3s_stun_permille(char_id, &wk.wu);
}

/*
 * The Rocket Uppercut does less stun damage.
 */
static void test_the_rocket_uppercut_stuns_less(void) {
    printf("dudley: the Rocket Uppercut stuns less\n");

    check(stun_permille(U3S_DUDLEY, U3S_KOC_SUPER, 48) == 750, "the light Rocket Uppercut stuns less");
    check(stun_permille(U3S_DUDLEY, U3S_KOC_SUPER, 49) == 750, "the medium Rocket Uppercut stuns less");
    check(stun_permille(U3S_DUDLEY, U3S_KOC_SUPER, 50) == 750, "the heavy Rocket Uppercut stuns less");
}

/*
 * The Rolling Thunder does more stun damage.
 */
static void test_the_rolling_thunder_stuns_more(void) {
    printf("dudley: the Rolling Thunder stuns more\n");

    check(stun_permille(U3S_DUDLEY, U3S_KOC_SUPER, 32) == 1250, "the light Rolling Thunder stuns more");
    check(stun_permille(U3S_DUDLEY, U3S_KOC_SUPER, 33) == 1250, "the medium Rolling Thunder stuns more");
    check(stun_permille(U3S_DUDLEY, U3S_KOC_SUPER, 34) == 1250, "the heavy Rolling Thunder stuns more");
}

/*
 * The index 44 is not one of the supers of Dudley: it is a special move
 * (the Corkscrew Blow, the waza 33 of the command table of Dudley), so it
 * must not be touched.
 */
static void test_the_special_moves_keep_their_stun(void) {
    printf("dudley: the special moves of Dudley keep their stun\n");

    check(stun_permille(U3S_DUDLEY, U3S_KOC_SUPER, 44) == 1000, "the index 44 is not a super of Dudley");
    check(stun_permille(U3S_DUDLEY, U3S_KOC_SUPER, 28) == 1000, "the SA III keeps its retail stun");
    check(stun_permille(U3S_DUDLEY, U3S_KOC_ATTACK, 24) == 1000, "the jab special keeps its retail stun");
}

int main(void) {
    test_the_rocket_uppercut_stuns_less();
    test_the_rolling_thunder_stuns_more();
    test_the_special_moves_keep_their_stun();

    printf("%d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
