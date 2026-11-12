/*
 * Third Strike Upper (3s') - the box changes of the design document
 * (include/sf33rd/Source/Game/upper3s_box.h). Each row says which item it is
 * for; the values are measured in the real engine by tests/match/.
 */

#include "sf33rd/Source/Game/upper3s_box.h"

const Upper3sBox upper3s_boxes[] = {
    { -1, 0, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } /* terminator */
};

const Upper3sMoveBox upper3s_move_boxes[] = {
    /*
     * RY4 - "Expand hitbox on LK Joudan Sokutou Geri (41236LK) downward so that it will not miss on
     * some characters after close Fierce (HP) or crouching Fierce (2HP)."
     *
     * The three Joudan Sokutou Geri are special move animations 60 (LK), 61 and 62 of Ryu, and they
     * share their attack box sets (30 on the first active frame, 31 on the next two), so the change
     * is made for animation 60 only. A box is x, width, y (its bottom, up from the character's feet)
     * and height. Measured: after close s.HP on a crouching opponent the LK one misses 15 of the 20
     * characters (after c.HP and on standing opponents it connects). Every box is made to reach 20
     * further down, its top unchanged: the least that connects on all 20 (Makoto needs 20; 12 leaves
     * 5 misses, 16-19 leave Makoto).
     */
    { 2, 5, 60, U3S_BOX_ATTACK, 30, 0, { -94, 22, 76, 18 }, { -94, 22, 56, 38 } }, /* RY4: 20 lower */
    { 2, 5, 60, U3S_BOX_ATTACK, 30, 1, { -50, 26, 50, 20 }, { -50, 26, 30, 40 } }, /* RY4: 20 lower */
    { 2, 5, 60, U3S_BOX_ATTACK, 30, 2, { -74, 26, 62, 20 }, { -74, 26, 42, 40 } }, /* RY4: 20 lower */
    { 2, 5, 60, U3S_BOX_ATTACK, 31, 2, { -88, 24, 72, 18 }, { -88, 24, 52, 38 } }, /* RY4: 20 lower */
    { -1, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } /* terminator */
};
