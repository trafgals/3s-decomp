#include "sf33rd/Source/Game/upper3s_input.h"

/*
 * B3 - the universal overhead (MP+MK) and the personal action (HP+HK) come
 * out while the stick is held away or towards.
 *
 * The retail check (PLS03.c:check_leap_attack() and PLS03.c:
 * check_chouhatsu()) rejects the input when the stick is not in the neutral
 * position, and the command turns into a normal attack instead.  3s' only
 * keeps the up and the down direction blocking: the away and the towards
 * direction are let through, so that the player can walk backwards into a
 * universal overhead.
 *
 * The decision is a pure function of the lever state, so that it can be
 * exercised on synthetic state (tests/upper3s/test_upper3s_input.c).
 */
s32 upper3s_input_combo_direction_ok(u16 sw_lvbt, u16 retail_blocked) {
    u16 blocking = sw_lvbt & retail_blocked;

    /* The away and the towards direction do not block the command any more. */
    blocking &= (u16)~U3S_INPUT_LEVER_AWAY_TOWARDS;

    return blocking == 0;
}
