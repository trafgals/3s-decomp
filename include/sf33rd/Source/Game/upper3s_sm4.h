#ifndef UPPER3S_SM4_H
#define UPPER3S_SM4_H

#include "structs.h"

/*
 * SM4 - charge partitioning determinism.
 *
 * A charge move is done by holding a direction (Back or Down) until the
 * charge is complete and then letting go of it.  The charge itself is not
 * lost when the direction is let go of for a moment - the charge counter
 * keeps its value - but the command does time out while the direction is
 * not held.  The timer of that timeout is the "charge partition timer": it
 * is the time in which the charge can be partitioned, and it only counts
 * down while the charged direction is not held.
 *
 * The timer is reloaded with its maximum only when the charge command
 * itself times out and is read from the command table again.  How much of
 * the timer is left when the player starts charging therefore depends on
 * where the hidden timer happened to be - the window is effectively
 * random.
 *
 * Third Strike Upper makes the window deterministic: as soon as a charged
 * direction has reached one third of its charge, the partition timer is
 * reset to its maximum, so partitioning is always as easy as it can be.
 */

/* The type of the charge command (check_1() of CMD_MAIN.c). */
#define UPPER3S_SM4_CHARGE_TYPE 2

/* The number of command slots a player has. */
#define UPPER3S_SM4_WAZA_COUNT 56

/*
 * The partition timer of a charge that has been held for `charge_frames`
 * frames out of a charge that takes `charge_time` frames.
 */
s16 upper3s_sm4_charge_partition_timer(s16 charge_frames, s16 charge_time, s16 timer, s16 timer_max);

/*
 * Applies the rule to the command work of one player.  `work` is the row
 * of waza_work[] that belongs to the player.
 */
void upper3s_sm4_charge_partition(WAZA_WORK* work, s16 work_count);

#endif
