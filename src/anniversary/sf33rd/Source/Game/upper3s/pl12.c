#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - urien overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data):
 *
 *   SA I   { 20, ..., gauge_len 104, store_max 2 }  Tyrant Slaughter
 *   SA II  { 21, ..., gauge_len  80, store_max 2 }  Tyrant Punish
 *   SA III { 22, ..., gauge_len  88, store_max 2 }
 *
 * "Tyrant Slaughter/Tyrant Punish (SAI) ... limit to 1 stock instead of 2;
 * increase bar length slightly" - SA I costs two stocks in the retail data,
 * it is reduced to one, and its gauge bar is stretched from 0x68 to 0x70.
 */

const Upper3sSuperArt upper3s_super_art_urien[4] = {
    { U3S_SA_STORE_MAX | U3S_SA_GAUGE_LEN, 1, 0x70, 0 }, /* SA I - retail 104 / 2 stocks */
    { 0, 0, 0, 0 },                                        /* SA II - retail 80 / 2 stocks */
    { 0, 0, 0, 0 },                                        /* SA III - retail 88 / 2 stocks */
    { 0, 0, 0, 0 }                                         /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Super art indices come from bin2obj/asstbl.c:_assadr_lv_9900[urien][0]; the
 * normal super art path (PLS03.c:execute_super_arts) takes the first entry
 * of the four entry group that belongs to the super art command index:
 *
 *   nmsa_g_ix 20 -> char_ix 70 (light), 71 (medium), 72 (heavy), 73 (EX)
 *   nmsa_g_ix 21 -> char_ix 66, 67, 68, 69
 *   nmsa_g_ix 22 -> char_ix 62, 63, 64, 65
 *
 * "increase damage by 25% to match Shin Shoryuken" - the base damage of
 * every hit of the super is scaled to 125%.
 */

const Upper3sMove upper3s_moves_urien[] = {
    { U3S_KOC_SUPER, 70, U3S_MV_DAMAGE_PERMILLE, 0, 1250, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* SA I, light */
    { U3S_KOC_SUPER, 71, U3S_MV_DAMAGE_PERMILLE, 0, 1250, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* SA I, medium */
    { U3S_KOC_SUPER, 72, U3S_MV_DAMAGE_PERMILLE, 0, 1250, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* SA I, heavy */
    { U3S_KOC_SUPER, 73, U3S_MV_DAMAGE_PERMILLE, 0, 1250, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* SA I, EX */
    /* TODO(3s'): "fix Tyrant Slaughter/Tyrant Punish to fully connect for
     * max damage vs all characters when crouching or in most juggle
     * situations" is a hit box / knock back problem and lives in the on
     * disc frame chain.
     */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
