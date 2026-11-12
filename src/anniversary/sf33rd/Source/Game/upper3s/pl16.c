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
 * variant. For the standing normals the base is 0: koa 0 is the close
 * standing normal (or the only one), 1 the far one and 2 the command normal
 * of the button (6X or 4X, depending on the character); a variant a
 * character does not have points at the chain of another one. Measured in
 * the fight harness (the "cix" of the log): Remy's close s.MK is 12 and his
 * far s.MK 13, Twelve's 4MK and Remy's 6MK are 14, and Q's far s.LP is 1.
 * (An earlier version of this comment read koa as a dash forward or back.)
 * For the crouching normals the base is 18 and koa 0 is 2X; what koa 1 and
 * 2 hold there was not checked.
 *
 *            close  far  command          close  far  command
 *   s.LP       0     1     2       s.MP     3     4     5
 *   s.HP       6     7     8       s.LK     9    10    11
 *   s.MK      12    13    14       s.HK    15    16    17
 *   c.LP 18   c.MP 21   c.HP 24   c.LK 27   c.MK 30   c.HK 33
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
    /* Hitstun so the move is not negative on hit: the hold of the defender (hs_you) that makes
     * it 0 on hit, measured in the real engine (tests/match/test_hitstun.py). */
    { U3S_KOC_NORMAL, 27, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0 }, /* c.LK (vanilla -2) */
    { U3S_KOC_NORMAL, 30, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -11, 0, 0, 0, 0, 0 }, /* c.MK (vanilla -2) */
    { U3S_KOC_NORMAL, 15, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -27, 0, 0, 0, 0, 0 }, /* s.HK, close and far share the entry (vanilla -10) */
    /*
     * Q4 - "Decrease the startup of far standing Jab from 6f to 4f". The far
     * s.LP (koa 1, normal 1) starts in 6 frames; the close one (0) in 4. Q
     * has no command normal on LP, so normal 2 points at the far s.LP's chain:
     * the slot holds the same byte offset for the two entries, so one change of
     * the chain changes both. The table holds a row for each anyway, so that it
     * says what the patch does; the second row finds the chain that already has
     * the target startup and leaves it alone. (An earlier version of this
     * comment called them the 6LP and the 4LP, "with a dash".)
     */
    { U3S_KOC_NORMAL, 1, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 6, 4, 0, 0 },  /* far s.LP: 6 -> 4 */
    { U3S_KOC_NORMAL, 2, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 6, 4, 0, 0 },  /* normal 2: the far s.LP's chain */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                              /* terminator */
};
