# Third Strike Upper - oro (character index 9)

The entries of the design document for Oro, with what the retail sources hold
and what Third Strike Upper does with them.

The frame data of the moves is not in the repository: the frame chain of a
move, its hitboxes and its pushboxes are read from the disc image at runtime
(`wk->char_table[koc]`, `CHARSET.c:set_new_attnum()`).  The items that change
a number of the chain are therefore `needs-disc`.

| # | Item of the design document | Retail | 3s' | Status |
|:--|:---|:---|:---|:---|
| O1 | "Examine pushboxes on Jinchu Nobori ("chicken kick")" | the pushbox of a move is the pushbox of the hit data of the disc | the pushbox of the chicken kick is the same for every character | needs-disc - the pushbox of the hit data of Jinchu Nobori |
| O2 | "Increase hitstop and allowed juggle time on Oniyama ([2]8P)" | the hit stop and the time the opponent can be juggled are the hit data of the attack (`UNK_7`) | both go up | needs-disc - the hit data of Oniyama |
| O3 | "Add forward throw (Towards+LP+LK) that uses the animation from his back throw but mirrored" | the throw is started by `PLS03.c:check_catch_attack()`, which takes the move from the assignment table of the disc (`_asstbl_lv_A010`, the table `nml_catch_h2_ok[2][20]` of PLS03.c:968 is the per character table of the throw input); the direction of the throw and the distance it throws the opponent are a branch of the chain of the throw | a forward throw that uses the back throw animation | needs-disc - the chain of the throw of Oro and a new entry in the assignment table of the disc |
| O4 | "Fix neutral throw so that it cannot be escaped before 3-4 hits" | the number of hits that a mashable grab needs is the chain of the throw (the state that counts the hits) | the grab cannot be escaped before the third or the fourth hit | needs-disc - the chain of the throw |
| O5 | "Increase length of the Kishi Riki (SAI) bar to match Tengu Stone's bar" | the length of the bar of a super is `PLCNT.c:super_arts_data` | the bar of Kishi Riki is the bar of Tengu Stone | done - the table of Oro is in `upper3s/pl08.c` |
| O6 | "make Human Pillar Driver (63214P) still be a hit-grab during Kishin Riki" | whether a move is a grab or a hit is the kind of the move (`wk->kind_of_waza`) and the state of the chain; the EX Kishi Riki is `PLPAT09.c:Att_PL09_EX_KISHINRIKI` | the move stays a grab during Kishi Riki | needs-disc - the kind of the move and the chain of Human Pillar Driver |
| O7 | "increase damage of the air regular Kishi Riki grab when combo'd into" | the damage of a move is the power of the attack data (`UNK_7.pow`), a plain value that the framework patches in `att_ix_table` | the damage goes up | needs-disc - the index of the move in the slot 5 and the power of the disc |
| O8 | "allow Oro to deactivate the regular Kishi Riki by inputting QCT+P during the super" | the deactivation of a super is a branch of the frame chain of the super that reads the lever and the buttons | the branch is added to the chain | needs-disc - the chain of the super |

## Verification

* `bash tools/verify/ps2_touched_sources.sh`: 36 files compared, 0 changed.
