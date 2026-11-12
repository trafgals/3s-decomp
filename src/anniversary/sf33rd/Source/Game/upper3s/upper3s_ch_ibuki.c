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

/* IB3, the kunai cancelled from Tsuiji Goe (upper3s_ch_ibuki.h). */
int upper3s_ibuki_cancel_faces_opponent(s16 player_number, s16 koc, s16 char_index, s16 height) {
    return (player_number == 7) && (koc == 5) && (char_index >= 107) && (char_index <= 110) && (height > 0);
}

/* IB4, the Fierce of the target combos always resets (upper3s_ch_ibuki.h). */
int upper3s_ibuki_tc_fierce_resets(s16 work_id, s16 player_number, s16 koc, s16 char_index) {
    return (work_id == 1) && (player_number == 7) && (koc == 4) && (char_index == 165);
}
