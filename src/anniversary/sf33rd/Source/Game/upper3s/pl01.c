#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - ryu overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data[2]):
 *
 *   SA I   { 20, ..., gauge_len 112, store_max 2 }  cmd20 - 236+P  (Hadoken)
 *   SA II  { 22, ..., gauge_len 128, store_max 1 }  cmd22 - 236+P+P  (Shinkuu Hadoken)
 *   SA III { 21, ..., gauge_len  96, store_max 1 }  cmd21 - 623+P  (Shin Shoryuken)
 *   SA IV  {  0, ..., gauge_len   0, store_max 1 }  unused
 *
 * The spec asks for no stock or gauge length change on Ryu.
 */

const Upper3sSuperArt upper3s_super_art_ryu[4] = {
    { 0, 0, 0, 0 }, /* SA I - retail 112 / 2 stocks */
    { 0, 0, 0, 0 }, /* SA II - retail 128 / 1 stock */
    { 0, 0, 0, 0 }, /* SA III - retail 96 / 1 stock */
    { 0, 0, 0, 0 }  /* SA IV - retail 0 / 1 stock */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * Index layout of the normal moves (bin2obj/asstbl.c):
 *
 *   _asstbl_lv_0010[player][kos][koa] -> char_ix  0 + kos * 3 + koa
 *   _asstbl_lv_1010[player][kos][koa] -> char_ix 18 + kos * 3 + koa
 *   _asstbl_lv_2010[player][kos][koa] -> char_ix 36 + kos * 3 + koa / 2
 *   _asstbl_lv_3010[player][kos][koa] -> char_ix 48 + kos * 3 + koa / 2
 *   _asstbl_lv_4010[player][kos][koa] -> char_ix 60 + kos * 3 + koa / 2
 *
 * kos is the button group (0 LP, 1 MP, 2 HP, 3 LK, 4 MK, 5 HK) and koa the
 * strength inside that group (0 light, 1 medium, 2 heavy).
 *
 * The three air tables hold two moves per button and not three: the koa 0 and
 * the koa 1 of a button are the same move and the koa 2 is the other one,
 * which is why the index of an air attack is the koa divided by two.
 *
 * TODO(3s'): "increase the hitstun of the far standing Forward, the
 * standing Roundhouse, the close Fierce, the crouching Fierce, the
 * crouching Short and the crouching Forward so that they are not negative on
 * hit" is implemented below.
 *
 * The advantage of a hit is not written down anywhere: it is the difference
 * between how long the defender is held and how long the attacker is locked.
 * Both are read from the data of the disc, and `tools/upper3s/hit_advantage.py`
 * computes the advantage of every normal of every character out of it.  The
 * hitstun of a move is the hold of the defender, and that is the field
 * `hs_you` of the attack data (`UNK_7.hs_you`), so that is what is raised
 * here.
 *
 * The values below are the ones that make the advantage of the hit zero, the
 * end state the design document asks for ("not negative on hit").  The three
 * other moves of the list are already at zero or better in the retail data
 * and are not changed:
 *
 *   6MK  far standing Forward   +1 on hit
 *   5HK  standing Roundhouse     0 on hit
 *   2LK  crouching Short        +2 on hit
 *
 * TODO(3s'): "less knockback on the forward Strong", "more forward movement
 * on the second hit of the target combo", "the hitbox of Joudan Sokutou
 * Geri has to reach further down" and "less knockup height on the Dragon
 * Punch and on the middle hits of Shinkuu Hadoken" are knockback, movement
 * and hitbox data of the disc (wk->wu.mvxy and the attack table).
 */

const Upper3sMove upper3s_moves_ryu[] = {
    { U3S_KOC_NORMAL, 6, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -12, 0, 0, 0, 0, 0 },  /* 5HP - close Fierce */
    { U3S_KOC_NORMAL, 24, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -12, 0, 0, 0, 0, 0 }, /* 2HP - crouching Fierce */
    { U3S_KOC_NORMAL, 30, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -11, 0, 0, 0, 0, 0 }, /* 2MK - crouching Forward */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                              /* terminator */
};
