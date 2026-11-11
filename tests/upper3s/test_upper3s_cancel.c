/*
 * Functional tests for the Third Strike Upper cancel dispatcher.
 *
 * The cancels that the design document grants to a few normals are in the
 * module of the character (upper3s/upper3s_ch_<name>.c); the dispatcher
 * (upper3s/upper3s_cancel.c) is the one place that knows which rule belongs
 * to which character, and the shared hook of PLS00.c:check_cg_cancel_data()
 * calls it.
 *
 * The move is addressed by two things:
 *
 *   - the kind of character data it lives in (the slot 4 holds the normal
 *     attacks, the slot 5 the special moves and the super arts), and
 *   - its char_ix, the index of the move in that kind of data.
 *
 * The test checks that the dispatcher sends the move to the rule of its
 * character, that it leaves the other characters alone, that it leaves the
 * special moves and the super arts alone even when their index is the one of
 * a normal, and that it keeps the bits the disc already had.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"
#include "sf33rd/Source/Game/upper3s_ch_alex.h"
#include "sf33rd/Source/Game/upper3s_ch_remy.h"
#include "sf33rd/Source/Game/upper3s_ch_q.h"
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
#define UNTOUCHED_MASK 0x00

int main(void) {
    u8 mask;

    printf("cancel: the dispatcher of the Third Strike Upper rules\n");

    /* The rule of alex reaches the dispatcher. */
    mask = upper3s_cancel_mask(U3S_ALEX, U3S_KOC_NORMAL, U3S_ALEX_CROUCHING_STRONG, 0, UNTOUCHED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the crouching Strong of alex is cancelled into a special move");
    check((mask & U3S_CANCEL_SUPER) != 0, "the crouching Strong of alex is cancelled into a super art");

    /* The rule of q reaches the dispatcher. */
    mask = upper3s_cancel_mask(U3S_Q, U3S_KOC_NORMAL, U3S_Q_STANDING_SHORT, 0, UNTOUCHED_MASK);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the standing Short of q is cancelled into a special move");

    /* The rule of twelve reaches the dispatcher. */
    mask = upper3s_cancel_mask(U3S_TWELVE, U3S_KOC_NORMAL, U3S_TWELVE_CROUCHING_STRONG, 0, UNTOUCHED_MASK);
    check((mask & U3S_CANCEL_SUPERJUMP) != 0, "the crouching Strong of twelve is cancelled into a super jump");

    /* A character without a rule is left alone, even on a move that another
     * character has a rule for. */
    mask = upper3s_cancel_mask(U3S_Q, U3S_KOC_ATTACK, 33, 0, UNTOUCHED_MASK);
    check((mask & U3S_CANCEL_SUPER) != 0, "Q's overhead rush, a special move, is reached through the hook");

    mask = upper3s_cancel_mask(U3S_ALEX, U3S_KOC_ATTACK, U3S_ALEX_CROUCHING_STRONG, 0, UNTOUCHED_MASK);
    check(mask == UNTOUCHED_MASK, "another character's special moves are not touched");

    mask = upper3s_cancel_mask(U3S_RYU, U3S_KOC_NORMAL, U3S_ALEX_CROUCHING_STRONG, 0, UNTOUCHED_MASK);
    check(mask == UNTOUCHED_MASK, "ryu is not given the rule of alex");

    mask = upper3s_cancel_mask(U3S_GILL, U3S_KOC_NORMAL, U3S_TWELVE_CROUCHING_STRONG, 0, UNTOUCHED_MASK);
    check(mask == UNTOUCHED_MASK, "gill is not given the rule of twelve");

    /* The rules are about the normal attacks. A special move or a super art
     * has its own numbering, which overlaps the one of the normals, so the
     * kind of character data has to be checked. */
    mask = upper3s_cancel_mask(U3S_ALEX, U3S_KOC_ATTACK, U3S_ALEX_CROUCHING_STRONG, 0, UNTOUCHED_MASK);
    check(mask == UNTOUCHED_MASK, "a special move of alex is not given the rule of the normals");

    mask = upper3s_cancel_mask(U3S_TWELVE, U3S_KOC_ATTACK, U3S_TWELVE_AIR_FIRST, 0, UNTOUCHED_MASK);
    check(mask == UNTOUCHED_MASK, "a super art of twelve is not given the rule of the air normals");

    /* The bits of the disc are kept. */
    mask = upper3s_cancel_mask(U3S_ALEX, U3S_KOC_NORMAL, U3S_ALEX_CROUCHING_STRONG, 0, U3S_CANCEL_CHAIN);
    check((mask & U3S_CANCEL_CHAIN) != 0, "the chain cancel of the disc is kept");
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the special cancel is added to the chain cancel");

    mask = upper3s_cancel_mask(U3S_Q, U3S_KOC_NORMAL, U3S_Q_CROUCHING_JAB, 0, U3S_CANCEL_NORMAL);
    check((mask & U3S_CANCEL_NORMAL) != 0, "the normal cancel of the disc is kept");
    check((mask & U3S_CANCEL_SUPER) != 0, "the super cancel is added to the normal cancel");

    /* A move of a character with a rule that the design document does not
     * touch keeps the mask of the disc. */
    mask = upper3s_cancel_mask(U3S_ALEX, U3S_KOC_NORMAL, 3, 0, UNTOUCHED_MASK);
    check(mask == UNTOUCHED_MASK, "the standing Strong of alex keeps the mask of the disc");

    mask = upper3s_cancel_mask(U3S_TWELVE, U3S_KOC_NORMAL, U3S_TWELVE_AIR_LAST, 0, UNTOUCHED_MASK);
    check((mask & U3S_CANCEL_SUPER) != 0, "the last air attack of twelve is cancelled into a super art");

    mask = upper3s_cancel_mask(U3S_TWELVE, U3S_KOC_NORMAL, U3S_TWELVE_AIR_LAST + 1, 0, UNTOUCHED_MASK);
    check(mask == UNTOUCHED_MASK, "a move past the last air attack of twelve keeps the mask of the disc");

    /* EL1, EL3, Q5, AL4: rules that depend on the frame. */
    mask = upper3s_cancel_mask(U3S_ELENA, U3S_KOC_NORMAL, 27, 0, U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER);
    check((mask & U3S_CANCEL_CHAIN) != 0, "Elena's c.LK chains in its cancel window");
    mask = upper3s_cancel_mask(U3S_ELENA, U3S_KOC_NORMAL, 27, U3S_FRAME_ACTIVE, 0);
    check(mask == 0, "Elena's c.LK does not chain outside its cancel window");
    mask = upper3s_cancel_mask(U3S_ELENA, U3S_KOC_NORMAL, 3, U3S_FRAME_ACTIVE, 0);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the active frames of Elena's s.MP can be cancelled");
    mask = upper3s_cancel_mask(U3S_ELENA, U3S_KOC_NORMAL, 3, U3S_FRAME_HITSTOP, 0);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "the hitstop of Elena's s.MP can be cancelled");
    mask = upper3s_cancel_mask(U3S_ELENA, U3S_KOC_NORMAL, 3, 0, 0);
    check(mask == 0, "the recovery of Elena's s.MP cannot");
    mask = upper3s_cancel_mask(U3S_Q, U3S_KOC_NORMAL, 17, U3S_FRAME_ACTIVE, 0);
    check((mask & U3S_CANCEL_SPECIAL) != 0, "every active frame of Q's 4HK can be cancelled");
    mask = upper3s_cancel_mask(U3S_Q, U3S_KOC_NORMAL, 17, 0, 0);
    check(mask == 0, "the recovery of Q's 4HK cannot");
{
        u16 sw = 0x200;
        u16 now = 0;

        upper3s_ch_alex_target_combo_buffer(0, 27, U3S_CANCEL_SPECIAL, &sw, &now);
        sw = 0;
        upper3s_ch_alex_target_combo_buffer(0, 27, U3S_CANCEL_SPECIAL, &sw, &now);
        check(sw == 0, "the buffered MK is not pressed in the cancel window");
        upper3s_ch_alex_target_combo_buffer(0, 27, U3S_CANCEL_TARGET_COMBO, &sw, &now);
        check(sw == 0x200 && now == 0x200, "MK pressed in the cancel window is pressed on the target combo frame");
        sw = 0;
        upper3s_ch_alex_target_combo_buffer(0, 27, U3S_CANCEL_TARGET_COMBO, &sw, &now);
        check(sw == 0, "it is pressed once");
        sw = 0x200;
        upper3s_ch_alex_target_combo_buffer(1, 21, U3S_CANCEL_SPECIAL, &sw, &now);
        sw = 0;
        upper3s_ch_alex_target_combo_buffer(1, 27, U3S_CANCEL_TARGET_COMBO, &sw, &now);
        check(sw == 0, "another move does not buffer it");
    }

    /* RM6: Remy's crouching lights chain into each other and into nothing else. */
    check((upper3s_ch_remy_chain(18, 0x60, 0x100 | 0x2, 0x2) & U3S_CANCEL_NORMAL) != 0, "Remy's c.LP chains into c.LK");
    check((upper3s_ch_remy_chain(27, 0x60, 0x010 | 0x2, 0x2) & U3S_CANCEL_NORMAL) != 0, "Remy's c.LK chains into c.LP");
    check(upper3s_ch_remy_chain(18, 0x60, 0x200 | 0x2, 0x2) == 0x60, "Remy's c.LP does not chain into c.MK");
    check(upper3s_ch_remy_chain(18, 0x60, 0x100, 0) == 0x60, "nor into a standing Short");
    check(upper3s_ch_remy_chain(18, 0, 0x100 | 0x2, 0x2) == 0, "nor outside its cancel window");

    printf("cancel: %d checks, %d failures\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
