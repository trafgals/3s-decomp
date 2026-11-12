# Third Strike Upper - elena (character index 8)

The entries of the design document for Elena, with what the retail sources
hold and what Third Strike Upper does with them.

The frame data of the moves is not in the repository: the frame chain of a
move, its hitboxes and its pushboxes are read from the disc image at runtime
(`wk->char_table[koc]`, `CHARSET.c:set_new_attnum()`).  The items that change
a number of the chain are therefore `needs-disc`.

| # | Item of the design document | Retail | 3s' | Status |
|:--|:---|:---|:---|:---|
| E1 | "Allow her to chain crouching Short (2LK) into itself" | the chain (renda) is the bit 0x10 of the cancel mask of the move, tested at `PLS00.c:1888`; the target of the chain is the table of the character `wk->cp->sw_now`, and `check_renda_cancel()` (PLS00.c) compares it with the move that is running | the crouching Short chains into itself | verified - `upper3s/upper3s_ch_elena.c`: the chain bit (0x10, rapid fire) on the c.LK's cancel-window frames (those with its special and super bits), not on its startup; a second c.LK pressed there hits before the first ends (`tests/match/test_cancels.py`) |
| E2 | "Make startup frames of all versions of Mallet Smash unthrowable" | the state that can be thrown is the state of the chain of the move (the frames before the move leaves the ground) | the startup of Mallet Smash cannot be thrown | needs-disc - the chain of Mallet Smash |
| E3 | "Allow cancels from the 2nd hit of standing Strong, not just the first hit" | the cancel window is the `cg_cancel` byte of the chain of the move, which is set during the frames of the hit that may be cancelled | the byte is set during the second hit as well | verified - `upper3s/upper3s_ch_elena.c`: the special and super bits on the active frames and the hitstops of s.MP; the second hit lands on the last active frame and its hitstop plays on a frame that had no bits (`tests/match/test_cancels.py`) |
| E4 | "Fix Brave Dance so it juggles properly vs airborne opponents outside the corner" | the knockdown and the juggle of a hit are the hit data of the attack (`UNK_7`) | the hit knocks the opponent down | needs-disc - the hit data of Brave Dance |
| E5 | "Speed up the cancel animation on Healing (SAIII), increase the rate of health recovery" | the recovery of the healing super is in `PLPAT08.c` (the switch on `wk->wu.cg_type` gives the health that is restored every frame); the animation of the cancel is the chain of the super | the recovery rate is doubled (3, 2 and 1 become 6, 4 and 2) | the rate is done - `PLPAT08.c` (the `#if defined(UPPER_3S)` block); the speed of the cancel animation is needs-disc - the chain of the super |
| E6 | "She dances okay! Leaving her with 3 stocks for Spinning Beat (SAIII)" | the stocks are `PLCNT.c:super_arts_data` | 3 stocks | done - the table of Elena is in `upper3s/pl07.c` |

## Verification

* `bash tools/verify/ps2_touched_sources.sh`: 36 files compared, 0 changed
  (the change of `PLPAT08.c` is behind `#if defined(UPPER_3S)`, so the vanilla
  PS2 build is untouched).
