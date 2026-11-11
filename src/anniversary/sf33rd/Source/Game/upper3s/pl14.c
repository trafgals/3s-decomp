#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - chunli overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail values (PLCNT.c:super_arts_data[15] = chunli, {cmd set, gauge, stock}):
 *   SA I   { 20,  88, 1 }   236+P        (Houyoku-sen, single hit)
 *   SA II  { 21, 104, 2 }   623+P        (Houyoku-sen)
 *   SA III { 22,  72, 3 }   236+P+P      (Tensei Ranka, multi hit)
 *
 * 3s' changes:
 *   SA II  - the stock cost goes from 2 to 1 and the bar becomes the longest
 *            one in the game (0x80 = 128), the same length as Shin-Shoryuken.
 *   SA III - the stock cost goes from 3 to 2 and the bar becomes as long as
 *            Shippu Jinrai Kyaku (80).
 */

const Upper3sSuperArt upper3s_super_art_chunli[4] = {
    { 0, 0, 0, 0 },                                     /* SA I */
    { U3S_SA_STORE_MAX | U3S_SA_GAUGE_LEN, 1, 128, 0 }, /* SA II - Houyoku-sen */
    { U3S_SA_STORE_MAX | U3S_SA_GAUGE_LEN, 2, 80, 0 },  /* SA III - Tensei Ranka */
    { 0, 0, 0, 0 }                                      /* SA IV */
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
 * 3s': "Increase hit advantage of Short/Forward Cold Blue Kicks (63214K) so
 * they are not negative on hit from up close; make Forward version +2 so that
 * supers can be linked off it at point-blank."
 *
 * The Cold Blue Kick is the special 29 of the retail data, in its four
 * versions: the light one (the move 28 of the slot 5), the medium one (29),
 * the heavy one (30) and the EX one (31, the 63214K of the document).
 *
 * The advantage of a hit is the difference between how long the defender is
 * held and how long the attacker is locked, and both are read from the data
 * of the disc.  `tools/upper3s/hit_advantage.py` computes the advantage of
 * every move out of that data.
 *
 * The hitstun of a move is the hold of the defender, and that is the field
 * `hs_you` of the attack data (`UNK_7.hs_you`), so that is what is raised
 * here.  The two versions of the Cold Blue Kick that are negative on hit in
 * the retail game are the light one (-10 on hit) and the medium one (-10);
 * both are raised to +2, which is the end state the document asks for.  The
 * heavy version (+10) and the EX version (+4) are already past it and are
 * not changed.
 *
 * TODO(3s'): "Tensei Ranka (SA III) - advantage on knockdown increased enough
 * to allow crossup attempts afterward" needs the index of the super inside
 * char_table[5] (U3S_KOC_SUPER); that index is not visible from the sources
 * (it comes from the disc data), so the entry is left out for now.
 */

const Upper3sMove upper3s_moves_chunli[] = {
    { U3S_KOC_ATTACK, 28, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -17, 0, 0, 0, 0, 0 }, /* 632LK - the light Cold Blue Kick */
    { U3S_KOC_ATTACK, 29, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -15, 0, 0, 0, 0, 0 }, /* 632MK - the medium Cold Blue Kick */
    /*
     * The crouching Jab (2LP) is the only normal of the whole cast that
     * starts in 2 frames, and the design document asks for 3 frames: it
     * is usable as a crouch tech, and a throw is still a 2 frame punish.
     */
    { U3S_KOC_NORMAL, 18, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 0 }, /* 2LP - the crouching Jab */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                                /* terminator */
};
