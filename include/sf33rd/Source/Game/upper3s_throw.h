#ifndef UPPER3S_THROW_H
#define UPPER3S_THROW_H

#include "structs.h"
#include "types.h"

/*
 * Third Strike Upper (3s') - throws, superflash and parries.
 *
 * The state of these rules is kept in this module instead of in PLW or
 * WORK: both structures are part of the PS2 memory map and may not grow.
 *
 * Every player is addressed by its player slot (`wk->id`, 0 or 1), never by
 * its character index.
 */

/* The two player slots. */
#define U3S_THROW_PLAYERS 2

/*
 * The kinds of parry.
 *
 * The retail game has three of them and they are the three guard levers of
 * the command buffer:
 *
 *  - U3S_PARRY_HIGH: the lever of the attacks that are blocked while
 *    standing (`att.guard & 2`). It is `cp->waza_flag[3]`.
 *  - U3S_PARRY_LOW: the lever of the attacks that have to be blocked while
 *    crouching (`att.guard & 1`). It is `cp->waza_flag[4]`.
 *  - U3S_PARRY_AIR: the lever of the attacks in the air (`att.guard & 4`).
 *    It is `cp->waza_flag[5]`.
 *
 * "The same way" of the design specification therefore means "the same kind
 * of parry".
 */
#define U3S_PARRY_HIGH 0
#define U3S_PARRY_LOW 1
#define U3S_PARRY_AIR 2
#define U3S_PARRY_KINDS 3

/* The index of the lever of every kind of parry in `cp->waza_flag[]`. */
#define U3S_PARRY_LEVER_HIGH 3
#define U3S_PARRY_LEVER_LOW 4
#define U3S_PARRY_LEVER_AIR 5

/*
 * SM2 - the cooldown of a parry, in frames.
 *
 * The parry that was attempted is not available for this many frames. The
 * frame of the attempt itself is still a parry: the cooldown starts on the
 * frame after it.
 */
#define U3S_PARRY_COOLDOWN 2

/*
 * SM2 - the extra damage of a hit that lands during the cooldown of the
 * parry that would have parried it, in permille.
 *
 * The design specification asks for the crouching damage bonus, which is
 * 25% more damage.
 */
#define U3S_PARRY_DAMAGE 1250

/*
 * SM3 - the window of a guess parry, in frames.
 *
 * The retail parry window is two frames long for a normal attack and three
 * frames for a special move (the number of frames the guard lever has to
 * be held for the parry to be active). A guess parry is only active for half
 * of it, rounded down: one frame for both of them.
 */
#define U3S_PARRY_GUESS_WINDOW 1

/*
 * B1 - a successful throw is invincible for its whole animation.
 *
 * `upper3s_throw_invuln_start` is called when the throw connects and
 * `upper3s_throw_frame` releases the victim again when the throw animation
 * is over.
 */
void upper3s_throw_invuln_start(PLW* victim);
void upper3s_throw_invuln_end(PLW* victim);
void upper3s_throw_frame(PLW* wk);

/*
 * B2 - a character must not be able to walk away during the superflash of
 * the opponent.
 *
 * `upper3s_superflash_hit` is called when the frozen character is hit: the
 * damage stop of the hit would cancel the hit stop of the superflash, and
 * with it the freeze.
 */
void upper3s_superflash_hit(PLW* frozen);

/*
 * SM2/SM3 - the parries.
 *
 * `upper3s_parry_frame` has to be called once per frame and per player with
 * the frame counter of the game. It watches the guard levers of the command
 * buffer: a lever that is loaded again is a new attempt at a parry.
 *
 * `upper3s_parry_cooling` tells whether the parry of a kind is on cooldown,
 * `upper3s_parry_threshold` returns the frame the lever has to be held for
 * until the parry is active - the guess parries need the lever to be held
 * twice as long - and `upper3s_parry_damage_permille` returns the scaling
 * of the damage of a hit that lands during the cooldown of the parry that
 * would have parried it.
 */
void upper3s_parry_frame(PLW* wk, u16 frame);
s16 upper3s_parry_cooling(s16 player, s16 kind);
s16 upper3s_parry_threshold(PLW* ds, s16 kind, s16 threshold);
s16 upper3s_parry_damage_permille(s16 player, u16 guard);
s16 upper3s_parry_guess_locked(s16 player);
s16 upper3s_parry_is_guess_state(PLW* wk);

#endif
