#ifndef UPPER3S_SYS_H
#define UPPER3S_SYS_H

#include "structs.h"
#include "types.h"

/*
 * Third Strike Upper (3s') - shared system layer.
 *
 * The helpers of this header implement the parts of the patch that are not
 * tied to a single character: the scaling of the metergain of whiffed
 * normals, the damage scaling of a cross up and the shared health / stun
 * bar tables.
 *
 * The state lives in this module because neither PLW nor WORK may grow -
 * their size is part of the PS2 memory map.
 *
 * Every character is addressed by its character index (the order of
 * AGENTS.md section 3.1), never by its charset id.
 */

/* The two player slots. */
#define U3S_SYS_PLAYERS 2

/* The kinds of contact used by upper3s_sys_whiff_contact(). */
#define U3S_SYS_CONTACT_HIT 0
#define U3S_SYS_CONTACT_BLOCK 1
#define U3S_SYS_CONTACT_PARING 2

/*
 * SM1 - whiffed normal metergain.
 *
 * `asag` is the metergain of the normal that is starting. The returned
 * value is the metergain that has to be granted: it is halved for every
 * consecutive whiffed normal (100%, 50%, 25%, 12.5%, ...).
 *
 * Only the normals are scaled - the metergain of a whiffed special is not
 * changed by the patch.
 */
s16 upper3s_sys_whiff_start(WORK* wk, s16 asag);

/*
 * SM1 - a move touched the opponent.
 *
 * `type` is one of U3S_SYS_CONTACT_*. The scaling of the characters that
 * were touched is reset. A blocked projectile resets nothing at all and a
 * parried projectile only resets the counter of the character that threw
 * it.
 */
void upper3s_sys_whiff_contact(WORK* as, WORK* ds, s16 type);

/*
 * SM5 - left right unblockables.
 *
 * Returns the scaling (in permille) of a hit that lands on a character
 * that is blocking while it is facing away from the attacker. 1000 means
 * "no change".
 */
s16 upper3s_sys_crossup_permille(WORK* as, WORK* ds);

/*
 * C - health.
 *
 * `char_id` is the character index. The function returns the character
 * index the health has to be taken from - the character itself when the
 * health is not overridden - and stores the scaling of the health in
 * `scale` (in permille).
 */
s16 upper3s_sys_health_source(s16 char_id, s16* scale);

/*
 * C - stun bar.
 *
 * Returns the size of the stun gauge of the character. `retail` is the
 * value of the retail table.
 */
s16 upper3s_sys_stun_genkai(s16 char_id, s16 retail);

#endif
