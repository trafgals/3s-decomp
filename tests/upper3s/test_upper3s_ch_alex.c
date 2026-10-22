/*
 * Functional tests for the Third Strike Upper cancel rules of alex.
 *
 * The design document grants two of Alex's normal attacks a cancel that the
 * retail game does not give them:
 *
 *   - the crouching Strong (2MP) can be cancelled into a special move and
 *     into a super art,
 *   - the crouching Forward (2MK) can be cancelled into a super art.
 *
 * The cancel mask of a move is the "cg_cancel" byte of the frame chain of the
 * move, which is read from the disc, so the rules live in
 * upper3s/upper3s_ch_alex.c and are applied by the shared hook of
 * PLS00.c:check_cg_cancel_data().
 *
 * The test checks the mask that the rule produces for the crouching Strong,
 * for the crouching Forward and for a move that the document does not touch.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_ch_alex.h"

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
#define ALEX_UNDISTURBED_MASK 0x00

int main(void) {
    u8 mask;

    printf("alex: the cancel rules of Third Strike Upper\n");

    /* The crouching Strong (2MP) can be cancelled into a special and a super. */
    mask = upper3s_ch_alex_cancel_mask(U3S_ALEX_CROUCHING_STRONG, ALEX_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the crouching Strong can be cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Strong can be cancelled into a super art");

    /* The crouching Forward (2MK) can only be cancelled into a super. */
    mask = upper3s_ch_alex_cancel_mask(U3S_ALEX_CROUCHING_FORWARD, ALEX_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Forward can be cancelled into a super art");
    check((mask & U3S_CANCEL_SPECIAL) == 0, "the crouching Forward is not cancelled into a special move");

    /* A move that the document does not touch keeps the mask of the disc. */
    mask = upper3s_ch_alex_cancel_mask(3, ALEX_UNDISTURBED_MASK);
    check(mask == ALEX_UNDISTURBED_MASK, "the standing Strong keeps the cancel mask of the disc");

    mask = upper3s_ch_alex_cancel_mask(24, ALEX_UNDISTURBED_MASK);
    check(mask == ALEX_UNDISTURBED_MASK, "the crouching Fierce keeps the cancel mask of the disc");

    /* The bits that are already in the mask are kept. */
    mask = upper3s_ch_alex_cancel_mask(U3S_ALEX_CROUCHING_STRONG, U3S_CANCEL_CHAIN);
    check((mask & U3S_CANCEL_CHAIN) != 0, "the chain cancel of the disc is kept");
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the crouching Strong keeps its special cancel");
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Strong keeps its super cancel");

    printf("alex: %d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
