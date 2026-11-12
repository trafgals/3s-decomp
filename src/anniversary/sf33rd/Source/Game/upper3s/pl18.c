#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - remy overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail values (PLCNT.c:super_arts_data[19] = remy, {cmd set, gauge, stock}):
 *   SA I   { 20, 104, 2 }
 *   SA II  { 22, 120, 2 }
 *   SA III { 21,  80, 1 }   Blue Nocturne
 *
 * 3s' changes:
 *   SA III - the bar of Blue Nocturne becomes a bit longer (80 -> 96, the
 *            same length as his SA I).
 */

const Upper3sSuperArt upper3s_super_art_remy[4] = {
    { 0, 0, 0, 0 },                /* SA I */
    { 0, 0, 0, 0 },                /* SA II */
    { U3S_SA_GAUGE_LEN, 0, 96, 0 }, /* SA III - Blue Nocturne */
    { 0, 0, 0, 0 }                 /* SA IV */
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
 * 3s' changes:
 *   - "Increase damage of crouching Strong by 6pt to almost match Ryu's."
 *   - "Increase hitstun of far Forward, far Roundhouse, crouching Forward,
 *     and Towards+Forward (6MK) so that they are not negative on hit."
 *
 * The damage of the crouching Strong of the retail game is 55 (the attack
 * data of the disc puts it at the eleventh step of POW_DATA.c:Power_Data).
 * The design document asks for six points more, and six points more is what
 * is done: the damage is scaled by 1110 permille, which turns the 55 of the
 * retail game into 61.
 *
 * The remark of the design document that the move should "almost match
 * Ryu's" does not hold for the retail data: the crouching Strong of Ryu does
 * 90, so the gap between the two characters is 35 points and not six.  The
 * six points are the number the document gives, and they are what is applied.
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
 */

const Upper3sMove upper3s_moves_remy[] = {
    /* RM2 "Increase damage of crouching Strong by 6pt to almost match Ryu's." The document's
     * points are the attack's `pow` (Power_Data[pow] = 5 * pow): Remy's c.MP is pow 11, Ryu's is
     * 18, and 11 + 6 = 17 "almost matches". So the base damage is Power_Data[17] = 85 (retail 55).
     * An earlier version scaled it by 1110 permille, about +1 point. */
    { U3S_KOC_NORMAL, 21, U3S_MV_DAMAGE, 85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* c.MP */
    /* RM10, the Cold Blue Kick (63214K): "Increase hit advantage of Short/Forward Cold Blue
     * Kicks so they are not negative on hit from up close; make Forward version +2". Both are
     * -4 on hit up close in the retail game, with a hold of 12 (koc 5, the specials: cix 46 and 47,
     * measured in the real engine by tests/match/test_hitstun.py). */
    { U3S_KOC_ATTACK, 46, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -16, 0, 0, 0, 0, 0 }, /* LK Cold Blue Kick: -4 -> 0 */
    { U3S_KOC_ATTACK, 47, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -18, 0, 0, 0, 0, 0 }, /* MK Cold Blue Kick: -4 -> +2 */
    /* Hitstun so the move is not negative on hit: the hold of the defender (hs_you) that makes
     * it 0 on hit, measured in the real engine (tests/match/test_hitstun.py). */
    { U3S_KOC_NORMAL, 13, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -11, 0, 0, 0, 0, 0 }, /* far s.MK (vanilla -2) */
    /*
     * RM5 (the knockback) - "Decrease grounded opponent knockback on standing Forward (MK)". The post
     * gives no amount. The knockback of a hit is its `impact`, an index into _select_hit_dsd: 16-31
     * are the steps of one family, so the close s.MK's 17 is one step above the least, 16. Measured:
     * Ken slides 66 apart 30 frames after the hit instead of 82; the hit advantage (+3) is unchanged.
     * The close s.MK (normal 12) is the one the target combo starts from; the far one (13), which the
     * post does not tie to the combo, keeps its knockback. Lower values are another family (14: 37
     * apart, 10: 124), so they are not "less of the same knockback".
     */
    { U3S_KOC_NORMAL, 12, U3S_MV_IMPACT, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0 }, /* RM5: close s.MK, impact 17 -> 16 */
    { U3S_KOC_NORMAL, 16, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -18, 0, 0, 0, 0, 0 }, /* far s.HK (vanilla -7) */
    { U3S_KOC_NORMAL, 30, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -17, 0, 0, 0, 0, 0 }, /* c.MK (vanilla -8) */
    { U3S_KOC_NORMAL, 14, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -15, 0, 0, 0, 0, 0 }, /* 6MK (vanilla -6) */
    /*
     * RM7 - "Decrease total length of Personal Action (taunt) from 131f to ~80f."
     * The taunt (HP+HK) is special move animation 36. Measured: Remy is in the attack state for
     * 131 frames (the post's number), then 5 actionable frames to the end of the chain, 135 frames
     * in all: 17 frames of 5, a pose of 40 (from frame 85), 2 frames of 5. The chain is brought to
     * 84 frames, so the taunt lasts 80; the longest frame, the pose, is shortened first.
     */
    { U3S_KOC_ATTACK, 36, U3S_MV_TAIL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 135, 84 }, /* the taunt */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                              /* terminator */
};

