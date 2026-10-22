#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - alex overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data[1]):
 *
 *   SA I   { 20, ..., gauge_len 128, store_max 1 }  cmd20
 *   SA II  { 21, ..., gauge_len  96, store_max 2 }  cmd21
 *   SA III { 22, ..., gauge_len  80, store_max 1 }  cmd22
 *   SA IV  {  0, ..., gauge_len  64, store_max 1 }  unused
 *
 * The spec asks for no stock or gauge length change on Alex.
 */

const Upper3sSuperArt upper3s_super_art_alex[4] = {
    { 0, 0, 0, 0 }, /* SA I - retail 128 / 1 stock */
    { 0, 0, 0, 0 }, /* SA II - retail 96 / 2 stocks */
    { 0, 0, 0, 0 }, /* SA III - retail 80 / 1 stock */
    { 0, 0, 0, 0 }  /* SA IV - unused */
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
 * strength inside that group (0 light, 1 medium, 2 heavy).  Which of the five
 * tables holds the standing, crouching and jumping normals is selected by
 * wk->wu.pat_status in PLS03.c:check_nm_attack.
 *
 * The three air tables hold two moves per button and not three: the koa 0 and
 * the koa 1 of a button are the same move and the koa 2 is the other one,
 * which is why the index of an air attack is the koa divided by two.
 *
 * TODO(3s'): every Alex item of the spec is a property of the on disc frame
 * chain or attack table and cannot be expressed with this table:
 *
 *   - "Hyper Bomb startup 0+1 -> 1+0" and "the target combo cancel window
 *     must be longer than 1 frame" are the startup/cancel entries of the
 *     frame chain (wk->wu.cg_cancel and the chain block times).
 *   - "Air Knee Smash must not be air parryable" is the guard flag of the
 *     attack data (UNK_7.guard bit 0x10 blocks the parry, bit 0x08 allows
 *     it, see Com_Pl.c:Check_Guard_Type and HITCHECK.c:set_paring_status).
 *   - "crouching Strong may be special and super cancelled" and "crouching
 *     Forward may be super cancelled" are the cancel bits 0x20 / 0x40 of the
 *     frame chain (PLS00.c:check_cg_cancel_data).
 *   - "the juggle after Flash Chops", "Air Stampede has to knock down",
 *     "the anchor point and pushbox of the air knockdown animation" and
 *     "the hitboxes of the dive punch" are hitbox and trajectory data of
 *     the disc.
 *
 * TODO(3s'): "Boomerang Raid has to track forward on its middle hits and
 * reach further with the final grab" is movement data (wk->wu.mvxy) of the
 * disc.
 */

const Upper3sMove upper3s_moves_alex[] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
