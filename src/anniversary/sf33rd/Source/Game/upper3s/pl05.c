#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - hugo overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data[6]):
 *
 *   SA I   { 20, ..., gauge_len 128, store_max 1 }  cmd20 - character specific
 *   SA II  { 21, ..., gauge_len 112, store_max 2 }  cmd21 - character specific
 *   SA III { 22, ..., gauge_len  88, store_max 2 }  cmd22 - 236+P+P
 *   SA IV  {  0, ..., gauge_len  64, store_max 1 }  unused
 *
 * The spec asks for no stock or gauge length change on Hugo.
 */

const Upper3sSuperArt upper3s_super_art_hugo[4] = {
    { 0, 0, 0, 0 }, /* SA I - retail 128 / 1 stock */
    { 0, 0, 0, 0 }, /* SA II - retail 112 / 2 stocks */
    { 0, 0, 0, 0 }, /* SA III - retail 88 / 2 stocks */
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
 * strength inside that group (0 light, 1 medium, 2 heavy).
 *
 * The three air tables hold two moves per button and not three: the koa 0 and
 * the koa 1 of a button are the same move and the koa 2 is the other one,
 * which is why the index of an air attack is the koa divided by two.
 *
 * 3s': "Increase hitstun of crouching .Jab (2LP) and crouching Short (2LK) by
 * 1-2f, to allow combos to EX Monster Lariat from farther away."
 *
 * The hitstun of a move is the hold of the defender, and that is the field
 * `hs_you` of the attack data (`UNK_7.hs_you`): `HITCHECK.c:dm_status_copy()`
 * puts it into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into
 * `hit_stop`, and `PLMAIN.c:check_hit_stop()` holds the defender for it.
 *
 * The design document asks for one or two frames more, and the upper end of
 * that range is used here, so that the link to the EX Monster Lariat is
 * reliable from the far end of the range as well.
 *
 * TODO(3s'): "Monster Lariat may be super cancelled on hit and supers have
 * to be able to juggle after it" is a cancel window (the cancel bits 0x20 /
 * 0x40 of the frame chain, PLS00.c:check_cg_cancel_data).  The 236+K of the
 * spec is Hugo's super art in the retail data (PLPAT05.c:pl05_exatt_table
 * maps the routine 16 to Att_CHOUCHUURENGEKI), so the special the designer
 * means still has to be identified before a routine can be added.
 *
 * TODO(3s'): the wrongly offset hitboxes of the first frame of the air
 * hitstun landing are hitbox data of the disc.
 *
 * TODO(3s'): "add the EX Shootdown Backbreaker" is a new move: it needs a
 * new command entry in cmd_data.c and a new move in the frame chain of the
 * disc.
 */

const Upper3sMove upper3s_moves_hugo[] = {
    { U3S_KOC_NORMAL, 18, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -11, 0, 0, 0, 0, 0 }, /* 2LP - crouching Jab */
    { U3S_KOC_NORMAL, 27, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -13, 0, 0, 0, 0, 0 }, /* 2LK - crouching Short */
    /*
     * The Giant Palm Bomber (360P) is the special move of the command 31,
     * and the four strengths are the entries 24 to 27 of the slot of the
     * special moves.  The design document asks for the EX version to
     * start in 17 frames - the number of the Jab version - and the data
     * of the disc says that the EX version starts in 22 frames.
     */
    { U3S_KOC_ATTACK, 27, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 22, 17, 0, 0 }, /* the EX Giant Palm Bomber */
    /*
     * 3s': "Reduce startup of L/M/H Meat Squasher (360K) from 21/31/44 to
     * 18/24/30; reduce recovery on whiff from 30/33/36 to 26/29/32."
     *
     * The move is the special move of the command 29: the three strengths
     * are the entries 64, 65 and 66 of the slot of the special moves.  The
     * identification does not come from the name but from the frame data:
     * the recovery on whiff of the document (30/33/36) is the number of
     * the game frames from the first frame that hits to the end of the
     * move, and the three strengths of this move are the only ones of the
     * whole cast that have that recovery (tools/upper3s/move_report.py and
     * the search of build/find_rec.py).
     *
     * The startup of the document does not match the data of the disc: the
     * disc says that the three strengths start in 24/52/67 frames and not
     * in the 21/31/44 of the document, and no move of the disc has the
     * startups of the document.  The two numbers are both written down
     * here, because the change has to be the number of the document and
     * because the guard that applies it has to hold the number of the
     * disc: the startup of the retail game is what the game does, and the
     * 21/31/44 of the document is a measurement that the data of the disc
     * does not confirm.
     *
     * The startup is therefore set to the target of the document - 18/24/30
     * - and not to the difference the document asks for applied to the
     * numbers of the disc, which would have given 21/45/53: a number that
     * is neither the retail one nor the one of the document.
     *
     * The recovery of the document is the number of the game frames from
     * the first frame that hits to the end of the move, which is one more
     * than the number of the frames that follow the last frame that hits
     * (29/32/35 in the data of the disc).  The targets of the document
     * (26/29/32) are therefore 25/28/31 frames after the last hit.
     */
    { U3S_KOC_ATTACK, 64, U3S_MV_STARTUP | U3S_MV_RECOVERY, 0, 0, 0, 0, 0, 0, 0, 24, 18, 29, 25 }, /* light */
    { U3S_KOC_ATTACK, 65, U3S_MV_STARTUP | U3S_MV_RECOVERY, 0, 0, 0, 0, 0, 0, 0, 52, 24, 32, 28 }, /* medium */
    { U3S_KOC_ATTACK, 66, U3S_MV_STARTUP | U3S_MV_RECOVERY, 0, 0, 0, 0, 0, 0, 0, 67, 30, 35, 31 }, /* heavy */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                                                /* terminator */
};
