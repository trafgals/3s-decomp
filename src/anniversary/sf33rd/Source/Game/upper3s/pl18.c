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
    { U3S_KOC_NORMAL, 21, U3S_MV_DAMAGE_PERMILLE, 0, 1110, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* 2MP - crouching Strong */
    { U3S_KOC_NORMAL, 14, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -24, 0, 0, 0, 0, 0 }, /* 4MK - far Forward */
    { U3S_KOC_NORMAL, 17, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -28, 0, 0, 0, 0, 0 }, /* 4HK - far Roundhouse */
    { U3S_KOC_NORMAL, 30, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -33, 0, 0, 0, 0, 0 }, /* 2MK - crouching Forward */
    { U3S_KOC_NORMAL, 13, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -30, 0, 0, 0, 0, 0 }, /* 6MK - Towards+Forward */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                              /* terminator */
};

/*
 * TODO(3s'): "Increase hit advantage of Short/Forward Cold Blue Kicks
 * (63214K) so they are not negative on hit from up close; make Forward
 * version +2" is not in the table yet: the Cold Blue Kick is a special move
 * (U3S_KOC_ATTACK) and its index inside char_table[4] is only visible in
 * the character's command table (pass19.c uses the tech numbers 0x1C-0x1F
 * for Remy's four specials, but the sources do not say which one is the
 * Cold Blue Kick).
 */
