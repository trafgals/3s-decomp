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
 * the hit queue when the flag is clear (PLCNT.c:400), and only the works
 * in the hit queue are checked against the attacks of the other works
 * (HITCHECK.c:attack_hit_check). A character with the flag set therefore
 * cannot be hit by anything - not by a normal move, not by a projectile and
 * not by a throw. The damage of the throw itself does not go through the hit
 * queue (PLPCA.c calls setup_catch_atthit() directly), so it still lands.
 *
 * Both characters get the flag when the throw connects. Measured on the
 * forward and back throws of all 20 characters: in the retail game the
 * victim has hurtboxes only on the first frame of being caught, but the
 * thrower has them on a large part of its throw (Ryu's forward throw: the
 * first frame, and the 26 frames of recovery after the victim is released);
 * that is where a character can be "hit out of a successful throw". PR #1
 * protected the victim only.
 *
 * The flag is cleared when the character leaves the throw: the thrower its
 * catch state (routine_no[1] 2), the victim its caught state (3).
 */
static u8 upper3s_throw_invuln[U3S_THROW_PLAYERS];

/*
 * The routine group the player was in when its throw invincibility started:
 * 2 for the thrower (PLPCA.c:Player_catch()), 3 for the victim
 * (PLPCU.c:Player_caught()). The invincibility ends when it leaves it.
 */
static s16 upper3s_throw_group[U3S_THROW_PLAYERS];

/*
 * SM2/SM3 - the parries of a player, per kind (U3S_PARRY_*).
 *
 * `upper3s_parry_lever` is the slot's `waza_flag[]` the last time it was
 * looked at: it goes up when the direction is tapped. `upper3s_parry_locked`
 * is set from the tap until the slot takes a new tap, which is the window
 * and the cooldown together (see U3S_PARRY_COOLDOWN). `upper3s_parry_guess`
 * tells whether the last tap was a guess parry (SM3).
 */
static s16 upper3s_parry_lever[U3S_THROW_PLAYERS][U3S_PARRY_KINDS];
static u8 upper3s_parry_locked[U3S_THROW_PLAYERS][U3S_PARRY_KINDS];
static u8 upper3s_parry_guess[U3S_THROW_PLAYERS][U3S_PARRY_KINDS];

/*
 * SM3 - the player is in a knockdown (it has been lying on the ground,
 * `pat_status` 38) or in a recovery roll (it teched the landing,
 * `ukemi_success`) in this damage state, and has not left it yet.
 */
static u8 upper3s_parry_down[U3S_THROW_PLAYERS];
static u8 upper3s_parry_roll[U3S_THROW_PLAYERS];

/* The command slot of every kind of parry. */
static const s16 upper3s_parry_lever_of[U3S_PARRY_KINDS] = { U3S_PARRY_LEVER_HIGH, U3S_PARRY_LEVER_LOW,
                                                              U3S_PARRY_LEVER_AIR, U3S_PARRY_LEVER_ANTIAIR };

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
    upper3s_throw_group[victim->wu.id] = 3;
}

/*
 * B1 - `thrower`'s throw connected: it is invincible until it leaves the
 * catch state, recovery included.
 */
void upper3s_throw_invuln_start_thrower(PLW* thrower) {
    if ((thrower->wu.id < 0) || (thrower->wu.id >= U3S_THROW_PLAYERS)) {
        return;
    }

    thrower->zettai_muteki_flag = 1;
    upper3s_throw_invuln[thrower->wu.id] = 1;
    upper3s_throw_group[thrower->wu.id] = 2;
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
 * The throw is over for the victim when it leaves the caught state
 * (`routine_no[1] == 3`, PLPCU.c:Player_caught), because the thrower has
 * finished or the throw has been teched, and for the thrower when it leaves
 * the catch state (`routine_no[1] == 2`, PLPCA.c:Player_catch).
 */
void upper3s_throw_frame(PLW* wk) {
    if ((wk->wu.id < 0) || (wk->wu.id >= U3S_THROW_PLAYERS)) {
        return;
    }

    if (!upper3s_throw_invuln[wk->wu.id]) {
        return;
    }

    if (wk->wu.routine_no[1] != upper3s_throw_group[wk->wu.id]) {
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
 * SM3 - the states in which a parry is only active for half as long: "any
 * wakeup animation, recovery roll, or air land-on-feet hitstun".
 *
 *  - The wakeup: a knockdown, from the first frame the character lies on
 *    the ground (`pat_status` 38) to the end of the damage state
 *    (routine_no[1] 1), which includes the frames of getting up. Measured: a
 *    sweep and a throw both leave the victim at `pat_status` 38 until the
 *    last frame of the damage state, and a parry tapped then is taken by
 *    the command buffer (it counts down while the character is down). 38 is
 *    not named in the source.
 *  - The recovery roll, the quick stand: from the frame `ukemi_success` is
 *    set (PLMAIN.c:look_after_timers(), the down tap of the landing was in
 *    time) to the end of the damage state. The landing turns into reaction
 *    12 or 13 (PLPDM.c:buttobi_chakuchi_cg_type_check(),
 *    `oki_select_table2`), which clears `ukemi_success`. Measured on a tech
 *    of Ryu's Shoryuken: `ukemi_success` from the frame after the landing,
 *    reaction 13 for one frame, then reaction 3 for 28 frames; the tech gets
 *    Ken up 27 frames earlier. Reaction 12 alone is not enough: it is also
 *    the ordinary standing hitstun (Damage_12000()).
 *  - The "air land-on-feet hitstun": damage reaction 17, the hitstun of a
 *    hit in the air that ends on the feet. When the character lands it turns
 *    into the landing of a jump (PLS00.c:dm_17000()). Measured: Ryu's jab
 *    hits a jumping Ken (reaction 88 on the frame of the hit, then 17 until
 *    he lands), and a forward tap made in 17 is taken by the ground slot and
 *    parries a hit that lands just after the landing. The landing itself
 *    (normal state 23) is not a hitstun: Ken can walk in it.
 * Confirmed from the source and the fights, except the meaning of
 * `pat_status` 38 and of reaction 88 (it is outside the 32 reactions of
 * PLPDM.c:plpdm_lv_00[] and lasts one frame).
 */
s16 upper3s_parry_is_guess_state(PLW* wk) {
    s16 rno1 = wk->wu.routine_no[1];
    s16 rno2 = wk->wu.routine_no[2];

    if ((wk->wu.id < 0) || (wk->wu.id >= U3S_THROW_PLAYERS)) {
        return 0;
    }

    if ((rno1 == 1) && (upper3s_parry_down[wk->wu.id] || upper3s_parry_roll[wk->wu.id])) {
        return 1;
    }

    if ((rno1 == 1) && ((rno2 == 17) || (rno2 == 88))) {
        return 1;
    }

    return 0;
}

/*
 * SM3 - follow the knockdown and the recovery roll, once per frame.
 */
static void upper3s_parry_follow_knockdown(PLW* wk) {
    s16 player = wk->wu.id;

    if (wk->wu.routine_no[1] != 1) {
        upper3s_parry_down[player] = 0;
        upper3s_parry_roll[player] = 0;
        return;
    }

    if ((wk->wu.pat_status == 38) && (wk->wu.xyz[1].disp.pos <= 0)) {
        upper3s_parry_down[player] = 1;
    }

    if (wk->ukemi_success) {
        upper3s_parry_roll[player] = 1;
    }
}

/*
 * SM2/SM3 - the parries of a player after the command interpreter has run.
 *
 * A tap is noticed by the slot's `waza_flag[]` going up. A tap in a guess
 * state (SM3) has its window halved here.
 *
 * A slot is locked from the tap until it takes a new one: check_10() moves
 * it to `shot_ok` 2 on the tap and keeps it in 2-4 while `free3` counts
 * down; then it sets `w_type` to 0 and check_init() starts it again with
 * `shot_ok` 0. A successful parry sets `w_type` to 0 at once
 * (waza_compel_all_init2()).
 */
void upper3s_parry_frame(PLW* wk, const WAZA_WORK* slots) {
    s16 player;
    s16 kind;

    if ((wk->wu.id < 0) || (wk->wu.id >= U3S_THROW_PLAYERS)) {
        return;
    }

    if ((wk->cp == NULL) || (slots == NULL)) {
        /* The command buffer of the player is not set up yet. */
        return;
    }

    player = wk->wu.id;
    upper3s_parry_follow_knockdown(wk);

    for (kind = 0; kind < U3S_PARRY_KINDS; kind++) {
        s16 lever = upper3s_parry_lever_of[kind];
        s16 value = wk->cp->waza_flag[lever];
        const WAZA_WORK* slot = &slots[lever];

        if (value > upper3s_parry_lever[player][kind]) {
            /* The direction of the parry was tapped. */
            upper3s_parry_guess[player][kind] = upper3s_parry_is_guess_state(wk);

            if (upper3s_parry_guess[player][kind]) {
                /*
                 * SM3: a guess parry is active for half as long. The tap
                 * loaded the flag with `reset` and this frame's countdown
                 * has run, so it is active for `value` more frames
                 * (this one included): 10 for a ground parry. Half of it,
                 * rounded down: 5. The cooldown is the slot's and is not
                 * changed.
                 */
                value /= 2;
                wk->cp->waza_flag[lever] = value;
            }
        }

        upper3s_parry_lever[player][kind] = value;
        upper3s_parry_locked[player][kind] = (slot->w_type == U3S_PARRY_STEP) && (slot->shot_ok >= 2);
    }
}

/*
 * SM2 - the parry of a kind is in its cooldown: the slot was tapped, its
 * window is over and it does not take a new tap yet.
 */
s16 upper3s_parry_cooling(s16 player, s16 kind) {
    if ((player < 0) || (player >= U3S_THROW_PLAYERS)) {
        return 0;
    }

    if ((kind < 0) || (kind >= U3S_PARRY_KINDS)) {
        return 0;
    }

    return upper3s_parry_locked[player][kind] && (upper3s_parry_lever[player][kind] == 0);
}

/*
 * SM2 - the damage of a hit from `as` that lands on `ds`, in percent.
 *
 * The kind of parry that would have parried the hit is the one
 * HITCHECK.c:defense_sky() or defense_ground() would have checked: the air
 * parry for a defender in the air, otherwise the forward parry (the
 * anti-air slot against a jumping attack) for `att.guard & 2` and the down
 * parry for `att.guard & 1`.
 *
 * The bonus is the crouching damage bonus, so it is not given a second
 * time to a defender that already gets a retail bonus from its `pat_status`
 * (the list of HITCHECK.c:set_damage_and_piyo()): a crouching character
 * that misses a down parry takes 125%, as it already did in the retail game,
 * not 156%. This is a reading of the spec ("you suffer crouching damage
 * bonus"), not something it states.
 *
 * Throws do not come here: only the hits of strikes and projectiles go
 * through HITCHECK.c:plef_at_vs_player_damage_union(), which applies this.
 */
s16 upper3s_parry_damage_percent(PLW* ds, WORK* as) {
    s16 player = ds->wu.id;
    s16 pat = ds->wu.pat_status;
    u16 guard = as->att.guard;
    s16 missed = 0;

    if ((player < 0) || (player >= U3S_THROW_PLAYERS)) {
        return 100;
    }

    if (ds->wu.xyz[1].disp.pos > 0) {
        missed = (guard & 4) && upper3s_parry_cooling(player, U3S_PARRY_AIR);
    } else {
        if (guard & 2) {
            missed |= upper3s_parry_cooling(player, as->jump_att_flag ? U3S_PARRY_ANTIAIR : U3S_PARRY_HIGH);
        }

        if (guard & 1) {
            missed |= upper3s_parry_cooling(player, U3S_PARRY_LOW);
        }
    }

    if (!missed) {
        return 100;
    }

    if ((pat == 32) || (pat == 3) || (pat == 25) || (pat == 7) || (pat == 23) || (pat == 35) || (pat == 1) ||
        (pat == 21) || (pat == 37)) {
        /* The retail game already gives this hit a damage bonus. */
        return 100;
    }

    return U3S_PARRY_DAMAGE_PERCENT;
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
        /* The slot is locked from the tap to the end of the cooldown. */
        if (upper3s_parry_guess[player][kind] && upper3s_parry_locked[player][kind]) {
            return 1;
        }
    }

    return 0;
}
