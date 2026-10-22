#ifndef UPPER3S_THROWTECH_H
#define UPPER3S_THROWTECH_H

#include "structs.h"
#include "types.h"

/*
 * Third Strike Upper (3s') - the throw tech.
 *
 * B4 - "Fix Bug Where Some Characters Can Attack Immediately After Teching
 * Air Throws" (Makoto, etc.).
 *
 * The retail game lets the character that teched an air throw start an
 * attack while it is still falling back to the ground, so the tech is worth a
 * free hit. The 3s' locks the character out of attacking until it has landed
 * again - the lock the retail game already applies to the ground throw tech.
 *
 * The lock is read out of the state of the character, so it needs no state of
 * its own and neither PLW nor WORK may grow.
 */

/*
 * The states of the normal state machine that the throw tech uses
 * (PLPCA.c check_nagenuke):
 *
 *   47 - the character that teched a ground throw;
 *   48 - the thrower of a ground throw;
 *   49 - the character that teched an air throw;
 *   50 - the thrower of an air throw.
 *
 * Only the state 49 is changed by the patch.
 */
#define U3S_THROWTECH_GROUND_TECH 47
#define U3S_THROWTECH_GROUND_THROW 48
#define U3S_THROWTECH_AIR_TECH 49
#define U3S_THROWTECH_AIR_THROW 50

/*
 * The sub state of the throw tech states that is the landing
 * (PLPNM.c Normal_47000): the sub state 3 is the fall back to the ground.
 * The character may attack again once the sub state has reached the landing.
 */
#define U3S_THROWTECH_LANDED 4

/*
 * Returns 1 while the character may not start an attack because it teched an
 * air throw and has not landed again yet.
 *
 * The character that teched the air throw is in the state 49 of the normal
 * state machine; the thrower of the air throw is in the state 50 and is not
 * affected by the patch.
 */
s32 upper3s_throwtech_attack_locked(PLW* wk);

#endif
