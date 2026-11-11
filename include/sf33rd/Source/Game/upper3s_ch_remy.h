#ifndef UPPER3S_CH_REMY_H
#define UPPER3S_CH_REMY_H

#include "types.h"

/*
 * Third Strike Upper (3s') - Remy.
 *
 * RM6 - "Allow chaining from crouching Jab->crouching Short and crouching
 * Short->crouching Jab."
 *
 * A chain into another normal is the normal cancel bit of a frame's cancel
 * byte (0x04): PLS00.c:check_cg_cancel_data() then lets check_nm_attack()
 * start the normal that is pressed, whichever it is, as long as its button
 * is not the current one. Remy's c.LP (char_ix 18) carries the rapid-fire
 * bit only (0x10, the same move again) and his c.LK (27) neither. The rule
 * gives the normal cancel bit on the frames of their cancel windows (the
 * frames with the special and super bits, 0x60), and only on a frame on which
 * the other light is pressed with the lever down, so they chain into each
 * other and into nothing else.
 */
#define U3S_REMY_CROUCHING_JAB 18
#define U3S_REMY_CROUCHING_SHORT 27

u8 upper3s_ch_remy_chain(s16 char_ix, u8 cg_cancel, u16 sw_new, u16 lever);

#endif
