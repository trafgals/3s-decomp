/*
 * Functional tests for the Third Strike Upper left-right unblockables (SM5).
 *
 * "any time a character is hit during block animation while they are facing
 * the opposite direction from the hit, the damage scaling is reduced to 50%
 * for that hit and the followup combo."
 *
 * The rule is upper3s_sys_crossup_permille() of upper3s/upper3s_sys.c; it is
 * checked here on synthetic works, and in the real engine by
 * tests/match/test_system.py (LeftRightUnblockable).
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_sys.h"

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

/* A player at `x`, facing right when `rl` is 1, in routine group `rno1`, state `rno2`. */
static void player(WORK* wk, s16 id, s16 x, s16 rl, s16 rno1, s16 rno2) {
    memset(wk, 0, sizeof(*wk));
    wk->work_id = 1;
    wk->id = id;
    wk->xyz[0].disp.pos = x;
    wk->rl_flag = rl;
    wk->routine_no[1] = rno1;
    wk->routine_no[2] = rno2;
}

int main(void) {
    WORK as;
    WORK ds;

    printf("SM5: a hit on a blocking character from behind, and its combo, are scaled to 50%%\n");

    /* The attacker is on the right of the defender. */
    player(&as, 0, 500, 0, 4, 0);

    /* Blocking (blockstun, reaction 5) and facing the attacker: an ordinary block. */
    player(&ds, 1, 450, 1, 1, 5);
    check(upper3s_sys_crossup_permille(&as, &ds) == 1000, "a hit from the side the defender faces is not scaled");

    /* Blocking and facing away from the attacker. */
    player(&ds, 1, 450, 0, 1, 5);
    check(upper3s_sys_crossup_permille(&as, &ds) == 500, "a hit from behind during blockstun is scaled to 50%");

    /* The followup combo: the defender is in hitstun now. */
    ds.routine_no[2] = 12;
    check(upper3s_sys_crossup_permille(&as, &ds) == 500, "the next hit of the combo is scaled too");
    ds.routine_no[2] = 16;
    check(upper3s_sys_crossup_permille(&as, &ds) == 500, "and the one after it");

    /* The defender recovers: a new combo is not scaled. */
    ds.routine_no[1] = 0;
    ds.routine_no[2] = 1;
    check(upper3s_sys_crossup_permille(&as, &ds) == 1000, "a hit after the defender has recovered is not scaled");
    ds.routine_no[1] = 1;
    ds.routine_no[2] = 12;
    check(upper3s_sys_crossup_permille(&as, &ds) == 1000, "nor is the combo that follows it");

    /* The block posture of the normal state group (27-30) is a block animation. */
    player(&ds, 1, 450, 0, 0, 28);
    check(upper3s_sys_crossup_permille(&as, &ds) == 500, "the block posture counts as blocking");

    /* The parry states (31-35) are not. */
    player(&ds, 1, 450, 0, 0, 31);
    check(upper3s_sys_crossup_permille(&as, &ds) == 1000, "a parry state is not a block");

    /* A crossup on a character that is not blocking is not scaled. */
    player(&ds, 1, 450, 0, 0, 1);
    check(upper3s_sys_crossup_permille(&as, &ds) == 1000, "a crossup on a free character is not scaled");

    /* The attacker on the left, the defender blocking and facing right (away). */
    player(&as, 0, 400, 1, 4, 0);
    player(&ds, 1, 450, 1, 1, 9);
    check(upper3s_sys_crossup_permille(&as, &ds) == 500, "the same from the other side, in an air block reaction");

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
