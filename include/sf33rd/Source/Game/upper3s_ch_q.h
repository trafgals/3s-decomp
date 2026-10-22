#ifndef UPPER3S_CH_Q_H
#define UPPER3S_CH_Q_H

#include "types.h"

/*
 * Third Strike Upper (3s') - q.
 *
 * The cancel rules of a move are the "cg_cancel" byte of the frame chain of
 * the move; check_cg_cancel_data() of PLS00.c reads it every frame:
 *
 *     0x01  the super jump cancel
 *     0x02  the forward and the back dash cancel
 *     0x04  the cancel into another normal attack
 *     0x08  the target combo (meoshi) cancel
 *     0x10  the chain (renda) cancel
 *     0x20  the cancel into a special move
 *     0x40  the cancel into a super art
 *
 * The byte comes from the disc, so the cancels that the design document grants
 * to a few of Q's normals are not in this repository.  The rules are kept here
 * instead, in the same shape as the rules the retail game itself has for
 * Ibuki and for Twelve (PLS00.c:check_cg_cancel_data(), the switch on
 * wk->player_number).
 *
 * The hook that applies the rules is in the shared file PLS00.c, in
 * check_cg_cancel_data(), right at the top of the function, before the test
 * that gives up when the mask of the disc is empty (PLS00.c:1822):
 *
 *     #if defined(UPPER_3S)
 *         wk->wu.cg_cancel = upper3s_cancel_mask(wk->player_number, wk->wu.now_koc, wk->as->char_ix,
 *                                                 wk->wu.cg_cancel);
 *     #endif
 *
 * The move is addressed by the kind of character data it lives in (the slot 4
 * holds the normal attacks) and by its char_ix.  The routine number of the
 * state machine cannot be used: it is the behaviour class of the normal, and
 * it is the same for the standing and for the crouching Medium Punch.
 *
 * The hook has to be placed before that test, because a move whose mask of the
 * disc is empty never reaches the tests of the cancels (PLS00.c:1860 for the
 * super art, PLS00.c:1877 for the special move).
 *
 * The hook gives the cancel for the whole move.  The retail gives it for the
 * frames of the chain of the move that carry the byte, so the exact timing of
 * the design document is a change of the disc data: the byte of the chain is
 * the value that the rule of this file reproduces.
 *
 * See docs/upper3s/spec-map.md for the row of q.
 */

#ifndef U3S_CANCEL_SUPERJUMP
#define U3S_CANCEL_SUPERJUMP 0x01
#endif
#ifndef U3S_CANCEL_DASH
#define U3S_CANCEL_DASH 0x02
#endif
#ifndef U3S_CANCEL_NORMAL
#define U3S_CANCEL_NORMAL 0x04
#endif
#ifndef U3S_CANCEL_TARGET_COMBO
#define U3S_CANCEL_TARGET_COMBO 0x08
#endif
#ifndef U3S_CANCEL_CHAIN
#define U3S_CANCEL_CHAIN 0x10
#endif
#ifndef U3S_CANCEL_SPECIAL
#define U3S_CANCEL_SPECIAL 0x20
#endif
#ifndef U3S_CANCEL_SUPER
#define U3S_CANCEL_SUPER 0x40
#endif

/*
 * The index of a normal attack inside the slot 4 of the character data.
 *
 * The index is the char_ix of the assignment table of the disc
 * (bin2obj/asstbl.c), and it is the same for every character:
 *
 *   5LP  0    6LP  1    4LP  2     5MP  3    6MP  4    4MP  5
 *   5HP  6    6HP  7    4HP  8     5LK  9    6LK 10    4LK 11
 *   5MK 12    6MK 13    4MK 14     5HK 15    6HK 16    4HK 17
 *   2LP 18    3LP 19    1LP 20     2MP 21    3MP 22    1MP 23
 *   2HP 24    3HP 25    1HP 26     2LK 27    3LK 28    1LK 29
 *   2MK 30    3MK 31    1MK 32     2HK 33    3HK 34    1HK 35
 */
enum {
    U3S_Q_STANDING_SHORT = 9,   /* 5LK */
    U3S_Q_CROUCHING_JAB = 18,   /* 2LP */
    U3S_Q_CROUCHING_SHORT = 27, /* 2LK */
    U3S_Q_CROUCHING_FORWARD = 30 /* 2MK */
};

/*
 * The cancel mask of a move of q, with the cancels that Third Strike Upper
 * grants to it.
 *
 *   - "Allow crouching Jab, standing Short and crouching Short to be special-
 *      and super-cancelled on hit/block/parry."
 *   - "Allow crouching Forward to be super-cancelled on hit/block/parry."
 *
 * The index is the char_ix of the move in the slot 4 of the character data,
 * that is, the index of the move in the assignment table of the disc.
 */
u8 upper3s_ch_q_cancel_mask(s16 char_ix, u8 cg_cancel);

#endif /* UPPER3S_CH_Q_H */
