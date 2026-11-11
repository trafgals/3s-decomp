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
 * The retail parries are command slots of the command buffer
 * (cmd_data.c:pc_cmd_03 ... pc_cmd_12), each one a single step of type 11
 * (CMD_MAIN.c:check_10(), a tap from neutral). A tap loads the slot's
 * `waza_flag[]` with its `reset[]`, the interpreter counts it down once per
 * frame, and the parry is active while it is not 0 (HITCHECK.c:
 * defense_ground() and defense_sky(); a character that is already blocking
 * needs it above the red blocking time instead, see make_red_blocking_time()).
 *
 *  - U3S_PARRY_HIGH: forward on the ground, slot 3 (reset 11: active for 10
 *    frames). Parries the attacks with `att.guard & 2`.
 *  - U3S_PARRY_LOW: down on the ground, slot 4 (reset 11). Parries the
 *    attacks with `att.guard & 1`.
 *  - U3S_PARRY_AIR: forward in the air, slot 5 (reset 8). Parries the
 *    attacks with `att.guard & 4` on a character in the air.
 *  - U3S_PARRY_ANTIAIR: forward on the ground against a jumping attack,
 *    slot 12 (reset 6). The same tap loads slots 3 and 12, but
 *    defense_ground() reads slot 12 when `as->wu.jump_att_flag` is set.
 *
 * Slot 6 (down in the air) is never read by the hit check: it only clears
 * slot 5 (check_10(), case 6). "The same way" of the design specification
 * therefore means "the same slot". Confirmed from the source.
 */
#define U3S_PARRY_HIGH 0
#define U3S_PARRY_LOW 1
#define U3S_PARRY_AIR 2
#define U3S_PARRY_ANTIAIR 3
#define U3S_PARRY_KINDS 4

/* The command slot (index of `waza_flag[]` and `waza_work[][]`) of every kind of parry. */
#define U3S_PARRY_LEVER_HIGH 3
#define U3S_PARRY_LEVER_LOW 4
#define U3S_PARRY_LEVER_AIR 5
#define U3S_PARRY_LEVER_ANTIAIR 12

/* The step type of a parry tap: an index of CMD_MAIN.c:chk_move_jp[], 11 is check_10(). */
#define U3S_PARRY_STEP 11

/*
 * SM2 - the frames added to the cooldown of a parry.
 *
 * The retail game already has a cooldown: a tap sets the slot's `free3` to
 * `reset + 10` (CMD_MAIN.c:check_10(), case 1), the slot counts it down and
 * does not take another tap until it goes below 0 and the slot restarts
 * (`w_type = 0`, then check_init()). That is 22 frames after the tap for the
 * ground parries, of which the first 10 are the parry window. The spec's
 * "+2f on the cooldown period" adds two frames to `free3`.
 *
 * A successful parry restarts every command slot
 * (HITCHECK.c:set_paring_status() calls waza_compel_all_init2()), so it has
 * no cooldown, as in the retail game.
 */
#define U3S_PARRY_COOLDOWN 2

/*
 * SM2 - the damage of a hit that lands in the cooldown of the parry that
 * would have parried it, in percent.
 *
 * The spec asks for "the crouching damage bonus". The retail game gives a
 * defender whose `pat_status` is 32, 3 or 25 125% damage
 * (HITCHECK.c:set_damage_and_piyo()). 32 is crouching: measured, Ken's
 * c.MK takes 13 from a standing Ryu and 17 from a crouching one.
 */
#define U3S_PARRY_DAMAGE_PERCENT 125

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
 * `upper3s_parry_frame` has to be called once per frame and per player,
 * after the command interpreter has run, with the player's command slots
 * (`waza_work[id]`). It records which parries are in their cooldown, and
 * notices a new tap (SM3).
 *
 * `upper3s_parry_cooling` tells whether the parry of a kind is in its
 * cooldown: the window of the last tap is over and the slot does not take a
 * new tap yet. `upper3s_parry_damage_percent` returns the damage, in
 * percent, of a hit from `as` that lands on `ds`: 125 when it lands in the
 * cooldown of the parry that would have parried it, 100 otherwise.
 *
 * SM3: `upper3s_parry_frame` also halves the window of a tap made in a guess
 * state (`upper3s_parry_is_guess_state`), and `upper3s_parry_guess_locked`
 * tells whether the player is in the window or the cooldown of a guess
 * parry, in which a throw cannot be teched.
 */
void upper3s_parry_frame(PLW* wk, const WAZA_WORK* slots);
s16 upper3s_parry_cooling(s16 player, s16 kind);
s16 upper3s_parry_damage_percent(PLW* ds, WORK* as);
s16 upper3s_parry_guess_locked(s16 player);
s16 upper3s_parry_is_guess_state(PLW* wk);

#endif
