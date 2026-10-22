#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - gill overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail values (PLCNT.c:super_arts_data[0] = gill, {cmd set, gauge, stock}):
 *   SA I   { 20, 128, 1 }
 *   SA II  { 21, 128, 1 }
 *   SA III { 22, 128, 1 }   Gill Metamorphosis
 *
 * The design spec does not name Gill in any of its balance changes, so
 * nothing is changed here yet.
 */

const Upper3sSuperArt upper3s_super_art_gill[4] = {
    { 0, 0, 0, 0 }, /* SA I */
    { 0, 0, 0, 0 }, /* SA II */
    { 0, 0, 0, 0 }, /* SA III - Gill Metamorphosis */
    { 0, 0, 0, 0 }  /* SA IV */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 */

const Upper3sMove upper3s_moves_gill[] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
