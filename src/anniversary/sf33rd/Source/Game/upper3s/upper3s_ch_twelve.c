/*
 * Third Strike Upper (3s') - twelve.
 *
 * The rules of this file are the cancels that the design document grants to a
 * few of Twelve's normal attacks.  The cancel mask of a move is the
 * "cg_cancel" byte of its frame chain, which is read from the disc, so the
 * rules are kept here and applied by the shared hook that is documented in
 * include/sf33rd/Source/Game/upper3s_ch_twelve.h.
 */

#include "sf33rd/Source/Game/upper3s_ch_twelve.h"

u8 upper3s_ch_twelve_cancel_mask(s16 char_ix, u8 cg_cancel) {
    switch (char_ix) {
    case U3S_TWELVE_STANDING_STRONG:
    case U3S_TWELVE_FAR_STRONG:
    case U3S_TWELVE_FAR_STRONG_REACH:
        // The standing Strong and the far Strong can be cancelled into a
        // special move and into a super art.
        return (u8)(cg_cancel | U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER);

    case U3S_TWELVE_CROUCHING_STRONG:
        // The crouching Strong can be cancelled into a special move, into a
        // super art and into a super jump.
        return (u8)(cg_cancel | U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER | U3S_CANCEL_SUPERJUMP);

    case U3S_TWELVE_CROUCHING_FORWARD:
        // The crouching Forward can only be cancelled into a super art.
        return (u8)(cg_cancel | U3S_CANCEL_SUPER);

    default:
        if (char_ix >= U3S_TWELVE_AIR_FIRST && char_ix <= U3S_TWELVE_AIR_LAST) {
            // Every air attack can be cancelled into a special move and into a
            // super art.
            return (u8)(cg_cancel | U3S_CANCEL_SPECIAL | U3S_CANCEL_SUPER);
        }

        return cg_cancel;
    }
}
