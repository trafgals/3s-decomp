#include "sf33rd/Source/Game/PLPAT12.h"
#include "common.h"
#include "sf33rd/Source/Game/CHARSET.h"
#include "sf33rd/Source/Game/EFFD7.h"
#include "sf33rd/Source/Game/Grade.h"
#include "sf33rd/Source/Game/PLPAT.h"
#include "sf33rd/Source/Game/PLPATUNI.h"
#include "sf33rd/Source/Game/PLS01.h"
#include "sf33rd/Source/Game/PLS02.h"

#if defined(UPPER_3S)
#include "sf33rd/Source/Game/CMD_MAIN.h"
#include "sf33rd/Source/Game/PLCNT.h"
#include "sf33rd/Source/Game/upper3s_ch_sean.h"
#endif

void (*const pl12_exatt_table[18])(PLW*);

void pl12_extra_attack(PLW* wk) {
#if defined(UPPER_3S)
    // Third Strike Upper (SN6): a kick in the startup of the Fierce Tackle fakes it. The move ends the way
    // a finished move does (PLS00.c:jumping_cg_type_check(), cg_type 0xFF); the kick is taken out of the
    // new presses first, so that it does not start a kick at once.
    if (upper3s_ch_sean_tackle_feint(wk->wu.id, wk->wu.now_koc, wk->wu.char_index, wk->wu.cg_ja.caix,
                                     wk->cp->sw_new)) {
        s16 i;

        // The commands the Tackle's motion and the kick have completed (41236 + K is also 236K, the Ryuubi
        // Kyaku) are dropped and the command slots start again, so the kick does not turn into a move.
        wk->cp->sw_new &= ~U3S_SEAN_KICKS;

        for (i = 0; i < 56; i++) {
            if (wk->cp->waza_flag[i] > 0) {
                wk->cp->waza_flag[i] = 0;
            }
        }

        waza_compel_all_init2(wk);
        wk->guard_flag = 0;
        clear_chainex_check(wk->wu.id);

        // Back to the neutral stance (PLS00.c:TO_nm_01000(), without its input check on this frame).
        wk->wu.routine_no[1] = 0;
        wk->wu.routine_no[2] = 1;
        wk->wu.routine_no[3] = 0;
        wk->wu.cg_type = 0;
        return;
    }
#endif
    pl12_exatt_table[wk->wu.routine_no[2] - 16](wk);
}

void Att_PL12_TOKUSHUKOUDOU(PLW* wk) {
#if defined(TARGET_PS2)
    void set_char_move_init(WORK * wk, s16 koc, s32 index);
    void setup_mvxy_data(WORK * wk, u32 ix);
    void grade_add_personal_action(s32 ix);
#endif

    wk->scr_pos_set_flag = 0;

    switch (wk->wu.routine_no[3]) {
    case 0:
        wk->wu.routine_no[3]++;
        wk->wu.rl_flag = wk->wu.rl_waza;
        hoken_muriyari_chakuchi(wk);
        wk->wu.mvxy.index = wk->as->data_ix;
        wk->wu.cmwk[6] = 0;
        wk->wu.cmwk[7] = 0;
        wk->tk_success++;

        if (wk->metamorphose) {
            set_char_move_init(&wk->wu, 5, wk->as->char_ix + 1);
            break;
        }

        if (effect_D7_init(wk)) {
            set_char_move_init(&wk->wu, 5, wk->as->char_ix + 1);
            break;
        }

        set_char_move_init(&wk->wu, 5, wk->as->char_ix);
        break;

    case 1:
        char_move(&wk->wu);

        if (wk->wu.cg_type == 20) {
            setup_mvxy_data(&wk->wu, wk->wu.mvxy.index);
            wk->wu.mvxy.index++;
            wk->wu.routine_no[3]++;
            wk->wu.cg_type = 0;
        }

        break;

    case 2:
        if (wk->wu.cmwk[7] != 0) {
            char_move_cmj4(&wk->wu);
            wk->wu.cmwk[7] = 0;
            wk->wu.mvxy.index++;
            add_sp_arts_gauge_tokushu(wk);
        }

        jumping_union_process(&wk->wu, 3);

        if (wk->wu.cg_type == 20) {
            setup_mvxy_data(&wk->wu, wk->wu.mvxy.index);
            wk->wu.cg_type = 0;
        }

        if (wk->wu.cg_type == 30) {
            wk->wu.cg_type = 0;
            wk->tk_kizetsu += 4;

            if (wk->tk_kizetsu > 12) {
                wk->tk_kizetsu = 12;
            }
        }

        break;

    case 3:
        char_move(&wk->wu);

        if (wk->wu.cg_type == 64) {
            grade_add_personal_action(wk->wu.id);
        }

        break;
    }
}

void Att_PL12_BONUS_STAGE(PLW* wk) {
#if defined(TARGET_PS2)
    void set_char_move_init(WORK * wk, s16 koc, s32 index);
#endif

    wk->scr_pos_set_flag = 0;

    switch (wk->wu.routine_no[3]) {
    case 0:
        wk->wu.routine_no[3]++;
        wk->wu.rl_flag = wk->wu.rl_waza;
        wk->wu.cmwk[6] = 0;
        wk->wu.cmwk[7] = 0;
        set_char_move_init(&wk->wu, 5, wk->wu.char_index);
        break;

    case 1:
        char_move(&wk->wu);

        if (wk->wu.cg_type == 20) {
            wk->wu.routine_no[3]++;
        }

        break;

    case 2:
        if (wk->wu.cmwk[7] != 0) {
            char_move_cmj4(&wk->wu);
            wk->wu.cmwk[7] = 0;
        }

        jumping_union_process(&wk->wu, 3);
        break;

    case 3:
        char_move(&wk->wu);
        break;
    }
}

void (*const pl12_exatt_table[18])(PLW*) = { Att_HADOUKEN,
                                             Att_SHOURYUUREPPA,
                                             Att_SLIDE_and_JUMP,
                                             Att_ABISEGERI,
                                             Att_CHOUCHUURENGEKI,
                                             Att_SHOURYUUKEN,
                                             Att_CHOUCHUURENGEKI,
                                             Att_KUUCHUUNICHIRINSHOU,
                                             Att_SENPUUKYAKU,
                                             Att_HOMING_JUMP,
                                             Att_DUMMY,
                                             Att_DUMMY,
                                             Att_DUMMY,
                                             Att_DUMMY,
                                             Att_PL12_TOKUSHUKOUDOU,
                                             Att_PL12_BONUS_STAGE,
                                             Att_METAMOR_WAIT,
                                             Att_METAMOR_REBIRTH };
