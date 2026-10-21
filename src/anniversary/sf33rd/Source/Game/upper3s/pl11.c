#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - makoto overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data):
 *
 *   SA I   { 22, ..., gauge_len 120, store_max 1 }  Seichusen Godanzuki
 *   SA II  { 20, ..., gauge_len  88, store_max 2 }  Abare Tosanami
 *   SA III { 21, ..., gauge_len  96, store_max 1, dtm 10922 }  Tanden Renki
 *
 * The design specification does not ask for a stock or gauge length change
 * on Makoto, so nothing is overridden here.
 */

const Upper3sSuperArt upper3s_super_art_makoto[4] = {
    { 0, 0, 0, 0 }, /* SA I - retail 120 / 1 stock */
    { 0, 0, 0, 0 }, /* SA II - retail 88 / 2 stocks */
    { 0, 0, 0, 0 }, /* SA III - retail 96 / 1 stock */
    { 0, 0, 0, 0 }  /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Super art indices come from bin2obj/asstbl.c:_assadr_lv_9900[makoto][0]; the
 * normal super art path (PLS03.c:execute_super_arts) takes the first entry
 * of the four entry group that belongs to the super art command index:
 *
 *   nmsa_g_ix 20 -> char_ix 43 (light), 44 (medium), 45 (heavy / EX)
 *   nmsa_g_ix 21 -> char_ix 54, 55, 56, 57
 *   nmsa_g_ix 22 -> char_ix 58, 59, 60, 61
 *
 * "Stun damage of Abare Tosanami (SAII) reduced by 50%".
 *
 * The stun of a move is the index `UNK_7.piyo` into the per character table
 * `bin2obj/gauge.c:_add_piyo_gauge[20][16]`, whose values run 0..50.  The row
 * of Makoto (the character index 16) is
 *
 *   { 0, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 24, 25, 29, 31 }
 *
 * so the index 14 is a stun of 29 and the index 15 a stun of 31.
 *
 * "Stun damage of Abare Tosanami (SAII) reduced by 50%".
 *
 * The stun of the super is halved relative to its retail value, which is
 * what the 500 permille below do.
 *
 * "Stun damage of L/M/H Fukiage reduced from 29/31/31 to 27/28/31".
 *
 * The attack table of Makoto holds exactly three entries with the stun
 * indexes 14 and 15 - the 29/31/31 of the document - and the fourth
 * strength of the same move has the index 10, which is the "L/M/H" of the
 * document.  The entries have the hitstun of a normal attack, so they are
 * the hits of a special move and not the hits of a super art.
 *
 * The Fukiage is the 623+P special of the character, so it is the command 28
 * of cmd_data.c (the 623 motion, the punch buttons).  The index of the move
 * is the char_ix of the entry of the assignment table that belongs to that
 * command, which is bin2obj/asstbl.c:_assadr_lv_9900[makoto][1][0..3] - the
 * four strengths of the command are four consecutive entries, and they are
 * the char_ix 36 (light), 37 (medium), 38 (heavy) and 39 (EX).
 *
 * The values are absolute: they are written to the stun of the defender
 * instead of the value the index of the disc would give, so that 27 and 28
 * can be used even though the row of the character does not hold them.
 */

const Upper3sMove upper3s_moves_makoto[] = {
    { U3S_KOC_SUPER, 43, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 500, 0, 0, 0, 0 }, /* Abare Tosanami, light */
    { U3S_KOC_SUPER, 44, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 500, 0, 0, 0, 0 }, /* Abare Tosanami, medium */
    { U3S_KOC_SUPER, 45, U3S_MV_STUN_PERMILLE, 0, 0, 0, 0, 0, 0, 500, 0, 0, 0, 0 }, /* Abare Tosanami, heavy / EX */
    { U3S_KOC_ATTACK, 36, U3S_MV_STUN, 0, 0, 27, 0, 0, 0, 0, 0, 0, 0, 0 },        /* Fukiage, light */
    { U3S_KOC_ATTACK, 37, U3S_MV_STUN, 0, 0, 28, 0, 0, 0, 0, 0, 0, 0, 0 },        /* Fukiage, medium */
    { U3S_KOC_ATTACK, 38, U3S_MV_STUN, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0 },        /* Fukiage, heavy */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                          /* terminator */
};
