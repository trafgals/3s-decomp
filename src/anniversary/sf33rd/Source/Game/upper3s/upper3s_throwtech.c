#include "common.h"
#include "sf33rd/Source/Game/upper3s_throwtech.h"

/*
 * Third Strike Upper (3s') - the throw tech.
 *
 * B4 - the character that teched an air throw may not attack until it has
 * landed again.
 *
 * The retail game knows the air throw tech: PLPCA.c check_nagenuke() puts the
 * character that broke the throw into the state 49 of the normal state
 * machine when it was in the air, and into the state 47 when it was on the
 * ground. PLPNM.c Normal_47000() plays the reaction of the tech - the sub
 * state 3 is the fall back to the ground, the sub state 4 the landing.
 *
 * The retail game only locks the ground throw tech out of its input
 * (PLS00.c nm_47000() waits for the sub state 4), so the character that
 * teched an air throw may start an attack while it is still in the air - and
 * the attack comes out the frame the tech reaction ends. The 3s' applies the
 * lock of the ground throw tech to the air throw tech as well.
 *
 * The lock is read out of the state of the character, so it cannot get stuck
 * and neither PLW nor WORK has to grow.
 */

s32 upper3s_throwtech_attack_locked(PLW* wk) {
    /* The normal state machine - the throw tech states live in it. */
    if (wk->wu.routine_no[1] != 0) {
        return 0;
    }

    /* The state of the character that teched an air throw. The state 50 is
     * the thrower of the air throw: it is not affected by the patch. */
    if (wk->wu.routine_no[2] != U3S_THROWTECH_AIR_TECH) {
        return 0;
    }

    /* The character may attack again once it has landed. */
    return (wk->wu.routine_no[3] < U3S_THROWTECH_LANDED);
}
