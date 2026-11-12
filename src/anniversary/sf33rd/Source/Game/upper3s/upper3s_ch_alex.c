/*
 * Third Strike Upper (3s') - alex.
 *
 * The rules of this file are the cancels that the design document grants to a
 * few of Alex's normal attacks.  The cancel mask of a move is the "cg_cancel"
 * byte of its frame chain, which is read from the disc, so the rules are kept
 * here and applied by the shared hook that is documented in
 * include/sf33rd/Source/Game/upper3s_ch_alex.h.
 */

#include "sf33rd/Source/Game/upper3s_ch_alex.h"

u8 upper3s_ch_alex_cancel_mask(s16 char_ix, u8 cg_cancel) {
    switch (char_ix) {
    case U3S_ALEX_CROUCHING_STRONG:
        // The crouching Strong can be cancelled into a special move and into
        // a super art.
        return (u8)(cg_cancel | U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER);

    case U3S_ALEX_CROUCHING_FORWARD:
        // The crouching Forward can only be cancelled into a super art.
        return (u8)(cg_cancel | U3S_CANCEL_SUPER);

    default:
        return cg_cancel;
    }
}

/*
 * AL4 - the button of the follow-up that was pressed in the cancel window of
 * the c.LK, per player slot.
 */
static u8 alex_tc_pressed[2];

void upper3s_ch_alex_target_combo_buffer(s16 player, s16 char_ix, u8 cg_cancel, u16* sw_new, u16* sw_now) {
    if ((player < 0) || (player > 1)) {
        return;
    }

    if (char_ix != U3S_ALEX_CROUCHING_SHORT) {
        alex_tc_pressed[player] = 0;
        return;
    }

    if (cg_cancel & U3S_CANCEL_TARGET_COMBO) {
        // A target combo frame: the follow-up pressed earlier is pressed now. check_meoshi_cancel()
        // wants the button new (sw_new) and held (sw_now) on this frame.
        if (alex_tc_pressed[player]) {
            *sw_new |= U3S_ALEX_TC_BUTTON;
            *sw_now |= U3S_ALEX_TC_BUTTON;
            alex_tc_pressed[player] = 0;
        }

        return;
    }

    if ((cg_cancel & (U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER)) && (*sw_new & U3S_ALEX_TC_BUTTON)) {
        // The cancel window, before the target combo frames.
        alex_tc_pressed[player] = 1;
    }
}
