/*
 * Functional tests for the Third Strike Upper box changes (upper3s/upper3s_box.c),
 * on a synthetic character.
 *
 * Build and run with `make test` from the repository root.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "sf33rd/Source/Game/upper3s_box.h"

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

int main(void) {
    static PLW wk;
    static UNK_5 attack[4];
    static UNK_1 body[4];
    static UNK_6 hosei[4];
    const Upper3sBox table[] = {
        { 7, U3S_BOX_ATTACK, 2, 1, { 10, 20, 30, 40 }, { 10, 20, 25, 45 } },
        { 7, U3S_BOX_BODY, 1, 0, { 1, 2, 3, 4 }, { 1, 2, 5, 4 } },
        { 8, U3S_BOX_ATTACK, 2, 1, { 10, 20, 30, 40 }, { 0, 0, 0, 0 } }, /* another character */
        { 7, U3S_BOX_HOSEI, 3, 0, { 9, 9, 9, 9 }, { 8, 8, 8, 8 } },       /* not the retail box */
        { -1, 0, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    };
    s16 failed;

    printf("boxes: the changes of the boxes of the character data\n");

    memset(&wk, 0, sizeof(wk));
    wk.player_number = 7;
    wk.wu.attack_adrs = attack;
    wk.wu.body_adrs = body;
    wk.wu.hosei_adrs = hosei;
    attack[2].att_box[1][0] = 10;
    attack[2].att_box[1][1] = 20;
    attack[2].att_box[1][2] = 30;
    attack[2].att_box[1][3] = 40;
    body[1].body_dm[0][0] = 1;
    body[1].body_dm[0][1] = 2;
    body[1].body_dm[0][2] = 3;
    body[1].body_dm[0][3] = 4;

    check(upper3s_box_ptr(&wk.wu, U3S_BOX_ATTACK, 2, 1) == attack[2].att_box[1], "the attack box is found");
    check(upper3s_box_ptr(&wk.wu, U3S_BOX_HOSEI, 3, 1) == NULL, "a hosei set has one box");
    check(upper3s_box_ptr(&wk.wu, U3S_BOX_HAND, 0, 0) == NULL, "a missing array gives no box");

    failed = upper3s_apply_boxes(&wk, table);
    check(attack[2].att_box[1][2] == 25 && attack[2].att_box[1][3] == 45, "the attack box is changed");
    check(body[1].body_dm[0][2] == 5, "the hurtbox is changed");
    check(failed == 1, "the box that does not hold the retail values is counted and left alone");
    check(hosei[3].hos_box[0] == 0, "it is not written");

    failed = upper3s_apply_boxes(&wk, table);
    check(attack[2].att_box[1][2] == 25 && failed == 1, "applying again is harmless (the loaded data may be kept)");

    printf("\nmove boxes: a box changed for the frames of one move only\n");
    {
        static UNK_5 sets[4];
        const Upper3sMoveBox moves[] = {
            { 7, 5, 60, U3S_BOX_ATTACK, 2, 0, { 1, 2, 30, 4 }, { 1, 2, 10, 24 } },
            { 7, 5, 60, U3S_BOX_ATTACK, 2, 3, { 5, 6, 70, 8 }, { 5, 6, 50, 28 } },
            { 7, 5, 60, U3S_BOX_ATTACK, 2, 1, { 9, 9, 9, 9 }, { 0, 0, 0, 0 } }, /* not the retail box */
            { -1, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
        };
        const UNK_5* retail;

        memset(&wk, 0, sizeof(wk));
        memset(sets, 0, sizeof(sets));
        wk.player_number = 7;
        wk.wu.work_id = 1;
        wk.wu.id = 1;
        wk.wu.attack_adrs = sets;
        sets[2].att_box[0][0] = 1;
        sets[2].att_box[0][1] = 2;
        sets[2].att_box[0][2] = 30;
        sets[2].att_box[0][3] = 4;
        sets[2].att_box[3][0] = 5;
        sets[2].att_box[3][1] = 6;
        sets[2].att_box[3][2] = 70;
        sets[2].att_box[3][3] = 8;
        sets[2].att_box[1][0] = 3;
        wk.wu.now_koc = 5;
        wk.wu.char_index = 60;
        wk.wu.cg_ja.atix = 2;
        wk.wu.h_att = retail = &sets[2];

        upper3s_box_frame_table(&wk.wu, moves);
        check(wk.wu.h_att != retail, "the hit check is given a copy on the move's frame");
        check(wk.wu.h_att->att_box[0][2] == 10 && wk.wu.h_att->att_box[0][3] == 24, "the first box is changed");
        check(wk.wu.h_att->att_box[3][2] == 50 && wk.wu.h_att->att_box[3][3] == 28, "a second row on the same set is applied too");
        check(wk.wu.h_att->att_box[1][0] == 3, "a box that does not hold the retail values is copied unchanged");
        check(sets[2].att_box[0][2] == 30 && sets[2].att_box[3][2] == 70, "the loaded set is not written");

        wk.wu.char_index = 61;
        wk.wu.h_att = retail;
        upper3s_box_frame_table(&wk.wu, moves);
        check(wk.wu.h_att == retail, "another move using the same set keeps it");

        wk.wu.char_index = 60;
        wk.wu.cg_ja.atix = 1;
        wk.wu.h_att = &sets[1];
        upper3s_box_frame_table(&wk.wu, moves);
        check(wk.wu.h_att == &sets[1], "another frame of the move with another set keeps it");

        wk.wu.cg_ja.atix = 2;
        wk.wu.h_att = retail;
        wk.player_number = 8;
        upper3s_box_frame_table(&wk.wu, moves);
        check(wk.wu.h_att == retail, "another character keeps it");

        wk.player_number = 7;
        wk.wu.work_id = 4;
        upper3s_box_frame_table(&wk.wu, moves);
        check(wk.wu.h_att == retail, "effects are not changed");
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
