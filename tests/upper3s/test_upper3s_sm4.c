/*
 * Functional tests for the Third Strike Upper charge partitioning (SM4).
 *
 * The charge command of the retail game is check_1() of CMD_MAIN.c.  It
 * counts the frames the charged direction is held in the work of the
 * command, and it counts the timer of the command down while the charged
 * direction is not held.  That timer is the charge partition timer and it
 * is only reloaded with its maximum when the command times out and is read
 * from the command table again, so the window in which a charge can be
 * partitioned depends on where that hidden timer happened to be when the
 * charge was started.
 *
 * SM4 resets the partition timer to its maximum as soon as a charged
 * direction has reached one third of its charge.  The test builds the
 * command table of a charge command the way the retail game stores it and
 * checks that the timer is reset - and that it is not reset before the
 * charge has reached one third.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_sm4.h"

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
 * The command table of a charge command, laid out the way the retail game
 * stores it: twelve header words, then the records of the command chain.
 * The chain is five words per record, the last word of a record is the
 * type of the next record.
 *
 *   12, 0, 0, 17, 17, 17, 17, 1029, 4, 5, 6, 23   the header
 *   2, 42, 42, 8                                    hold Back for 42 frames
 *   1, 10, 0, -32764, 28                            then hold Back exactly
 */
#define SM4_TABLE_SIZE 21
#define SM4_RECORD_OFFSET 12
#define SM4_NEXT_RECORD (SM4_RECORD_OFFSET + 4)

#define SM4_CHARGE_TIME 42
#define SM4_TIMER_MAX 42

/* The frames of charge that are one third of the charge. */
#define SM4_ONE_THIRD (SM4_CHARGE_TIME / 3)

static s16 sm4_table_down[SM4_TABLE_SIZE] = {
    12, 0, 0, 17, 17, 17, 17, 1029, 4, 5, 6, 23, 2, SM4_CHARGE_TIME, SM4_CHARGE_TIME, 0x2, 1, 10, 0, 1, 28
};

static s16 sm4_table_back[SM4_TABLE_SIZE] = {
    12, 0, 0, 17, 17, 17, 17, 1029, 4, 5, 6, 23, 2, SM4_CHARGE_TIME, SM4_CHARGE_TIME, 0x8, 1, 10, 0, -32764, 28
};

/*
 * Builds the work of a charge command.
 *
 * `held` is the number of frames the direction has been held for, `timer`
 * is the partition timer the command is left with.  The counter of the
 * charge is the one check_1() uses: free1 when the lever does not have to
 * match exactly, free2 when it does.
 */
static void build_charge(WAZA_WORK* work, s16* table, s16 lever, s16 held, s16 timer) {
    s16 counter = SM4_CHARGE_TIME - held;

    memset(work, 0, sizeof(*work));

    work->w_type = UPPER3S_SM4_CHARGE_TYPE;
    work->w_int = timer;
    work->w_lvr = lever;
    work->free1 = counter;
    work->free2 = counter;
    work->w_ptr = &table[SM4_NEXT_RECORD];
}

/*
 * The partition timer of a charge is reset to its maximum once the charge
 * has reached one third.
 */
static void test_one_third_resets_the_timer(void) {
    WAZA_WORK work[UPPER3S_SM4_WAZA_COUNT];

    // A charge that has just reached one third, with a partition timer that
    // is nearly over - the situation the retail game leaves to chance.
    build_charge(&work[0], sm4_table_back, 0x8, SM4_ONE_THIRD, 3);
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == SM4_TIMER_MAX, "the partition timer is reset to its maximum at one third of the charge");

    // The same for the charge that is done down.
    build_charge(&work[0], sm4_table_down, 0x2, SM4_ONE_THIRD, 3);
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == SM4_TIMER_MAX, "the partition timer of a down charge is reset at one third");

    // A charge that is further along is reset as well.
    build_charge(&work[0], sm4_table_back, 0x8, SM4_CHARGE_TIME, 3);
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == SM4_TIMER_MAX, "the partition timer stays at its maximum while the charge is held");

    // A charge that has not reached one third keeps the timer it has.
    build_charge(&work[0], sm4_table_back, 0x8, SM4_ONE_THIRD - 1, 3);
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == 3, "the partition timer is untouched before one third of the charge");

    // A charge that has not started keeps the timer it has.
    build_charge(&work[0], sm4_table_back, 0x8, 0, 3);
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == 3, "the partition timer is untouched before the charge has started");
}

/*
 * The lever of a charge has to match exactly for the counter of the charge
 * to be the one in free2.
 */
static void test_exact_lever_uses_free2(void) {
    WAZA_WORK work[UPPER3S_SM4_WAZA_COUNT];

    build_charge(&work[0], sm4_table_down, (s16)(0x8000 | 0x2), SM4_ONE_THIRD, 3);
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == SM4_TIMER_MAX, "a charge whose lever has to match exactly is reset at one third");

    build_charge(&work[0], sm4_table_down, (s16)(0x8000 | 0x2), SM4_ONE_THIRD - 1, 3);
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == 3, "a charge whose lever has to match exactly is not reset before one third");
}

/*
 * Only the charge commands are touched.
 */
static void test_other_commands_are_untouched(void) {
    WAZA_WORK work[UPPER3S_SM4_WAZA_COUNT];

    // A command that is not a charge.
    memset(&work[0], 0, sizeof(work[0]));
    work[0].w_type = 1;
    work[0].w_int = 3;
    work[0].w_lvr = 0x8;
    work[0].free1 = SM4_CHARGE_TIME - SM4_ONE_THIRD;
    work[0].w_ptr = &sm4_table_back[SM4_NEXT_RECORD];
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == 3, "a command that is not a charge is not touched");

    // A charge that is done up is not a charge direction.
    build_charge(&work[0], sm4_table_back, 0x1, SM4_ONE_THIRD, 3);
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == 3, "a charge that is not done in back or down is not touched");

    // A command without a table behind it must not be read.
    memset(&work[0], 0, sizeof(work[0]));
    work[0].w_type = UPPER3S_SM4_CHARGE_TYPE;
    work[0].w_int = 3;
    work[0].w_lvr = 0x8;
    work[0].free1 = SM4_CHARGE_TIME - SM4_ONE_THIRD;
    work[0].w_ptr = NULL;
    upper3s_sm4_charge_partition(work, UPPER3S_SM4_WAZA_COUNT);
    check(work[0].w_int == 3, "a charge without a command table is not touched");
}

/*
 * The rule on its own.
 */
static void test_partition_timer_rule(void) {
    check(upper3s_sm4_charge_partition_timer(SM4_ONE_THIRD - 1, SM4_CHARGE_TIME, 7, SM4_TIMER_MAX) == 7,
          "the timer is kept before one third of the charge");
    check(upper3s_sm4_charge_partition_timer(SM4_ONE_THIRD, SM4_CHARGE_TIME, 7, SM4_TIMER_MAX) == SM4_TIMER_MAX,
          "the timer is set to its maximum at one third of the charge");
    check(upper3s_sm4_charge_partition_timer(SM4_CHARGE_TIME, SM4_CHARGE_TIME, 7, SM4_TIMER_MAX) == SM4_TIMER_MAX,
          "the timer is set to its maximum for a finished charge");
    check(upper3s_sm4_charge_partition_timer(0, 0, 7, SM4_TIMER_MAX) == 7,
          "a charge without a charge time keeps the timer");
}

int main(void) {
    printf("Third Strike Upper - charge partitioning (SM4)\n");

    test_one_third_resets_the_timer();
    test_exact_lever_uses_free2();
    test_other_commands_are_untouched();
    test_partition_timer_rule();

    printf("%d checks, %d failures\n", checks, failures);

    if (failures != 0) {
        printf("FAILED\n");
        return 1;
    }

    printf("PASSED\n");
    return 0;
}
