#include "sf33rd/Source/Game/Pow_Pow.h"
#include "common.h"
#include "sf33rd/Source/Game/PLCNT.h"
#include "sf33rd/Source/Game/POW_DATA.h"
#include "sf33rd/Source/Game/workuser.h"
#if defined(UPPER_3S)
#include "sf33rd/Source/Game/upper3s.h"
#endif

#if defined(UPPER_3S)
/*
 * Third Strike Upper - damage overrides.
 *
 * `pow` is an index into Power_Data, so the base damage of a move cannot
 * be changed by patching the table; it is changed here instead.
 */
static s16 upper3s_power(s16 char_id, WORK* as, s16 power) {
    s16 base = upper3s_damage_base(char_id, as);
    s16 permille = upper3s_damage_permille(char_id, as);

    if (base >= 0) {
        power = base;
    }

    if (permille != 1000) {
        power = (power * permille) / 1000;
    }

    return power;
}
#endif

void cal_damage_vitality(PLW* as, PLW* ds) {
    u16 xx = as->wu.att.pow;
    s16 yy;
    s16 power = Power_Data[xx];
#if defined(UPPER_3S)
    power = upper3s_power(as->player_number, &as->wu, power);
#endif

    if (Play_Type == 1) {
        yy = Pow_Control_Data_1[0][3];
    } else {
        yy = Pow_Control_Data_1[0][Round_Level];
    }

    ds->wu.dm_vital = (power * yy) / 100;

    if (as->wu.work_id == 1) {
        ds->wu.dm_vital = (ds->wu.dm_vital * as->att_plus) / 8;
    }

    if (ds->wu.work_id == 1) {
        ds->wu.dm_vital = (ds->wu.dm_vital * ds->def_plus) / 8;
    }
}

void cal_damage_vitality_eff(WORK_Other* as, PLW* ds) {
    u16 xx = as->wu.att.pow;
    s16 yy;
    s16 power = Power_Data[xx];
#if defined(UPPER_3S)
    // A projectile carries the character index of its owner.
    power = upper3s_power(as->master_player, &as->wu, power);
#endif

    if (Play_Type == 1) {
        yy = Pow_Control_Data_1[0][3];
    } else {
        yy = Pow_Control_Data_1[0][Round_Level];
    }

    ds->wu.dm_vital = (power * yy) / 100;

    if (as->wu.work_id == 1) {
        ds->wu.dm_vital = (ds->wu.dm_vital * ((PLW*)as)->att_plus) / 8;
    }

    if (ds->wu.work_id == 1) {
        ds->wu.dm_vital = (ds->wu.dm_vital * ds->def_plus) / 8;
    }
}

void Additinal_Score_DM(WORK_Other* wk, u16 ix) {
    s16 id;

    if (wk->wu.work_id == 1) {
        id = wk->wu.id;
    } else {
        if (((WORK*)wk->my_master)->work_id != 1) {
            return;
        }

        id = wk->master_id;
    }

    Score[id][2] += Score_Data[ix];

    if (Score[id][2] >= 99999900) {
        Score[id][2] = 99999900;
    }

    if ((Mode_Type != 1) && (Mode_Type != 5)) {
        if (!plw[id].wu.operator) {
            return;
        }

        Score[id][Play_Type] += Score_Data[ix];

        if (Score[id][Play_Type] >= 99999900) {
            Score[id][Play_Type] = 99999900;
        }
    } else {
        Score[id][Play_Type] += Score_Data[ix];

        if (Score[id][Play_Type] >= 99999900) {
            Score[id][Play_Type] = 99999900;
        }
    }
}
