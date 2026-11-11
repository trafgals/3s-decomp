#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - sean overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail values (PLCNT.c:super_arts_data[12] = sean, {cmd set, gauge, stock}):
 *   SA I   { 20,  72, 3 }
 *   SA II  { 21,  96, 2 }
 *   SA III { 22, 120, 1 }   Hyper Tornado
 *
 * 3s' does not change any of Sean's super art stocks or bar lengths.
 */

const Upper3sSuperArt upper3s_super_art_sean[4] = {
    { 0, 0, 0, 0 }, /* SA I */
    { 0, 0, 0, 0 }, /* SA II */
    { 0, 0, 0, 0 }, /* SA III - Hyper Tornado */
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
 * 3s': "Increase hitstun of close and far Fierce, crouching Fierce, standing
 * Forward, far Roundhouse, crouching Forward, and Towards+Fierce (6HP) to
 * not be negative on hit."
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
 * The standing Forward and the far Roundhouse are already at zero or better
 * in the retail data and are not changed.
 */

const Upper3sMove upper3s_moves_sean[] = {
    { U3S_KOC_NORMAL, 7, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -24, 0, 0, 0, 0, 0 },  /* 6HP - close / Towards+Fierce */
    { U3S_KOC_NORMAL, 8, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -26, 0, 0, 0, 0, 0 },  /* 4HP - far Fierce */
    { U3S_KOC_NORMAL, 24, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -10, 0, 0, 0, 0, 0 }, /* 2HP - crouching Fierce */
    { U3S_KOC_NORMAL, 30, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -12, 0, 0, 0, 0, 0 }, /* 2MK - crouching Forward */
    /*
     * The Ryuubi Kyaku (236K) is the special move of the command 30, and
     * the three strengths are the entries 32, 33 and 34 of the slot of the
     * special moves.  The data of the disc puts the three of them in ONE
     * chain: the slot holds the same byte offset for the three entries.
     * That chain holds the three frames that hit - one per strength, with
     * the attack data 100, 101 and 102 of the attack table, which have the
     * same damage and the same stun and a different hit box - and the wind
     * up in front of them.
     *
     * A change of the chain is a change of all three strengths, so the
     * table holds one entry and not three.
     *
     * 3s': "Decrease the startup of L/M/H Ryuubi Kyaku (236K) from all
     * being 25f to 17/19/21f."  The data of the disc says that the three
     * of them start in 25 frames, which is what the document says as well.
     * The three targets of the document cannot be told apart in the data of
     * the disc, because the three strengths share one chain: a change of
     * that chain changes the three of them at once.  The middle target of
     * the document, 19 frames, is used - it is the target of the medium
     * version and it is the value that is the closest to all three of them.
     */
    { U3S_KOC_ATTACK, 32, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 25, 19, 0, 0 }, /* light, medium and heavy */
    /*
     * 3s': "Reduce startup of L/M/H/EX Dragon Smash (623P) from 5/5/9/8f to
     * 3/4/5/3f.  Somewhere between Ken and Ryu."
     *
     * The Dragon Smash is the special move of the command 29, and the four
     * strengths are the entries 64 to 67 of the slot of the special moves.
     * The data of the disc says that they start in 5/7/9/8 frames; the
     * document gives 5/5/9/8, which is the same three of the four and a
     * slip on the medium.  The targets of the document (3/4/5/3) are used
     * as they are: they are below the numbers of the disc and they keep
     * the progression of the strengths (three, four and five frames).
     */
    { U3S_KOC_ATTACK, 64, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 5, 3, 0, 0 }, /* light */
    { U3S_KOC_ATTACK, 65, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 7, 4, 0, 0 }, /* medium */
    { U3S_KOC_ATTACK, 66, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 9, 5, 0, 0 }, /* heavy */
    /*
     * The EX version is the one the document cannot have: it asks for three
     * frames, but the startup of the EX version is five frames of the chain
     * and a frame of a chain cannot be held for less than one game frame.
     * Five frames - the shortest the chain of the move can be made - is
     * used instead.
     */
    { U3S_KOC_ATTACK, 67, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 8, 5, 0, 0 }, /* EX */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                              /* terminator */
};

/*
 * TODO(3s'): "Increase hit advantage of L/M versions of Ryuubi Kyaku (236K),
 * to allow links to supers after hitting" is not in the table yet: the
 * Ryuubi Kyaku is a special move (U3S_KOC_ATTACK) and the indices of the
 * moves are known now - the Ryuubi Kyaku is the special move of the
 * command 30 - but the hit advantage of a special move is not read out of
 * the attack table alone.
 */
