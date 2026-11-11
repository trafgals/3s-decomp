#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - ken overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data):
 *
 *   SA I   { 20, ..., gauge_len 112, store_max 2 }  236+P, Shinkuu Hadou
 *   SA II  { 21, ..., gauge_len 104, store_max 1 }  623+P, Shin Shoryuken
 *   SA III { 22, ..., gauge_len  80, store_max 3 }  multi hit, Shippu
 *
 * "Shippu Jinrai Kyaku (SAIII) now has a maximum of 2 stocks instead of 3"
 * - SA III is the multi hit super of the retail table and it costs three
 * stocks there, so the stock count is reduced to two.
 */

const Upper3sSuperArt upper3s_super_art_ken[4] = {
    { 0, 0, 0, 0 },                /* SA I - retail 112 / 2 stocks */
    { 0, 0, 0, 0 },                /* SA II - retail 104 / 1 stock */
    { U3S_SA_STORE_MAX, 2, 0, 0 }, /* SA III - retail 80 / 3 stocks */
    { 0, 0, 0, 0 }                 /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Super art indices come from bin2obj/asstbl.c:_assadr_lv_9900[ken][0]; the
 * normal super art path (PLS03.c:execute_super_arts) takes the first entry
 * of the four entry group that belongs to the super art command index:
 *
 *   nmsa_g_ix 20 -> char_ix 36 (light), 37 (medium), 38 (heavy / EX)
 *   nmsa_g_ix 21 -> char_ix 40, 41, 42
 *   nmsa_g_ix 22 -> char_ix 44 (light), 45 (medium), 46 (heavy / EX)
 *
 * The special moves are addressed by the command index of their own: the
 * command set of the waza 28 is the dragon punch of Ken
 * (cmd_data.c:pB_cmd_28, the header holds the routine numbers 17 that
 * PLPAT11.c:pl11_exatt_table maps to Att_SHOURYUUKEN), and the four
 * strengths of that command are four consecutive entries of the group 1 of
 * the assignment table:
 *
 *   waza 28 -> char_ix 28 (light, 623+LP), 29 (medium), 30 (heavy), 31 (EX)
 *
 * "Shippu Jinrai Kyaku (SAIII) ... damage reduced by 15%" - the base damage
 * of every hit of the super is scaled to 85%.
 *
 * "Jab DP (623LP) damage reduced from 15 to 11.5" - the base damage of the
 * light dragon punch is scaled by 11.5 / 15, so that the relation between
 * the jab and the strong dragon punch of the document is kept whatever the
 * retail base damage of the move is.  The damage pipeline of the retail
 * game is an integer one (Pow_Pow.c:cal_damage_vitality), so the result is
 * the reachable value that is the closest to 11.5.
 */

const Upper3sMove upper3s_moves_ken[] = {
    { U3S_KOC_SUPER, 44, U3S_MV_DAMAGE_PERMILLE, 0, 850, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* SA III, light */
    { U3S_KOC_SUPER, 45, U3S_MV_DAMAGE_PERMILLE, 0, 850, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* SA III, medium */
    { U3S_KOC_SUPER, 46, U3S_MV_DAMAGE_PERMILLE, 0, 850, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* SA III, heavy / EX */
    /* The jab dragon punch (623+LP): 11.5 / 15 = 767 permille. */
    { U3S_KOC_ATTACK, 28, U3S_MV_DAMAGE_PERMILLE, 0, 767, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
