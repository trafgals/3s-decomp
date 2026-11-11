/*
 * Functional tests for the Third Strike Upper cancel rules of twelve.
 *
 * The design document grants a cancel to a few of Twelve's normal attacks:
 *
 *   - the standing Strong (5MP), the far Strong (6MP) and the crouching
 *     Strong (2MP) can be cancelled into a special move and into a super art,
 *   - the crouching Strong can also be cancelled into a super jump,
 *   - the crouching Forward (2MK) can be cancelled into a super art,
 *   - every air attack can be cancelled into a special move and into a super
 *     art.
 *
 * The cancel mask of a move is the "cg_cancel" byte of the frame chain of the
 * move, which is read from the disc, so the rules live in
 * upper3s/upper3s_ch_twelve.c and are applied by the shared hook of
 * PLS00.c:check_cg_cancel_data().
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_ch_twelve.h"

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
#define TWELVE_UNDISTURBED_MASK 0x00

int main(void) {
    u8 mask;
    s16 move;

    printf("twelve: the cancel rules of Third Strike Upper\n");

    /* The standing Strong and the far Strong. */
    mask = upper3s_ch_twelve_cancel_mask(U3S_TWELVE_STANDING_STRONG, TWELVE_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the standing Strong can be cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the standing Strong can be cancelled into a super art");

    mask = upper3s_ch_twelve_cancel_mask(U3S_TWELVE_FAR_STRONG, TWELVE_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the far Strong can be cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the far Strong can be cancelled into a super art");

    /* The other extended Medium Punch, the one the retail selection of twelve
     * reaches when the forward lever is held. */
    mask = upper3s_ch_twelve_cancel_mask(U3S_TWELVE_FAR_STRONG_REACH, TWELVE_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the far Strong of the retail can be cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the far Strong of the retail can be cancelled into a super art");

    /* The crouching Strong, with the super jump cancel. */
    mask = upper3s_ch_twelve_cancel_mask(U3S_TWELVE_CROUCHING_STRONG, TWELVE_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the crouching Strong can be cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Strong can be cancelled into a super art");
    check((mask & U3S_CANCEL_SUPERJUMP) != 0, "the crouching Strong can be cancelled into a super jump");

    /* The crouching Forward can only be cancelled into a super art. */
    mask = upper3s_ch_twelve_cancel_mask(U3S_TWELVE_CROUCHING_FORWARD, TWELVE_UNDISTURBED_MASK);
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Forward can be cancelled into a super art");
    check((mask & U3S_CANCEL_SPECIAL) == 0, "the crouching Forward is not cancelled into a special move");

    /* Every air attack gets the cancel. */
    for (move = U3S_TWELVE_AIR_FIRST; move <= U3S_TWELVE_AIR_LAST; move++) {
        mask = upper3s_ch_twelve_cancel_mask(move, TWELVE_UNDISTURBED_MASK);

        if (((mask & U3S_CANCEL_SPECIAL) == 0) || ((mask & U3S_CANCEL_SUPER) == 0)) {
            check(0, "an air attack can be cancelled into a special move and into a super art");
            break;
        }
    }

    if (move > U3S_TWELVE_AIR_LAST) {
        check(1, "every air attack can be cancelled into a special move and into a super art");
    }

    /* A move that the document does not touch keeps the mask of the disc. */
    mask = upper3s_ch_twelve_cancel_mask(0, TWELVE_UNDISTURBED_MASK);
    check(mask == TWELVE_UNDISTURBED_MASK, "the standing Jab keeps the cancel mask of the disc");

    mask = upper3s_ch_twelve_cancel_mask(24, TWELVE_UNDISTURBED_MASK);
    check(mask == TWELVE_UNDISTURBED_MASK, "the crouching Fierce keeps the cancel mask of the disc");

    /* The bits that are already in the mask are kept. */
    mask = upper3s_ch_twelve_cancel_mask(U3S_TWELVE_CROUCHING_STRONG, U3S_CANCEL_CHAIN);
    check((mask & U3S_CANCEL_CHAIN) != 0, "the chain cancel of the disc is kept");
    check((mask & U3S_CANCEL_SUPERJUMP) != 0, "the crouching Strong keeps its super jump cancel");

    printf("twelve: %d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
