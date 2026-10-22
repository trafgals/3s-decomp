#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - yang overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data):
 *
 *   SA I   { 21, ..., gauge_len 120, store_max 1 }
 *   SA II  { 22, ..., gauge_len 104, store_max 2 }
 *   SA III { 20, ..., gauge_len  64, store_max 1, dtm 13107 }
 *
 * "Yang's pretty okay; leaving Sei'ei Enbu with the shortest bar" - SA III
 * already owns the shortest bar of the three (0x40), so the entry below
 * only restates the retail value to make the intent explicit.
 */

const Upper3sSuperArt upper3s_super_art_yang[4] = {
    { 0, 0, 0, 0 },                   /* SA I - retail 120 / 1 stock */
    { 0, 0, 0, 0 },                   /* SA II - retail 104 / 2 stocks */
    { U3S_SA_GAUGE_LEN, 0, 0x40, 0 }, /* SA III - retail 64 / 1 stock */
    { 0, 0, 0, 0 }                    /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Super art indices come from bin2obj/asstbl.c:_assadr_lv_9900[yang][0]:
 *
 *   nmsa_g_ix 20 -> char_ix 61, 62, 63
 *   nmsa_g_ix 21 -> char_ix 48, 49, 50, 51
 *   nmsa_g_ix 22 -> char_ix 44, 45, 46
 *
 * TODO(3s'): "close standing Forward (MK) will sometimes not launch the
 * opponent" is a hit box and knock back problem, it lives in the on disc
 * frame chain.
 */

const Upper3sMove upper3s_moves_yang[] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
