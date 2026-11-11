#include "common.h"
#include "sf33rd/Source/Game/upper3s_throw.h"

/*
 * Third Strike Upper (3s') - throws, superflash and parries.
 *
 * The state of these rules is kept here instead of in PLW or WORK: both
 * structures are part of the PS2 memory map and may not grow.
 *
 * Every player is addressed by its player slot (`wk->wu.id`), never by its
 * character index.
 */

/*
 * B1 - a successful throw is invincible for its whole animation.
 *
 * The retail game has a flag for a character that cannot be hit at all:
 * `PLW.zettai_muteki_flag`. The player process only puts a character into
 * the hit queue when the flag is clear (PLCNT.c:378), and only the works
 * in the hit queue are checked against the attacks of the other works
 * (HITCHECK.c:attack_hit_check). A character with the flag set therefore
 * cannot be hit by anything - not by a normal move, not by a projectile and
 * not by a throw.
 *
 * The flag is set when the throw connects and cleared again when the victim
 * leaves the caught state, which is the end of the throw animation.
 */
static u8 upper3s_throw_invuln[U3S_THROW_PLAYERS];

/*
 * SM2/SM3 - the parries of a player.
 *
 * `upper3s_parry_attempt_frame` is the frame of the last attempt at a parry
 * of a kind, `upper3s_parry_guess` tells whether that attempt was a guess
 * parry and `upper3s_parry_lever` is the value the guard lever of the
 * command buffer had the last time it was looked at.
 *
 * The guard levers of the command buffer are the retail parries: they are
 * loaded with `reset[]` when the direction is pressed and they are counted
 * down once per frame, and the parry of HITCHECK.c:defense_ground is active
 * while the lever of the parry is still above the red blocking time.
 */
static u16 upper3s_parry_attempt_frame[U3S_THROW_PLAYERS][U3S_PARRY_KINDS];
static u8 upper3s_parry_attempted[U3S_THROW_PLAYERS][U3S_PARRY_KINDS];
static u8 upper3s_parry_guess[U3S_THROW_PLAYERS][U3S_PARRY_KINDS];
static s16 upper3s_parry_lever[U3S_THROW_PLAYERS][U3S_PARRY_KINDS];

/* The frame the game is in. */
static u16 upper3s_parry_now;

/* The index of the lever of every kind of parry. */
static const s16 upper3s_parry_lever_of[U3S_PARRY_KINDS] = { U3S_PARRY_LEVER_HIGH, U3S_PARRY_LEVER_LOW,
                                                              U3S_PARRY_LEVER_AIR };

/*
 * B1 - the throw of `victim` connected: the victim is invincible until the
 * throw animation is over.
 */
void upper3s_throw_invuln_start(PLW* victim) {
    if ((victim->wu.id < 0) || (victim->wu.id >= U3S_THROW_PLAYERS)) {
        return;
    }

    /* The retail flag of a character that cannot be hit. */
    victim->zettai_muteki_flag = 1;
    upper3s_throw_invuln[victim->wu.id] = 1;
}

/*
 * B1 - the throw animation of `victim` is over.
 */
void upper3s_throw_invuln_end(PLW* victim) {
    if ((victim->wu.id < 0) || (victim->wu.id >= U3S_THROW_PLAYERS)) {
        return;
    }

    if (!upper3s_throw_invuln[victim->wu.id]) {
        return;
    }

    victim->zettai_muteki_flag = 0;
    upper3s_throw_invuln[victim->wu.id] = 0;
}

/*
 * B1 - the end of the frame of a player.
 *
 * The caught state of the victim of a throw is `routine_no[1] == 3`
 * (PLPCU.c:Player_caught). The throw animation is over when the victim
 * leaves it, either because the thrower has finished or because the throw
 * has been teched.
 */
void upper3s_throw_frame(PLW* wk) {
    if ((wk->wu.id < 0) || (wk->wu.id >= U3S_THROW_PLAYERS)) {
        return;
    }

    if (!upper3s_throw_invuln[wk->wu.id]) {
        return;
    }

    if (wk->wu.routine_no[1] != 3) {
        upper3s_throw_invuln_end(wk);
    }
}

/*
 * B2 - the frozen character of a superflash was hit.
 *
 * The superflash gives the frozen character a hit stop that counts down
 * (CHARSET.c:comm_stop sets `sa_stop_flag` to 2 and `hit_stop` to the
 * length of the superflash). The hit stop of a hit would replace the hit
 * stop of the superflash with its own (PLMAIN.c:check_hit_stop), and the
 * character would be able to walk away while the superflash is running.
 *
 * The damage stop of the hit is dropped instead: the hit stop of the
 * superflash is the longer one anyway.
 */
void upper3s_superflash_hit(PLW* frozen) {
    if (frozen->sa_stop_flag != 2) {
        /* Not in the superflash of the opponent. */
        return;
    }

    frozen->wu.dm_stop = 0;
}

/*
 * SM3 - the states in which a parry is only active for half as long.
 *
 * The design specification names the wakeup animation, the recovery roll
 * and the hitstun of landing on the feet from the air.
 */
s16 upper3s_parry_is_guess_state(PLW* wk) {
    /* The recovery roll: the retail marks it with `my_roll_flag`. */
    if (wk->wu.my_roll_flag != 0) {
        return 1;
    }

    if (wk->wu.routine_no[1] != 1) {
        /* Not in the damage state: neither the wakeup nor a hitstun. */
        return 0;
    }

    /* The wakeup animation: the reaction 88 is the get up that follows the
     * end of a throw (PLPCU.c:check_tsukamare_keizoku_check). */
    if (wk->wu.routine_no[2] == 88) {
        return 1;
    }

    /* The hitstun of landing on the feet from the air: the character was in
     * the air in the previous frame and stands on the ground now. */
    if ((wk->wu.xyz[1].disp.pos <= 0) && (wk->wu.old_pos[1] > 0)) {
        return 1;
    }

    return 0;
}

/*
 * SM2/SM3 - the end of the frame of a player.
 *
 * A guard lever that is loaded again is a new attempt at a parry: the
 * command buffer loads the lever with `reset[]` when the direction is
 * pressed and counts it down once per frame.
 */
void upper3s_parry_frame(PLW* wk, u16 frame) {
    s16 player;
    s16 kind;

    if ((wk->wu.id < 0) || (wk->wu.id >= U3S_THROW_PLAYERS)) {
        return;
    }

    if (wk->cp == NULL) {
        /* The command buffer of the player is not set up yet. */
        return;
    }

    player = wk->wu.id;
    upper3s_parry_now = frame;

    for (kind = 0; kind < U3S_PARRY_KINDS; kind++) {
        s16 lever = upper3s_parry_lever_of[kind];
        s16 value = wk->cp->waza_flag[lever];

        if (value > upper3s_parry_lever[player][kind]) {
            /* The direction of the parry was pressed again. */
            upper3s_parry_attempt_frame[player][kind] = frame;
            upper3s_parry_attempted[player][kind] = 1;
            upper3s_parry_guess[player][kind] = upper3s_parry_is_guess_state(wk);
        }

        upper3s_parry_lever[player][kind] = value;
    }
}

/*
 * SM2 - the parry of a kind is on cooldown.
 *
 * The cooldown starts on the frame after the attempt and lasts
 * U3S_PARRY_COOLDOWN frames.
 */
s16 upper3s_parry_cooling(s16 player, s16 kind) {
    u16 delta;

    if ((player < 0) || (player >= U3S_THROW_PLAYERS)) {
        return 0;
    }

    if ((kind < 0) || (kind >= U3S_PARRY_KINDS)) {
        return 0;
    }

    if (!upper3s_parry_attempted[player][kind]) {
        return 0;
    }

    delta = (u16)(upper3s_parry_now - upper3s_parry_attempt_frame[player][kind]);

    return (delta >= 1) && (delta <= U3S_PARRY_COOLDOWN);
}

/*
 * SM3 - the frame the guard lever has to be held for until the parry of a
 * kind is active.
 *
 * `threshold` is the value of the retail red blocking time of the parry.
 * A guess parry is only active for half of the retail window, rounded
 * down, so the lever has to be held twice as long.
 */
s16 upper3s_parry_threshold(PLW* ds, s16 kind, s16 threshold) {
    s16 lever;
    s16 reset;
    s16 window;
    s16 half;

    if ((ds->wu.id < 0) || (ds->wu.id >= U3S_THROW_PLAYERS)) {
        return threshold;
    }

    if ((kind < 0) || (kind >= U3S_PARRY_KINDS)) {
        return threshold;
    }

    if (ds->cp == NULL) {
        return threshold;
    }

    if (!upper3s_parry_guess[ds->wu.id][kind]) {
        /* Not a guess parry: the retail window. */
        return threshold;
    }

    lever = upper3s_parry_lever_of[kind];
    reset = ds->cp->reset[lever];

    /*
     * The retail window of the parry, in frames.
     *
     * The command buffer loads the lever with `reset[]` when the direction
     * is pressed and counts it down once per frame, so the lever is at
     * `reset - 1` on the frame of the press. The parry is active while the
     * lever is at or above the red blocking time.
     */
    window = reset - threshold;

    if (window <= 1) {
        return threshold;
    }

    /* Half of the window, rounded down. */
    half = window / 2;

    return reset - half;
}

/*
 * SM2 - the scaling of the damage of a hit that lands during the cooldown
 * of the parry that would have parried it.
 *
 * A throw is not an attack that can be parried, so it never gets the extra
 * damage: the bits of the guard flags of a throw are not set.
 */
s16 upper3s_parry_damage_permille(s16 player, u16 guard) {
    if ((guard & 2) && upper3s_parry_cooling(player, U3S_PARRY_HIGH)) {
        return U3S_PARRY_DAMAGE;
    }

    if ((guard & 1) && upper3s_parry_cooling(player, U3S_PARRY_LOW)) {
        return U3S_PARRY_DAMAGE;
    }

    if ((guard & 4) && upper3s_parry_cooling(player, U3S_PARRY_AIR)) {
        return U3S_PARRY_DAMAGE;
    }

    return 1000;
}

/*
 * SM3 - the player is in the window or in the cooldown of a guess parry.
 *
 * A throw that lands in that time cannot be teched.
 */
s16 upper3s_parry_guess_locked(s16 player) {
    s16 kind;

    if ((player < 0) || (player >= U3S_THROW_PLAYERS)) {
        return 0;
    }

    for (kind = 0; kind < U3S_PARRY_KINDS; kind++) {
        u16 delta;

        if (!upper3s_parry_attempted[player][kind]) {
            continue;
        }

        if (!upper3s_parry_guess[player][kind]) {
            continue;
        }

        delta = (u16)(upper3s_parry_now - upper3s_parry_attempt_frame[player][kind]);

        /* The window of the guess parry or its cooldown. */
        if (delta <= U3S_PARRY_GUESS_WINDOW) {
            return 1;
        }

        if (upper3s_parry_cooling(player, kind)) {
            return 1;
        }
    }

    return 0;
}
