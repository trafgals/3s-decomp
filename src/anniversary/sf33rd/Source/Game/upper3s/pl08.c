#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - oro overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data):
 *
 *   SA I   { 22, ..., gauge_len 104, store_max 1, dtm 16384 }
 *   SA II  { 21, ..., gauge_len  88, store_max 3, exsa_g_ix 24 }
 *   SA III { 20, ..., gauge_len 112, store_max 1, dtm 13107 }
 *
 * "Increase the length of the Kishi Riki (SAI) bar to match Tengu Stone's
 * bar" - SA I takes the gauge bar length of SA III, 0x68 -> 0x70.  A longer
 * bar means that the super needs less gauge, which is the intent of the
 * change.
 *
 * TODO(3s'): the names of the two supers come from the design specification.
 * The retail data only tells that SA II is the one with the EX versions
 * (exsa_g_ix 24, the EX TENGUIWA and EX KISHINRIKI of PLPAT09.c) and that
 * SA I and SA III are the one stock supers.
 */

const Upper3sSuperArt upper3s_super_art_oro[4] = {
    { U3S_SA_GAUGE_LEN, 0, 0x70, 0 }, /* SA I - retail 104 / 1 stock */
    { 0, 0, 0, 0 },                  /* SA II - retail 88 / 3 stocks */
    { 0, 0, 0, 0 },                  /* SA III - retail 112 / 1 stock */
    { 0, 0, 0, 0 }                   /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Super art indices come from bin2obj/asstbl.c:_assadr_lv_9900[oro][0]:
 *
 *   nmsa_g_ix 20 -> char_ix 44 (light), 45 (medium), 46 (heavy), 47 (EX)
 *   nmsa_g_ix 21 -> char_ix 48, 49, 50
 *   nmsa_g_ix 22 -> char_ix 52, 53, 54, 55
 *
 * TODO(3s'): "increase damage of the air regular Kishi Riki grab when
 * combo'd into" needs the atca index of the grab, which is only reachable
 * through the command byte code of cmd_data.c.
 */

const Upper3sMove upper3s_moves_oro[] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
