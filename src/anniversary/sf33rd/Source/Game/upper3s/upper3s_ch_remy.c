/*
 * Third Strike Upper (3s') - remy. See include/sf33rd/Source/Game/upper3s_ch_remy.h.
 */

#include "sf33rd/Source/Game/upper3s_ch_remy.h"

#define REMY_LP 0x010
#define REMY_LK 0x100
#define REMY_BUTTONS 0x770
#define REMY_DOWN 0x2
#define REMY_CANCEL_NORMAL 0x04
#define REMY_CANCEL_WINDOW 0x60

u8 upper3s_ch_remy_chain(s16 char_ix, u8 cg_cancel, u16 sw_new, u16 lever) {
    u16 wanted;

    if (char_ix == U3S_REMY_CROUCHING_JAB) {
        wanted = REMY_LK;
    } else if (char_ix == U3S_REMY_CROUCHING_SHORT) {
        wanted = REMY_LP;
    } else {
        return cg_cancel;
    }

    if (!(cg_cancel & REMY_CANCEL_WINDOW)) {
        return cg_cancel;
    }

    /* The other light, alone, pressed now, with the lever down. */
    if (((sw_new & REMY_BUTTONS) != wanted) || !(lever & REMY_DOWN)) {
        return cg_cancel;
    }

    return (u8)(cg_cancel | REMY_CANCEL_NORMAL);
}
