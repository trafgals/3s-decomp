#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - twelve overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail values (PLCNT.c:super_arts_data[18] = twelve, {cmd set, gauge, stock}):
 *   SA I   { 20,  96, 2 }
 *   SA II  {  0, 112, 1 }   X.F.L.A.T.
 *   SA III { 21, 128, 1 }   X.C.O.P.Y.
 *
 * 3s' changes:
 *   SA II  - X.F.L.A.T. costs 2 gauge stocks instead of 1.
 */

const Upper3sSuperArt upper3s_super_art_twelve[4] = {
    { 0, 0, 0, 0 },                                 /* SA I */
    { U3S_SA_STORE_MAX, 2, 0, 0 },                  /* SA II - X.F.L.A.T. */
    { 0, 0, 0, 0 },                                 /* SA III - X.C.O.P.Y. */
    { 0, 0, 0, 0 }                                  /* SA IV */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * The normal move indexes (U3S_KOC_NORMAL) are the char_ix of the disc's
 * assignment table (bin2obj/asstbl.c), not the index of the move in the
 * move list:
 *
 *   char_ix = base + kos * 3 + koa
 *
 * with kos the button (0 LP, 1 MP, 2 HP, 3 LK, 4 MK, 5 HK) and koa the
 * variant.  For the standing normals the base is 0 and koa is 0 without a
 * dash (5X), 1 with a dash forward (6X) and 2 with a dash back (4X); for
 * the crouching normals the base is 18 and koa is 0 without a dash (2X),
 * 1 with a dash forward (3X) and 2 with a dash back (1X).
 *
 *   5LP  0    6LP  1    4LP  2     5MP  3    6MP  4    4MP  5
 *   5HP  6    6HP  7    4HP  8     5LK  9    6LK 10    4LK 11
 *   5MK 12    6MK 13    4MK 14     5HK 15    6HK 16    4HK 17
 *   2LP 18    3LP 19    1LP 20     2MP 21    3MP 22    1MP 23
 *   2HP 24    3HP 25    1HP 26     2LK 27    3LK 28    1LK 29
 *   2MK 30    3MK 31    1MK 32     2HK 33    3HK 34    1HK 35
 *
 * 3s': "Increase hitstun on standing Strong, far Strong, standing Fierce,
 * crouching Fierce, standing Short, standing Forward, standing Roundhouse,
 * crouching Forward, and Back+Forward (4MK) so they are not negative on hit."
 *
 * The advantage of a hit is the difference between how long the defender is
 * held and how long the attacker is locked, and both are read from the data
 * of the disc.  `tools/upper3s/hit_advantage.py` computes the advantage of
 * every normal out of that data.
 *
 * The hitstun of a move is the hold of the defender, and that is the field
 * `hs_you` of the attack data (`UNK_7.hs_you`), so that is what is raised
 * here.  The values below are the ones that make the advantage of the hit
 * zero, the end state the design document asks for ("not negative on hit").
 * The far Strong, the standing Fierce and the standing Short are already at
 * zero or better in the retail data and are not changed.
 */

const Upper3sMove upper3s_moves_twelve[] = {
    { U3S_KOC_NORMAL, 3, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -10, 0, 0, 0, 0, 0 },  /* 5MP - standing Strong */
    { U3S_KOC_NORMAL, 24, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -13, 0, 0, 0, 0, 0 }, /* 2HP - crouching Fierce */
    { U3S_KOC_NORMAL, 12, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -11, 0, 0, 0, 0, 0 }, /* 5MK - standing Forward */
    { U3S_KOC_NORMAL, 15, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -16, 0, 0, 0, 0, 0 }, /* 5HK - standing Roundhouse */
    { U3S_KOC_NORMAL, 30, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -10, 0, 0, 0, 0, 0 }, /* 2MK - crouching Forward */
    { U3S_KOC_NORMAL, 14, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -15, 0, 0, 0, 0, 0 }, /* 4MK - Back+Forward */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                              /* terminator */
};

/*
 * TODO(3s'): "Increase hit and block advantage of Jab A.X.E. (214LP) so that
 * it is at most -1 or -2 on block" is not in the table yet: A.X.E. is a
 * special move (U3S_KOC_ATTACK) and its index inside char_table[4] is only
 * visible in the character's command table (pass18.c uses the tech numbers
 * 0x1C and 0x1D for Twelve's two specials, A.X.E. and D.R.A., but the
 * sources do not say which of the two is which).  The hit part of that move
 * is raised through its hitstun, the block part needs the block data.
 */
