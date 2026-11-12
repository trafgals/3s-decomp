// The harness of the headless port (see port/headless/headless.h): runs one scripted
// fight per process and logs the players' state on every frame.
//
// How a fight is set up: the attract-mode demo fight (DEMO02.c:Demo00()) is a normal
// fight run by Game.c:Game02(), and DEMO02.c:Setup_Demo_PL() already takes its
// characters from Debug_w[0x1D] / Debug_w[0x1E] (character index + 1). The harness sets
// those, overrides Super_Arts[], and makes both players human-operated so that their
// input comes from Convert_User_Setting() (SYS_sub.c), where harness_input() supplies
// it. Demo_Flag only changes CPU behaviour (Weak_PL), not the fight engine. The script
// starts on the first frame of the round on which Allow_a_battle_f is set.
//
// Environment:
//   SF33_P1, SF33_P2        character index of each player (AGENTS.md 3.1); default 2 (Ryu)
//   SF33_SA1, SF33_SA2      super art slot, 0 = SA I .. 2 = SA III; default 0
//   SF33_SCRIPT=path        input script (below); without one the players stand still
//   SF33_LOG=path           per-frame JSON lines from the first scripted frame on
//   SF33_FRAMES=N           give up after N frames in total (default 6000)
//   SF33_TRACE_EVERY=N      print the game's state numbers every N frames
//   SF33_LIST_ATTACKS=koc   (3s' build) list the chains of player 1's slot koc; with
//   SF33_LIST_FRAMES=ix     also the frames of chain ix, SF33_LIST_BLOCKS=ix its raw blocks
//
// Script: one step per line, "#" starts a comment.
//   <frames> <p1> [<p2>]    hold these inputs for <frames> frames. An input is a numpad
//                           direction relative to the character's facing (6 = towards
//                           the opponent, 5 = neutral) plus optional +buttons, e.g.
//                           5, 2+MK, 6+LP+LK, 5+HP+HK. Buttons: LP MP HP LK MK HK.
//   @pos <x1> <x2>          place the players at these x positions (screen units)
//   @stocks <p> <n>         give player p (1 or 2) n super art stocks
//   @vit <p> <n>            set player p's health (bar units, 160 is full)
//   @vitmode <n>            the health mode of both players (SysDir.c:omop_vital_ix, read by
//                           PLMAIN.c:check_omop_vital(); 0 is a normal fight, in which health does
//                           not come back and a character can be knocked out)
//   @end                    stop here (the run also stops at the end of the script)
// Exit code: 0 when the script ran to its end, 2 when the fight never started.

#include "port/headless/headless.h"
#include "common.h"
#include "sf33rd/Source/Game/PLCNT.h"
#include "sf33rd/Source/Game/SysDir.h"
#include "sf33rd/Source/Game/cmd_data.h"
#include "sf33rd/Source/Game/debug/Debug.h"
#include "sf33rd/Source/Game/workuser.h"
#include "structs.h"
#if defined(UPPER_3S)
#include "sf33rd/Source/Game/upper3s_chain.h"
#include "sf33rd/Source/Game/upper3s_box.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Game.c; not declared in a header.
void Loop_Demo_Sub();

// --- input encoding -----------------------------------------------------------------
//
// The game's input word (the result of Convert_User_Setting(), SYS_sub.c): the lever in
// the low four bits and the buttons as in SYS_sub.c:Convert_Data. The lever bits are
// absolute here; CMD_MAIN.c:pl_lvr_set() makes the horizontal ones relative to facing.
#define IN_UP 0x1
#define IN_DOWN 0x2
#define IN_LEFT 0x4
#define IN_RIGHT 0x8
#define IN_LP 0x10
#define IN_MP 0x20
#define IN_HP 0x40
#define IN_LK 0x100
#define IN_MK 0x200
#define IN_HK 0x400

typedef enum { STEP_INPUT, STEP_POS, STEP_STOCKS, STEP_VIT, STEP_VITMODE, STEP_END } StepKind;

typedef struct {
    StepKind kind;
    int frames;
    int dir[2];        // numpad direction per player
    u16 buttons[2];    // internal button bits per player
    int args[2];
} Step;

#define MAX_STEPS 4096
static Step steps[MAX_STEPS];
static int step_count = 0;

static int cfg_char[2] = { 2, 2 };
static int cfg_sa[2] = { 0, 0 };
static unsigned long max_frames = 6000;
static unsigned long trace_every = 0;
static FILE* log_file = NULL;

static int fight_started = 0;
static unsigned long fight_frame = 0; // scripted frames run so far
static int step_ix = 0;
static int step_left = 0;
static int quit = 0;
static int exit_code = 2;
static unsigned long last_frame = 0;

static u16 current_input[2] = { 0, 0 };

static unsigned long env_ulong(const char* name, unsigned long fallback) {
    const char* value = getenv(name);
    return (value != NULL && *value != '\0') ? strtoul(value, NULL, 10) : fallback;
}

static void die(const char* message, const char* detail) {
    fprintf(stderr, "harness: %s%s%s\n", message, detail ? ": " : "", detail ? detail : "");
    exit(3);
}

// "6+LP+LK" -> direction 6, buttons LP|LK.
static void parse_input(const char* token, int* dir, u16* buttons) {
    if (!isdigit((unsigned char)token[0]) || token[0] == '0') {
        die("an input must start with a numpad direction 1-9", token);
    }

    *dir = token[0] - '0';
    *buttons = 0;

    const char* p = token + 1;

    while (*p == '+') {
        p += 1;
        static const struct {
            const char* name;
            u16 bit;
        } names[] = { { "LP", IN_LP }, { "MP", IN_MP }, { "HP", IN_HP }, { "LK", IN_LK }, { "MK", IN_MK }, { "HK", IN_HK } };
        int found = 0;

        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
            if (strncmp(p, names[i].name, 2) == 0) {
                *buttons |= names[i].bit;
                p += 2;
                found = 1;
                break;
            }
        }

        if (!found) {
            die("unknown button", token);
        }
    }

    if (*p != '\0') {
        die("trailing characters in input", token);
    }
}

static void load_script(const char* path) {
    FILE* f = fopen(path, "r");
    char line[512];

    if (f == NULL) {
        die("cannot open the script", path);
    }

    while (fgets(line, sizeof(line), f) != NULL) {
        char* hash = strchr(line, '#');
        char a[64] = "", b[64] = "", c[64] = "";

        if (hash != NULL) {
            *hash = '\0';
        }

        int n = sscanf(line, "%63s %63s %63s", a, b, c);

        if (n <= 0) {
            continue;
        }

        if (step_count >= MAX_STEPS) {
            die("the script is too long", path);
        }

        Step* s = &steps[step_count++];
        memset(s, 0, sizeof(*s));

        if (strcmp(a, "@end") == 0) {
            s->kind = STEP_END;
        } else if (strcmp(a, "@pos") == 0 && n == 3) {
            s->kind = STEP_POS;
            s->args[0] = atoi(b);
            s->args[1] = atoi(c);
        } else if (strcmp(a, "@stocks") == 0 && n == 3) {
            s->kind = STEP_STOCKS;
            s->args[0] = atoi(b) - 1;
            s->args[1] = atoi(c);
        } else if (strcmp(a, "@vit") == 0 && n == 3) {
            s->kind = STEP_VIT;
            s->args[0] = atoi(b) - 1;
            s->args[1] = atoi(c);
        } else if (strcmp(a, "@vitmode") == 0 && n == 2) {
            s->kind = STEP_VITMODE;
            s->args[0] = atoi(b);
        } else if (isdigit((unsigned char)a[0]) && n >= 2) {
            s->kind = STEP_INPUT;
            s->frames = atoi(a);
            parse_input(b, &s->dir[0], &s->buttons[0]);
            parse_input(n == 3 ? c : "5", &s->dir[1], &s->buttons[1]);
        } else {
            die("cannot read the script line", line);
        }
    }

    fclose(f);
}

void harness_init(void) {
    const char* script = getenv("SF33_SCRIPT");
    const char* log_path = getenv("SF33_LOG");

    cfg_char[0] = (int)env_ulong("SF33_P1", cfg_char[0]);
    cfg_char[1] = (int)env_ulong("SF33_P2", cfg_char[1]);
    cfg_sa[0] = (int)env_ulong("SF33_SA1", cfg_sa[0]);
    cfg_sa[1] = (int)env_ulong("SF33_SA2", cfg_sa[1]);
    max_frames = env_ulong("SF33_FRAMES", max_frames);
    trace_every = env_ulong("SF33_TRACE_EVERY", trace_every);

    for (int i = 0; i < 2; i++) {
        if (cfg_char[i] < 0 || cfg_char[i] > 19 || cfg_sa[i] < 0 || cfg_sa[i] > 2) {
            die("SF33_P1/SF33_P2 must be 0-19 and SF33_SA1/SF33_SA2 0-2", NULL);
        }
    }

    if (script != NULL && *script != '\0') {
        load_script(script);
    }

    if (log_path != NULL && *log_path != '\0') {
        log_file = fopen(log_path, "w");

        if (log_file == NULL) {
            die("cannot open the log", log_path);
        }
    }
}

// Numpad direction relative to the character's facing -> absolute lever bits.
// rl_flag != 0 means the character faces right (checked by tests/headless).
static u16 lever_bits(int dir, int faces_right) {
    u16 bits = 0;
    int forward = (dir == 3 || dir == 6 || dir == 9);
    int back = (dir == 1 || dir == 4 || dir == 7);

    if (dir >= 7) {
        bits |= IN_UP;
    }

    if (dir <= 3) {
        bits |= IN_DOWN;
    }

    if (forward) {
        bits |= faces_right ? IN_RIGHT : IN_LEFT;
    }

    if (back) {
        bits |= faces_right ? IN_LEFT : IN_RIGHT;
    }

    return bits;
}

int harness_input(int player, unsigned short* word) {
    if (!fight_started || player < 0 || player > 1) {
        return 0;
    }

    *word = current_input[player];
    return 1;
}

// Steps that take no time run at once; returns 0 when the script is over.
static int advance_script(void) {
    while (step_left == 0) {
        if (step_ix >= step_count) {
            return 0;
        }

        Step* s = &steps[step_ix++];

        switch (s->kind) {
        case STEP_END:
            step_ix = step_count;
            return 0;

        case STEP_POS:
            plw[0].wu.xyz[0].disp.pos = s->args[0];
            plw[1].wu.xyz[0].disp.pos = s->args[1];
            break;

        case STEP_STOCKS:
            if (s->args[0] == 0 || s->args[0] == 1) {
                SA_WORK* sa = plw[s->args[0]].sa;
                sa->store = s->args[1] > sa->store_max ? sa->store_max : s->args[1];
            }
            break;

        case STEP_VIT:
            if (s->args[0] == 0 || s->args[0] == 1) {
                plw[s->args[0]].wu.vital_new = s->args[1];
                plw[s->args[0]].wu.vital_old = s->args[1];
            }
            break;

        case STEP_VITMODE:
            omop_vital_ix[0] = omop_vital_ix[1] = s->args[0];
            break;

        case STEP_INPUT:
            step_left = s->frames;
            break;
        }
    }

    return 1;
}

void harness_begin_frame(unsigned long frame) {
    if (!fight_started) {
        // The attract loop (Game.c, G_No[0] == 1) runs: 1 logo, 2 title, 3 a character
        // select demo and its fight, 4 ranking, 5 the demo fight Demo00(), 6 ranking. The
        // ranking screen (4) calls Setup_Demo_PL() / Setup_Demo_Arts() for the fight of
        // 5, so the overrides start there. Earlier they would break the select demo of 3,
        // which needs one CPU side. The players must be human before the round sets up:
        // PLS02.c:setup_vitality() gives a CPU player a different health.
        if (G_No[0] == 1 && G_No[1] == 3) {
            // The character select demo of 3 never finishes in the headless port
            // (Select_Player() does not complete), and it is not needed: skip to the
            // ranking screen the way the attract loop itself moves on.
            Loop_Demo_Sub();
            return;
        }

        if (G_No[0] != 1 || (G_No[1] != 4 && G_No[1] != 5)) {
            return;
        }

        Debug_w[0x1D] = cfg_char[0] + 1;
        Debug_w[0x1E] = cfg_char[1] + 1;
        Super_Arts[0] = cfg_sa[0];
        Super_Arts[1] = cfg_sa[1];

        for (int i = 0; i < 2; i++) {
            plw[i].wu.operator = 1;
            Operator_Status[i] = 1;
        }

        if (G_No[1] == 5 && Allow_a_battle_f && My_char[0] == cfg_char[0] && My_char[1] == cfg_char[1]) {
            fight_started = 1;
#if defined(UPPER_3S)
            // SF33_DUMP_BOX=kind,index: print player 1's box set (upper3s_box.h kinds).
            if (getenv("SF33_DUMP_BOX") != NULL) {
                int kind = 0;
                int index = 0;

                if (sscanf(getenv("SF33_DUMP_BOX"), "%d,%d", &kind, &index) == 2) {
                    for (s16 b = 0; b < 4; b++) {
                        s16* box = upper3s_box_ptr(&plw[0].wu, (s16)kind, (s16)index, b);

                        if (box != NULL) {
                            printf("box kind %d index %d box %d: %d %d %d %d\n", kind, index, b, box[0], box[1], box[2], box[3]);
                        }
                    }
                }
            }

            // SF33_LIST_ATTACKS=koc: print the attack entries of every move of player 1's
            // slot koc (4 normals, 5 specials and supers), as the override layer sees them.
            if (getenv("SF33_LIST_ATTACKS") != NULL) {
                s16 koc = (s16)atoi(getenv("SF33_LIST_ATTACKS"));

                for (s16 ix = 0; ix < 256; ix++) {
                    Upper3sChain chain;
                    s16 entries[U3S_CHAIN_MAX_ATTACKS];
                    Upper3sChainInfo info;

                    if (!upper3s_chain_open(&plw[0].wu, koc, ix, &chain)) {
                        continue;
                    }

                    if (getenv("SF33_LIST_BLOCKS") != NULL && atoi(getenv("SF33_LIST_BLOCKS")) == ix) {
                        // The raw blocks of the chain: w0 (type | hold << 8 | sound << 16, or a command code when
                        // below 0x100), w1, w2 (hit box index | attack index << 22), w3 (cg_extdat, the on-hit
                        // jump | cg_cancel << 8 | effect << 16 | effect type << 24), and the attack index.
                        for (s32 b = 0; b < 48 && chain.base + b * chain.cgd + 2 < chain.count; b++) {
                            u32* w = chain.words + chain.base + b * chain.cgd;
                            printf("  block %2d w0 %08x w1 %08x w2 %08x w3 %08x att %d\n", b, w[0], w[1], w[2],
                                   (chain.cgd > 3) ? w[3] : 0, (s16)(w[2] >> 16) >> 6);
                        }
                    }

                    if (getenv("SF33_LIST_FRAMES") != NULL && atoi(getenv("SF33_LIST_FRAMES")) == ix) {
                        // The frames of the straight line of the chain: the game frame each starts on, its
                        // hold and its attack index.
                        Upper3sFrame frames[U3S_CHAIN_MAX_FRAMES];
                        s32 nf = upper3s_chain_list(&chain, frames, U3S_CHAIN_MAX_FRAMES);
                        s32 t = 0;

                        for (s32 f = 0; f < nf; f++) {
                            printf("  frame %2d at %3d hold %3d att %d\n", (int)f, (int)t, (int)frames[f].hold,
                                   (int)frames[f].att);
                            t += frames[f].hold;
                        }
                    }

                    s32 n = upper3s_chain_attacks(&chain, entries, U3S_CHAIN_MAX_ATTACKS);
                    upper3s_chain_info(&chain, &info);
                    printf("attacks koc %d cix %d base %d startup %d recovery %d entries", koc, ix, chain.base,
                           info.startup, info.recovery);
                    for (s32 k = 0; k < n; k++) {
                        printf(" %d(hs_you %d hs_me %d impact %d)", entries[k], plw[0].wu.att_ix_table[entries[k]].hs_you,
                               plw[0].wu.att_ix_table[entries[k]].hs_me, plw[0].wu.att_ix_table[entries[k]].impact);
                    }
                    n = upper3s_chain_block_attacks(&chain, entries, U3S_CHAIN_MAX_ATTACKS);
                    printf(" | all blocks");
                    for (s32 k = 0; k < n; k++) {
                        printf(" %d", entries[k]);
                    }
                    printf("\n");
                }
                fflush(stdout);
            }
#endif
        } else {
            return;
        }
    }

    if (!advance_script()) {
        exit_code = 0;
        quit = 1;
        current_input[0] = current_input[1] = 0;
        return;
    }

    Step* s = &steps[step_ix - 1];

    for (int i = 0; i < 2; i++) {
        current_input[i] = lever_bits(s->dir[i], plw[i].wu.rl_flag != 0) | s->buttons[i];
    }

    step_left -= 1;
}

// The number of hurtboxes of a work on this frame: the boxes of body_dm (HITCHECK.c:attack_hit_check() reads
// four of them) whose width, the second word, is not 0. A frame without one cannot be hit.
static int hurtboxes(const WORK* w) {
    int n = 0;

    if (w->h_bod == NULL) {
        return -1;
    }

    for (int i = 0; i < 4; i++) {
        if (w->h_bod->body_dm[i][1] != 0) {
            n += 1;
        }
    }

    return n;
}

// The lowest bottom edge of the attack boxes the hit check is given on this frame (WORK.h_att, after the 3s'
// box changes), or -9999 when the frame has none (a bottom can be below the character's feet, so negative).
static int attack_bottom(const WORK* w) {
    int low = -9999;

    if ((w->h_att == NULL) || (w->cg_ja.atix == 0)) {
        return -9999;
    }

    for (int i = 0; i < 4; i++) {
        if ((w->h_att->att_box[i][1] != 0) && ((low == -9999) || (w->h_att->att_box[i][2] < low))) {
            low = w->h_att->att_box[i][2];
        }
    }

    return low;
}

// "hb" is hurtboxes() and "mu" zettai_muteki_flag: a player with the flag is not put into the hit queue
// (PLCNT.c), so it can be hit only on a frame with hb > 0 and mu 0. "sas" is sa_stop_flag: 2 while the
// character is frozen by the opponent's superflash (PLS01.c:sa_stop_check()). "meo" is cg_meoshi, the
// frame's target combo route (PLS03.c:check_meoshi_cancel()), "swn" the command buffer's sw_new, "cab" the frame's catch box (cg_ja.caix), "bx" its body, attack, hosei and hand box sets (boix, atix,
// hoix, bhix + haix; upper3s_box.h), "aby" attack_bottom(), "aho" att_hit_ok (an attack is armed and
// can still hit; "anum" is its attack_num, a new number for each attack started, CHARSET.c:set_new_attnum()), "blk"
// the block of the animation the frame is (cg_ix / cgd_type; command blocks count), "dmp" the dm_point of
// the last hit taken (the defender's box it landed on: 0-3 body, 4-7 limbs; HITCHECK.c), "rea" the reaction of the
// player's current attack data (UNK_7.reaction: the damage routine a hit gives, before the air map). "par" is the forward ground parry slot of the command buffer: its waza_flag[3] (the window: the parry is
// active while it is not 0) and its waza_work[id][3] w_type, shot_ok and free3 (the cooldown: the slot takes
// no new tap while shot_ok >= 2 and w_type is 11, see CMD_MAIN.c:check_10()). "uk" is ukemi_success, set
// when the down tap that techs a landing was in time.
static void log_player(const PLW* p) {
    const WORK* w = &p->wu;

    fprintf(log_file,
            "{\"x\":%d,\"y\":%d,\"rl\":%d,\"rno\":[%d,%d,%d,%d],\"koc\":%d,\"cix\":%d,\"cgt\":%d,\"hix\":%d,"
            "\"aix\":%d,\"can\":%d,\"pat\":%d,\"waza\":%d,\"hst\":%d,\"dst\":%d,\"vit\":%d,\"dmv\":%d,\"anum\":%d,"
            "\"pow\":%d,\"piyo\":%d,\"hsme\":%d,\"hsyou\":%d,\"guard\":%d,\"stun\":%d,\"stunmax\":%d,"
            "\"stunned\":%d,\"gauge\":%d,\"stock\":%d,\"stockmax\":%d,\"glen\":%d,\"in\":%d,\"char\":%d,"
            "\"sa\":%d,\"ovit\":%d,\"gtot\":%d,\"gi\":%d,\"ren\":%d,\"par\":[%d,%d,%d,%d],\"uk\":%d,\"hb\":%d,\"mu\":%d,\"sas\":%d,\"meo\":%d,\"swn\":%d,\"cab\":%d,\"bx\":[%d,%d,%d,%d],\"aby\":%d,\"aho\":%d,\"blk\":%d,\"dmp\":%d,\"rea\":%d}",
            w->xyz[0].disp.pos, w->xyz[1].disp.pos, w->rl_flag, w->routine_no[0], w->routine_no[1],
            w->routine_no[2], w->routine_no[3], w->now_koc, w->char_index, w->cg_type, w->cg_hit_ix, w->cg_att_ix,
            w->cg_cancel, w->pat_status, w->kind_of_waza, w->hit_stop, w->dm_stop, w->vital_new, w->dm_vital,
            w->attack_num, w->att.pow, w->att.piyo, w->att.hs_me, w->att.hs_you, p->guard_flag,
            p->py ? p->py->now.quantity.h : -1, p->py ? p->py->genkai : -1, p->py ? p->py->flag : -1,
            p->sa ? p->sa->gauge.s.h : -1, p->sa ? p->sa->store : -1, p->sa ? p->sa->store_max : -1,
            p->sa ? p->sa->gauge_len : -1, current_input[p == &plw[0] ? 0 : 1], p->player_number,
            p->sa ? p->sa->kind_of_arts : -1, w->original_vitality, p->sa ? p->sa->total_gauge : -1,
            p->sa ? p->sa->gauge.i : -1, w->renew_attack < 0 ? -w->renew_attack : w->renew_attack,
            p->cp ? p->cp->waza_flag[3] : -1, waza_work[w->id & 1][3].w_type, waza_work[w->id & 1][3].shot_ok,
            waza_work[w->id & 1][3].free3, p->ukemi_success, hurtboxes(w),
            p->zettai_muteki_flag, p->sa_stop_flag, w->cg_meoshi, p->cp ? p->cp->sw_new : -1, w->cg_ja.caix, w->cg_ja.boix,
            w->cg_ja.atix, w->cg_ja.hoix, w->cg_ja.bhix + w->cg_ja.haix, attack_bottom(w), w->att_hit_ok, (w->cgd_type > 0) ? w->cg_ix / w->cgd_type : -1, p->dm_point, w->att.reaction);
}

void harness_end_frame(unsigned long frame) {
    last_frame = frame;

    if (trace_every != 0 && frame % trace_every == 0) {
        printf("frame %lu G_No %d %d %d %d E_No %d %d %d %d Allow %d My_char %d %d\n", frame, G_No[0], G_No[1],
               G_No[2], G_No[3], E_No[0], E_No[1], E_No[2], E_No[3], Allow_a_battle_f, My_char[0], My_char[1]);
        fflush(stdout);
    }

    if (fight_started && log_file != NULL) {
        fight_frame += 1;
        fprintf(log_file, "{\"f\":%lu,\"p\":[", fight_frame);
        log_player(&plw[0]);
        fputc(',', log_file);
        log_player(&plw[1]);
        fputs("]}\n", log_file);
    }

    if (frame >= max_frames) {
        quit = 1;
    }
}

int harness_should_quit(void) {
    return quit;
}

int harness_exit_code(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }

    return exit_code;
}
