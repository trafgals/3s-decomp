#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - dudley overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data[4]):
 *
 *   SA I   { 20, ..., gauge_len  96, store_max 2 }  cmd20 - 236+P
 *   SA II  { 21, ..., gauge_len 112, store_max 1 }  cmd21 - 623+P  (Rocket Uppercut)
 *   SA III { 22, ..., gauge_len  80, store_max 3 }  cmd22 - 236+P+P  (Rolling Thunder)
 *   SA IV  {  0, ..., gauge_len  64, store_max 1 }  unused
 *
 * The design spec does not ask for a stock or gauge length change on Dudley,
 * so nothing is overridden here.
 */

const Upper3sSuperArt upper3s_super_art_dudley[4] = {
    { 0, 0, 0, 0 }, /* SA I - retail 96 / 2 stocks */
    { 0, 0, 0, 0 }, /* SA II - retail 112 / 1 stock */
    { 0, 0, 0, 0 }, /* SA III - retail 80 / 3 stocks */
    { 0, 0, 0, 0 }  /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Super art indices come from bin2obj/asstbl.c:_assadr_lv_9900[dudley][0];
 * the normal super art path (PLS03.c:execute_super_arts) takes the first
 * entry of the four entry group that belongs to the super art command index.
 * The routine of each group is the one the command set of the group asks
 * for (cmd_data.c:p4_cmd_20 / p4_cmd_21 / p4_cmd_22, the headers hold the
 * routine numbers 19, 20 and 21), and PLPAT04.c:pl04_exatt_table maps them
 * to the names of the moves:
 *
 *   nmsa_g_ix 20 -> char_ix 48 (light), 49 (medium), 50 (heavy / EX)
 *                  Att_SHOURYUUREPPA - the 623+P super, the Rocket Uppercut
 *   nmsa_g_ix 21 -> char_ix 32 (light), 33 (medium), 34 (heavy / EX)
 *                  Att_HADOUKEN2 - the 236+P+P super, the Rolling Thunder
 *   nmsa_g_ix 22 -> char_ix 28 (light), 29 (medium), 30 (heavy / EX)
 *                  Att_CHOUCHUURENGEKI - the 236+P+P+P super
 *
 * The slots of the retail table (PLCNT.c:super_arts_data[4]) and the names
 * of the document agree: the SA I is the 623+P super and the SA II is the
 * 236+P+P super.
 *
 * TODO(3s'): the retail stun of both supers lives in the on disc attack
 * table (UNK_7.piyo is an index into _add_piyo_gauge[player][16], whose
 * values run 0..50), so it cannot be read from the sources.  The stun is
 * therefore changed relative to the retail value: 75% for the Rocket
 * Uppercut and 125% for the Rolling Thunder, which keeps the relation
 * between the two supers that the document asks for ("try to differentiate
 * them somewhat") without having to guess the absolute numbers.
 */

const Upper3sMove upper3s_moves_dudley[] = {
    /* Rocket Uppercut (623+P, SA I): less stun damage. */
    { U3S_KOC_SUPER, 48, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 750, 0, 0, 0, 0 },
    { U3S_KOC_SUPER, 49, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 750, 0, 0, 0, 0 },
    { U3S_KOC_SUPER, 50, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 750, 0, 0, 0, 0 },
    /* Rolling Thunder (236+P+P, SA II): more stun damage. */
    { U3S_KOC_SUPER, 32, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 1250, 0, 0, 0, 0 },
    { U3S_KOC_SUPER, 33, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 1250, 0, 0, 0, 0 },
    { U3S_KOC_SUPER, 34, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 1250, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
