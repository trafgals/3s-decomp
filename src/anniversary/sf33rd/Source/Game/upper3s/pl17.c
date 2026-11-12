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
    /* Hitstun so the move is not negative on hit: the hold of the defender (hs_you) that makes
     * it 0 on hit, measured in the real engine (tests/match/test_hitstun.py). */
    { U3S_KOC_NORMAL, 3, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -10, 0, 0, 0, 0, 0 }, /* s.MP (vanilla -1) */
    { U3S_KOC_NORMAL, 4, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -10, 0, 0, 0, 0, 0 }, /* far s.MP (vanilla -1) */
    { U3S_KOC_NORMAL, 6, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -12, 0, 0, 0, 0, 0 }, /* s.HP, close and far share the entry (vanilla -1) */
    /* c.HP (cix 24) hits three times; the second and third hit (entries 49 and 50) are on the
     * branch it takes after the first hit connects, which the chain walk does not follow, so they
     * are named directly. The last hit decides the advantage (vanilla -1). */
    { U3S_KOC_ENTRY, 49, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -5, 0, 0, 0, 0, 0 }, /* c.HP, second hit */
    { U3S_KOC_ENTRY, 50, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -5, 0, 0, 0, 0, 0 }, /* c.HP, third hit */
    /*
     * TW8 - "Increase hit and block advantage on Jab A.X.E. (214LP), make it at most -1/-2 on block."
     * Jab A.X.E. is special move animation 32 (the Strong and Fierce versions start in it and go on in
     * 33/34); its two hits load the attack entries 90 and 43. Measured: -1 on hit, -5 on block. Its
     * recovery is six frames of one game frame each, which cannot be shortened, so the defender's hold
     * of the last hit (hs_you, which the defender gets on hit and on block, HITCHECK.c:dm_status_copy())
     * is made 3 frames longer: -9 -> -12.
     */
    { U3S_KOC_ENTRY, 43, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -12, 0, 0, 0, 0, 0 }, /* Jab A.X.E., last hit */
    { U3S_KOC_NORMAL, 9, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -8, 0, 0, 0, 0, 0 }, /* s.LK (vanilla -1) */
    { U3S_KOC_NORMAL, 12, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -10, 0, 0, 0, 0, 0 }, /* s.MK (vanilla -1) */
    { U3S_KOC_NORMAL, 15, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -12, 0, 0, 0, 0, 0 }, /* s.HK (vanilla -1) */
    { U3S_KOC_NORMAL, 30, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -10, 0, 0, 0, 0, 0 }, /* c.MK (vanilla -1) */
    { U3S_KOC_NORMAL, 14, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -10, 0, 0, 0, 0, 0 }, /* 4MK (vanilla -1) */
    /*
     * TW4 - "Activate the earlier hitboxes in Back+Forward (4MK) that exist but cannot connect. The
     * hit spark is positioned as if the earlier low hitboxes are supposed to connect, but they can't
     * for some reason."
     *
     * Found (SF33_LIST_BLOCKS): frame 2 of the chain (game frames 5-6 of the move) has low attack
     * boxes (set 16, from the floor up to 51, reaching 75 forward), but no attack data (index 0), so no
     * attack is armed and they cannot hit. The attack starts on frame 3 (index -8: a new attack, entry
     * 8), with the higher, shorter boxes of set 18, and a hit there skips the rest of frame 3
     * (cg_extdat 0x80): the next frame is shown through the hitstop.
     *
     * The change: frame 2 starts the attack (-8) and frame 3 continues it (8), so the move still hits
     * once - on frame 2 when the opponent is in reach of the low boxes, else on frame 3. A hit on frame
     * 2 jumps to block 4, the frame a hit on frame 3 goes to (0x85: 0x80 | 5 is block 4), so the move
     * after the hit is the same; without it frame 3 played after the hitstop and 4MK went from 0 to -4
     * on hit (2 frames earlier, 2 frames longer). The whiff is unchanged.
     */
    { U3S_KOC_NORMAL, 14, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, U3S_FRAME_ATTACK, 0, -8 },     /* TW4 */
    { U3S_KOC_NORMAL, 14, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, U3S_FRAME_EXTDAT, 0, 0x85 },  /* TW4 */
    { U3S_KOC_NORMAL, 14, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, U3S_FRAME_ATTACK, -8, 8 },    /* TW4 */
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
