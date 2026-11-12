/*
 * Functional tests for the Third Strike Upper input layer.
 *
 * The universal overhead (the simultaneous press of MP and MK) and the
 * personal action (the simultaneous press of HP and HK, the taunt of the
 * character) only come out in the retail game when the stick is in the
 * neutral position: when the stick is held away or towards, the engine
 * throws the input away and the command turns into a normal attack.
 *
 * 3s' lets both of them through while the stick is held away or towards.
 * The decision is a pure function of the lever state, so it can be
 * exercised on synthetic state here.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>

#include "sf33rd/Source/Game/upper3s_input.h"

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
 * The retail check of PLS03.c: the command is rejected when the stick is
 * not in the neutral position.  Kept here as the reference the 3s' decision
 * is compared against.
 */
static int retail_direction_ok(u16 sw_lvbt, u16 retail_blocked) {
    return (sw_lvbt & retail_blocked) == 0;
}

/* The universal overhead (MP+MK) of everybody but Hugo. */
static int overhead_ok(u16 sw_lvbt) {
    return upper3s_input_combo_direction_ok(sw_lvbt, U3S_INPUT_LEVER_ANY);
}

/* The personal action (HP+HK) of everybody but Hugo. */
static int personal_action_ok(u16 sw_lvbt) {
    return upper3s_input_combo_direction_ok(sw_lvbt, U3S_INPUT_LEVER_ANY);
}

/* The personal action (HP+HK) of Hugo, whose retail check does not block up. */
static int hugo_personal_action_ok(u16 sw_lvbt) {
    return upper3s_input_combo_direction_ok(sw_lvbt, U3S_INPUT_LEVER_ANY_BUT_UP);
}

static void test_the_masks_are_the_retail_ones(void) {
    printf("the masks of the retail checks\n");

    check(U3S_INPUT_LEVER_UP == 0x1, "up is the bit 0 of the lever");
    check(U3S_INPUT_LEVER_DOWN == 0x2, "down is the bit 1 of the lever");
    check(U3S_INPUT_LEVER_LEFT == 0x4, "left is the bit 2 of the lever");
    check(U3S_INPUT_LEVER_RIGHT == 0x8, "right is the bit 3 of the lever");
    check(U3S_INPUT_LEVER_ANY == 0xF, "the retail mask of everybody is 0xF");
    check(U3S_INPUT_LEVER_ANY_BUT_UP == 0xE, "the retail mask of Hugo is 0xE");
}

static void test_the_neutral_stick_still_works(void) {
    printf("the neutral stick\n");

    check(overhead_ok(0) == 1, "the universal overhead comes out with a neutral stick");
    check(personal_action_ok(0) == 1, "the personal action comes out with a neutral stick");
    check(hugo_personal_action_ok(0) == 1, "Hugo does his personal action with a neutral stick");
}

static void test_the_overhead_comes_out_while_holding_away_or_towards(void) {
    printf("the universal overhead while holding away or towards\n");

    check(overhead_ok(U3S_INPUT_LEVER_LEFT) == 1, "the universal overhead comes out while holding away");
    check(overhead_ok(U3S_INPUT_LEVER_RIGHT) == 1, "the universal overhead comes out while holding towards");
    check(overhead_ok(U3S_INPUT_LEVER_LEFT | U3S_INPUT_LEVER_RIGHT) == 1,
          "the universal overhead comes out while holding both sides");
}

static void test_the_personal_action_comes_out_while_holding_away_or_towards(void) {
    printf("the personal action while holding away or towards\n");

    check(personal_action_ok(U3S_INPUT_LEVER_LEFT) == 1, "the personal action comes out while holding away");
    check(personal_action_ok(U3S_INPUT_LEVER_RIGHT) == 1, "the personal action comes out while holding towards");
    check(hugo_personal_action_ok(U3S_INPUT_LEVER_LEFT) == 1, "Hugo does his personal action while holding away");
    check(hugo_personal_action_ok(U3S_INPUT_LEVER_RIGHT) == 1, "Hugo does his personal action while holding towards");
}

static void test_the_up_and_the_down_direction_still_block(void) {
    printf("the up and the down direction\n");

    check(overhead_ok(U3S_INPUT_LEVER_UP) == 0, "the universal overhead does not come out while holding up");
    check(overhead_ok(U3S_INPUT_LEVER_DOWN) == 0, "the universal overhead does not come out while holding down");
    check(personal_action_ok(U3S_INPUT_LEVER_UP) == 0, "the personal action does not come out while holding up");
    check(personal_action_ok(U3S_INPUT_LEVER_DOWN) == 0, "the personal action does not come out while holding down");

    /* Hugo is the exception of the retail game: his personal action comes
     * out while the stick is held up. */
    check(hugo_personal_action_ok(U3S_INPUT_LEVER_UP) == 1, "Hugo does his personal action while holding up");
    check(hugo_personal_action_ok(U3S_INPUT_LEVER_DOWN) == 0, "Hugo does not do his personal action while holding down");
    check(hugo_personal_action_ok(U3S_INPUT_LEVER_UP | U3S_INPUT_LEVER_LEFT) == 1,
          "Hugo does his personal action while holding up and away");
}

static void test_the_buttons_of_the_frame_do_not_matter(void) {
    printf("the buttons of the frame\n");

    /* sw_lvbt holds the buttons of the frame above the lever: MP is 0x20 and
     * MK is 0x200, so the universal overhead is 0x220, and HP is 0x40 and HK
     * is 0x400, so the personal action is 0x440. */
    check(overhead_ok(0x220) == 1, "MP+MK with a neutral stick comes out");
    check(overhead_ok(0x220 | U3S_INPUT_LEVER_LEFT) == 1, "MP+MK while holding away comes out");
    check(personal_action_ok(0x440 | U3S_INPUT_LEVER_RIGHT) == 1, "HP+HK while holding towards comes out");
    check(personal_action_ok(0x440 | U3S_INPUT_LEVER_DOWN) == 0, "HP+HK while holding down does not come out");
}

static void test_only_the_away_and_the_towards_direction_changed(void) {
    u16 lever;
    u16 difference;

    printf("the comparison with the retail check\n");

    difference = 0;

    /*
     * The retail check of everybody but Hugo only lets the neutral stick
     * through.  3s' lets the away and the towards direction through as well,
     * and nothing else: the stick that is held up or down is still rejected.
     */
    for (lever = 0; lever < 0x10; lever++) {
        int retail = retail_direction_ok(lever, U3S_INPUT_LEVER_ANY);
        int upper = overhead_ok(lever);
        int vertical = (lever & (U3S_INPUT_LEVER_UP | U3S_INPUT_LEVER_DOWN)) != 0;

        if (upper != (!vertical)) {
            check(0, "the decision of 3s' does not follow the rule of the patch");
            return;
        }

        if (upper != retail) {
            difference |= lever;
        }
    }

    check(1, "the decision of 3s' only lets the away and the towards direction through");
    check(difference == U3S_INPUT_LEVER_AWAY_TOWARDS,
          "the decision of 3s' differs from the retail one only for the away and the towards direction");

    /*
     * The retail check of Hugo only blocks the down, the away and the
     * towards direction, so his personal action comes out while the stick is
     * held up.  3s' adds the away and the towards direction to that.
     */
    difference = 0;

    for (lever = 0; lever < 0x10; lever++) {
        int retail = retail_direction_ok(lever, U3S_INPUT_LEVER_ANY_BUT_UP);
        int upper = hugo_personal_action_ok(lever);

        if (upper != !(lever & U3S_INPUT_LEVER_DOWN)) {
            check(0, "the decision of 3s' of Hugo does not follow the rule of the patch");
            return;
        }

        if (upper != retail) {
            difference |= lever;
        }
    }

    check(1, "the decision of 3s' of Hugo only lets the away and the towards direction through");
    check(difference == (U3S_INPUT_LEVER_UP | U3S_INPUT_LEVER_AWAY_TOWARDS),
          "the decision of 3s' of Hugo differs from the retail one only for the away and the towards direction");
}

int main(void) {
    printf("Third Strike Upper - input layer tests\n\n");

    test_the_masks_are_the_retail_ones();
    test_the_neutral_stick_still_works();
    test_the_overhead_comes_out_while_holding_away_or_towards();
    test_the_personal_action_comes_out_while_holding_away_or_towards();
    test_the_up_and_the_down_direction_still_block();
    test_the_buttons_of_the_frame_do_not_matter();
    test_only_the_away_and_the_towards_direction_changed();

    printf("\n%d checks, %d failures\n", checks, failures);

    if (failures != 0) {
        printf("FAILED\n");
        return 1;
    }

    printf("PASSED\n");
    return 0;
}
