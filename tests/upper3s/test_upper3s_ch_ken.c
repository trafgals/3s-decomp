/*
 * Functional tests for the Third Strike Upper changes of Ken.
 *
 * Ken - "Jab DP (623LP) damage reduced from 15 to 11.5" and
 *       "Shippu Jinrai Kyaku (SAIII) now has a maximum of 2 stocks instead
 *        of 3; damage reduced by 15%".
 *
 * The jab dragon punch is the light version of the dragon punch of Ken: the
 * command set of the waza 28 (cmd_data.c:pB_cmd_28) asks for the routine 17,
 * which PLPAT11.c:pl11_exatt_table maps to Att_SHOURYUUKEN, and the four
 * strengths of that command are the four consecutive entries 28 (light),
 * 29 (medium), 30 (heavy) and 31 (EX) of the group 1 of the assignment
 * table of Ken (bin2obj/asstbl.c:_assadr_lv_9900[11][1]).
 *
 * The base damage of a move is the index of the power table
 * (POW_DATA.c:Power_Data), so the damage cannot be written in place: it is
 * scaled at runtime (Pow_Pow.c:upper3s_power).  The document gives the
 * retail value and the wanted value of the jab dragon punch, so the change
 * is the ratio between them: 11.5 / 15 = 767 permille.
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

/* The scale of the damage of a move, in permille. */
static s16 damage_permille(s16 char_id, s16 koc, s16 index) {
    PLW wk;

    memset(&wk, 0, sizeof(wk));
    wk.wu.now_koc = koc;
    wk.wu.char_index = index;

    return upper3s_damage_permille(char_id, &wk.wu);
}

/*
 * The jab dragon punch (623+LP) does 11.5 instead of 15.
 */
static void test_the_jab_dragon_punch_is_nerfed(void) {
    printf("ken: the jab dragon punch does 11.5 instead of 15\n");

    check(damage_permille(U3S_KEN, U3S_KOC_ATTACK, 28) == 767,
          "the damage of the jab dragon punch is scaled to 11.5 / 15");
}

/*
 * The other versions of the dragon punch and the other moves of Ken keep
 * their retail damage.
 */
static void test_the_other_moves_keep_their_damage(void) {
    printf("ken: the other moves of Ken keep their damage\n");

    check(damage_permille(U3S_KEN, U3S_KOC_ATTACK, 29) == 1000,
          "the medium dragon punch keeps its damage");
    check(damage_permille(U3S_KEN, U3S_KOC_ATTACK, 30) == 1000,
          "the heavy dragon punch keeps its damage");
    check(damage_permille(U3S_KEN, U3S_KOC_ATTACK, 31) == 1000,
          "the EX dragon punch keeps its damage");
    check(damage_permille(U3S_KEN, U3S_KOC_ATTACK, 24) == 1000,
          "the hadouken keeps its damage");
    check(damage_permille(U3S_KEN, U3S_KOC_ATTACK, 32) == 1000,
          "the senpukyaku keeps its damage");
    check(damage_permille(U3S_KEN, U3S_KOC_SUPER, 36) == 1000,
          "the SA I keeps its damage");
}

/*
 * Shippu Jinrai Kyaku (the SA III) does 15% less damage and costs two
 * stocks.
 */
static void test_shippu_is_nerfed_and_costs_two_stocks(void) {
    SA_DATA retail;
    const SA_DATA* patched;
    s16 sa;

    printf("ken: Shippu Jinrai Kyaku does 15 percent less damage and costs two stocks\n");

    check(damage_permille(U3S_KEN, U3S_KOC_SUPER, 44) == 850, "the light Shippu does 15% less damage");
    check(damage_permille(U3S_KEN, U3S_KOC_SUPER, 45) == 850, "the medium Shippu does 15% less damage");
    check(damage_permille(U3S_KEN, U3S_KOC_SUPER, 46) == 850, "the heavy Shippu does 15% less damage");

    memset(&retail, 0, sizeof(retail));
    retail.store_max = 3;
    retail.gauge_len = 80;
    retail.dtm = 65536;

    patched = upper3s_super_art_data(U3S_KEN, U3S_SA3, &retail);

    check(patched != NULL && patched->store_max == 2, "the Shippu costs two stocks");
    check(patched != NULL && patched->gauge_len == 80, "the bar of the Shippu keeps its retail length");
    check(patched != NULL && patched->dtm == 65536, "the damage multiplier of the Shippu is untouched");

    /* The other supers of Ken are not touched. */
    patched = upper3s_super_art_data(U3S_KEN, U3S_SA1, &retail);

    check(patched != NULL && patched->store_max == 3, "the SA I keeps its retail stock count");
}

int main(void) {
    test_the_jab_dragon_punch_is_nerfed();
    test_the_other_moves_keep_their_damage();
    test_shippu_is_nerfed_and_costs_two_stocks();

    printf("%d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
