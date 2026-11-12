#ifndef UPPER3S_INPUT_H
#define UPPER3S_INPUT_H

#include "types.h"

/*
 * Third Strike Upper (3s') - the input handling of the two button commands.
 *
 * The universal overhead (the simultaneous press of MP and MK) and the
 * personal action (the simultaneous press of HP and HK, the taunt of the
 * character) are the only two commands of the retail game that need the
 * stick to be in the neutral position.  When the stick is held away or
 * towards, the engine throws the input away and the command turns into a
 * normal attack.
 *
 * 3s' lets both of them through while the stick is held away or towards,
 * so that a player can walk backwards into a universal overhead.
 *
 * The state of the command is in WORK_CP (structs.h): sw_lvbt holds the
 * lever and the buttons of the frame, and the low nibble of sw_lvbt is the
 * direction of the stick.  The button pairs live in the same struct: ca25
 * is MP+MK and ca36 is HP+HK.
 */

/* The direction bits of the low nibble of WORK_CP.sw_lvbt. */
#define U3S_INPUT_LEVER_UP 0x1
#define U3S_INPUT_LEVER_DOWN 0x2
#define U3S_INPUT_LEVER_LEFT 0x4
#define U3S_INPUT_LEVER_RIGHT 0x8

/* The directions the retail game uses to block a command. */
#define U3S_INPUT_LEVER_ANY 0xF
#define U3S_INPUT_LEVER_ANY_BUT_UP 0xE

/*
 * The directions 3s' lets through: away (left) and towards (right).
 *
 * The up and the down direction keep blocking the two button commands, so
 * that the jump and the crouch of the retail game are not changed.
 */
#define U3S_INPUT_LEVER_AWAY_TOWARDS (U3S_INPUT_LEVER_LEFT | U3S_INPUT_LEVER_RIGHT)

/*
 * B3 - the direction of the stick does not block the two button command.
 *
 * `sw_lvbt` is WORK_CP.sw_lvbt and `retail_blocked` is the mask of the
 * retail check that is replaced: U3S_INPUT_LEVER_ANY for the universal
 * overhead and for the personal action of everybody but Hugo, and
 * U3S_INPUT_LEVER_ANY_BUT_UP for Hugo, whose personal action the retail
 * game already accepts while the stick is held up.
 *
 * Returns 1 when the command may come out.
 */
s32 upper3s_input_combo_direction_ok(u16 sw_lvbt, u16 retail_blocked);

#endif
