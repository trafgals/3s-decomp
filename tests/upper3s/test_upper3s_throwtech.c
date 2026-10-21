/*
 * Functional tests for the Third Strike Upper throw tech (B4).
 *
 * B4 - "Fix Bug Where Some Characters Can Attack Immediately After Teching
 * Air Throws" (Makoto, etc.).
 *
 * The retail game lets the character that teched an air throw start an
 * attack while it is still falling back to the ground, so the tech is worth a
 * free hit. The 3s' locks the character out of attacking until it has landed
 * again - the lock the retail game already applies to the ground throw tech.
 *
 * The test drives the lock with synthetic states: the state of the character
 * is what the retail code writes when it detects the tech (PLPCA.c
 * check_nagenuke) and what the state machine of the reaction walks through
 * (PLPNM.c Normal_47000).
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_throwtech.h"

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
 * Puts `wk` into the state of the normal state machine that the throw tech
 * uses. `state` is the state of the character (47 the ground throw tech, 49
 * the air throw tech, 50 the thrower of an air throw) and `sub_state` is the
 * step of the reaction.
 */
static void setup_throw_tech(PLW* wk, s16 state, s16 sub_state) {
    memset(wk, 0, sizeof(PLW));

    wk->wu.work_id = 1; /* a character, not a projectile */
    wk->wu.id = 0;
    wk->player_number = 0;

    /* routine_no[0] is 4 while the character is playing (PLMAIN.c
     * plmain_lv_00), routine_no[1] is the kind of the state machine. */
    wk->wu.routine_no[0] = 4;
    wk->wu.routine_no[1] = 0;
    wk->wu.routine_no[2] = state;
    wk->wu.routine_no[3] = sub_state;
}

/*
 * The character that teched an air throw is locked out of attacking from the
 * tech until it has landed again.
 */
static void test_the_air_throw_tech_is_locked(void) {
    PLW wk;

    printf("throw tech: the character that teched an air throw cannot attack\n");

    /* The tech itself: the sub state 0 starts the reaction of the tech. */
    setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 0);
    check(upper3s_throwtech_attack_locked(&wk) == 1, "the frame of the tech is locked");

    /* The reaction of the tech: the sub states 1 and 2. */
    setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 1);
    check(upper3s_throwtech_attack_locked(&wk) == 1, "the reaction of the tech is locked");

    setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 2);
    check(upper3s_throwtech_attack_locked(&wk) == 1, "the end of the reaction is locked");

    /* The fall back to the ground: the sub state 3. */
    setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 3);
    check(upper3s_throwtech_attack_locked(&wk) == 1, "the fall back to the ground is locked");

    /* The landing: the sub state 4. */
    setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 4);
    check(upper3s_throwtech_attack_locked(&wk) == 0, "the character may attack once it has landed");
}

/*
 * The lockout of the air throw tech covers the whole reaction of the tech and
 * the fall back to the ground: the character cannot attack on any frame
 * before it touches the ground.
 */
static void test_the_lockout_covers_the_whole_reaction(void) {
    PLW wk;
    s16 frame;
    s16 locked_frames = 0;

    printf("throw tech: the lockout covers the reaction and the fall\n");

    /* The retail reaction of the tech freezes the character for 18 frames
     * (PLPNM.c Normal_47000 sets hit_stop to -18). The frame of the tech is
     * the frame the tech is detected in, the reaction is frozen on the
     * frames 1 to 18 and the character falls back to the ground after
     * that. */
    for (frame = 0; frame < 18; frame++) {
        setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 0);
        wk.wu.hit_stop = -18;

        if (upper3s_throwtech_attack_locked(&wk)) {
            locked_frames++;
        }
    }

    check(locked_frames == 18, "the whole tech reaction is locked out");

    /* The fall back to the ground is locked out as well. */
    setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 3);
    check(upper3s_throwtech_attack_locked(&wk) == 1, "the fall back to the ground is locked out");
}

/*
 * The patch only touches the character that teched the air throw. The ground
 * throw tech is already locked out by the retail game (PLS00.c nm_47000 waits
 * for the landing) and the thrower of the air throw is not affected by the
 * design document.
 */
static void test_the_other_states_are_not_locked(void) {
    PLW wk;

    printf("throw tech: the other states keep the retail behaviour\n");

    /* The ground throw tech: the retail game locks it out itself. */
    setup_throw_tech(&wk, U3S_THROWTECH_GROUND_TECH, 0);
    check(upper3s_throwtech_attack_locked(&wk) == 0, "the ground throw tech is not touched");

    setup_throw_tech(&wk, U3S_THROWTECH_GROUND_TECH, 3);
    check(upper3s_throwtech_attack_locked(&wk) == 0, "the fall of the ground throw tech is not touched");

    /* The thrower of the air throw. */
    setup_throw_tech(&wk, U3S_THROWTECH_AIR_THROW, 0);
    check(upper3s_throwtech_attack_locked(&wk) == 0, "the thrower of the air throw is not touched");

    setup_throw_tech(&wk, U3S_THROWTECH_AIR_THROW, 3);
    check(upper3s_throwtech_attack_locked(&wk) == 0, "the fall of the thrower is not touched");

    /* The thrower of the ground throw. */
    setup_throw_tech(&wk, U3S_THROWTECH_GROUND_THROW, 0);
    check(upper3s_throwtech_attack_locked(&wk) == 0, "the thrower of the ground throw is not touched");

    /* An ordinary state of the normal state machine. */
    setup_throw_tech(&wk, 1, 0);
    check(upper3s_throwtech_attack_locked(&wk) == 0, "an ordinary state is not locked");

    /* A state of another state machine. */
    setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 0);
    wk.wu.routine_no[1] = 4;
    check(upper3s_throwtech_attack_locked(&wk) == 0, "a state of another state machine is not locked");
}

/*
 * The lock is read out of the state of the character, so it cannot get stuck:
 * as soon as the character leaves the state of the air throw tech it may
 * attack again.
 */
static void test_the_lock_ends_with_the_state(void) {
    PLW wk;

    printf("throw tech: the lock ends with the state of the tech\n");

    setup_throw_tech(&wk, U3S_THROWTECH_AIR_TECH, 0);
    check(upper3s_throwtech_attack_locked(&wk) == 1, "the lock is on inside the state of the tech");

    /* The reaction of the tech ends and the character walks away. */
    setup_throw_tech(&wk, 1, 0);
    check(upper3s_throwtech_attack_locked(&wk) == 0, "the lock is off outside the state of the tech");
}

int main(void) {
    printf("Third Strike Upper - throw tech tests\n\n");

    test_the_air_throw_tech_is_locked();
    test_the_lockout_covers_the_whole_reaction();
    test_the_other_states_are_not_locked();
    test_the_lock_ends_with_the_state();

    printf("\n%d checks, %d failures\n", checks, failures);

    if (failures != 0) {
        printf("FAILED\n");
        return 1;
    }

    printf("PASSED\n");
    return 0;
}
