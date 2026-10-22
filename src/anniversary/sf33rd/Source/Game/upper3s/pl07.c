#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - elena overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data):
 *
 *   SA I   { 20, ..., gauge_len  80, store_max 3 }  236+P
 *   SA II  { 21, ..., gauge_len 104, store_max 2 }  623+P
 *   SA III { 22, ..., gauge_len 128, store_max 1 }  multi hit
 *
 * "She dances okay!  Leaving her with 3 stocks for Spinning Beat, also." -
 * the three stock super of the retail table is SA I, it already costs three
 * stocks, so the entry below only restates the retail value to make the
 * intent explicit.
 */

const Upper3sSuperArt upper3s_super_art_elena[4] = {
    { U3S_SA_STORE_MAX, 3, 0, 0 }, /* SA I - retail 80 / 3 stocks */
    { 0, 0, 0, 0 },                /* SA II - retail 104 / 2 stocks */
    { 0, 0, 0, 0 },                /* SA III - retail 128 / 1 stock */
    { 0, 0, 0, 0 }                 /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Super art indices come from bin2obj/asstbl.c:_assadr_lv_9900[elena][0]; the
 * normal super art path (PLS03.c:execute_super_arts) takes the first entry
 * of the four entry group that belongs to the super art command index:
 *
 *   nmsa_g_ix 20 -> char_ix 36 (light), 37 (medium), 38 (heavy / EX)
 *   nmsa_g_ix 21 -> char_ix 40, 41, 42
 *   nmsa_g_ix 22 -> char_ix 48
 *
 * TODO(3s'): the healing super of the game has no numeric entry here.  Its
 * recovery rate is a code change in PLPAT08.c, the length of its cancel
 * animation lives in the on disc frame chain.
 */

const Upper3sMove upper3s_moves_elena[] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
