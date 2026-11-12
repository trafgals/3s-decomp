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
    /*
     * RM5 (the hitboxes) - "enlarge hitboxes on 2nd hit of Target Combo (standing Forward->RH) so that
     * it connects on everyone standing and most people crouching. Bugfix, it completely whiffs against
     * Yun/Yang/etc at point blank when standing."
     *
     * The target combo is normal animation 157 of Remy (the close s.MK, normal 12, branches into it when
     * it hits). Its s.HK uses attack box sets 25 (lowest edge 84) and 26 (75); the straight line of the
     * chain is the s.MK part, so these rows are reached through the branch. Measured at 20 and 50
     * apart: retail misses standing Yun, Oro and Yang and every crouching character but Hugo. 18 lower
     * is the least that reaches everyone standing but still misses 9 of the 20 crouching (barely
     * "most"); 20 misses 4 crouching (Elena, Oro, Urien, Chun-Li), and 3 with the s.MK's lower
     * knockback (pl18.c), which brings Urien in; 30 reaches all, more than the post asks. Each box
     * reaches 20 further down, its top unchanged.
     */
    { 19, 4, 157, U3S_BOX_ATTACK, 25, 0, { -84, 13, 88, 8 }, { -84, 13, 68, 28 } },   /* RM5: 20 lower */
    { 19, 4, 157, U3S_BOX_ATTACK, 25, 1, { -72, 20, 84, 12 }, { -72, 20, 64, 32 } },  /* RM5: 20 lower */
    { 19, 4, 157, U3S_BOX_ATTACK, 26, 0, { -82, 29, 79, 18 }, { -82, 29, 59, 38 } },  /* RM5: 20 lower */
    { 19, 4, 157, U3S_BOX_ATTACK, 26, 2, { -51, 18, 75, 17 }, { -51, 18, 55, 37 } },  /* RM5: 20 lower */
    /*
     * CL4 (the startup hitbox) - "Tensei Ranka (SAIII) startup hitbox reaches down to the floor so it
     * can't be crouched".
     *
     * Tensei Ranka is 236236+K, special animation 52 of Chun-Li. Its startup hit is block 4 (attack
     * entry 106, 3 frames); the later hits (entries 102-105) use the same box set 94 and hit index
     * 436, so the row names the block. A box is placed from the character's position, and Chun-Li
     * is already rising during the startup hit (54, 66 and 76 above the floor on its three frames),
     * so the box's bottom (26) is 80-102 above the floor and misses crouching opponents. Its bottom
     * goes to -76, the floor on the highest of the three frames; its top (77) is unchanged.
     */
    { 15, 5, 52, U3S_BOX_ATTACK, 94, 0, { -59, 45, 26, 51 }, { -59, 45, -76, 153 }, 5 }, /* CL4: block 4 reaches the floor */
    /*
     * YU4 - "Upkicks (623K) hitbox adjusted to reach further downward, so it misses standing/crouching
     * opponents less often. Bugfix."
     *
     * The upkicks are special animations 32-35 of Yun (LK, MK, HK, EX), with attack box sets 107 and
     * 108 (lowest edges 66 and 69). Measured: point-blank they miss every crouching character but
     * Gill and Hugo, and cancelled from s.LP, c.LP, c.MP, s.MP or close s.HP they miss 2 to 12
     * standing characters. The post gives no amount, so the value is the user's choice (issue #20):
     * 40 lower, the least of the measured values with which no standing character is missed after
     * any of those cancels (20 still left 3 after s.MP and s.HP, 30 three after s.HP, 38 two). Each
     * box reaches 40 further down, its top unchanged, on the four animations only.
     */
    { 3, 5, 32, U3S_BOX_ATTACK, 107, 0, { -48, 24, 90, 27 }, { -48, 24, 50, 67 } }, /* YU4: LK, 40 lower */
    { 3, 5, 32, U3S_BOX_ATTACK, 107, 1, { -35, 24, 66, 24 }, { -35, 24, 26, 64 } }, /* YU4: LK, 40 lower */
    { 3, 5, 32, U3S_BOX_ATTACK, 108, 0, { -58, 24, 81, 27 }, { -58, 24, 41, 67 } }, /* YU4: LK, 40 lower */
    { 3, 5, 32, U3S_BOX_ATTACK, 108, 1, { -43, 24, 69, 24 }, { -43, 24, 29, 64 } }, /* YU4: LK, 40 lower */
    { 3, 5, 33, U3S_BOX_ATTACK, 107, 0, { -48, 24, 90, 27 }, { -48, 24, 50, 67 } }, /* YU4: MK, 40 lower */
    { 3, 5, 33, U3S_BOX_ATTACK, 107, 1, { -35, 24, 66, 24 }, { -35, 24, 26, 64 } }, /* YU4: MK, 40 lower */
    { 3, 5, 33, U3S_BOX_ATTACK, 108, 0, { -58, 24, 81, 27 }, { -58, 24, 41, 67 } }, /* YU4: MK, 40 lower */
    { 3, 5, 33, U3S_BOX_ATTACK, 108, 1, { -43, 24, 69, 24 }, { -43, 24, 29, 64 } }, /* YU4: MK, 40 lower */
    { 3, 5, 34, U3S_BOX_ATTACK, 107, 0, { -48, 24, 90, 27 }, { -48, 24, 50, 67 } }, /* YU4: HK, 40 lower */
    { 3, 5, 34, U3S_BOX_ATTACK, 107, 1, { -35, 24, 66, 24 }, { -35, 24, 26, 64 } }, /* YU4: HK, 40 lower */
    { 3, 5, 34, U3S_BOX_ATTACK, 108, 0, { -58, 24, 81, 27 }, { -58, 24, 41, 67 } }, /* YU4: HK, 40 lower */
    { 3, 5, 34, U3S_BOX_ATTACK, 108, 1, { -43, 24, 69, 24 }, { -43, 24, 29, 64 } }, /* YU4: HK, 40 lower */
    { 3, 5, 35, U3S_BOX_ATTACK, 107, 0, { -48, 24, 90, 27 }, { -48, 24, 50, 67 } }, /* YU4: EX, 40 lower */
    { 3, 5, 35, U3S_BOX_ATTACK, 107, 1, { -35, 24, 66, 24 }, { -35, 24, 26, 64 } }, /* YU4: EX, 40 lower */
    { 3, 5, 35, U3S_BOX_ATTACK, 108, 0, { -58, 24, 81, 27 }, { -58, 24, 41, 67 } }, /* YU4: EX, 40 lower */
    { 3, 5, 35, U3S_BOX_ATTACK, 108, 1, { -43, 24, 69, 24 }, { -43, 24, 29, 64 } }, /* YU4: EX, 40 lower */
    { -1, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } /* terminator */
};
