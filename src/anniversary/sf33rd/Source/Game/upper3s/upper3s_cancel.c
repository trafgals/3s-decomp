#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"
#include "sf33rd/Source/Game/upper3s_ch_alex.h"
#include "sf33rd/Source/Game/upper3s_ch_q.h"
#include "sf33rd/Source/Game/upper3s_ch_twelve.h"

/*
 * Third Strike Upper (3s') - the cancel mask of a move.
 *
 * The cancel permission of a move is not in the sources: it is the "cg_cancel"
 * byte of the frame chain of the move, which the disc holds. The design
 * document grants a few cancels to a few normals of a few characters, and the
 * rules of those changes are in the module of the character
 * (upper3s/upper3s_ch_<name>.c), in the same shape as the rules the retail
 * game itself has for Ibuki and for Twelve (PLS00.c:check_cg_cancel_data(),
 * the switch on wk->player_number).
 *
 * This is the one place that knows which rule belongs to which character, so
 * that the shared file needs a single hook:
 *
 *     #if defined(UPPER_3S)
 *         wk->wu.cg_cancel = upper3s_cancel_mask(wk->player_number, wk->wu.now_koc, wk->as->char_ix,
 *                                                 wk->wu.cg_cancel);
 *     #endif
 *
 * The move is addressed by two things:
 *
 *   - `koc`, the kind of character data the move lives in.  The rules of the
 *     design document are about the normal attacks, which are the slot 4; the
 *     special moves and the super arts are the slot 5 and they have their own
 *     numbering, which overlaps the one of the normals.
 *
 *   - `char_ix`, the index of the move in that kind of data.  For a normal it
 *     is the char_ix of the assignment table of the disc
 *     (bin2obj/asstbl.c), which is the only thing that identifies the move.
 *
 * The hook gives the cancel for the whole move, while the retail gives it for
 * the frames of the chain that carry the byte. The cancels of the design
 * document are the ones into a special move, into a super art and into a super
 * jump, and all of them are behind the test of the hit of the move in
 * PLS00.c:check_cg_cancel_data(), so the cancel cannot come out before the
 * move has connected.
 */
u8 upper3s_cancel_mask(s16 char_id, s16 koc, s16 char_ix, u8 cg_cancel) {
    /* The rules of the design document are about the normal attacks only. */
    if (koc != U3S_KOC_NORMAL) {
        return cg_cancel;
    }

    switch (char_id) {
    case U3S_ALEX:
        return upper3s_ch_alex_cancel_mask(char_ix, cg_cancel);

    case U3S_Q:
        return upper3s_ch_q_cancel_mask(char_ix, cg_cancel);

    case U3S_TWELVE:
        return upper3s_ch_twelve_cancel_mask(char_ix, cg_cancel);

    default:
        break;
    }

    return cg_cancel;
}
