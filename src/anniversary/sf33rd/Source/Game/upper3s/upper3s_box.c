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
