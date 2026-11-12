#ifndef UPPER3S_CH_IBUKI_H
#define UPPER3S_CH_IBUKI_H

#include "types.h"

/*
 * Third Strike Upper (3s') - the changes of Ibuki.
 *
 * Ibuki is the character 7 of the game (AGENTS.md section 3.1).
 *
 * The state of the patch is not kept in PLW or in WORK: both structures are
 * part of the PS2 memory map and may not grow.  Every helper of this header
 * is a pure function of its arguments, so that it can be exercised on
 * synthetic state (tests/upper3s/test_upper3s_ch_ibuki.c).
 */

/*
 * Ibuki - "Remove the ability to throw-tech her Personal Action (taunt)".
 *
 * The retail game opens a throw tech window while the personal action is
 * played: PLPAT07.c:Att_PL07_TOKUSHUKOUDOU adds 14 to PLW.tk_dageki and to
 * PLW.tk_nage when the personal action reaches the frame that opens the
 * window, and clamps both of them to 28 right afterwards, so that the
 * personal action can be teched for 14 frames.
 *
 * The design document wants the personal action of Ibuki to be unteachable.
 * The window is therefore never opened: both counters stay at zero, and a
 * window that would still be open when the personal action starts is closed.
 *
 * The return value is the length of the window, in frames.
 */
u8 upper3s_ibuki_personal_action_tech_window(void);

/*
 * The number of frames the retail game adds to the throw tech counters of
 * the personal action, and the upper bound it clamps them to.  Kept here so
 * that the retail values are documented in one place only.
 */
#define U3S_IBUKI_RETAIL_TECH_WINDOW 14
#define U3S_IBUKI_RETAIL_TECH_WINDOW_MAX 28

/*
 * Ibuki - IB3, "make Kunai cancelled from it [Tsuiji Goe] face the correct
 * direction if it hasn't passed over the opponent yet".
 *
 * Tsuiji Goe (623P, special animations 107-110) is a jump over the opponent:
 * Ibuki turns round as she leaves the ground (her facing, rl_flag, flips at
 * takeoff), and a move cancelled from it starts with the facing kept in
 * rl_waza (the kunai's routine sets rl_flag = rl_waza on its first frame),
 * which still points away from the opponent until she has passed over them.
 * Measured: a kunai cancelled before she passes over flies away from the
 * opponent. The fix recomputes rl_waza from the two positions
 * (PLS01.c:set_rl_waza()) and turns her to it (rl_flag) when a special or a
 * super starts out of Tsuiji Goe in the air (PLS03.c:hissatsu_setup_union()),
 * so the kunai, and the kunai of her air super, face the opponent on either
 * side. (Setting rl_waza alone changed nothing in a fight: the kunai keeps
 * rl_flag.)
 *
 * Returns 1 when the facing is to be recomputed: the player is Ibuki, in a
 * special move (koc 5) that is Tsuiji Goe, and in the air.
 */
int upper3s_ibuki_cancel_faces_opponent(s16 player_number, s16 koc, s16 char_index, s16 height);

#endif
