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
    /*
     * Q8 - "Extend hitbox of first hit of EX High Speed Barrage downward so that it more consistently
     * connects against crouching characters."
     *
     * High Speed Barrage is 214+P (`cmd_data.c:p12_cmd_30`), special animations 36-39 of Q; 39 is the
     * EX one. Its first slap uses attack box set 73 (the later ones 74-77), and no other version uses
     * it. Measured: the set's lowest edge (59, box 2) is exactly the top of the crouching hurtbox of
     * Elena, Oro and Chun-Li, so the first slap misses them crouching or crouch-blocking at every
     * distance, and Makoto too when she is in crouching hitstun (after Q's c.LK, a 3s' cancel). One
     * lower is enough for the first three; 4 is the least that also reaches Makoto (2 and 3 do not).
     * Each of the three boxes reaches 4 further down, its top unchanged. The second slap still misses
     * Elena, Oro and Chun-Li after c.LK: that is Q7 (the opponent's movement on the middle hits).
     */
    { 17, 5, 39, U3S_BOX_ATTACK, 73, 0, { -96, 63, 80, 19 }, { -96, 63, 76, 23 } }, /* Q8: 4 lower */
    { 17, 5, 39, U3S_BOX_ATTACK, 73, 1, { -81, 59, 68, 20 }, { -81, 59, 64, 24 } }, /* Q8: 4 lower */
    { 17, 5, 39, U3S_BOX_ATTACK, 73, 2, { -61, 40, 59, 9 }, { -61, 40, 55, 13 } },  /* Q8: 4 lower */
    { -1, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } /* terminator */
};
