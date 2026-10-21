/*
 * Third Strike Upper (3s') - q.
 *
 * The rules of this file are the cancels that the design document grants to a
 * few of Q's normal attacks.  The cancel mask of a move is the "cg_cancel"
 * byte of its frame chain, which is read from the disc, so the rules are kept
 * here and applied by the shared hook that is documented in
 * include/sf33rd/Source/Game/upper3s_ch_q.h.
 */

#include "sf33rd/Source/Game/upper3s_ch_q.h"

u8 upper3s_ch_q_cancel_mask(s16 char_ix, u8 cg_cancel) {
    switch (char_ix) {
    case U3S_Q_STANDING_SHORT:
    case U3S_Q_CROUCHING_JAB:
    case U3S_Q_CROUCHING_SHORT:
        // The crouching Jab, the standing Short and the crouching Short can
        // be cancelled into a special move and into a super art.
        return (u8)(cg_cancel | U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER);

    case U3S_Q_CROUCHING_FORWARD:
        // The crouching Forward can only be cancelled into a super art.
        return (u8)(cg_cancel | U3S_CANCEL_SUPER);

    default:
        return cg_cancel;
    }
}
