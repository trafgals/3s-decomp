/*
 * Functional tests for Sean's Third Strike Upper rules (upper3s/upper3s_ch_sean.c).
 *
 * SN6: a kick in the startup of the Fierce Tackle fakes it. The fight test is
 * tests/match/test_cancels.py (SeanTackle).
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_ch_sean.h"

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

int main(void) {
    printf("SN6: the Fierce Tackle can be faked with a kick in its startup\n");

    check(upper3s_ch_sean_tackle_feint(0, 5, U3S_SEAN_FIERCE_TACKLE, 0, 0x100) == 1, "a kick in the dash fakes it");
    check(upper3s_ch_sean_tackle_feint(0, 5, U3S_SEAN_FIERCE_TACKLE, 0, 0x10) == 0, "a punch does not");
    check(upper3s_ch_sean_tackle_feint(0, 5, 25, 0, 0x100) == 0, "the Strong Tackle cannot be faked");
    check(upper3s_ch_sean_tackle_feint(0, 4, U3S_SEAN_FIERCE_TACKLE, 0, 0x100) == 0, "a normal with the same index is not the Tackle");

    /* Once the catch box is out, the startup is over. */
    check(upper3s_ch_sean_tackle_feint(1, 5, U3S_SEAN_FIERCE_TACKLE, 3, 0x400) == 0, "not when it can grab");
    check(upper3s_ch_sean_tackle_feint(1, 5, U3S_SEAN_FIERCE_TACKLE, 0, 0x400) == 0, "nor after it could grab");

    /* A new Tackle can be faked again. */
    upper3s_ch_sean_tackle_feint(1, 0, 0, 0, 0);
    check(upper3s_ch_sean_tackle_feint(1, 5, U3S_SEAN_FIERCE_TACKLE, 0, 0x400) == 1, "a new Tackle can be faked");

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
