/*
 * Functional tests for the Third Strike Upper throws, superflash and parries.
 *
 * The rules live in the module `upper3s/upper3s_throw.c`, so the tests build
 * a synthetic player and check the decisions of the module directly:
 *
 *  - B1: a successful throw is invincible for its whole animation.
 *  - B2: the hit stop of a hit must not cancel the hit stop of the
 *    superflash of the opponent.
 *  - SM2: the cooldown of a parry and the extra damage of a hit that lands
 *    during it.
 *  - SM3: the guess parries - their states, the halved window and the lock
 *    of the throw tech.
 *
 * The parry slots of the command buffer are simulated the way
 * CMD_MAIN.c:check_10() runs them (see `tap()` and `next_frame()`); the
 * fight tests (tests/match/test_system.py) check the same rules in the real
 * engine.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_throw.h"

static int failures;
static int checks;

static void check(int condition, const char* what) {
    checks++;

    if (!condition) {
        failures++;
        printf("  FAIL  %s\n", what);
    } else {
        printf("  ok    %s\n", what);
    }
}

/*
 * The parry slots of the command buffer of the retail game.
 *
 * A tap loads the slot's `waza_flag[]` with its `reset[]` (11 for a ground
 * parry) and the interpreter counts it down once per frame, so it is at 10
 * when the frame of the tap is over: the parry is active for 10 frames. The
 * slot is in states 2-4 (`shot_ok`) while `free3` counts down from
 * `reset + 10` (+2 in 3s'), then restarts with `w_type` 0.
 */
#define GUARD_RESET 11
#define COOLDOWN_FREE3 (GUARD_RESET + 10 + U3S_PARRY_COOLDOWN)

static WORK_CP command_buffer;
static WAZA_WORK slots[56];

static void setup_player(PLW* wk, s16 player) {
    s16 i;

    memset(wk, 0, sizeof(PLW));
    memset(&command_buffer, 0, sizeof(WORK_CP));
    memset(slots, 0, sizeof(slots));

    wk->wu.id = player;
    wk->cp = &command_buffer;

    for (i = 0; i < 56; i++) {
        wk->cp->reset[i] = GUARD_RESET;
        slots[i].w_type = U3S_PARRY_STEP;
        slots[i].shot_ok = 1; /* waiting for a tap */
    }

    upper3s_parry_frame(wk, slots);
}

/* The player taps the direction of the parry in `slot`; the frame ends. */
static void tap(PLW* wk, s16 slot) {
    wk->cp->waza_flag[slot] = wk->cp->reset[slot] - 1;
    slots[slot].shot_ok = 2;
    slots[slot].free3 = COOLDOWN_FREE3;
    upper3s_parry_frame(wk, slots);
}

/* One frame of the command interpreter, then the hit check's call. */
static void next_frame(PLW* wk) {
    s16 i;

    for (i = 0; i < 56; i++) {
        if (wk->cp->waza_flag[i] > 0) {
            wk->cp->waza_flag[i]--;
        }

        if ((slots[i].w_type == U3S_PARRY_STEP) && (slots[i].shot_ok >= 2)) {
            if (--slots[i].free3 < 0) {
                slots[i].w_type = 0;
            }
        } else if (slots[i].w_type == 0) {
            /* check_init() starts the slot again. */
            slots[i].w_type = U3S_PARRY_STEP;
            slots[i].shot_ok = 1;
        }
    }

    upper3s_parry_frame(wk, slots);
}

static void frames(PLW* wk, s16 n) {
    while (n-- > 0) {
        next_frame(wk);
    }
}

/*
 * B1 - a successful throw is invincible for its whole animation.
 */
static void test_throw_invulnerability(void) {
    PLW victim;

    printf("B1: a successful throw is invincible for the whole animation\n");

    setup_player(&victim, 0);

    check(victim.zettai_muteki_flag == 0, "a character can be hit before it is thrown");

    /* The throw connects: the victim enters the caught state. */
    victim.wu.routine_no[1] = 3;
    upper3s_throw_invuln_start(&victim);

    check(victim.zettai_muteki_flag == 1, "the victim of a successful throw cannot be hit");

    /* The throw animation plays. */
    upper3s_throw_frame(&victim);
    check(victim.zettai_muteki_flag == 1, "the victim of a throw is invincible during the throw animation");

    /* The throw animation is over: the victim leaves the caught state. */
    victim.wu.routine_no[1] = 1;
    upper3s_throw_frame(&victim);

    check(victim.zettai_muteki_flag == 0, "the victim of a throw can be hit again after the throw");
}

/*
 * B2 - the character must not be able to walk away during the superflash of
 * the opponent.
 */
static void test_superflash_lock(void) {
    PLW frozen;

    printf("B2: the hit stop of the superflash is not cancelled by a hit\n");

    setup_player(&frozen, 0);

    /* No superflash: the hit stop of the hit is kept. */
    frozen.sa_stop_flag = 0;
    frozen.wu.dm_stop = 12;
    upper3s_superflash_hit(&frozen);
    check(frozen.wu.dm_stop == 12, "the hit stop of a hit is kept outside of a superflash");

    /* The superflash is over: the hit stop of the hit is kept. */
    frozen.sa_stop_flag = 1;
    frozen.wu.dm_stop = 12;
    upper3s_superflash_hit(&frozen);
    check(frozen.wu.dm_stop == 12, "the hit stop of a hit is kept when the superflash is over");

    /* The superflash is running: its hit stop must survive the hit. */
    frozen.sa_stop_flag = 2;
    frozen.wu.dm_stop = 12;
    upper3s_superflash_hit(&frozen);
    check(frozen.wu.dm_stop == 0, "the hit stop of the superflash is not cancelled by a hit");
}

/*
 * SM2 - the cooldown of a parry.
 */
static void test_parry_cooldown(void) {
    PLW wk;
    s16 n;

    printf("SM2: the cooldown of a parry is the slot's lockout after the window\n");

    setup_player(&wk, 0);
    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 0, "there is no cooldown without a tap");

    tap(&wk, U3S_PARRY_LEVER_HIGH);
    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 0, "the frame of the tap is in the window");

    frames(&wk, 9);
    check(wk.cp->waza_flag[U3S_PARRY_LEVER_HIGH] == 1, "the window is 10 frames long");
    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 0, "the last frame of the window is not the cooldown");

    next_frame(&wk);
    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 1, "the cooldown starts when the window is over");
    check(upper3s_parry_cooling(0, U3S_PARRY_LOW) == 0, "the cooldown of a parry is not the other parry's");

    /* The slot is locked until free3 goes below 0: 24 frames after the tap
     * with the +2f of 3s', 22 in the retail game. */
    for (n = 11; upper3s_parry_cooling(0, U3S_PARRY_HIGH); n++) {
        next_frame(&wk);
    }

    check(n == GUARD_RESET + 10 + U3S_PARRY_COOLDOWN + 2, "the slot takes a new tap 24 frames after the tap");
    check(U3S_PARRY_COOLDOWN == 2, "3s' adds two frames to the cooldown");
}

/*
 * SM2 - the damage of a hit that lands during the cooldown.
 */
static void test_missed_parry_damage(void) {
    PLW ds;
    WORK as;

    printf("SM2: a hit in the cooldown of the parry that would have parried it takes the crouching bonus\n");

    memset(&as, 0, sizeof(as));

    setup_player(&ds, 0);
    tap(&ds, U3S_PARRY_LEVER_HIGH);

    as.att.guard = 2;
    check(upper3s_parry_damage_percent(&ds, &as) == 100, "a hit in the window gets no bonus");

    frames(&ds, 12);
    check(upper3s_parry_damage_percent(&ds, &as) == 125, "an attack parried forward takes 25 percent more");

    as.att.guard = 1;
    check(upper3s_parry_damage_percent(&ds, &as) == 100, "an attack parried down takes no bonus");

    as.att.guard = 0;
    check(upper3s_parry_damage_percent(&ds, &as) == 100, "an attack that cannot be parried takes no bonus");

    as.att.guard = 2;
    as.jump_att_flag = 1;
    check(upper3s_parry_damage_percent(&ds, &as) == 100, "a jumping attack is parried with the anti-air slot");
    as.jump_att_flag = 0;

    ds.wu.pat_status = 32;
    check(upper3s_parry_damage_percent(&ds, &as) == 100, "a crouching defender already has the crouching bonus");
    ds.wu.pat_status = 0;

    ds.wu.xyz[1].disp.pos = 40;
    check(upper3s_parry_damage_percent(&ds, &as) == 100, "a defender in the air needs the air parry");
    ds.wu.xyz[1].disp.pos = 0;

    frames(&ds, 30);
    check(upper3s_parry_damage_percent(&ds, &as) == 100, "the damage is the retail one after the cooldown");

    /* The down parry and the low attacks. */
    setup_player(&ds, 0);
    tap(&ds, U3S_PARRY_LEVER_LOW);
    frames(&ds, 12);
    as.att.guard = 1;
    check(upper3s_parry_damage_percent(&ds, &as) == 125, "an attack parried down takes 25 percent more");

    /* The forward tap loads the ground and the anti-air slots together. */
    setup_player(&ds, 0);
    tap(&ds, U3S_PARRY_LEVER_ANTIAIR);
    frames(&ds, 12);
    as.att.guard = 2;
    as.jump_att_flag = 1;
    check(upper3s_parry_damage_percent(&ds, &as) == 125, "a jumping attack in the anti-air cooldown takes the bonus");
    as.jump_att_flag = 0;

    /* The air parry. */
    setup_player(&ds, 0);
    tap(&ds, U3S_PARRY_LEVER_AIR);
    frames(&ds, 12);
    ds.wu.xyz[1].disp.pos = 40;
    as.att.guard = 4;
    check(upper3s_parry_damage_percent(&ds, &as) == 125, "an air attack in the air parry's cooldown takes the bonus");
}

/*
 * SM3 - the states of the guess parries.
 */
static void test_guess_parry_states(void) {
    PLW wk;

    printf("SM3: the wakeup, the roll and the air land-on-feet hitstun are guess parry states\n");

    setup_player(&wk, 0);
    check(upper3s_parry_is_guess_state(&wk) == 0, "a standing character is not in a guess state");

    /* An ordinary hitstun (reaction 12 is the standing hitstun as well). */
    wk.wu.routine_no[1] = 1;
    wk.wu.routine_no[2] = 12;
    next_frame(&wk);
    check(upper3s_parry_is_guess_state(&wk) == 0, "an ordinary hitstun is not a guess state");

    /* A knockdown: lying on the ground, then getting up. */
    wk.wu.routine_no[2] = 16;
    wk.wu.pat_status = 38;
    next_frame(&wk);
    check(upper3s_parry_is_guess_state(&wk) == 1, "lying on the ground is the wakeup");
    wk.wu.pat_status = 0;
    next_frame(&wk);
    check(upper3s_parry_is_guess_state(&wk) == 1, "getting up is the wakeup");
    wk.wu.routine_no[1] = 0;
    wk.wu.routine_no[2] = 36;
    next_frame(&wk);
    check(upper3s_parry_is_guess_state(&wk) == 0, "the wakeup ends with the damage state");

    /* The recovery roll. */
    wk.wu.routine_no[1] = 1;
    wk.wu.routine_no[2] = 18;
    wk.ukemi_success = 1;
    next_frame(&wk);
    wk.ukemi_success = 0;
    wk.wu.routine_no[2] = 3;
    next_frame(&wk);
    check(upper3s_parry_is_guess_state(&wk) == 1, "the recovery roll is a guess state");
    wk.wu.routine_no[1] = 0;
    next_frame(&wk);
    check(upper3s_parry_is_guess_state(&wk) == 0, "the roll ends with the damage state");

    /* The air hitstun that ends on the feet. */
    wk.wu.routine_no[1] = 1;
    wk.wu.routine_no[2] = 17;
    wk.wu.xyz[1].disp.pos = 30;
    next_frame(&wk);
    check(upper3s_parry_is_guess_state(&wk) == 1, "the air land-on-feet hitstun is a guess state");
    wk.wu.routine_no[1] = 0;
    wk.wu.routine_no[2] = 23;
    wk.wu.xyz[1].disp.pos = 0;
    next_frame(&wk);
    check(upper3s_parry_is_guess_state(&wk) == 0, "the landing after it is not");
}

/*
 * SM3 - the window of a guess parry is halved, its cooldown is not.
 */
static void test_guess_parry_window(void) {
    PLW wk;
    s16 n;

    printf("SM3: a guess parry is only active for half as long\n");

    setup_player(&wk, 0);
    tap(&wk, U3S_PARRY_LEVER_HIGH);
    check(wk.cp->waza_flag[U3S_PARRY_LEVER_HIGH] == GUARD_RESET - 1, "a normal parry keeps the retail window");

    setup_player(&wk, 0);
    wk.wu.routine_no[1] = 1;
    wk.wu.pat_status = 38;
    next_frame(&wk);
    tap(&wk, U3S_PARRY_LEVER_HIGH);
    check(wk.cp->waza_flag[U3S_PARRY_LEVER_HIGH] == (GUARD_RESET - 1) / 2, "a guess parry is active for 5 frames");

    for (n = 1; !upper3s_parry_cooling(0, U3S_PARRY_HIGH); n++) {
        next_frame(&wk);
    }

    check(n == 6, "the cooldown of a guess parry starts after its 5 frames");

    for (; upper3s_parry_cooling(0, U3S_PARRY_HIGH); n++) {
        next_frame(&wk);
    }

    check(n == GUARD_RESET + 10 + U3S_PARRY_COOLDOWN + 2, "a guess parry has the same cooldown");
}

/*
 * SM3 - a throw that lands in the window or in the cooldown of a guess
 * parry cannot be teched.
 */
static void test_guess_parry_throw_lock(void) {
    PLW wk;

    printf("SM3: a throw in the window or cooldown of a guess parry cannot be teched\n");

    setup_player(&wk, 0);
    wk.wu.routine_no[1] = 1;
    wk.wu.pat_status = 38;
    next_frame(&wk);
    tap(&wk, U3S_PARRY_LEVER_HIGH);
    check(upper3s_parry_guess_locked(0) == 1, "the throw cannot be teched in the window of the guess parry");

    frames(&wk, 10);
    check(upper3s_parry_guess_locked(0) == 1, "the throw cannot be teched in the cooldown of the guess parry");

    frames(&wk, 20);
    check(upper3s_parry_guess_locked(0) == 0, "the throw can be teched again after the cooldown");

    /* A normal parry does not lock the throw tech. */
    setup_player(&wk, 0);
    tap(&wk, U3S_PARRY_LEVER_HIGH);
    check(upper3s_parry_guess_locked(0) == 0, "a normal parry does not lock the throw tech");
}

int main() {
    test_throw_invulnerability();
    test_superflash_lock();
    test_parry_cooldown();
    test_missed_parry_damage();
    test_guess_parry_states();
    test_guess_parry_window();
    test_guess_parry_throw_lock();

    printf("\n%d checks, %d failures\n", checks, failures);

    if (failures) {
        return 1;
    }

    return 0;
}
