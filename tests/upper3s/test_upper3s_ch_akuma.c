/*
 * Functional tests for the Third Strike Upper changes of Akuma.
 *
 * Akuma - "KKZ (hidden super) motion changed from D,D,D+PPP to D,D+PP;
 *          damage reduced from 107 to 95" and
 *          "Messatsu Gou Rasen (SAII) bar length is now shorter than SAI or
 *           SAIII".
 *
 * The KKZ is the hidden super of Akuma: it is the alternative command of the
 * SA I (PLCNT.c:super_arts_data[14] holds nmsa_a_ix 38 for the SA I), and
 * it is the entry that the group 2 of the assignment table of Akuma
 * (bin2obj/asstbl.c:_assadr_lv_9900[14][2]) holds at its first four
 * entries: char_ix 48, 49, 50 and 51.  The four entries share one routine
 * number (r_no 82), so they are the four strength entries of one single
 * move, and the command set of the alternative (cmd_data.c:pE_cmd_38) asks
 * for the routine 25, which PLPAT14.c:pl14_exatt_table maps to
 * Att_PL14_AT2 - a routine of Akuma alone.
 *
 * The base damage of a move is the index of the power table
 * (POW_DATA.c:Power_Data), so it is scaled at runtime
 * (Pow_Pow.c:upper3s_power).  The document gives the retail value and the
 * wanted value of the KKZ, so the change is the ratio between them:
 * 95 / 107 = 888 permille.
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
 * The KKZ does 95 instead of 107.
 */
static void test_the_kkz_is_nerfed(void) {
    printf("akuma: the KKZ does 95 instead of 107\n");

    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 48) == 888, "the KKZ is scaled to 95 / 107");
    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 49) == 888, "the KKZ is scaled to 95 / 107");
    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 50) == 888, "the KKZ is scaled to 95 / 107");
    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 51) == 888, "the KKZ is scaled to 95 / 107");
}

/*
 * The other supers of Akuma keep their damage.
 */
static void test_the_other_supers_keep_their_damage(void) {
    printf("akuma: the other supers of Akuma keep their damage\n");

    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 36) == 1000, "the SA I keeps its damage");
    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 40) == 1000, "the SA II keeps its damage");
    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 44) == 1000, "the SA III keeps its damage");
    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 76) == 1000,
          "the alternative of the SA III keeps its damage");
    check(damage_permille(U3S_AKUMA, U3S_KOC_SUPER, 57) == 1000,
          "the index 57 is not one of the moves of Akuma");
}

/*
 * The bar of the Messatsu Gou Rasen (the SA II) is shorter than the bars of
 * the SA I and of the SA III.
 */
static void test_the_bar_of_the_raging_demon_is_shorter(void) {
    SA_DATA retail;
    const SA_DATA* sa1;
    const SA_DATA* sa2;
    const SA_DATA* sa3;

    printf("akuma: the bar of the Messatsu Gou Rasen is the shortest one\n");

    memset(&retail, 0, sizeof(retail));
    retail.store_max = 2;
    retail.gauge_len = 112;
    retail.dtm = 65536;

    sa1 = upper3s_super_art_data(U3S_AKUMA, U3S_SA1, &retail);
    sa2 = upper3s_super_art_data(U3S_AKUMA, U3S_SA2, &retail);
    sa3 = upper3s_super_art_data(U3S_AKUMA, U3S_SA3, &retail);

    check(sa2 != NULL && sa2->gauge_len < sa1->gauge_len, "the SA II bar is shorter than the SA I bar");
    check(sa2 != NULL && sa2->gauge_len < sa3->gauge_len, "the SA II bar is shorter than the SA III bar");
    check(sa1 != NULL && sa1->gauge_len == 112, "the SA I bar keeps its retail length");
    check(sa3 != NULL && sa3->gauge_len == 112, "the SA III bar keeps its retail length");
    check(sa2 != NULL && sa2->store_max == 2, "the SA II keeps its retail stock count");
}

int main(void) {
    test_the_kkz_is_nerfed();
    test_the_other_supers_keep_their_damage();
    test_the_bar_of_the_raging_demon_is_shorter();

    printf("%d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
