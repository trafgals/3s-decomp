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

s16 upper3s_ch_q_taunt_konjyou(s16 konjyou) {
    // Q12: the bonus after one more taunt (see upper3s_ch_q.h).
    if (konjyou < U3S_Q_TAUNT_KONJYOU_1) {
        return U3S_Q_TAUNT_KONJYOU_1;
    }

    if (konjyou < U3S_Q_TAUNT_KONJYOU_2) {
        return U3S_Q_TAUNT_KONJYOU_2;
    }

    return U3S_Q_TAUNT_KONJYOU_3;
}

u8 upper3s_ch_q_special_cancel_mask(s16 char_ix, u8 cg_cancel) {
    // Q3: the low ([4]6+K) and the overhead ([4]6+P held) rushes can be
    // cancelled into a super art, like the mid rush in the retail game.
    if ((char_ix >= U3S_Q_RUSH_LOW_FIRST) && (char_ix <= U3S_Q_RUSH_OVERHEAD_LAST)) {
        return (u8)(cg_cancel | U3S_CANCEL_SUPER);
    }

    return cg_cancel;
}

u8 upper3s_ch_q_active_cancel_mask(s16 char_ix, s16 frame, u8 cg_cancel) {
    // Q5: every active frame of 4HK, and the hitstop after its hit, can be cancelled into a special
    // move or a super art. (0x03: U3S_FRAME_ACTIVE | U3S_FRAME_HITSTOP of upper3s.h.)
    if ((char_ix == U3S_Q_BACK_ROUNDHOUSE) && (frame & 0x03)) {
        return (u8)(cg_cancel | U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER);
    }

    return cg_cancel;
}
