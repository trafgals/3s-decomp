/*
 * Functional tests for the Third Strike Upper changes of Ibuki.
 *
 * Ibuki - "Remove the ability to throw-tech her Personal Action (taunt),
 * and allow Ibuki to parry or attack in the air once the opponent recovers."
 *
 * The retail game opens a throw tech window while the personal action is
 * played: PLPAT07.c:Att_PL07_TOKUSHUKOUDOU adds 14 to PLW.tk_dageki and to
 * PLW.tk_nage when the personal action reaches the frame that opens the
 * window, and clamps both of them to 28 right afterwards.  The personal
 * action can therefore be teched for 28 frames.
 *
 * 3s' never opens the window: the personal action of Ibuki cannot be throw
 * teched any more, and a window that would still be open when the personal
 * action starts is closed.
 *
 * The decision is a pure function, so it can be exercised on synthetic state
 * here.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_ch_ibuki.h"

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
 * The retail routine of the personal action, as PLPAT07.c writes it: the
 * window is opened with 14 frames and the counters are clamped to 28.
 */
static void retail_personal_action(s16* dageki, s16* nage) {
    *dageki += U3S_IBUKI_RETAIL_TECH_WINDOW;
    *nage += U3S_IBUKI_RETAIL_TECH_WINDOW;

    if (*dageki > U3S_IBUKI_RETAIL_TECH_WINDOW_MAX) {
        *dageki = U3S_IBUKI_RETAIL_TECH_WINDOW_MAX;
    }

    if (*nage > U3S_IBUKI_RETAIL_TECH_WINDOW_MAX) {
        *nage = U3S_IBUKI_RETAIL_TECH_WINDOW_MAX;
    }
}

/*
 * The 3s' routine of the personal action: the window of the retail game is
 * not opened at all.
 */
static void upper3s_personal_action(s16* dageki, s16* nage) {
    *dageki = upper3s_ibuki_personal_action_tech_window();
    *nage = upper3s_ibuki_personal_action_tech_window();
}

/*
 * The personal action cannot be throw teched.
 */
static void test_the_personal_action_cannot_be_teched(void) {
    s16 dageki;
    s16 nage;

    printf("ibuki: the personal action cannot be throw teched\n");

    dageki = 0;
    nage = 0;
    upper3s_personal_action(&dageki, &nage);

    check(dageki == 0, "the throw tech window of the personal action is not opened");
    check(nage == 0, "the throw tech window of the throw tech of the personal action is not opened");
}

/*
 * The window that is still open when the personal action starts is closed.
 */
static void test_an_open_window_is_closed(void) {
    s16 dageki;
    s16 nage;

    printf("ibuki: an open throw tech window is closed by the personal action\n");

    /* The character has been thrown and the window of the throw is open. */
    dageki = 28;
    nage = 28;
    upper3s_personal_action(&dageki, &nage);

    check(dageki == 0, "the throw tech window of the throw is closed");
    check(nage == 0, "the throw tech window of the normal throw is closed");
}

/*
 * The retail game opens a window of 14 frames: the reference of the test.
 */
static void test_the_retail_window_is_the_reference(void) {
    s16 dageki;
    s16 nage;

    printf("ibuki: the retail personal action can be throw teched for 14 frames\n");

    dageki = 0;
    nage = 0;
    retail_personal_action(&dageki, &nage);

    check(dageki == 14, "the retail game opens a window of 14 frames");
    check(nage == 14, "the retail game opens a window of 14 frames");
}

int main(void) {
    test_the_personal_action_cannot_be_teched();
    test_an_open_window_is_closed();
    test_the_retail_window_is_the_reference();

    printf("%d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
