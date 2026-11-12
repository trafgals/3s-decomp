#ifndef UPPER3S_CH_SEAN_H
#define UPPER3S_CH_SEAN_H

#include "structs.h"
#include "types.h"

/*
 * Third Strike Upper (3s') - Sean.
 *
 * SN6 - "Allow Fierce Tackle to be faked/cancelled by tapping K during
 * startup."
 *
 * The Tackle is 41236+P (cmd_data.c), a dash that grabs (a catch box, not an
 * attack box: `cg_ja.caix`). Measured: it is special move animation 24 for
 * Jab, 25 then 24 for Strong, 26 then 24 for Fierce and 27 then 24 for EX;
 * the attack routine is PLPATUNI.c:Att_CHOUCHUURENGEKI(). Its startup is the
 * dash before the first frame with a catch box.
 *
 * `upper3s_ch_sean_tackle_feint` is called by PLPAT12.c:pl12_extra_attack()
 * on every frame of a special move of Sean's. It returns 1 when a kick was
 * pressed in the startup of the Fierce Tackle; the caller then ends the move
 * the way the retail game ends one (PLS00.c:jumping_cg_type_check(), cg_type
 * 0xFF), after the kick has been taken out of the new presses so that it
 * does not start a kick at once.
 */
#define U3S_SEAN_FIERCE_TACKLE 26
#define U3S_SEAN_KICKS 0x700

s16 upper3s_ch_sean_tackle_feint(s16 player, s16 koc, s16 char_index, u16 catch_box, u16 sw_new);

/*
 * SN2 - "Allow Sean Roll (214P) to be super-cancelled during the middle."
 *
 * The middle frames of Sean Roll (special animations 48-50) get the super cancel
 * bit 0x40 in upper3s/pl15.c. A super cancel also needs meoshi_hit_flag
 * (PLS00.c:check_cg_cancel_data()): a hit sets it, and so does a frame of a
 * special move with the bit (CHARSET.c, when `kow & 0x60`), but the roll is not
 * such a move - measured: with the bit alone, no super comes out of it. Returns
 * 1 when the frame arms the cancel itself: a frame of Sean Roll (koc 5,
 * animations 48-50) that has the super bit.
 */
int upper3s_ch_sean_roll_arms_super(s16 koc, s16 char_index, u8 cg_cancel);

#endif
