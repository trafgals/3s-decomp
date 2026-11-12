#ifndef UPPER3S_BOX_H
#define UPPER3S_BOX_H

#include "structs.h"
#include "types.h"

/*
 * Third Strike Upper (3s') - changes of the boxes of the character data.
 *
 * The boxes of a frame are found through its `cg_hit_ix`: the row
 * `hit_ix_table[cg_hit_ix]` (WORK.cg_ja, UNK_0) holds an index into each of the
 * character's box arrays, and CHARSET.c:set_jugde_area() points the work at
 * them:
 *
 *   body_adrs[boix]   UNK_1, four hurtboxes          (U3S_BOX_BODY)
 *   hand_adrs[bhix+haix] UNK_2, four hurtboxes of the limbs (U3S_BOX_HAND)
 *   attack_adrs[atix] UNK_5, four hitboxes           (U3S_BOX_ATTACK)
 *   hosei_adrs[hoix]  UNK_6, one box                 (U3S_BOX_HOSEI)
 *   catch_adrs[caix]  UNK_3, one throw box           (U3S_BOX_CATCH)
 *   caught_adrs[cuix] UNK_4, one throwable box       (U3S_BOX_CAUGHT)
 *
 * A box is four s16; the second one is its width, and a box with width 0 is no
 * box (HITCHECK.c:attack_hit_check()). The arrays are loaded from the disc with
 * the character (CHARID.c:set_char_base_data()), so a change is made after the
 * character is loaded, every time it is loaded, and only when the box still holds
 * the retail values (`from`): a box that holds `to` already counts as done.
 * Every frame that uses the box set changes with it.
 */
enum {
    U3S_BOX_BODY = 1,
    U3S_BOX_HAND = 2,
    U3S_BOX_ATTACK = 3,
    U3S_BOX_HOSEI = 4,
    U3S_BOX_CATCH = 5,
    U3S_BOX_CAUGHT = 6
};

typedef struct {
    s16 char_id; /* the character index (AGENTS.md 3.1); -1 ends the table */
    s16 kind;    /* U3S_BOX_* */
    s16 index;   /* the index of the box set in its array */
    s16 box;     /* the box of the set, 0-3 (0 for the sets of one box) */
    s16 from[4]; /* the retail box */
    s16 to[4];   /* the 3s' box */
} Upper3sBox;

/*
 * A change of a box for the frames of one move only. A box set is often shared by
 * several moves (the three Joudan Sokutou Geri of Ryu use the same attack boxes), so a
 * change of the set itself would change all of them. A row of this kind leaves the
 * set alone: on the frames of the animation `char_index` (WORK.char_index) of the slot
 * `koc` of the character, when the frame uses the set `index` (and, with `block`, only on
 * one block of the animation), the hit check is given a copy of the set with the box
 * changed (upper3s_box_frame(), called by CHARSET.c:set_jugde_area()). Only attack boxes
 * for now.
 */
typedef struct {
    s16 char_id; /* -1 ends the table */
    s16 koc;
    s16 char_index;
    s16 kind;    /* U3S_BOX_ATTACK */
    s16 index;
    s16 box;
    s16 from[4];
    s16 to[4];
    /*
     * 0: every frame of the animation that uses the set. k: only block k - 1 of the
     * animation (WORK.cg_ix / cgd_type, the "blk" of the fight log), for a set that
     * several frames of one move share. Rows that leave it out apply to every frame.
     */
    s16 block;
} Upper3sMoveBox;

extern const Upper3sMoveBox upper3s_move_boxes[];

/* Called on every frame of a work after its box pointers are set (players only). */
void upper3s_box_frame(WORK* wk);

/* The same with another table, for the tests. */
void upper3s_box_frame_table(WORK* wk, const Upper3sMoveBox* table);

/* The changes, defined by upper3s/upper3s_boxes.c. */
extern const Upper3sBox upper3s_boxes[];

/* A pointer to the box of the character data, or NULL. */
s16* upper3s_box_ptr(WORK* wk, s16 kind, s16 index, s16 box);

/* Applies the changes of `table` to the character of `wk`. Returns the number of the
 * changes of its character that could not be made (the box did not hold `from`). */
s16 upper3s_apply_boxes(PLW* wk, const Upper3sBox* table);

#endif
