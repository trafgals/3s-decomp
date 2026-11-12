/*
 * Third Strike Upper (3s') - elena.
 *
 * The cancels the design document grants to two of Elena's normals (EL1,
 * EL3); see include/sf33rd/Source/Game/upper3s_ch_elena.h.
 */

#include "sf33rd/Source/Game/upper3s_ch_elena.h"

u8 upper3s_ch_elena_cancel_mask(s16 char_ix, s16 frame, u8 cg_cancel) {
    switch (char_ix) {
    case U3S_ELENA_CROUCHING_SHORT:
        // EL1: the c.LK chains into itself in its cancel window.
        if (cg_cancel & (U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER)) {
            return (u8)(cg_cancel | U3S_CANCEL_CHAIN);
        }

        return cg_cancel;

    case U3S_ELENA_STANDING_STRONG:
        // EL3: the second hit can be cancelled as well as the first.
        if (frame & U3S_ELENA_FRAME_ACTIVE_OR_HITSTOP) {
            return (u8)(cg_cancel | U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER);
        }

        return cg_cancel;

    default:
        return cg_cancel;
    }
}
