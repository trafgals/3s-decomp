#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"
#include "sf33rd/Source/Game/upper3s_chain.h"

#include "sf33rd/Source/Game/PLCNT.h"
#include "sf33rd/Source/Game/POW_DATA.h"

/*
 * Third Strike Upper - data level.
 *
 * The per character move data is read from the disc image at runtime and
 * is not part of the repository, so the balance changes that touch it are
 * applied here, right after the data has been loaded.
 *
 * The chain that leads from a move to its numbers is:
 *
 *   char_table[koc][index]  -> offset (in u32) of the move's first frame
 *   set_char_ad[2]          -> { cg_hit_ix, cg_att_ix }
 *   att_ix_table[cg_att_ix] -> UNK_7 { pow, impact, piyo, hs_me, hs_you, ... }
 *
 * `pow` is an index into POW_DATA.c:Power_Data (5 damage per step) and
 * `piyo` is an index into the per player stun table, so both of them are
 * patched through the runtime helpers instead of a plain field write.
 */

#if defined(UPPER_3S)

/*
 * The per character tables live in upper3s/plXX.c.
 */
extern const Upper3sSuperArt upper3s_super_art_alex[4];
extern const Upper3sSuperArt upper3s_super_art_ryu[4];
extern const Upper3sSuperArt upper3s_super_art_yun[4];
extern const Upper3sSuperArt upper3s_super_art_dudley[4];
extern const Upper3sSuperArt upper3s_super_art_necro[4];
extern const Upper3sSuperArt upper3s_super_art_hugo[4];
extern const Upper3sSuperArt upper3s_super_art_ibuki[4];
extern const Upper3sSuperArt upper3s_super_art_elena[4];
extern const Upper3sSuperArt upper3s_super_art_oro[4];
extern const Upper3sSuperArt upper3s_super_art_yang[4];
extern const Upper3sSuperArt upper3s_super_art_ken[4];
extern const Upper3sSuperArt upper3s_super_art_makoto[4];
extern const Upper3sSuperArt upper3s_super_art_urien[4];
extern const Upper3sSuperArt upper3s_super_art_akuma[4];
extern const Upper3sSuperArt upper3s_super_art_chunli[4];
extern const Upper3sSuperArt upper3s_super_art_sean[4];
extern const Upper3sSuperArt upper3s_super_art_q[4];
extern const Upper3sSuperArt upper3s_super_art_twelve[4];
extern const Upper3sSuperArt upper3s_super_art_remy[4];
extern const Upper3sSuperArt upper3s_super_art_gill[4];

/*
 * The tables are indexed by the character index of the game - see the
 * character list in upper3s.h.  The order is the order of My_char[] and of
 * PLW.player_number, NOT the order of AGENTS.md section 3.1.
 */
static const Upper3sSuperArt* const upper3s_super_art_of[U3S_CHAR_COUNT] = {
    upper3s_super_art_gill,   upper3s_super_art_alex,    upper3s_super_art_ryu,
    upper3s_super_art_yun,    upper3s_super_art_dudley,  upper3s_super_art_necro,
    upper3s_super_art_hugo,   upper3s_super_art_ibuki,   upper3s_super_art_elena,
    upper3s_super_art_oro,    upper3s_super_art_yang,    upper3s_super_art_ken,
    upper3s_super_art_sean,   upper3s_super_art_urien,   upper3s_super_art_akuma,
    upper3s_super_art_chunli, upper3s_super_art_makoto,  upper3s_super_art_q,
    upper3s_super_art_twelve, upper3s_super_art_remy
};

/*
 * The move tables are per character files; they are pulled in here so that
 * the dispatch stays in one place. A character without overrides uses a
 * NULL pointer and is skipped.
 */
extern const Upper3sMove upper3s_moves_alex[];
extern const Upper3sMove upper3s_moves_ryu[];
extern const Upper3sMove upper3s_moves_yun[];
extern const Upper3sMove upper3s_moves_dudley[];
extern const Upper3sMove upper3s_moves_necro[];
extern const Upper3sMove upper3s_moves_hugo[];
extern const Upper3sMove upper3s_moves_ibuki[];
extern const Upper3sMove upper3s_moves_elena[];
extern const Upper3sMove upper3s_moves_oro[];
extern const Upper3sMove upper3s_moves_yang[];
extern const Upper3sMove upper3s_moves_ken[];
extern const Upper3sMove upper3s_moves_makoto[];
extern const Upper3sMove upper3s_moves_urien[];
extern const Upper3sMove upper3s_moves_akuma[];
extern const Upper3sMove upper3s_moves_chunli[];
extern const Upper3sMove upper3s_moves_sean[];
extern const Upper3sMove upper3s_moves_q[];
extern const Upper3sMove upper3s_moves_twelve[];
extern const Upper3sMove upper3s_moves_remy[];
extern const Upper3sMove upper3s_moves_gill[];

const Upper3sMove* upper3s_moves[U3S_CHAR_COUNT] = {
    upper3s_moves_gill,   upper3s_moves_alex,    upper3s_moves_ryu,      upper3s_moves_yun,
    upper3s_moves_dudley, upper3s_moves_necro,   upper3s_moves_hugo,     upper3s_moves_ibuki,
    upper3s_moves_elena,  upper3s_moves_oro,     upper3s_moves_yang,     upper3s_moves_ken,
    upper3s_moves_sean,   upper3s_moves_urien,   upper3s_moves_akuma,    upper3s_moves_chunli,
    upper3s_moves_makoto, upper3s_moves_q,       upper3s_moves_twelve,   upper3s_moves_remy
};

/*
 * Looks up the move header of (koc, index) and returns the index of its
 * attack data, or -1 when the move has none.
 */
static s16 upper3s_att_index(WORK* wk, s16 koc, s16 index) {
    u32* set_char_ad;

    if (koc < 0 || koc >= U3S_KOC_COUNT || index < 0) {
        return -1;
    }

    if (wk->char_table[koc] == NULL) {
        return -1;
    }

    set_char_ad = &wk->char_table[koc][wk->char_table[koc][index] / 4];

    /* set_char_ad[2] holds { cg_hit_ix, cg_att_ix }. */
    return (s16)(set_char_ad[2] >> 16);
}

/*
 * Applies a single move override to the loaded attack table.
 *
 * The move header carries the index of the attack data it uses; the attack
 * data itself lives in `att_ix_table` and is shared by every move that uses
 * the same data, exactly like in the retail game.
 */
/*
 * The record of the changes of the frame data that could not be applied.
 *
 * The change is dropped when the chain of the move does not hold the
 * value of the retail data, or when the chain cannot be brought to the
 * target.  The game plays the retail data in that case, so the change of
 * the patch is not there and it has to be visible somewhere.
 */
static Upper3sFailure upper3s_failure_record;

/*
 * The changes that were already counted.
 *
 * The overrides are applied again every time the character is loaded and
 * every round, so a change that is dropped would be counted again and
 * again and the counter would count the calls instead of the changes.  A
 * change is therefore counted once: the slot, the move and the change.
 */
#define UPPER3S_FAILURE_MAX 64

typedef struct {
    s16 koc;
    s16 index;
    u16 flags;
} Upper3sFailureKey;

static Upper3sFailureKey upper3s_failure_keys[UPPER3S_FAILURE_MAX];
static s32 upper3s_failure_key_count;

const Upper3sFailure* upper3s_failure(void) {
    return &upper3s_failure_record;
}

static void upper3s_note_failure(const Upper3sMove* move, u16 flags) {
    s32 i;

    for (i = 0; i < upper3s_failure_key_count; i++) {
        if (upper3s_failure_keys[i].koc == move->koc &&
            upper3s_failure_keys[i].index == move->index &&
            upper3s_failure_keys[i].flags == flags) {
            /* the change is already known */
            return;
        }
    }

    if (upper3s_failure_key_count < UPPER3S_FAILURE_MAX) {
        upper3s_failure_keys[upper3s_failure_key_count].koc = move->koc;
        upper3s_failure_keys[upper3s_failure_key_count].index = move->index;
        upper3s_failure_keys[upper3s_failure_key_count].flags = flags;
        upper3s_failure_key_count++;
    }

    upper3s_failure_record.count++;
    upper3s_failure_record.koc = move->koc;
    upper3s_failure_record.index = move->index;
    upper3s_failure_record.flags = flags;
}

void upper3s_apply_move(WORK* wk, const Upper3sMove* move) {
    s16 att_ix;
    UNK_7* att;

    /*
     * The frame data of the move is in the chain of the move and not in
     * the attack table, so it is patched first and on its own: a move
     * whose startup is changed does not have to carry attack data.
     */
    if (move->flags & (U3S_MV_STARTUP | U3S_MV_RECOVERY)) {
        Upper3sChain chain;

        if (upper3s_chain_open(wk, move->koc, move->index, &chain)) {
            if (move->flags & U3S_MV_STARTUP) {
                if (!upper3s_chain_set_startup(&chain, move->startup_from, move->startup)) {
                    upper3s_note_failure(move, U3S_MV_STARTUP);
                }
            }

            if (move->flags & U3S_MV_RECOVERY) {
                if (!upper3s_chain_set_recovery(&chain, move->recovery_from, move->recovery)) {
                    upper3s_note_failure(move, U3S_MV_RECOVERY);
                }
            }
        } else {
            /*
             * The chain of the move could not be opened, so the change is
             * not made either.  That is a change of the patch that is not
             * there, so it is counted like the others.
             */
            if (move->flags & U3S_MV_STARTUP) {
                upper3s_note_failure(move, U3S_MV_STARTUP);
            }

            if (move->flags & U3S_MV_RECOVERY) {
                upper3s_note_failure(move, U3S_MV_RECOVERY);
            }
        }
    }

    att_ix = upper3s_att_index(wk, move->koc, move->index);

    if (att_ix < 0) {
        return;
    }

    att = &wk->att_ix_table[att_ix];

    /*
     * `pow` is an index into Power_Data and `piyo` is an index into a
     * per player table, so neither of them can be patched in place.
     * They are handled by the runtime helpers instead.
     */

    if (move->flags & U3S_MV_IMPACT) {
        att->impact = (s8)move->impact;
    }

    if (move->flags & U3S_MV_HS_ME) {
        att->hs_me = (s8)move->hs_me;
    }

    if (move->flags & U3S_MV_HS_YOU) {
        att->hs_you = (s8)move->hs_you;
    }
}

void upper3s_apply_character(PLW* wk) {
    const Upper3sMove* moves;
    s16 char_id = wk->player_number;
    s16 i;

    if (char_id < 0 || char_id >= U3S_CHAR_COUNT) {
        return;
    }

    moves = upper3s_moves[char_id];

    if (moves == NULL) {
        return;
    }

    if (wk->wu.att_ix_table == NULL) {
        return;
    }

    for (i = 0; moves[i].flags != 0; i++) {
        upper3s_apply_move(&wk->wu, &moves[i]);
    }
}

void upper3s_apply_super_art(const Upper3sSuperArt* sa, SA_DATA* out) {
    if (sa == NULL || out == NULL) {
        return;
    }

    if (sa->flags & U3S_SA_STORE_MAX) {
        out->store_max = sa->store_max;
    }

    if (sa->flags & U3S_SA_GAUGE_LEN) {
        out->gauge_len = sa->gauge_len;
    }

    if (sa->flags & U3S_SA_DTM) {
        out->dtm = sa->dtm;
    }
}

int upper3s_super_art_is_listed(s16 char_id, s16 sa) {
    if (char_id < 0 || char_id >= U3S_CHAR_COUNT) {
        return 0;
    }

    if (sa < 0 || sa > 3) {
        return 0;
    }

    return upper3s_super_art_of[char_id][sa].flags != 0;
}

const SA_DATA* upper3s_super_art_data(s16 char_id, s16 sa, const SA_DATA* retail) {
    static SA_DATA patched;
    const Upper3sSuperArt* sa_data;

    if (retail == NULL) {
        return NULL;
    }

    if (char_id < 0 || char_id >= U3S_CHAR_COUNT) {
        return retail;
    }

    if (sa < 0 || sa > 3) {
        return retail;
    }

    sa_data = &upper3s_super_art_of[char_id][sa];

    if (sa_data->flags == 0) {
        return retail;
    }

    patched = *retail;
    upper3s_apply_super_art(sa_data, &patched);

    return &patched;
}

/*
 * Finds the override of the attack that `wk` is currently resolving.
 */
static const Upper3sMove* upper3s_find_move(s16 char_id, WORK* wk) {
    const Upper3sMove* moves;
    s16 i;

    if (char_id < 0 || char_id >= U3S_CHAR_COUNT) {
        return NULL;
    }

    moves = upper3s_moves[char_id];
    if (moves == NULL) {
        return NULL;
    }

    for (i = 0; moves[i].flags != 0; i++) {
        if (moves[i].koc == wk->now_koc && moves[i].index == wk->char_index) {
            return &moves[i];
        }
    }

    return NULL;
}

s16 upper3s_damage_permille(s16 char_id, WORK* wk) {
    const Upper3sMove* move = upper3s_find_move(char_id, wk);

    if (move == NULL || !(move->flags & U3S_MV_DAMAGE_PERMILLE)) {
        return 1000;
    }

    return move->damage_permille;
}

s16 upper3s_damage_base(s16 char_id, WORK* wk) {
    const Upper3sMove* move = upper3s_find_move(char_id, wk);

    if (move == NULL || !(move->flags & U3S_MV_DAMAGE)) {
        return -1;
    }

    return move->damage;
}

s16 upper3s_stun_value(s16 char_id, WORK* wk) {
    const Upper3sMove* move = upper3s_find_move(char_id, wk);

    if (move == NULL || !(move->flags & U3S_MV_STUN)) {
        return -1;
    }

    return move->stun;
}

s16 upper3s_stun_permille(s16 char_id, WORK* wk) {
    const Upper3sMove* move = upper3s_find_move(char_id, wk);

    if (move == NULL || !(move->flags & U3S_MV_STUN_PERMILLE)) {
        return 1000;
    }

    if (move->stun_permille <= 0) {
        return 1000;
    }

    return move->stun_permille;
}

#endif
