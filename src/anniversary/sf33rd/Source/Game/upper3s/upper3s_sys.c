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

    return asag / divisor;
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

s16 upper3s_sys_crossup_permille(WORK* as, WORK* ds) {
    /* The defender has to be in a block animation. */
    if (ds->routine_no[1] == 0) {
        if ((ds->routine_no[2] <= 30) || (ds->routine_no[2] >= 36)) {
            return 1000;
        }
    } else if (ds->routine_no[1] == 1) {
        if ((ds->routine_no[2] <= 3) || (ds->routine_no[2] >= 8)) {
            return 1000;
        }
    } else {
        return 1000;
    }

    /* The defender has to be facing away from the attacker. */
    if (ds->xyz[0].disp.pos > as->xyz[0].disp.pos) {
        /* The attacker is on the left: the defender has to look right. */
        if (ds->rl_flag != 1) {
            return 1000;
        }
    } else {
        /* The attacker is on the right: the defender has to look left. */
        if (ds->rl_flag != 0) {
            return 1000;
        }
    }

    return 500;
}

s16 upper3s_sys_health_source(s16 char_id, s16* scale) {
    *scale = 1000;

    switch (char_id) {
    case U3S_IBUKI:
        /*
         * Third Strike Upper: "Health increased from 1035 to 1050, same as
         * Twelve's health."
         *
         * The numbers of the design document do not exist in the retail
         * table (the table has no 1035 for Ibuki and no 1050 for Twelve in
         * the same cell), and its ordering - Ibuki below Twelve below the
         * standard cast - is the opposite of the retail one (Ibuki has the
         * most health of the whole game with 1657, Twelve has 1530 and the
         * standard cast has 1466).
         *
         * The intent is followed instead of the literal numbers:
         *
         *   - Twelve gets the health of the standard cast.  The standard
         *     cast of the document is "Sean/Remy/Necro/etc."; Necro is
         *     named explicitly and his retail row is the one that Yun,
         *     Makoto and Q share as well, so it is the row the document
         *     means.  Twelve goes from 1530 to 1466.
         *   - Ibuki gets the same row, scaled by the ratio the document
         *     gives between Ibuki and the standard cast (1050 / 1120 =
         *     937 permille), so that the order of the document (Ibuki
         *     below the standard cast) is kept.
         */
        *scale = 937;
        return U3S_NECRO;

    case U3S_TWELVE:
        /* Third Strike Upper: "Health increased from 1050 to 1120, the same
         * as Sean/Remy/Necro/etc." - see the comment of Ibuki above. */
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
