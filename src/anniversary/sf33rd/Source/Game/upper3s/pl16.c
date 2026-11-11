#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - q overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail values (PLCNT.c:super_arts_data[17] = q, {cmd set, gauge, stock}):
 *   SA I   { 20,  96, 2 }
 *   SA II  { 21, 120, 1 }
 *   SA III { 22, 112, 1 }   Total Destruction
 *
 * 3s' does not change any of Q's super art stocks or bar lengths.
 */

const Upper3sSuperArt upper3s_super_art_q[4] = {
    { 0, 0, 0, 0 }, /* SA I */
    { 0, 0, 0, 0 }, /* SA II */
    { 0, 0, 0, 0 }, /* SA III - Total Destruction */
    { 0, 0, 0, 0 }  /* SA IV */
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
 * 3s': "Increase hitstun on crouching Short, crouching Forward, and standing
 * Roundhouse so they are not negative on hit.  Standing Roundhouse is -10 on
 * hit, for goodness' sake."
 *
 * The advantage of a hit is the difference between how long the defender is
 * held and how long the attacker is locked, and both are read from the data
 * of the disc.  `tools/upper3s/hit_advantage.py` computes the advantage of
 * every normal out of that data, and it puts the standing Roundhouse of the
 * retail game at -10 on hit, which is the number of the design document.
 *
 * The hitstun of a move is the hold of the defender, and that is the field
 * `hs_you` of the attack data (`UNK_7.hs_you`), so that is what is raised
 * here.  The values below are the ones that make the advantage of the hit
 * zero.  The crouching Forward is already at +1 on hit in the retail game and
 * is not changed.
 */

const Upper3sMove upper3s_moves_q[] = {
    { U3S_KOC_NORMAL, 27, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0 },  /* 2LK - crouching Short */
    { U3S_KOC_NORMAL, 15, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -27, 0, 0, 0, 0, 0 }, /* 5HK - standing Roundhouse */
    /*
     * The standing Jab of Q starts in 6 frames when it is done with a
     * dash - the 6LP and the 4LP, which share their chain - and in 4
     * frames when it is done without one.  The design document asks for
     * 4 frames, so the two versions with a dash are brought to the
     * startup of the version without one.
     *
     * The data of the disc puts the 6LP and the 4LP in one chain: the slot
     * holds the same byte offset for the two entries, so one change of the
     * chain changes both of them.  The table holds an entry for each of the
     * two moves anyway, so that the table says what the patch does; the
     * second entry finds the chain that already has the startup of the
     * target and leaves it alone.
     */
    { U3S_KOC_NORMAL, 1, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 6, 4, 0, 0 },  /* 6LP - the dash standing Jab */
    { U3S_KOC_NORMAL, 2, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 6, 4, 0, 0 },  /* 4LP - the dash standing Jab */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                              /* terminator */
};
