/*
 * Functional tests for the Third Strike Upper data layer.
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
#include "sf33rd/Source/Game/upper3s_sys.h"

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
 * Builds a synthetic character.
 *
 * The retail layout puts the index of a move and the data of the move in
 * the same array: `char_table[koc][index]` holds the offset (in u32) of
 * the move's first frame block. The test mirrors that: the first
 * `MOVE_COUNT` entries are the move indices, the blocks start after them.
 */
#define MOVE_COUNT 4
#define BLOCK_POS 8
#define ATT_COUNT 4

static u32 char_table[U3S_KOC_COUNT][MOVE_COUNT + BLOCK_POS];
static UNK_7 att_ix_table[ATT_COUNT];

static void setup_character(PLW* wk, s16 char_id, s16 koc, s16 index, s16 att_ix) {
    int i;

    memset(wk, 0, sizeof(PLW));
    memset(char_table, 0, sizeof(char_table));
    memset(att_ix_table, 0, sizeof(att_ix_table));

    for (i = 0; i < ATT_COUNT; i++) {
        att_ix_table[i].pow = (s8)(i + 1);
        att_ix_table[i].impact = (s8)(10 + i);
        att_ix_table[i].piyo = (s8)i;
        att_ix_table[i].hs_me = (s8)(i + 1);
        att_ix_table[i].hs_you = (s8)(i + 2);
    }

    /* The move header of move `index` lives at u32 offset `BLOCK_POS`. */
    char_table[koc][index] = BLOCK_POS * 4;
    char_table[koc][BLOCK_POS + 2] = (u32)((att_ix & 0xFFFF) << 16);

    for (i = 0; i < U3S_KOC_COUNT; i++) {
        wk->wu.char_table[i] = char_table[i];
    }

    wk->wu.att_ix_table = att_ix_table;
    wk->player_number = char_id;
    wk->wu.now_koc = koc;
    wk->wu.char_index = index;
}

static void test_apply_move_rewrites_the_right_entry(void) {
    PLW wk;
    Upper3sMove move;

    printf("apply_move: only the targeted attack entry is rewritten\n");

    setup_character(&wk, U3S_KEN, U3S_KOC_ATTACK, 1, 2);

    memset(&move, 0, sizeof(move));
    move.koc = U3S_KOC_ATTACK;
    move.index = 1;
    move.flags = U3S_MV_IMPACT | U3S_MV_HS_ME | U3S_MV_HS_YOU;
    move.impact = 21;
    move.hs_me = -3;
    move.hs_you = 4;

    upper3s_apply_move(&wk.wu, &move);

    check(att_ix_table[2].impact == 21, "the knockback of the target entry is rewritten");
    check(att_ix_table[2].hs_me == -3, "the hitstun of the attacker is rewritten");
    check(att_ix_table[2].hs_you == 4, "the hitstun of the defender is rewritten");

    check(att_ix_table[0].impact == 10, "a neighbouring entry keeps its knockback");
    check(att_ix_table[1].impact == 11, "a neighbouring entry keeps its knockback");
    check(att_ix_table[3].impact == 13, "a neighbouring entry keeps its knockback");
    check(att_ix_table[0].hs_me == 1, "a neighbouring entry keeps the hitstun of the attacker");
    check(att_ix_table[3].hs_you == 5, "a neighbouring entry keeps the hitstun of the defender");
}

static void test_apply_move_without_flags_changes_nothing(void) {
    PLW wk;
    Upper3sMove move;

    printf("apply_move: an override without flags is a no-op\n");

    setup_character(&wk, U3S_KEN, U3S_KOC_ATTACK, 1, 2);

    memset(&move, 0, sizeof(move));
    move.koc = U3S_KOC_ATTACK;
    move.index = 1;
    move.impact = 21;

    upper3s_apply_move(&wk.wu, &move);

    check(att_ix_table[2].impact == 12, "the knockback is untouched");
    check(att_ix_table[2].hs_me == 3, "the hitstun of the attacker is untouched");
    check(att_ix_table[2].hs_you == 4, "the hitstun of the defender is untouched");
}

static void test_apply_move_ignores_moves_without_attack_data(void) {
    PLW wk;
    Upper3sMove move;

    printf("apply_move: a move without attack data is skipped\n");

    setup_character(&wk, U3S_KEN, U3S_KOC_ATTACK, 1, -1);

    memset(&move, 0, sizeof(move));
    move.koc = U3S_KOC_ATTACK;
    move.index = 1;
    move.flags = U3S_MV_IMPACT;
    move.impact = 21;

    upper3s_apply_move(&wk.wu, &move);

    check(att_ix_table[0].impact == 10, "no entry is rewritten");
    check(att_ix_table[2].impact == 12, "no entry is rewritten");
}

static void test_apply_move_ignores_an_out_of_range_move(void) {
    PLW wk;
    Upper3sMove move;

    printf("apply_move: an out of range move is skipped\n");

    setup_character(&wk, U3S_KEN, U3S_KOC_ATTACK, 1, 2);

    memset(&move, 0, sizeof(move));
    move.koc = U3S_KOC_ATTACK;
    move.index = -1;
    move.flags = U3S_MV_IMPACT;
    move.impact = 21;

    upper3s_apply_move(&wk.wu, &move);

    check(att_ix_table[2].impact == 12, "a negative index rewrites nothing");

    memset(&move, 0, sizeof(move));
    move.koc = 99;
    move.index = 1;
    move.flags = U3S_MV_IMPACT;
    move.impact = 21;

    upper3s_apply_move(&wk.wu, &move);

    check(att_ix_table[2].impact == 12, "an out of range slot rewrites nothing");
}

static void test_apply_character_leaves_unlisted_moves_alone(void) {
    PLW wk;

    printf("apply_character: a character without overrides changes nothing\n");

    setup_character(&wk, U3S_GILL, U3S_KOC_ATTACK, 1, 2);
    upper3s_apply_character(&wk);

    check(att_ix_table[2].impact == 12, "the knockback is untouched");
    check(att_ix_table[2].hs_me == 3, "the hitstun of the attacker is untouched");
    check(att_ix_table[2].hs_you == 4, "the hitstun of the defender is untouched");
}

static void test_super_art_override(void) {
    Upper3sSuperArt sa;
    SA_DATA data;

    printf("super art: the flagged fields replace the retail base\n");

    memset(&data, 0, sizeof(data));
    data.store_max = 2;
    data.gauge_len = 112;
    data.dtm = 65536;

    memset(&sa, 0, sizeof(sa));
    sa.flags = U3S_SA_STORE_MAX | U3S_SA_GAUGE_LEN;
    sa.store_max = 1;
    sa.gauge_len = 64;

    upper3s_apply_super_art(&sa, &data);

    check(data.store_max == 1, "the stock count is replaced");
    check(data.gauge_len == 64, "the gauge length is replaced");
    check(data.dtm == 65536, "the damage multiplier is untouched");

    memset(&data, 0, sizeof(data));
    data.store_max = 2;
    data.gauge_len = 112;
    data.dtm = 65536;

    memset(&sa, 0, sizeof(sa));
    sa.flags = U3S_SA_DTM;
    sa.dtm = 13107;

    upper3s_apply_super_art(&sa, &data);

    check(data.store_max == 2, "the stock count is untouched");
    check(data.gauge_len == 112, "the gauge length is untouched");
    check(data.dtm == 13107, "the damage multiplier is replaced");
}

static void test_super_art_data_falls_back_to_the_retail_entry(void) {
    const SA_DATA* retail;
    SA_DATA probe;

    printf("super art: an unlisted slot returns the retail entry\n");

    memset(&probe, 0, sizeof(probe));
    probe.store_max = 2;

    retail = upper3s_super_art_data(U3S_KEN, U3S_SA1, &probe);

    if (upper3s_super_art_is_listed(U3S_KEN, U3S_SA1)) {
        check(retail != &probe, "a listed slot returns the patched copy");
    } else {
        check(retail == &probe, "an unlisted slot returns the retail entry");
    }

    check(upper3s_super_art_data(U3S_KEN, U3S_SA1, NULL) == NULL, "a missing entry is passed through");
}

static void test_damage_and_stun_lookup(void) {
    PLW wk;

    printf("runtime lookups: damage and stun are resolved per move\n");

    setup_character(&wk, U3S_GILL, U3S_KOC_ATTACK, 1, 2);

    check(upper3s_damage_base(wk.player_number, &wk.wu) == -1, "no override means no base damage");
    check(upper3s_damage_permille(wk.player_number, &wk.wu) == 1000, "no override means no damage scale");
    check(upper3s_stun_value(wk.player_number, &wk.wu) == -1, "no override means no stun value");
    check(upper3s_stun_permille(wk.player_number, &wk.wu) == 1000, "no override means no stun scale");
}

/*
 * The character index of the game is not the order of AGENTS.md section
 * 3.1.  Every per character table of the patch is keyed by the index of the
 * game, so a wrong order would give every character the balance changes of
 * another one.
 *
 * The test pins the order with the characters the design document singles
 * out: each of them has an override that only makes sense for that very
 * character, and the retail values of the neighbours are different enough
 * to notice a mix up.
 */
static void test_the_character_order_is_the_retail_one(void) {
    SA_DATA retail;
    const SA_DATA* patched;

    printf("character order: the tables are keyed by the index of the game\n");

    check(U3S_GILL == 0 && U3S_ALEX == 1 && U3S_RYU == 2 && U3S_YUN == 3,
          "the first four characters have the retail index");
    check(U3S_NECRO == 5 && U3S_IBUKI == 7 && U3S_ELENA == 8 && U3S_ORO == 9,
          "Necro, Ibuki, Elena and Oro have the retail index");
    check(U3S_KEN == 11 && U3S_SEAN == 12 && U3S_URIEN == 13 && U3S_AKUMA == 14,
          "Ken, Sean, Urien and Akuma have the retail index");
    check(U3S_CHUNLI == 15 && U3S_MAKOTO == 16 && U3S_Q == 17 && U3S_TWELVE == 18,
          "Chun-Li, Makoto, Q and Twelve have the retail index");
    check(U3S_REMY == 19, "Remy has the retail index");

    /* Ken's SA III goes from three stocks to two. */
    memset(&retail, 0, sizeof(retail));
    retail.store_max = 3;
    retail.gauge_len = 80;
    patched = upper3s_super_art_data(U3S_KEN, U3S_SA3, &retail);
    check(patched->store_max == 2, "Ken gets the stock change of his SA III");
    check(patched->gauge_len == 80, "Ken keeps the gauge length of his SA III");

    /* Akuma's SA II bar becomes shorter than the other two. */
    memset(&retail, 0, sizeof(retail));
    retail.store_max = 2;
    retail.gauge_len = 112;
    patched = upper3s_super_art_data(U3S_AKUMA, U3S_SA2, &retail);
    check(patched->gauge_len == 96, "Akuma gets the shorter bar of his SA II");

    /* Chun-Li's SA II goes from two stocks to one and gets the longest bar. */
    memset(&retail, 0, sizeof(retail));
    retail.store_max = 2;
    retail.gauge_len = 104;
    patched = upper3s_super_art_data(U3S_CHUNLI, U3S_SA2, &retail);
    check(patched->store_max == 1, "Chun-Li gets the stock change of her SA II");
    check(patched->gauge_len == 128, "Chun-Li gets the longest bar of her SA II");

    /* Twelve's SA II goes from one stock to two. */
    memset(&retail, 0, sizeof(retail));
    retail.store_max = 1;
    retail.gauge_len = 112;
    patched = upper3s_super_art_data(U3S_TWELVE, U3S_SA2, &retail);
    check(patched->store_max == 2, "Twelve gets the stock change of his SA II");

    /* Remy's SA III bar grows. */
    memset(&retail, 0, sizeof(retail));
    retail.store_max = 1;
    retail.gauge_len = 80;
    patched = upper3s_super_art_data(U3S_REMY, U3S_SA3, &retail);
    check(patched->gauge_len == 96, "Remy gets the longer bar of his SA III");

    /* Gill has no change at all, so nothing of his neighbours leaks in. */
    memset(&retail, 0, sizeof(retail));
    retail.store_max = 3;
    retail.gauge_len = 80;
    patched = upper3s_super_art_data(U3S_GILL, U3S_SA1, &retail);
    check(patched == &retail, "Gill keeps the retail entry of his SA I");
    check(retail.store_max == 3, "Gill keeps the retail stock count");
}

/*
 * The stun bar of Akuma and of Remy is the short one in the retail table
 * (56) and has to become the regular one (64), which is the size that the
 * majority of the cast uses.
 */
static void test_the_stun_bar_of_akuma_and_remy(void) {
    printf("stun bar: Akuma and Remy go from the short to the regular size\n");

    check(upper3s_sys_stun_genkai(U3S_AKUMA, 56) == 64, "Akuma gets the regular stun bar");
    check(upper3s_sys_stun_genkai(U3S_REMY, 56) == 64, "Remy gets the regular stun bar");

    check(upper3s_sys_stun_genkai(U3S_GILL, 72) == 72, "Gill keeps his stun bar");
    check(upper3s_sys_stun_genkai(U3S_ALEX, 72) == 72, "Alex keeps his stun bar");
    check(upper3s_sys_stun_genkai(U3S_KEN, 64) == 64, "Ken keeps his stun bar");
    check(upper3s_sys_stun_genkai(U3S_CHUNLI, 64) == 64, "Chun-Li keeps her stun bar");
}

/*
 * The health of Twelve and of Ibuki is taken from the row of the standard
 * cast; Ibuki gets that row scaled by the ratio of the design document.
 */
static void test_the_health_of_twelve_and_ibuki(void) {
    s16 scale;

    printf("health: Twelve and Ibuki take the health of the standard cast\n");

    scale = 0;
    check(upper3s_sys_health_source(U3S_TWELVE, &scale) == U3S_NECRO,
          "Twelve takes the health of the standard cast");
    check(scale == 1000, "Twelve takes the health unscaled");

    scale = 0;
    check(upper3s_sys_health_source(U3S_IBUKI, &scale) == U3S_NECRO,
          "Ibuki takes the health of the standard cast");
    check(scale == 937, "Ibuki takes 937 permille of it");

    scale = 0;
    check(upper3s_sys_health_source(U3S_KEN, &scale) == U3S_KEN, "Ken keeps his own health");
    check(scale == 1000, "Ken keeps his health unscaled");
}

/*
 * The metergain of a whiffed normal is halved for every consecutive
 * whiff.
 */
static void test_the_whiffed_meter_gain(void) {
    WORK wk;

    printf("meter: the gain of a whiffed normal is halved every time\n");

    memset(&wk, 0, sizeof(wk));
    wk.id = 0;
    wk.work_id = 1; /* a character, not a projectile */

    check(upper3s_sys_whiff_start(&wk, 100) == 100, "the first normal keeps its full gain");
    check(upper3s_sys_whiff_start(&wk, 100) == 50, "the second whiffed normal is halved");
    check(upper3s_sys_whiff_start(&wk, 100) == 25, "the third whiffed normal is a quarter");
    check(upper3s_sys_whiff_start(&wk, 100) == 12, "the fourth whiffed normal is an eighth");

    /* A contact resets the counter, so the next normal starts over. */
    upper3s_sys_whiff_contact(&wk, &wk, U3S_SYS_CONTACT_HIT);
    check(upper3s_sys_whiff_start(&wk, 100) == 100, "the counter restarts after a hit");
    check(upper3s_sys_whiff_start(&wk, 100) == 50, "the counter counts again after a hit");
}

int main(void) {
    printf("Third Strike Upper - data layer tests\n\n");

    test_apply_move_rewrites_the_right_entry();
    test_apply_move_without_flags_changes_nothing();
    test_apply_move_ignores_moves_without_attack_data();
    test_apply_move_ignores_an_out_of_range_move();
    test_apply_character_leaves_unlisted_moves_alone();
    test_super_art_override();
    test_super_art_data_falls_back_to_the_retail_entry();
    test_damage_and_stun_lookup();
    test_the_character_order_is_the_retail_one();
    test_the_stun_bar_of_akuma_and_remy();
    test_the_health_of_twelve_and_ibuki();
    test_the_whiffed_meter_gain();

    printf("\n%d checks, %d failures\n", checks, failures);

    if (failures != 0) {
        printf("FAILED\n");
        return 1;
    }

    printf("PASSED\n");
    return 0;
}

