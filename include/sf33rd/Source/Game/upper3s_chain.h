#ifndef UPPER3S_CHAIN_H
#define UPPER3S_CHAIN_H

#include "sf33rd/Source/Game/upper3s.h"

/*
 * Third Strike Upper - the frame chain of the disc.
 *
 * The frame data of the moves is not part of the repository: it is read
 * from the disc at runtime.  The data of a move is a *chain*: a list of
 * blocks of `cgd_type` u32 words that the engine walks once per game
 * frame (CHARSET.c:char_move()).
 *
 *   word 0   cg_type | cg_ctr << 8 | cg_se << 16
 *   word 1   cg_olc_ix | cg_number << 16
 *   word 2   cg_hit_ix | cg_att_ix << 16
 *
 * `cg_ctr` - the second byte of the first word - is the number of game
 * frames the frame is held for.  A block whose first word is below 0x100
 * has a `cg_ctr` of zero: it is a *command* block and not a frame.
 *
 * The **startup** of a move is the sum of the holds of the frames before
 * the first frame that carries the attack data - the frame that hits - and
 * the **recovery** is the sum of the holds of the frames after the last
 * frame that carries the attack data.
 *
 * The design document changes the startup of a few moves of a few
 * characters.  This module reads those numbers out of the data that was
 * loaded from the disc and writes them back, so that the change is made
 * in the data of the move and not in the engine.
 */

/* The maximum number of the frames of a chain that is walked. */
#define U3S_CHAIN_MAX_FRAMES 64

/*
 * The hold of the frame that marks the end of the animation.
 *
 * The chain of a move ends with a frame that is held for a long time: the
 * character state machine decides when the move ends and the chain only
 * waits there.  The walk of the machinery stops at that frame.
 */
#define U3S_CHAIN_WAIT_HOLD 250

/*
 * The type of the frame that marks the end of the animation.
 *
 * The low byte of the first word of a block is the type of the frame, and
 * the type 0xFF is the end of the animation: the game reads it in appear.c
 * and in the character state machines to know that the chain is over.
 */
#define U3S_CHAIN_END_CODE 0xFF

/* The chain of one move of the character data of one character. */
typedef struct {
    u32* words; /* the slot of the character data */
    s32 count;  /* the number of the words of the slot */
    s32 base;   /* the word index of the first block of the move */
    s32 cgd;    /* the number of the words of a block */
} Upper3sChain;

/* One frame of a chain. */
typedef struct {
    s32 word; /* the word index of the block of the frame */
    s32 hold; /* the number of the game frames the frame is held for */
    s32 att;  /* the index of the attack data, 0 when the frame has none */
} Upper3sFrame;

/* The frame data of a move. */
typedef struct {
    s16 startup;  /* the game frames before the first hit, -1 when unknown */
    s16 recovery; /* the game frames after the last hit, -1 when unknown */
} Upper3sChainInfo;

/*
 * Opens the chain of the move (koc, index) of the character `wk`.
 *
 * Returns 0 when the chain cannot be located: the slot is not loaded, the
 * index is outside of it, or the block size of the move is not a sane
 * value.
 */
int upper3s_chain_open(WORK* wk, s16 koc, s16 index, Upper3sChain* chain);

/*
 * Reads the startup and the recovery of the chain out of its data.
 *
 * The startup is the number of the game frames before the first frame of
 * the chain that carries the attack data, and the recovery is the number
 * of the game frames that follow the last frame that carries it.
 *
 * The walk follows the straight line of the chain: it skips the command
 * blocks and it follows the two commands that jump over the blocks of the
 * chain, because they are part of that line.  A command block that calls
 * another move moves the animation into the chain of that move, so the
 * frames that follow such a block are not the frames of this move; the
 * caller therefore checks the numbers it reads against the numbers of the
 * retail data before it changes anything.
 *
 * The recovery is only known when the walk reached the end of the chain,
 * so it is -1 when the walk was cut short.
 *
 * Returns 0 when the chain holds no frame with attack data at all.
 */
int upper3s_chain_info(const Upper3sChain* chain, Upper3sChainInfo* info);

/*
 * Sets the startup of the move to `frames` game frames.
 *
 * The change is only made when the chain still has the startup `from`,
 * which is the startup of the retail data: the walk of the machinery
 * follows the straight line of the chain and not the commands that call
 * another move, so the startup it reads is the startup of the retail data
 * only when the chain is the one that is expected.
 *
 * The frames of the startup are shortened from the longest one, and no
 * frame is shortened below a single game frame: a block with a hold of
 * zero is a command block and not a frame.  A startup that is longer than
 * the one of the retail data is made longer instead: the last frame of the
 * startup is the one that is held the longest before the hit.  When the
 * startup cannot be brought to `frames` - because the chain holds too few
 * frames, or because a frame would have to be held for more than 255 game
 * frames - the chain is left alone and 0 is returned.
 *
 * A chain that already has the startup `frames` is left alone and the
 * change is reported as done.  That is the case when the patch is applied
 * again - it is applied every time the character is loaded - and when
 * several moves of the patch share one chain: the disc keeps the light,
 * the medium and the heavy version of some moves in one chain, so a
 * change of that chain changes all of them at once.
 */
int upper3s_chain_set_startup(Upper3sChain* chain, s16 from, s16 frames);

/*
 * Sets the recovery of the move to `frames` game frames.  The frames of
 * the recovery are shortened the same way the frames of the startup are,
 * and the change is only made when the chain still has the recovery
 * `from` - or when it already has the recovery `frames`.
 */
int upper3s_chain_set_recovery(Upper3sChain* chain, s16 from, s16 frames);

/*
 * The startup of the move (koc, index) of the character `wk`, or -1 when
 * it cannot be read.
 */
s16 upper3s_chain_startup(WORK* wk, s16 koc, s16 index);

#endif
