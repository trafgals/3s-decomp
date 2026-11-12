#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - sean overrides.
 *
 * Super art overrides. The fields that are not flagged are left at their
 * retail value.
 *
 *   { flags, store_max, gauge_len, dtm }
 *
 * Retail values (PLCNT.c:super_arts_data[12] = sean, {cmd set, gauge, stock}):
 *   SA I   { 20,  72, 3 }
 *   SA II  { 21,  96, 2 }
 *   SA III { 22, 120, 1 }   Hyper Tornado
 *
 * 3s' does not change any of Sean's super art stocks or bar lengths.
 */

const Upper3sSuperArt upper3s_super_art_sean[4] = {
    { 0, 0, 0, 0 }, /* SA I */
    { 0, 0, 0, 0 }, /* SA II */
    { 0, 0, 0, 0 }, /* SA III - Hyper Tornado */
    { 0, 0, 0, 0 }  /* SA IV */
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
 * 3s': "Increase hitstun of close and far Fierce, crouching Fierce, standing
 * Forward, far Roundhouse, crouching Forward, and Towards+Fierce (6HP) to
 * not be negative on hit."
 *
 * The advantage of a hit is the difference between how long the defender is
 * held and how long the attacker is locked, and both are read from the data
 * of the disc.  `tools/upper3s/hit_advantage.py` computes the advantage of
 * every normal out of that data.
 *
 * The hitstun of a move is the hold of the defender, and that is the field
 * `hs_you` of the attack data (`UNK_7.hs_you`), so that is what is raised
 * here.  The values below are the ones that make the advantage of the hit
 * zero, the end state the design document asks for ("not negative on hit").
 * The standing Forward and the far Roundhouse are already at zero or better
 * in the retail data and are not changed.
 */

const Upper3sMove upper3s_moves_sean[] = {
    /*
     * SN2 - "Allow Sean Roll (214P) to be super-cancelled during the middle."
     *
     * Sean Roll is special animations 48-50 (LP, MP, HP), and no frame of it has cancel bits. Found
     * (the "blk" and "x" of the fight log): each starts with one block, rolls on blocks 1-4 (LP) or
     * 1-13 (MP, HP; the HP one plays 6-13 twice), stops on one block and stands up. "The middle" is
     * taken as the rolling blocks but the first and the last: blocks 2-3 (LP, game frames 6-13) and
     * 2-12 (MP 4-24, HP 4-23). They get the super cancel bit (0x40); a special move that has it arms
     * its own cancel, without a hit (CHARSET.c, `kow & 0x60`), as Tsuiji Goe does.
     */
    { U3S_KOC_ATTACK, 48, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, U3S_FRAME_CANCEL, 0, 0x40, 8 },  /* SN2: LP */
    { U3S_KOC_ATTACK, 49, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, U3S_FRAME_CANCEL, 0, 0x40, 21 }, /* SN2: MP */
    { U3S_KOC_ATTACK, 50, U3S_MV_FRAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, U3S_FRAME_CANCEL, 0, 0x40, 20 }, /* SN2: HP */
    /* Hitstun so the move is not negative on hit: the hold of the defender (hs_you) that makes
     * it 0 on hit, measured in the real engine (tests/match/test_hitstun.py). */
    { U3S_KOC_NORMAL, 6, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -13, 0, 0, 0, 0, 0 }, /* close s.HP (vanilla -2) */
    { U3S_KOC_NORMAL, 7, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -6, 0, 0, 0, 0, 0 }, /* far s.HP (vanilla -5) */
    { U3S_KOC_NORMAL, 24, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -15, 0, 0, 0, 0, 0 }, /* c.HP (vanilla -7) */
    { U3S_KOC_NORMAL, 12, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -13, 0, 0, 0, 0, 0 }, /* s.MK (vanilla -1) */
    { U3S_KOC_NORMAL, 16, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -18, 0, 0, 0, 0, 0 }, /* far s.HK (vanilla -7) */
    { U3S_KOC_NORMAL, 30, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -12, 0, 0, 0, 0, 0 }, /* c.MK (vanilla -3) */
    { U3S_KOC_NORMAL, 8, U3S_MV_HS_YOU, 0, 0, 0, 0, 0, -14, 0, 0, 0, 0, 0 }, /* 6HP (vanilla -4; two entries, the second decides) */
    /*
     * The Ryuubi Kyaku (236K) is the special move of the command 30, and
     * the three strengths are the entries 32, 33 and 34 of the slot of the
     * special moves.  The data of the disc puts the three of them in ONE
     * chain: the slot holds the same byte offset for the three entries.
     * That chain holds the three frames that hit - one per strength, with
     * the attack data 100, 101 and 102 of the attack table, which have the
     * same damage and the same stun and a different hit box - and the wind
     * up in front of them.
     *
     * A change of the chain is a change of all three strengths, so the
     * table holds one entry and not three.
     *
     * 3s': "Decrease the startup of L/M/H Ryuubi Kyaku (236K) from all
     * being 25f to 17/19/21f."  The data of the disc says that the three
     * of them start in 25 frames, which is what the document says as well.
     * The three targets of the document cannot be told apart in the data of
     * the disc, because the three strengths share one chain: a change of
     * that chain changes the three of them at once.  The middle target of
     * the document, 19 frames, is used - it is the target of the medium
     * version and it is the value that is the closest to all three of them.
     */
    { U3S_KOC_ATTACK, 32, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 25, 19, 0, 0 }, /* light, medium and heavy */
    /*
     * SN11 - "Decrease recovery on Hadou Burst from 61f to 51f."
     * The Hadou Burst (SA I) is special move animation 36 (37-39 share its chain). Its chain has no
     * attack data: the projectile is an effect of its own. Measured: the move starts on the frame of
     * the superflash; from chain frame 53 (the frames after the throw) Sean is not actionable for 60
     * game frames (61 in the post's count), then actionable for the last 10 frames of the chain. The
     * frames from 53 to the end, 70 in all, are brought to 60, the longest first, so the recovery
     * becomes 51 (50 in this count).
     */
    { U3S_KOC_ATTACK, 36, U3S_MV_TAIL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 53, 70, 60 }, /* Hadou Burst */
    /*
     * 3s': "Reduce startup of L/M/H/EX Dragon Smash (623P) from 5/5/9/8f to
     * 3/4/5/3f.  Somewhere between Ken and Ryu."
     *
     * The Dragon Smash is the special move of the command 29, and the four
     * strengths are the entries 64 to 67 of the slot of the special moves.
     * The data of the disc says that they start in 5/7/9/8 frames; the
     * document gives 5/5/9/8, which is the same three of the four and a
     * slip on the medium.  The targets of the document (3/4/5/3) are used
     * as they are: they are below the numbers of the disc and they keep
     * the progression of the strengths (three, four and five frames).
     */
    { U3S_KOC_ATTACK, 64, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 5, 3, 0, 0 }, /* light */
    { U3S_KOC_ATTACK, 65, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 7, 4, 0, 0 }, /* medium */
    { U3S_KOC_ATTACK, 66, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 9, 5, 0, 0 }, /* heavy */
    /*
     * The EX version is the one the document cannot have: it asks for three
     * frames, but the startup of the EX version is five frames of the chain
     * and a frame of a chain cannot be held for less than one game frame.
     * Five frames - the shortest the chain of the move can be made - is
     * used instead.
     */
    { U3S_KOC_ATTACK, 67, U3S_MV_STARTUP, 0, 0, 0, 0, 0, 0, 0, 8, 5, 0, 0 }, /* EX */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }                              /* terminator */
};

/*
 * TODO(3s'): "Increase hit advantage of L/M versions of Ryuubi Kyaku (236K),
 * to allow links to supers after hitting" is not in the table yet: the
 * Ryuubi Kyaku is a special move (U3S_KOC_ATTACK) and the indices of the
 * moves are known now - the Ryuubi Kyaku is the special move of the
 * command 30 - but the hit advantage of a special move is not read out of
 * the attack table alone.
 */
