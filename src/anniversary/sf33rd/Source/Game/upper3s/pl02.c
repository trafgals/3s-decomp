#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - yun overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data[3]):
 *
 *   SA I   { 22, ..., gauge_len 120, store_max 1 }  cmd22 - 236+P+P  (Senpuukyaku)
 *   SA II  { 21, ..., gauge_len  88, store_max 3 }  cmd21 - 236+P+P  (Sourai Rengeki)
 *   SA III { 20, ..., gauge_len  72, store_max 1 }  cmd20 - 236+P  (Genei Jin), dtm 16384
 *   SA IV  {  0, ..., gauge_len   0, store_max 1 }  unused
 *
 * The design spec names the supers "Sourai Rengeki (SA II)" and "Genei Jin
 * (SA III)", and the retail data agrees: the SA II entry costs three
 * stocks, which is exactly the "3 stocks instead of 2" of the document, and
 * the SA III entry has the shortest bar of Yun's three, which is the one
 * the document wants to grow to the length of SA II.
 */

const Upper3sSuperArt upper3s_super_art_yun[4] = {
    { 0, 0, 0, 0 },                /* SA I - retail 120 / 1 stock */
    /* SA II - "Sourai Rengeki": a maximum of 2 stocks instead of 3.
     * Retail costs 3 stocks, so the spec target of 2 is applied here. */
    { U3S_SA_STORE_MAX, 2, 0, 0 },
    /* SA III - "Genei Jin": the gauge bar grows to the length of
     * "Sourai Rengeki" (SA II, 88). Retail is 72. */
    { U3S_SA_GAUGE_LEN, 0, 88, 0 },
    { 0, 0, 0, 0 } /* SA IV - retail 0 / 1 stock */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * TODO(3s'): "Genei Jin no longer grants all of Yun's normals the super hit
 * priority" needs the attack level (UNK_7.level) of the normals, which only
 * exists in the on disc attack table.  See the report for the mechanism.
 */

const Upper3sMove upper3s_moves_yun[] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
