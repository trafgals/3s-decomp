#ifndef UPPER3S_H
#define UPPER3S_H

#include "sf33rd/Source/Game/PLCNT.h"
#include "structs.h"
#include "types.h"

/*
 * Third Strike Upper (3s') - shared declarations.
 *
 * The balance patch is split in two layers:
 *
 *  1. Source level. Everything that lives in the C sources (system
 *     mechanics, cancel rules, the super art stock/gauge table, the stun
 *     bar table and the health table) is patched in place behind
 *     `#ifdef UPPER_3S`. The vanilla build is left byte identical.
 *
 *  2. Data level. The per character move data (startup, damage, stun,
 *     hitstun, hit advantage, hitboxes) is *not* part of the repository -
 *     it is read from the disc image at runtime and interpreted by
 *     CHARSET.c / HITCHECK.c. Those numbers are therefore patched at
 *     runtime, right after the character data has been loaded, through
 *     the override tables in this directory.
 *
 * See docs/upper3s/implementation-guide.md for the full description of
 * both layers and for the exact layout of the on-disc structures.
 */

/*
 * Character indexes.
 *
 * These are the indexes the *game* uses - they are not the order of
 * AGENTS.md section 3.1.  The game addresses a character by the index that
 * PLCNT.c:plid_data maps to its charset id:
 *
 *     0 Gill    1 Alex    2 Ryu     3 Yun     4 Dudley
 *     5 Necro   6 Hugo    7 Ibuki   8 Elena   9 Oro
 *    10 Yang   11 Ken    12 Sean   13 Urien  14 Akuma
 *    15 Chun-Li 16 Makoto 17 Q     18 Twelve 19 Remy
 *
 * The index is the value of My_char[ix] and of PLW.player_number, and it is
 * the index that every per character table of the game uses:
 * PLCNT.c:super_arts_data[], PLCNT.c:pl_piyo_tbl[], Com_Data.c:
 * Com_Vital_Unit_Data[], PLCNT.c:plid_data[] and bin2obj/asstbl.c.
 *
 * Proof of the order (all of them agree):
 *   - Grade.c:577 and sel_pl.c:2020 test My_char == 17 for Q.
 *   - win_pl.c:winner_type_tbl[13] selects Win_14000, which calls
 *     urien_dash; [17] selects Win_10000, which calls q_keeping_action and
 *     [18] selects Win_11000, which calls twelve_win_away.
 *   - PLCNT.c:setup_gouki_wins is the Akuma win, and PLCNT.c indexes it
 *     with 14.
 *   - PLPAT09.c carries Tengu Stone and Kishi Riki, the Oro supers.
 *   - PLPAT15.c has neither Hadouken nor Chou, which is Chun-Li.
 *   - The stock counts of the design document match the retail table only
 *     in this order (Ken 3 stocks in his SA III, Chun-Li 2 stocks in her
 *     SA II, Twelve 1 stock in his SA II, Remy the shortest SA III bar,
 *     Akuma and Remy the short stun gauge).
 */
enum {
    U3S_GILL = 0,
    U3S_ALEX = 1,
    U3S_RYU = 2,
    U3S_YUN = 3,
    U3S_DUDLEY = 4,
    U3S_NECRO = 5,
    U3S_HUGO = 6,
    U3S_IBUKI = 7,
    U3S_ELENA = 8,
    U3S_ORO = 9,
    U3S_YANG = 10,
    U3S_KEN = 11,
    U3S_SEAN = 12,
    U3S_URIEN = 13,
    U3S_AKUMA = 14,
    U3S_CHUNLI = 15,
    U3S_MAKOTO = 16,
    U3S_Q = 17,
    U3S_TWELVE = 18,
    U3S_REMY = 19,
    U3S_CHAR_COUNT = 20
};

/* Super art slot inside super_arts_data[char][4]. */
enum {
    U3S_SA1 = 0,
    U3S_SA2 = 1,
    U3S_SA3 = 2,
    U3S_SA4 = 3
};

/*
 * The "kind of character data" slots of WORK.char_table[].
 *
 * The value is the one that set_char_move_init() is called with; it ends up
 * in WORK.now_koc and selects the sub table of the character data that the
 * move is looked up in.  What the engine actually uses is:
 *
 *   0  the reaction moves - PLSxx.c calls set_char_move_init(wk, 0, ...)
 *   4  the normal attacks - PLPAT.c:Attack_00000() and its siblings
 *   5  the special moves *and* the super arts - PLPAT.c, PLPATUNI.c and
 *      the per character PLPATxx.c
 *
 * The special moves and the super arts share the slot 5; they are told
 * apart by their index inside the slot, which is unique per move.
 */
enum {
    U3S_KOC_REACTION = 0, /* dmca - damage reactions */
    U3S_KOC_DAMAGE = 1,   /* dmca */
    U3S_KOC_CA = 2,       /* caca */
    U3S_KOC_CU = 3,       /* cuca */
    U3S_KOC_NORMAL = 4,   /* atca - the normal attacks */
    U3S_KOC_ATTACK = 5,   /* saca - the special moves and the super arts */
    U3S_KOC_BUTTOBI = 6,
    U3S_KOC_EX = 7, /* exca */
    U3S_KOC_CB = 8, /* cbca */
    U3S_KOC_YU = 9, /* yuca */
    U3S_KOC_COUNT = 10
};

/*
 * The super arts use the same slot as the special moves; the alias keeps
 * the tables of upper3s/plXX.c readable.
 */
#define U3S_KOC_SUPER U3S_KOC_ATTACK

/*
 * Super art overrides.
 *
 * The values replace the retail base numbers found in
 * PLCNT.c:super_arts_data[char][sa]. The in-game option modifiers
 * (sag_stock_omake / sag_length_omake) are still applied on top of them.
 */
#define U3S_SA_STORE_MAX 0x01 /* gauge stocks the super costs */
#define U3S_SA_GAUGE_LEN 0x02 /* length of the gauge bar (0x40 - 0x80) */
#define U3S_SA_DTM 0x04       /* damage multiplier (SA_DATA.dtm) */

typedef struct {
    u16 flags;
    s16 store_max;
    s16 gauge_len;
    s32 dtm;
} Upper3sSuperArt;

/*
 * Move data overrides.
 *
 * A move is addressed by the "kind of character data" slot it lives in and
 * by its index inside that slot - the same pair the character state
 * machines use when they call set_char_move_init()/Command_Attack().
 *
 * The index of a normal attack is the char_ix of the disc's assignment
 * table, not the index of the move in the move list.  The assignment
 * tables (bin2obj/asstbl.c) are indexed by [player][button][variant]:
 *
 *   _asstbl_lv_0010  standing  5X / 6X / 4X   char_ix  0 - 17
 *   _asstbl_lv_1010  crouching 2X / 3X / 1X   char_ix 18 - 35
 *   _asstbl_lv_2010  jumping                  char_ix 36 - 47
 *   _asstbl_lv_3010  jumping                  char_ix 48 - 59
 *   _asstbl_lv_4010  jumping                  char_ix 60 - 71
 *
 * The button is the kos of the table - 0 LP, 1 MP, 2 HP, 3 LK, 4 MK,
 * 5 HK - and the variant is the koa - 0 without a dash, 1 with a dash
 * forward and 2 with a dash back, so that
 *
 *   char_ix = base + kos * 3 + koa
 *
 * The super arts are addressed by the char_ix of their entry in
 * bin2obj/asstbl.c:_assadr_lv_9900, the four strengths of a super being
 * four consecutive entries.
 *
 * damage is the raw base damage, expressed in the same unit as
 * POW_DATA.c:Power_Data (5 per step). Use damage_permille when the target
 * value is not a multiple of that unit.
 *
 * The "hs" of the retail fields is "hit stun" and not "hit advantage":
 * `hs_me` is the hitstun of the attacker - how long it is locked - and
 * `hs_you` is the hitstun of the defender, which is the hold that
 * HITCHECK.c:dm_status_copy() puts into `dm_stop` and that
 * PLMAIN.c:check_hit_stop() holds the defender for.  The knockback of the
 * hit is the field `impact`.  The stun is the field `piyo` as well, but
 * `piyo` is an index and not a number: the stun of a move is
 * `bin2obj/gauge.c:_add_piyo_gauge[player][piyo]`, which is what
 * upper3s_stun_value() reads.
 */
#define U3S_MV_DAMAGE 0x01        /* base damage, absolute */
#define U3S_MV_DAMAGE_PERMILLE 0x02 /* base damage, scaled by permille/1000 */
#define U3S_MV_STUN 0x04          /* stun added to the defender, absolute */
#define U3S_MV_IMPACT 0x08        /* the knockback of the hit (UNK_7.impact) */
#define U3S_MV_HS_ME 0x10         /* the hitstun of the attacker (UNK_7.hs_me) */
#define U3S_MV_HS_YOU 0x20        /* the hitstun of the defender (UNK_7.hs_you) */
#define U3S_MV_STUN_PERMILLE 0x40 /* stun, scaled by permille/1000 */
#define U3S_MV_STARTUP 0x80       /* the startup of the move, in game frames */
#define U3S_MV_RECOVERY 0x100     /* the recovery of the move, in game frames */

typedef struct {
    s16 koc;
    s16 index;
    u16 flags;
    s16 damage;
    s16 damage_permille;
    s16 stun;
    s16 impact;
    s16 hs_me;
    s16 hs_you;
    s16 stun_permille; /* only used when U3S_MV_STUN_PERMILLE is set */
    /*
     * The `from` fields are the value the frame chain of the disc has to
     * hold for the override to be applied: the walk of the machinery follows
     * the straight line of the chain and not the commands that call another
     * move, so the startup it reads is the startup of the retail data only
     * when the chain is the one that is expected.  A chain that does not
     * hold the value is left alone, so a wrong `from` silently skips the
     * change.
     *
     * A chain that already holds the target counts as done: the patch is
     * applied again every time the character is loaded, and the disc keeps
     * some moves that share one chain - the light, the medium and the heavy
     * version of the Ryuubi Kyaku of Sean, for example.  A change of such a
     * chain changes all the moves that share it, so the table holds one
     * entry per chain and not one entry per move.
     */
    s16 startup_from;  /* only used when U3S_MV_STARTUP is set */
    s16 startup;       /* only used when U3S_MV_STARTUP is set */
    s16 recovery_from; /* only used when U3S_MV_RECOVERY is set */
    s16 recovery;      /* only used when U3S_MV_RECOVERY is set */
} Upper3sMove;

/* Per character override tables. Defined by upper3s/plXX.c. */
extern const Upper3sMove* upper3s_moves[U3S_CHAR_COUNT];

/*
 * The record of the changes of the frame data that could not be applied.
 *
 * A change of the startup or of the recovery is only made when the chain
 * of the move holds the value of the retail data, and a change that the
 * chain cannot be brought to is not made either.  A change that is dropped
 * is not an error of the game - the game plays the retail data - but it is
 * a change of the patch that is not there, so it is counted and the last
 * one is kept.
 */
typedef struct {
    s32 count; /* how many changes could not be applied */
    s16 koc;   /* the slot of the last one */
    s16 index; /* the index of the last one */
    u16 flags; /* the change that could not be applied */
} Upper3sFailure;

const Upper3sFailure* upper3s_failure(void);

/*
 * Applies a single move override to the loaded attack table of `wk`.
 * Exposed so that the machinery can be tested on a synthetic character.
 */
void upper3s_apply_move(WORK* wk, const Upper3sMove* move);

/*
 * Applies the 3s' overrides for the character `wk` belongs to. Called
 * from CHARID.c:set_char_base_data() once the disc data is in place.
 */
void upper3s_apply_character(PLW* wk);

/*
 * Applies a super art override to a copy of the retail data. Exposed so
 * that the machinery can be tested without the game.
 */
void upper3s_apply_super_art(const Upper3sSuperArt* sa, SA_DATA* out);

/*
 * Returns the super art data that has to be used for (char_id, sa).
 *
 * `retail` is the entry of PLCNT.c:super_arts_data[char_id][sa]. The
 * 3s' values replace the retail base numbers; the in-game option
 * modifiers (sag_stock_omake / sag_length_omake) are still applied on
 * top of them by PLCNT.c.
 */
const SA_DATA* upper3s_super_art_data(s16 char_id, s16 sa, const SA_DATA* retail);

/*
 * Whether the 3s' layer overrides the super art slot of a character.
 */
int upper3s_super_art_is_listed(s16 char_id, s16 sa);

/*
 * Scale, in permille, that has to be applied to the base damage of the
 * attack `wk` is currently resolving. 1000 means "no change".
 *
 * `char_id` is the character index of the attacker (0 - 19).
 */
s16 upper3s_damage_permille(s16 char_id, WORK* wk);

/*
 * Base damage of the attack `wk` is currently resolving, or -1 when the
 * attack is not overridden.
 */
s16 upper3s_damage_base(s16 char_id, WORK* wk);

/*
 * The cancel mask of the move the character is playing.
 *
 * The retail game reads the mask out of the frame chain of the move, one
 * byte per frame (PLS00.c:check_cg_cancel_data()). The design document
 * changes the cancels of some moves of some characters, and the rules of
 * those changes are in the module of the character
 * (upper3s/upper3s_ch_<name>.c).
 *
 * `char_id` is the character index of the game (0 - 19), `koc` is the kind of
 * character data the move lives in (4 = the normal attacks, 5 = the special
 * moves and the super arts), `char_ix` is the index of the move inside that
 * kind of data and `cg_cancel` is the mask of the disc. The mask that is
 * returned is the one the move has to be played with.
 *
 * The move is addressed by its char_ix and not by the routine number of the
 * state machine (routine_no[2]): the routine number is the *behaviour class*
 * of the normal, not the move.  The assignment tables of the disc
 * (bin2obj/asstbl.c) give the routine number of a normal as r_no, and it is
 * 0 for most of the normals and 4 for the medium punches, so the routine
 * number cannot tell the crouching Medium Punch (char_ix 21) from the
 * standing one (char_ix 3).  The char_ix is the index of the move in the
 * character data and it is the only thing that identifies it.
 */
u8 upper3s_cancel_mask(s16 char_id, s16 koc, s16 char_ix, u8 cg_cancel);

#if defined(UPPER3S_DUMP)
/*
 * Dumps the character data of the disc to upper3s_dump.txt: the frame
 * chain and the attack data of every move of every character.
 *
 * The dump is the source of the values of the balance changes: the frame
 * data of the moves is not in this repository, it is read from the disc at
 * runtime, so the retail values have to be read out of the disc to be able
 * to change them.
 */
void upper3s_dump_start(void);

/*
 * Asks the loader for the next character to dump. Called every frame: the
 * loader reads one request per frame, and its request queue can be full,
 * so the request is retried until it goes through.
 */
void upper3s_dump_pump(void);

/*
 * Dumps the character `char_index` (the index of the game, 0 - 19). `cit`
 * is the data of the character, as it has just been read from the disc.
 */
void upper3s_dump_character(s16 char_index, const CharInitData* cit);

/*
 * Dumps a character whose data the loader has read before: the data is then
 * already in the table of the loader.
 */
void upper3s_dump_character_loaded(s16 char_index);
#endif

/*
 * Stun the attack `wk` is currently resolving adds to the defender, or -1
 * when the attack is not overridden. `piyo` is an index into a per player
 * table, so the stun value cannot be patched in place.
 */
s16 upper3s_stun_value(s16 char_id, WORK* wk);

/*
 * Scale, in permille, that has to be applied to the stun of the attack
 * `wk` is currently resolving. 1000 means "no change".
 *
 * This is the preferred way to change a stun: the retail stun is read from
 * the disc (the index att.piyo into _add_piyo_gauge), so an absolute value
 * can only be a guess, while a scale keeps the relation to the retail
 * value.
 */
s16 upper3s_stun_permille(s16 char_id, WORK* wk);

#endif
