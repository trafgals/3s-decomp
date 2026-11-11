#include "common.h"
#include "sf33rd/Source/Game/upper3s_ch_ibuki.h"

/*
 * Third Strike Upper (3s') - the changes of Ibuki.
 *
 * Ibuki is the character 7 of the game (AGENTS.md section 3.1).
 */

/*
 * Ibuki - "Remove the ability to throw-tech her Personal Action (taunt)".
 *
 * The retail game opens a throw tech window while the personal action is
 * played (PLPAT07.c:Att_PL07_TOKUSHUKOUDOU adds 14 to PLW.tk_dageki and to
 * PLW.tk_nage and clamps both of them to 28), so that the personal action
 * can be teched for 14 frames.
 *
 * Third Strike Upper removes the window: the personal action of Ibuki cannot
 * be throw teched any more, and the window that would still be open when the
 * personal action starts is closed.
 */
u8 upper3s_ibuki_personal_action_tech_window(void) {
    return 0;
}
