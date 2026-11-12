/*
 * Third Strike Upper (3s') - changes of the boxes of the character data.
 * See include/sf33rd/Source/Game/upper3s_box.h.
 */

#include "common.h"
#include "sf33rd/Source/Game/upper3s_box.h"

s16* upper3s_box_ptr(WORK* wk, s16 kind, s16 index, s16 box) {
    if ((index < 0) || (box < 0) || (box > 3)) {
        return NULL;
    }

    switch (kind) {
    case U3S_BOX_BODY:
        return (wk->body_adrs == NULL) ? NULL : wk->body_adrs[index].body_dm[box];
    case U3S_BOX_HAND:
        return (wk->hand_adrs == NULL) ? NULL : wk->hand_adrs[index].hand_dm[box];
    case U3S_BOX_ATTACK:
        return (wk->attack_adrs == NULL) ? NULL : wk->attack_adrs[index].att_box[box];
    case U3S_BOX_HOSEI:
        return ((wk->hosei_adrs == NULL) || box) ? NULL : wk->hosei_adrs[index].hos_box;
    case U3S_BOX_CATCH:
        return ((wk->catch_adrs == NULL) || box) ? NULL : wk->catch_adrs[index].cat_box;
    case U3S_BOX_CAUGHT:
        return ((wk->caught_adrs == NULL) || box) ? NULL : wk->caught_adrs[index].cau_box;
    default:
        return NULL;
    }
}

static int upper3s_box_is(const s16* box, const s16* values) {
    return (box[0] == values[0]) && (box[1] == values[1]) && (box[2] == values[2]) && (box[3] == values[3]);
}

s16 upper3s_apply_boxes(PLW* wk, const Upper3sBox* table) {
    s16 failed = 0;
    s16 i;
    s16 k;

    if (table == NULL) {
        return 0;
    }

    for (i = 0; table[i].char_id >= 0; i++) {
        const Upper3sBox* row = &table[i];
        s16* box;

        if (row->char_id != wk->player_number) {
            continue;
        }

        box = upper3s_box_ptr(&wk->wu, row->kind, row->index, row->box);

        if (box == NULL) {
            failed++;
            continue;
        }

        if (upper3s_box_is(box, row->to)) {
            /* Already changed: the character data was loaded before. */
            continue;
        }

        if (!upper3s_box_is(box, row->from)) {
            /* Not the retail box: left alone. */
            failed++;
            continue;
        }

        for (k = 0; k < 4; k++) {
            box[k] = row->to[k];
        }
    }

    return failed;
}

/* The copies of the sets the hit check is given, one per player slot. */
static UNK_5 upper3s_box_attack_copy[2];

void upper3s_box_frame_table(WORK* wk, const Upper3sMoveBox* table) {
    PLW* plw_wk = (PLW*)wk;
    s16 i;
    s16 k;

    if ((wk->work_id != 1) || (table == NULL) || (wk->id < 0) || (wk->id > 1) || (wk->attack_adrs == NULL)) {
        return;
    }

    for (i = 0; table[i].char_id >= 0; i++) {
        const Upper3sMoveBox* row = &table[i];
        UNK_5* copy = &upper3s_box_attack_copy[wk->id];
        const s16* box;

        if ((row->char_id != plw_wk->player_number) || (row->koc != wk->now_koc) ||
            (row->char_index != wk->char_index) || (row->kind != U3S_BOX_ATTACK) || (row->index != wk->cg_ja.atix)) {
            continue;
        }

        box = wk->attack_adrs[row->index].att_box[row->box];

        if (!upper3s_box_is(box, row->from)) {
            /* Not the retail box. */
            continue;
        }

        *copy = wk->attack_adrs[row->index];

        for (k = 0; k < 4; k++) {
            copy->att_box[row->box][k] = row->to[k];
        }

        /* Further rows may change other boxes of the same set: they are applied to the copy. */
        for (i = (s16)(i + 1); table[i].char_id >= 0; i++) {
            const Upper3sMoveBox* more = &table[i];

            if ((more->char_id == row->char_id) && (more->koc == row->koc) && (more->char_index == row->char_index) &&
                (more->kind == U3S_BOX_ATTACK) && (more->index == row->index) &&
                upper3s_box_is(wk->attack_adrs[more->index].att_box[more->box], more->from)) {
                for (k = 0; k < 4; k++) {
                    copy->att_box[more->box][k] = more->to[k];
                }
            }
        }

        wk->h_att = copy;
        return;
    }
}

void upper3s_box_frame(WORK* wk) {
    upper3s_box_frame_table(wk, upper3s_move_boxes);
}
