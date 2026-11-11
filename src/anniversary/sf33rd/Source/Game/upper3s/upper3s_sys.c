#include "common.h"
#include "sf33rd/Source/Game/upper3s.h"
#include "sf33rd/Source/Game/upper3s_sys.h"

/*
 * Third Strike Upper (3s') - shared system layer.
 *
 * The state of the system wide rules is kept here instead of in PLW or
 * WORK: both structures are part of the PS2 memory map and may not grow.
 */

/*
 * SM1 - whiffed normal metergain.
 *
 * `upper3s_whiff_count[player]` counts the normals that whiffed in a row,
 * `upper3s_whiff_pending[player]` tells whether a normal is running that
 * has not touched the opponent yet.
 */
static s16 upper3s_whiff_count[U3S_SYS_PLAYERS];
static u8 upper3s_whiff_pending[U3S_SYS_PLAYERS];

/* The scaling of the metergain is halved once per whiff. */
#define U3S_WHIFF_MAX 12

/* The player slot of the character that owns `wk`. */
static s16 upper3s_sys_player_of(WORK* wk) {
    if (wk->work_id == 1) {
        return wk->id;
    }

    /* A projectile or an effect: its owner is the character that made it. */
    return ((PLW*)((WORK_Other*)wk)->my_master)->wu.id;
}

/* Forgets the whiffed normals of a character. */
static void upper3s_sys_whiff_reset(s16 player) {
    if ((player < 0) || (player >= U3S_SYS_PLAYERS)) {
        return;
    }

    upper3s_whiff_count[player] = 0;
    upper3s_whiff_pending[player] = 0;
}

s16 upper3s_sys_whiff_start(WORK* wk, s16 asag) {
    s16 player;
    s16 divisor;

    player = wk->id;

    if ((player < 0) || (player >= U3S_SYS_PLAYERS)) {
        return asag;
    }

    if (upper3s_whiff_pending[player]) {
        /* The previous normal did not touch the opponent: it whiffed. */
        if (upper3s_whiff_count[player] < U3S_WHIFF_MAX) {
            upper3s_whiff_count[player]++;
        }
    }

    upper3s_whiff_pending[player] = 1;

    divisor = 1 << upper3s_whiff_count[player];

    /*
     * Rounded to the nearest unit, halves up. The gauge only counts whole units
     * (PLS02.c:add_super_arts_gauge() adds asag to gauge.s.h), and a whiffed normal is
     * worth about 2 of them, so truncating made every whiff after the second worth
     * nothing (2, 1, 0, 0 ...). Rounded, the gains of a streak add up to what the
     * document's 100% + 50% + 25% + 12.5% ... adds up to (2, 1, 1, 0 ...: 4 units, twice
     * one whiff). Measured by tests/match/test_system.py.
     */
    return (s16)((asag * 2 + divisor) / (divisor * 2));
}

void upper3s_sys_whiff_contact(WORK* as, WORK* ds, s16 type) {
    s16 attacker;
    s16 defender;

    attacker = upper3s_sys_player_of(as);
    defender = upper3s_sys_player_of(ds);

    if (as->work_id == 1) {
        /* The move of the character itself touched the opponent and the
         * character was hit, blocked or parried as well. */
        upper3s_sys_whiff_reset(attacker);
        upper3s_sys_whiff_reset(defender);
        return;
    }

    /* A projectile. */
    if (type != U3S_SYS_CONTACT_BLOCK) {
        /* A projectile that hits or that is parried resets the scaling of
         * the character that threw it. */
        upper3s_sys_whiff_reset(attacker);
    }

    if (type == U3S_SYS_CONTACT_HIT) {
        /* A projectile that hits resets the scaling of the character that
         * was hit. A blocked or a parried projectile does not. */
        upper3s_sys_whiff_reset(defender);
    }
}

/*
 * SM5 - the defender is in a combo that started with a hit from the side it
 * was not facing while it was blocking.
 */
static u8 upper3s_crossup_combo[U3S_SYS_PLAYERS];

/*
 * SM5 - the defender is blocking: in the block posture of the normal state
 * group (states 27-30, PLPNM.c:Normal_27000(), taken when an attack comes
 * near while back is held) or in a block reaction of the damage group
 * (reactions 4-11, PLPDM.c:Damage_04000()/Damage_07000()). Both set
 * `guard_chuu` from `guard_kind[]`. Normal states 31-35 are parries, not
 * blocks (set_paring_status()). Confirmed from the source.
 */
static s16 upper3s_sys_blocking(WORK* ds) {
    if (ds->routine_no[1] == 0) {
        return (ds->routine_no[2] >= 27) && (ds->routine_no[2] <= 30);
    }

    if (ds->routine_no[1] == 1) {
        return (ds->routine_no[2] >= 4) && (ds->routine_no[2] <= 11);
    }

    return 0;
}

s16 upper3s_sys_crossup_permille(WORK* as, WORK* ds) {
    s16 player = upper3s_sys_player_of(ds);
    s16 away;

    if ((player < 0) || (player >= U3S_SYS_PLAYERS)) {
        return 1000;
    }

    /* The defender faces away from the attacker. */
    if (ds->xyz[0].disp.pos > as->xyz[0].disp.pos) {
        /* The attacker is on the left: facing right is facing away. */
        away = ds->rl_flag == 1;
    } else {
        away = ds->rl_flag == 0;
    }

    if (upper3s_sys_blocking(ds)) {
        /* "any time a character is hit during block animation while they
         * are facing the opposite direction from the hit, the damage scaling
         * is reduced to 50% for that hit and the followup combo" */
        upper3s_crossup_combo[player] = away;
        return away ? 500 : 1000;
    }

    if (upper3s_crossup_combo[player] && (ds->routine_no[1] == 1)) {
        /* The followup combo: the defender has been in hitstun since. */
        return 500;
    }

    /* A new hit on a defender that was free (or a new combo). */
    upper3s_crossup_combo[player] = 0;
    return 1000;
}

s16 upper3s_sys_health_source(s16 char_id, s16* scale) {
    *scale = 1000;

    switch (char_id) {
    case U3S_IBUKI:
        /*
         * Third Strike Upper: "Health increased from 1035 to 1050, same as
         * Twelve's health."
         *
         * The document's numbers are from another version; the relation it
         * states is followed. In a fight (a human player reads column 2 of
         * Com_Vital_Unit_Data, see PLS02.c:setup_vitality()) the retail
         * health is Ibuki 950, Twelve 997, Necro and Remy 1016: the same
         * order as the document's. So Ibuki gets Twelve's retail row (950 ->
         * 997). An earlier version read another column of the table, where
         * Ibuki looks like the healthiest character, and scaled Necro's row
         * instead (950 -> 951). Measured by tests/match/test_data.py.
         */
        return U3S_TWELVE;

    case U3S_TWELVE:
        /* Third Strike Upper: "Health increased from 1050 to 1120, the same
         * as Sean/Remy/Necro/etc." Necro's (and Remy's) retail row: 997 ->
         * 1016 in a fight. (Sean's is 1064 in this version, so the three are
         * not equal here; Necro and Remy are two of the three.) */
        return U3S_NECRO;

    default:
        return char_id;
    }
}

s16 upper3s_sys_stun_genkai(s16 char_id, s16 retail) {
    switch (char_id) {
    case U3S_AKUMA:
    case U3S_REMY:
        /*
         * Third Strike Upper: the stun bar of Akuma and of Remy is raised
         * from the "short" size to the "regular" one.
         *
         * The retail table (PLCNT.c:pl_piyo_tbl) has three sizes: 56 for
         * Akuma and Remy - the "short" one - 64 for thirteen characters and
         * 72 for seven of them.  The regular size is the one that the
         * majority of the cast uses, so it is 64.
         */
        return 64;

    default:
        return retail;
    }
}
