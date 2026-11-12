#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - ibuki overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail base data (PLCNT.c:super_arts_data[7]):
 *
 *   SA I   {  0, ..., gauge_len  88, store_max 3 }  cmd00 - character specific
 *   SA II  { 21, ..., gauge_len 120, store_max 1 }  cmd21 - character specific
 *   SA III { 20, ..., gauge_len  80, store_max 1 }  cmd20 - 236+P+P  (Yami Shigure)
 *   SA IV  {  0, ..., gauge_len  64, store_max 1 }  unused
 *
 * TODO(3s'): the spec says "Yami Shigure (SA III) now has 2 stocks instead
 * of 1".  The retail table gives the SA III slot one stock
 * (PLCNT.c:super_arts_data[7] = { 20, ..., gauge_len 80, store_max 1 }),
 * so the entry below raises it to the two stocks of the document.  The
 * entry of the SA I slot is left alone: that slot is the one that is
 * reached through the alternative command of the character (nmsa_a_ix 38)
 * and it already costs three stocks.
 */

const Upper3sSuperArt upper3s_super_art_ibuki[4] = {
    { 0, 0, 0, 0 }, /* SA I - retail 88 / 3 stocks */
    { 0, 0, 0, 0 }, /* SA II - retail 120 / 1 stock */
    /* SA III - "Yami Shigure": a maximum of 2 stocks. */
    { U3S_SA_STORE_MAX, 2, 0, 0 },
    { 0, 0, 0, 0 } /* SA IV - unused */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * TODO(3s'): "Tsuiji Goe may be special and super cancelled much earlier"
 * and "the unique animation standing Fierce of the target combo must never
 * knock down" are both properties of the on disc frame chain and attack
 * table (wk->wu.cg_cancel and UNK_7.level).  They cannot be expressed with
 * this table.
 *
 * The removal of the throw tech window of the personal action is a source
 * level change: it lives in PLPAT07.c:Att_PL07_TOKUSHUKOUDOU and in
 * upper3s/upper3s_ch_ibuki.c.
 */

const Upper3sMove upper3s_moves_ibuki[] = {
    /*
     * IB3 (the cancel) - "Allow Tsuiji Goe (623P, command jump) to be special- and super-cancelled much
     * earlier in the animation, almost directly after takeoff".
     *
     * Tsuiji Goe is special animations 107-110 (LP, MP, HP, EX). Found (SF33_LIST_BLOCKS): each chain
     * leaves the ground at a command block (0x6a) at game frame 4, 5, 6 and 6, and its frames carry the
     * cancel bits 0x60 (special and super; not tied to a hit, as the kunai from the jump shows) only
     * from game frame 28, 29, 33 and 33 - 24 to 27 frames after takeoff. The window now opens on the
     * second frame in the air (2 game frames after takeoff) and runs into the retail one.
     */
    { U3S_KOC_ATTACK, 107, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, U3S_FRAME_CANCEL, 0, 0x60, 22 }, /* IB3: 6-27 */
    { U3S_KOC_ATTACK, 108, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, U3S_FRAME_CANCEL, 0, 0x60, 22 }, /* IB3: 7-28 */
    { U3S_KOC_ATTACK, 109, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, U3S_FRAME_CANCEL, 0, 0x60, 25 }, /* IB3: 8-32 */
    { U3S_KOC_ATTACK, 110, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, U3S_FRAME_CANCEL, 0, 0x60, 25 }, /* IB3: 8-32 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } /* terminator */
};
