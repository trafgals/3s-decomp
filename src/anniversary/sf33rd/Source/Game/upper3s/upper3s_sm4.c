/*
 * SM4 - charge partitioning determinism.
 *
 * The charge command of the retail game is check_1() of CMD_MAIN.c.  It
 * counts the frames the charged direction is held in the work of the
 * command, and it counts down the timer of the command while the charged
 * direction is *not* held.  That timer is the charge partition timer: it
 * is the time in which the charge can be partitioned, and it is only
 * reloaded with its maximum when the command times out and is read from
 * the command table again.  The window a player gets is therefore decided
 * by where that hidden timer happened to be when the charge was started.
 *
 * Third Strike Upper resets the partition timer to its maximum as soon as
 * a charged direction has reached one third of its charge, so the charge
 * can always be partitioned in the full window.
 */

#include "sf33rd/Source/Game/upper3s_sm4.h"

/*
 * A command of the command table is five words:
 *
 *   [0] the type of the command (2 is the charge command)
 *   [1] the timer of the command, this is the charge partition timer
 *   [2] the number of frames the direction has to be held for the charge
 *       to be complete
 *   [3] the direction that has to be held
 *   [4] the type of the next command of the chain
 *
 * check_init() and check_next() of CMD_MAIN.c read such a record into the
 * work of the command and leave w_ptr pointing at the first word of the
 * *next* record, so the record of the command that is running is the four
 * words in front of w_ptr.  The table itself is never written to, so the
 * values in front of w_ptr are the values the command was started with.
 */
#define SM4_REC_TYPE 0
#define SM4_REC_TIMER 1
#define SM4_REC_CHARGE 2
#define SM4_REC_LEVER 3
#define SM4_REC_NEXT 4

/* The lever of the command has to match exactly when this bit is set. */
#define SM4_LEVER_EXACT 0x8000

/* The directions a charge can be done in: Down, and the two sides. */
#define SM4_LEVER_DOWN 0x2
#define SM4_LEVER_SIDE_A 0x4
#define SM4_LEVER_SIDE_B 0x8

s16 upper3s_sm4_charge_partition_timer(s16 charge_frames, s16 charge_time, s16 timer, s16 timer_max) {
    if (charge_time <= 0) {
        return timer;
    }

    // One third of the charge is enough to make the partition window as
    // long as it can be.
    if (charge_frames >= charge_time / 3) {
        return timer_max;
    }

    return timer;
}

/*
 * The partition timer of a charge has been reset in this charge already.
 * The reset happens once, when the charge reaches one third: after it the
 * timer counts down again while the direction is released, as in the retail
 * game. (Resetting it on every frame, as PR #1 did, kept a charge alive for
 * ever once a third of it was done; measured: Remy's charge survived a
 * release of 116 frames, against 0 to 42 in the retail game.)
 */
static u8 sm4_reset_done[2][UPPER3S_SM4_WAZA_COUNT];

void upper3s_sm4_charge_partition(s16 player, WAZA_WORK* work, s16 work_count) {
    s16 i;

    if ((work == NULL) || (player < 0) || (player >= 2)) {
        return;
    }

    if (work_count > UPPER3S_SM4_WAZA_COUNT) {
        work_count = UPPER3S_SM4_WAZA_COUNT;
    }

    for (i = 0; i < work_count; i++) {
        WAZA_WORK* w = &work[i];
        const s16* record;
        s16 lever;
        s16 timer_max;
        s16 charge_time;
        s16 counter;
        s16 charge_frames;

        if (w->w_type != UPPER3S_SM4_CHARGE_TYPE || w->w_ptr == NULL) {
            sm4_reset_done[player][i] = 0;
            continue;
        }

        lever = w->w_lvr & 0xF;

        // The charged directions are Back and Down.  Which bit of the lever
        // holds the back direction depends on the side the player faces, so
        // both horizontal directions are accepted here; the command tables
        // of the retail game only use Down (0x2) and Back (0x8).
        if (lever != SM4_LEVER_DOWN && lever != SM4_LEVER_SIDE_A && lever != SM4_LEVER_SIDE_B) {
            continue;
        }

        record = ((const s16*)w->w_ptr) - SM4_REC_NEXT;
        timer_max = record[SM4_REC_TIMER];
        charge_time = record[SM4_REC_CHARGE];

        if (charge_time <= 0) {
            continue;
        }

        // check_1() counts the frames the direction is held in free2 when
        // the lever has to match exactly, and in free1 otherwise.
        counter = (w->w_lvr & SM4_LEVER_EXACT) ? w->free2 : w->free1;

        charge_frames = charge_time - counter;

        if (charge_frames < 0) {
            charge_frames = 0;
        }

        if (charge_frames < charge_time / 3) {
            // A new charge (the step restarted and reloaded free1), or one
            // that has not reached a third yet.
            sm4_reset_done[player][i] = 0;
            continue;
        }

        if (!sm4_reset_done[player][i]) {
            w->w_int = upper3s_sm4_charge_partition_timer(charge_frames, charge_time, w->w_int, timer_max);
            sm4_reset_done[player][i] = 1;
        }
    }
}
