#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - chunli overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail values (PLCNT.c:super_arts_data[15] = chunli, {cmd set, gauge, stock}):
 *   SA I   { 20,  88, 1 }   236236+P     (Kikoushou)
 *   SA II  { 21, 104, 2 }   236236+K     (Houyoku-sen)
 *   SA III { 22,  72, 3 }   236236+K     (Tensei Ranka)
 * The commands are cmd_data.c:p10_cmd_20-22 (tools/upper3s/decode_cmd.py: all three are 236236,
 * with the punches for SA I and the kicks for the other two; a character has one super art per
 * round, so they can share the motion).
 *
 * 3s' changes:
 *   SA II  - the stock cost goes from 2 to 1 and the bar becomes the longest
 *            one in the game (0x80 = 128), the same length as Shin-Shoryuken.
 *   SA III - the stock cost goes from 3 to 2 and the bar becomes as long as
 *            Shippu Jinrai Kyaku (80).
 */

const Upper3sSuperArt upper3s_super_art_chunli[4] = {
    { 0, 0, 0, 0 },                                     /* SA I */
    { U3S_SA_STORE_MAX | U3S_SA_GAUGE_LEN, 1, 128, 0 }, /* SA II - Houyoku-sen */
    { U3S_SA_STORE_MAX | U3S_SA_GAUGE_LEN, 2, 80, 0 },  /* SA III - Tensei Ranka */
    { 0, 0, 0, 0 }                                      /* SA IV */
};

/*
 * Move data overrides. A move is addressed by the "kind of character
 * data" slot it lives in and by its index inside that slot.
 *
 *   { koc, index, flags, damage, damage_permille, stun, impact, hs_me,
 *     hs_you, stun_permille, startup_from, startup, recovery_from, recovery }
 *
 * The normal move indexes (U3S_KOC_NORMAL) are the char_ix of the disc's
 * assignment table (bin2obj/asstbl.c), not the index of the move in the
 * move list:
 *
 *   char_ix = base + kos * 3 + koa
 *
 * with kos the button (0 LP, 1 MP, 2 HP, 3 LK, 4 MK, 5 HK) and koa the
 * variant. For the standing normals the base is 0: koa 0 is the close
 * standing normal (or the only one), 1 the far one and 2 the command normal
 * of the button (6X or 4X, depending on the character); a variant a
 * character does not have points at the chain of another one. Measured in
 * the fight harness (the "cix" of the log): Remy's close s.MK is 12 and his
 * far s.MK 13, Twelve's 4MK and Remy's 6MK are 14, and Q's far s.LP is 1.
 * (An earlier version of this comment read koa as a dash forward or back.)
 * For the crouching normals the base is 18 and koa 0 is 2X; what koa 1 and
 * 2 hold there was not checked.
 *
 *            close  far  command          close  far  command
 *   s.LP       0     1     2       s.MP     3     4     5
 *   s.HP       6     7     8       s.LK     9    10    11
 *   s.MK      12    13    14       s.HK    15    16    17
 *   c.LP 18   c.MP 21   c.HP 24   c.LK 27   c.MK 30   c.HK 33
 *
 * The Cold Blue Kick (63214K) is a move of Remy, not of Chun-Li: an earlier version of
 * this table changed two of Chun-Li's specials under its name. Its change is in
 * upper3s/pl18.c (docs/upper3s/remaining.md RM10).
 *
 * TODO(3s'): "Tensei Ranka (SA III) - advantage on knockdown increased enough
 * to allow crossup attempts afterward" needs the index of the super inside
 * char_table[5] (U3S_KOC_SUPER); that index is not visible from the sources
 * (it comes from the disc data), so the entry is left out for now.
 */

const Upper3sMove upper3s_moves_chunli[] = {
    /*
     * The crouching Jab (2LP) is the only normal of the whole cast that
     * starts in 2 frames, and the design document asks for 3 frames: it
     * is usable as a crouch tech, and a throw is still a 2 frame punish.
     */
    { U3S_KOC_NORMAL, 18, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 0 }, /* 2LP - the crouching Jab */
    /*
     * CL2 - "Cancel window on cr.Fwd (c.MK) fixed to not allow super-late cancels."
     *
     * Found (SF33_LIST_BLOCKS, and the "can" of the fight log): the c.MK (normal 30) has its cancel
     * window on the hit frame and the next one (blocks 3 and 4, cancel bits 0x60: special and super),
     * like the c.MK of Ken, Ryu, Yun and Sean, then a frame without, and then a second window on
     * frame 6 of the chain (block 6, game frames 13-14), long after the hit: the super-late cancel.
     * Its cancel bits are cleared. The first window is unchanged.
     */
    { U3S_KOC_NORMAL, 30, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, U3S_FRAME_CANCEL, 0x60, 0 }, /* CL2 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                                /* terminator */
};
