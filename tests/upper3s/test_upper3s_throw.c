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
 *  - SM3: the guess parries - the halved window and the lock of the throw
 *    tech.
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
 * The guard levers of the command buffer of the retail game.
 *
 * `reset[]` is the value the lever is loaded with when the direction is
 * pressed, `waza_flag[]` counts it down once per frame. The red blocking
 * time of a parry of a normal attack is 9, so the parry is active while
 * the lever is at 10 or 9.
 */
#define GUARD_RESET 11
#define RED_BLOCKING_TIME 9
#define RED_BLOCKING_TIME_SPECIAL 8

static WORK_CP command_buffer;

static void setup_player(PLW* wk, s16 player) {
    memset(wk, 0, sizeof(PLW));
    memset(&command_buffer, 0, sizeof(WORK_CP));

    wk->wu.id = player;
    wk->cp = &command_buffer;
    wk->cp->reset[U3S_PARRY_LEVER_HIGH] = GUARD_RESET;
    wk->cp->reset[U3S_PARRY_LEVER_LOW] = GUARD_RESET;
    wk->cp->reset[U3S_PARRY_LEVER_AIR] = GUARD_RESET;
}

/*
 * The player presses the direction of a parry.
 *
 * The command buffer loads the lever with `reset[]` and counts it down once
 * per frame, so the lever is at `reset - 1` when the frame is over.
 */
static void press_parry(PLW* wk, s16 kind) {
    s16 lever;

    switch (kind) {
    case U3S_PARRY_LOW:
        lever = U3S_PARRY_LEVER_LOW;
        break;

    case U3S_PARRY_AIR:
        lever = U3S_PARRY_LEVER_AIR;
        break;

    default:
        lever = U3S_PARRY_LEVER_HIGH;
        break;
    }

    wk->cp->waza_flag[lever] = wk->cp->reset[lever] - 1;
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

    printf("SM2: a parry cannot be attempted again for two frames\n");

    setup_player(&wk, 0);
    upper3s_parry_frame(&wk, 100);

    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 0, "there is no cooldown without an attempt");

    /* The player presses the direction of the parry. */
    press_parry(&wk, U3S_PARRY_HIGH);
    upper3s_parry_frame(&wk, 101);

    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 0, "the frame of the attempt is not on cooldown");

    upper3s_parry_frame(&wk, 102);
    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 1, "the parry is on cooldown one frame after the attempt");

    upper3s_parry_frame(&wk, 103);
    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 1, "the parry is on cooldown two frames after the attempt");

    upper3s_parry_frame(&wk, 104);
    check(upper3s_parry_cooling(0, U3S_PARRY_HIGH) == 0, "the cooldown of the parry is over after two frames");

    /* The cooldown of one parry does not block the other one. */
    check(upper3s_parry_cooling(0, U3S_PARRY_LOW) == 0, "the cooldown of a parry does not block the other parry");
}

/*
 * SM2 - the extra damage of a hit that lands during the cooldown.
 */
static void test_missed_parry_damage(void) {
    PLW wk;

    printf("SM2: a hit that lands during the cooldown of a parry does 25 percent more damage\n");

    setup_player(&wk, 0);
    upper3s_parry_frame(&wk, 199);

    /* The player attempts a parry and misses it. */
    press_parry(&wk, U3S_PARRY_HIGH);
    upper3s_parry_frame(&wk, 200);
    upper3s_parry_frame(&wk, 201);

    check(upper3s_parry_damage_permille(0, 2) == 1250, "an attack that is parried the same way does 25 percent more damage");
    check(upper3s_parry_damage_permille(0, 1) == 1000, "an attack that is parried another way does no extra damage");
    check(upper3s_parry_damage_permille(0, 0) == 1000, "a throw does no extra damage");
    check(upper3s_parry_damage_permille(0, 4) == 1000, "an attack in the air does no extra damage");

    /* The cooldown is over: the damage is the retail one again. */
    upper3s_parry_frame(&wk, 204);
    check(upper3s_parry_damage_permille(0, 2) == 1000, "the damage is the retail one after the cooldown");

    /* The same for the parry of the attacks that have to be blocked low. */
    setup_player(&wk, 0);
    upper3s_parry_frame(&wk, 299);
    press_parry(&wk, U3S_PARRY_LOW);
    upper3s_parry_frame(&wk, 300);
    upper3s_parry_frame(&wk, 301);

    check(upper3s_parry_damage_permille(0, 1) == 1250, "an attack that has to be parried down does 25 percent more damage");
    check(upper3s_parry_damage_permille(0, 2) == 1000, "an attack that is parried another way does no extra damage");
}

/*
 * SM3 - the guess parries.
 */
static void test_guess_parry(void) {
    PLW wk;

    printf("SM3: a guess parry is only active for half as long\n");

    setup_player(&wk, 0);
    upper3s_parry_frame(&wk, 399);

    /* The retail window of the parry of a normal attack is two frames: the
     * lever is loaded with 11, counted down to 10 on the frame of the press,
     * and the parry is active while it is at 10 or 9. */
    check(GUARD_RESET - RED_BLOCKING_TIME == 2, "the retail parry window is two frames");
    check(GUARD_RESET - RED_BLOCKING_TIME_SPECIAL == 3, "the retail parry window of a special move is three frames");

    /* A normal parry: the retail window. */
    press_parry(&wk, U3S_PARRY_HIGH);
    upper3s_parry_frame(&wk, 400);

    check(upper3s_parry_threshold(&wk, U3S_PARRY_HIGH, RED_BLOCKING_TIME) == RED_BLOCKING_TIME,
          "the window of a normal parry is the retail one");

    /* A guess parry: the window is halved, rounded down. */
    setup_player(&wk, 0);
    upper3s_parry_frame(&wk, 499);
    wk.wu.routine_no[1] = 1;
    wk.wu.routine_no[2] = 88;
    press_parry(&wk, U3S_PARRY_HIGH);
    upper3s_parry_frame(&wk, 500);

    check(upper3s_parry_threshold(&wk, U3S_PARRY_HIGH, RED_BLOCKING_TIME) == GUARD_RESET - 1,
          "the window of a guess parry is half of the retail one");

    /* The window of a special move is three frames: half of it is one. */
    check(upper3s_parry_threshold(&wk, U3S_PARRY_HIGH, RED_BLOCKING_TIME_SPECIAL) == GUARD_RESET - 1,
          "the window of a guess parry of a special move is half of the retail one");
}

/*
 * SM3 - the states of the guess parries.
 */
static void test_guess_parry_states(void) {
    PLW wk;

    printf("SM3: the wakeup, the roll and the landing are guess parry states\n");

    setup_player(&wk, 0);

    check(upper3s_parry_is_guess_state(&wk) == 0, "a character that is not recovering is no guess parry state");

    /* The wakeup animation. */
    wk.wu.routine_no[1] = 1;
    wk.wu.routine_no[2] = 88;
    check(upper3s_parry_is_guess_state(&wk) == 1, "the wakeup is a guess parry state");

    /* A normal hitstun is not a guess parry state. */
    wk.wu.routine_no[2] = 5;
    wk.wu.xyz[1].disp.pos = 0;
    wk.wu.old_pos[1] = 0;
    check(upper3s_parry_is_guess_state(&wk) == 0, "a normal hitstun is no guess parry state");

    /* The hitstun of landing on the feet from the air. */
    wk.wu.old_pos[1] = 100;
    check(upper3s_parry_is_guess_state(&wk) == 1, "the landing on the feet is a guess parry state");

    /* The recovery roll. */
    wk.wu.old_pos[1] = 0;
    wk.wu.routine_no[1] = 0;
    wk.wu.my_roll_flag = 1;
    check(upper3s_parry_is_guess_state(&wk) == 1, "the recovery roll is a guess parry state");
}

/*
 * SM3 - a throw that lands in the window or in the cooldown of a guess
 * parry cannot be teched.
 */
static void test_guess_parry_throw_lock(void) {
    PLW wk;

    printf("SM3: a throw in the window of a guess parry cannot be teched\n");

    setup_player(&wk, 0);
    upper3s_parry_frame(&wk, 599);

    /* The player is in the wakeup animation and attempts a parry. */
    wk.wu.routine_no[1] = 1;
    wk.wu.routine_no[2] = 88;
    press_parry(&wk, U3S_PARRY_HIGH);
    upper3s_parry_frame(&wk, 600);

    check(upper3s_parry_guess_locked(0) == 1, "the throw cannot be teched in the window of the guess parry");

    upper3s_parry_frame(&wk, 601);
    check(upper3s_parry_guess_locked(0) == 1, "the throw cannot be teched in the cooldown of the guess parry");

    upper3s_parry_frame(&wk, 603);
    check(upper3s_parry_guess_locked(0) == 0, "the throw can be teched again after the guess parry");

    /* A normal parry does not lock the throw tech. */
    setup_player(&wk, 0);
    upper3s_parry_frame(&wk, 699);
    press_parry(&wk, U3S_PARRY_HIGH);
    upper3s_parry_frame(&wk, 700);

    check(upper3s_parry_guess_locked(0) == 0, "a normal parry does not lock the throw tech");
}

int main() {
    test_throw_invulnerability();
    test_superflash_lock();
    test_parry_cooldown();
    test_missed_parry_damage();
    test_guess_parry();
    test_guess_parry_states();
    test_guess_parry_throw_lock();

    printf("\n%d checks, %d failures\n", checks, failures);

    if (failures) {
        return 1;
    }

    return 0;
}
