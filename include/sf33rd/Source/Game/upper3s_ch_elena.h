#ifndef UPPER3S_CH_ELENA_H
#define UPPER3S_CH_ELENA_H

#include "types.h"

/*
 * Third Strike Upper (3s') - Elena's cancels, applied through
 * upper3s/upper3s_cancel.c (the hook is in PLS00.c:check_cg_cancel_data()).
 *
 * The bits are those of the `cg_cancel` byte of a frame (see
 * upper3s_ch_q.h): 0x10 is the rapid-fire chain, which restarts the same
 * normal when its button is pressed again in the same stance
 * (PLS03.c:check_renda_cancel(); it does not need the move to have hit), 0x20
 * the special cancel and 0x40 the super cancel (both need a hit).
 */

#ifndef U3S_CANCEL_CHAIN
#define U3S_CANCEL_CHAIN 0x10
#endif
#ifndef U3S_CANCEL_SPECIAL
#define U3S_CANCEL_SPECIAL 0x20
#endif
#ifndef U3S_CANCEL_SUPER
#define U3S_CANCEL_SUPER 0x40
#endif

/* The normals the rules are about (char_ix of the assignment table). */
enum {
    U3S_ELENA_STANDING_STRONG = 3,  /* 5MP: two hits */
    U3S_ELENA_CROUCHING_SHORT = 27, /* 2LK */
};

/*
 * EL1 - "Allow her to chain crouching Short (2LK) into itself." The c.LK gets
 * the chain bit on the frames of its cancel window (the frames that carry its
 * special and super bits in the retail data), the window Ryu's and Ken's
 * c.LK chain in. Not on its startup: a chain there would restart the move
 * before it hits.
 *
 * EL3 - "Allow cancels from the 2nd hit of standing Strong, not just the first
 * hit." The retail data has the special and super bits on the first hit's
 * frames and hitstop only (measured: frames 10-15, hits on 10 and 15). The
 * second hit lands on the last active frame and its hitstop plays on the next
 * frame, which has neither an attack box nor cancel bits; the active frames
 * and the hitstops of the move get the bits.
 *
 * `frame` holds U3S_FRAME_* (upper3s.h): 0x01 active, 0x02 in a hitstop.
 */
#define U3S_ELENA_FRAME_ACTIVE_OR_HITSTOP 0x03

u8 upper3s_ch_elena_cancel_mask(s16 char_ix, s16 frame, u8 cg_cancel);

#endif
