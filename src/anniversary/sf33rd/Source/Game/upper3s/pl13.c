#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - akuma overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data):
 *
 *   SA I   { 20, 24, 25, 38, ..., gauge_len 112, store_max 2 }
 *   SA II  { 21, 24, 25,  0, ..., gauge_len 112, store_max 2 }
 *   SA III { 22, 24, 25, 39, ..., gauge_len 112, store_max 2 }
 *
 * "Messatsu Gou Rasen (SAII) bar length is now shorter than SAI or SAIII.
 * Earlier access to Raging Demon/KKZ at the cost of having a terrible super
 * otherwise." - all three supers of the retail table share the same gauge
 * bar length of 0x70, SA II is shortened to 0x60 so that it becomes the
 * most expensive super of the character.
 */

const Upper3sSuperArt upper3s_super_art_akuma[4] = {
    { 0, 0, 0, 0 },                   /* SA I - retail 112 / 2 stocks */
    { U3S_SA_GAUGE_LEN, 0, 0x60, 0 }, /* SA II - retail 112 / 2 stocks */
    { 0, 0, 0, 0 },                   /* SA III - retail 112 / 2 stocks */
    { 0, 0, 0, 0 }                    /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Super art indices come from bin2obj/asstbl.c:_assadr_lv_9900[akuma][0]; the
 * normal super art path (PLS03.c:execute_super_arts) takes the first entry
 * of the four entry group that belongs to the super art command index:
 *
 *   nmsa_g_ix 20 -> char_ix 36 (light), 37 (medium), 38 (heavy), 39 (EX)
 *   nmsa_g_ix 21 -> char_ix 40, 41, 42, 43
 *   nmsa_g_ix 22 -> char_ix 44, 45, 46, 47
 *
 * The hidden fourth super (the KKZ) is not part of the normal super art
 * path, it uses the nmsa_a_ix of its slot:
 *
 *   nmsa_a_ix 38 -> _assadr_lv_9900[akuma][2] + 0 -> char_ix 48, 49, 50, 51
 *   nmsa_a_ix 39 -> _assadr_lv_9900[akuma][2] + 4 -> char_ix 76, 77, 78, 79
 *
 * The KKZ is the nmsa_a_ix 38 group: it is the alternative of the SA I, it
 * is a punch command (the group of the SA I and of the SA II uses the
 * punch buttons, the group of the SA III uses the kick buttons) and it is
 * the only one of the two that has a character specific routine
 * (cmd_data.c:pE_cmd_38 asks for the routine 25, and
 * PLPAT14.c:pl14_exatt_table maps it to Att_PL14_AT2 - a routine of Akuma
 * alone, which is what a hidden super needs).  The four entries of the
 * group share one routine number (r_no 82), so they are the four strength
 * entries of one single move.
 *
 * "KKZ (hidden super) ... damage reduced from 107 to 95" - the base damage
 * of the hit is scaled by 95 / 107, so that the relation of the document is
 * kept whatever the retail base damage of the move is.  The damage pipeline
 * of the retail game is an integer one (Pow_Pow.c:cal_damage_vitality), so
 * the result is the reachable value that is the closest to 95.
 *
 * TODO(3s'): the motion of the hidden super (D,D,D+PPP -> D,D+PP) lives in
 * the command tables of cmd_data.c and is not a source level change.
 */

const Upper3sMove upper3s_moves_akuma[] = {
    /* The KKZ: 95 / 107 = 888 permille. */
    { U3S_KOC_SUPER, 48, U3S_MV_DAMAGE_PERMILLE, 0, 888, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { U3S_KOC_SUPER, 49, U3S_MV_DAMAGE_PERMILLE, 0, 888, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { U3S_KOC_SUPER, 50, U3S_MV_DAMAGE_PERMILLE, 0, 888, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { U3S_KOC_SUPER, 51, U3S_MV_DAMAGE_PERMILLE, 0, 888, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
