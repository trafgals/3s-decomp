#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - necro overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data[5]):
 *
 *   SA I   { 20, ..., gauge_len 88, store_max 1 }  cmd20 - 236+P
 *   SA II  { 22, ..., gauge_len 104, store_max 1 }  cmd22 - 236+P+P
 *   SA III { 21, ..., gauge_len  72, store_max 2 }  cmd21 - 623+P
 *   SA IV  {  0, ..., gauge_len  64, store_max 1 }  unused
 *
 * The spec asks for no stock or gauge length change on Necro.
 */

const Upper3sSuperArt upper3s_super_art_necro[4] = {
    { 0, 0, 0, 0 }, /* SA I - retail 88 / 1 stock */
    { 0, 0, 0, 0 }, /* SA II - retail 104 / 1 stock */
    { 0, 0, 0, 0 }, /* SA III - retail 72 / 2 stocks */
    { 0, 0, 0, 0 }  /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * TODO(3s'): "the startup of Slam Dance has to change from 0+2 to 2+0" is
 * the startup block of the frame chain of the disc (the "0+2" is the
 * superflash time plus the first active frames), so it cannot be expressed
 * with this table.
 *
 * TODO(3s'): "the pushbox of Electric Shock is wrong and the middle hits
 * have to vacuum the opponent in" is pushbox and movement data of the disc
 * (wk->wu.mvxy and the hitbox entries).
 *
 * TODO(3s'): "add the EX Electric Shock" is a new move: it needs a new
 * command entry in cmd_data.c and a new move in the frame chain of the
 * disc, so there is no char_ix to point at yet.
 */

const Upper3sMove upper3s_moves_necro[] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
