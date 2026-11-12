/*
 * Third Strike Upper (3s') - sean. See include/sf33rd/Source/Game/upper3s_ch_sean.h.
 */

#include "sf33rd/Source/Game/upper3s_ch_sean.h"

/* SN6: the Fierce Tackle of the player has reached its catch box. */
static u8 sean_tackle_grabbing[2];

s16 upper3s_ch_sean_tackle_feint(s16 player, s16 koc, s16 char_index, u16 catch_box, u16 sw_new) {
    if ((player < 0) || (player > 1)) {
        return 0;
    }

    if ((koc != 5) || (char_index != U3S_SEAN_FIERCE_TACKLE)) {
        /* Not the dash of the Fierce Tackle: a new one starts armed. */
        sean_tackle_grabbing[player] = 0;
        return 0;
    }

    if (catch_box != 0) {
        sean_tackle_grabbing[player] = 1;
    }

    if (sean_tackle_grabbing[player]) {
        /* Past the startup. */
        return 0;
    }

    return (sw_new & U3S_SEAN_KICKS) != 0;
}

/* SN2, the super cancel of Sean Roll (upper3s_ch_sean.h). */
int upper3s_ch_sean_roll_arms_super(s16 koc, s16 char_index, u8 cg_cancel) {
    return (koc == 5) && (char_index >= 48) && (char_index <= 50) && ((cg_cancel & 0x40) != 0);
}
