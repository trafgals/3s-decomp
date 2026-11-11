/*
 * Functional tests for the Third Strike Upper cancel rules of q.
 *
 * The design document grants a cancel to four of Q's normal attacks:
 *
 *   - the crouching Jab (2LP), the standing Short (5LK) and the crouching
 *     Short (2LK) can be cancelled into a special move and into a super art,
 *   - the crouching Forward (2MK) can be cancelled into a super art.
 *
 * The cancel mask of a move is the "cg_cancel" byte of the frame chain of the
 * move, which is read from the disc, so the rules live in
 * upper3s/upper3s_ch_q.c and are applied by the shared hook of
 * PLS00.c:check_cg_cancel_data().
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_ch_q.h"

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

/* The mask of a move that the design document does not touch. */
#define Q_UNDISTURBED_MASK 0x00

int main(void) {
    u8 mask;

    printf("q: the cancel rules of Third Strike Upper\n");

    /* The crouching Jab, the standing Short and the crouching Short. */
    mask = upper3s_ch_q_cancel_mask(U3S_Q_CROUCHING_JAB, Q_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the crouching Jab can be cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Jab can be cancelled into a super art");

    mask = upper3s_ch_q_cancel_mask(U3S_Q_STANDING_SHORT, Q_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the standing Short can be cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the standing Short can be cancelled into a super art");

    mask = upper3s_ch_q_cancel_mask(U3S_Q_CROUCHING_SHORT, Q_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the crouching Short can be cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Short can be cancelled into a super art");

    /* The crouching Forward can only be cancelled into a super art. */
    mask = upper3s_ch_q_cancel_mask(U3S_Q_CROUCHING_FORWARD, Q_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Forward can be cancelled into a super art");
    check((mask & U3S_CANCEL_SPECIAL) == 0, "the crouching Forward is not cancelled into a special move");

    /* A move that the document does not touch keeps the mask of the disc. */
    mask = upper3s_ch_q_cancel_mask(0, Q_UNDISTURBED_MASK);
    check(mask == Q_UNDISTURBED_MASK, "the standing Jab keeps the cancel mask of the disc");

    mask = upper3s_ch_q_cancel_mask(24, Q_UNDISTURBED_MASK);
    check(mask == Q_UNDISTURBED_MASK, "the crouching Fierce keeps the cancel mask of the disc");

    /* The bits that are already in the mask are kept. */
    mask = upper3s_ch_q_cancel_mask(U3S_Q_CROUCHING_JAB, U3S_CANCEL_CHAIN);
    check((mask & U3S_CANCEL_CHAIN) != 0, "the chain cancel of the disc is kept");
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the crouching Jab keeps its special cancel");

    printf("q: %d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
