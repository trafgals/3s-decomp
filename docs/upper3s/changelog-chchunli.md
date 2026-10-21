# Third Strike Upper (3s') - Chun-Li

Chun-Li is the character 15 of the game (AGENTS.md section 3.1). She has no
per character file of her own in the repository: her attacks are the shared
ones of `PLPATUNI.c`, and the retail sources of the character are
`pass15.c` (the command dispatch) and the shared state machines. The data
layer of the patch is `src/anniversary/sf33rd/Source/Game/upper3s/pl14.c`.

Retail base data (`PLCNT.c:super_arts_data[15]`):

| slot | command | gauge length | stocks | move |
|:--|:--|:--|:--|:---|
| SA I | 20 | 88 | 1 | the 236+P super |
| SA II | 21 | 104 | 2 | "Houyoku-sen" (623+P) |
| SA III | 22 | 72 | 3 | "Tensei Ranka" (236+P+P, multi hit) |

The four strengths of a super are four consecutive entries of the group 0 of
the assignment table of Chun-Li (`bin2obj/asstbl.c:_assadr_lv_9900[15][0]`):
the SA I is 44 to 47, the SA II is 48 to 51 and the SA III is 52 to 55.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Houyoku-sen (SAII) now has a maximum of 1 stock instead of 2; the bar is now the maximum length possible, the same as Shin-Shoryuken" | the retail SA II costs two stocks and its bar is 104; the longest bar of the retail table is 128, and the SA II of Ryu (the Shin Shoryuken) is 128 | done - `upper3s/pl14.c`: the stock count is 1 and the bar is 128 |
| "Cancel window on cr.Fwd (c.MK) fixed to not allow super-late cancels" | the cancel window of a move is the `cg_cancel` bits of the frame chain of the disc, read by `PLS00.c:check_cg_cancel_data()` | needs-disc |
| "Crouching Jab (2LP) startup increased from 2f to 3f" | the startup is the number of the game frames before the first frame of the chain that carries the attack data; the frame that carries the hit data runs one game frame later | done - `upper3s/pl14.c`, the entry 18 of the slot of the normal attacks starts in 2 frames in the data of the disc and is brought to 3 frames |
| "Tensei Ranka (SAIII) startup hitbox reaches down to the floor so it can't be crouched; changed to pull in and downward so all hits connect vs airborne opponents more often; advantage on knockdown increased enough to allow crossup attempts afterward; maximum of 2 stocks available; bar is a bit longer, the same length as Shippu" | the hitboxes, the movement and the hit advantage are the frame chain and the attack table of the disc; the stock count and the bar are in the retail table | done for the stock count (2 instead of 3) and the bar (80, the length of the Shippu of Ken) - `upper3s/pl14.c`; needs-disc for the hitbox and the cross up advantage |
| "Middle hits of Houyoku-sen fixed to not un-combo occasionally vs crouching Makoto" | the hitboxes and the movement of the frame chain of the disc | needs-disc |
| "Increase hit advantage of Short/Forward Cold Blue Kicks (63214K) so they are not negative on hit from up close; make Forward version +2 so that supers can be linked off it at point-blank" (the design document lists this item in the section of Remy, but the Cold Blue Kick is the Houyoku-sen of Chun-Li) | the hitstun of a move is the hold of the defender, the field `hs_you` of the attack data (`UNK_7.hs_you`): `HITCHECK.c:dm_status_copy()` puts it into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into `hit_stop` and `PLMAIN.c:check_hit_stop()` holds the defender for it.  The Cold Blue Kick is the special 29 of the retail data, in its four versions: the light one (the move 28 of the slot 5), the medium one (29), the heavy one (30) and the EX one (31, the 63214K of the document) | the hold of the defender goes up until the hit is not negative on hit | done - the values are derived by `tools/upper3s/hit_advantage.py` out of the data of the disc and are in `upper3s/pl14.c`: the light version goes from -10 on hit to +2 and the medium version from -10 to +2.  The heavy version (+10 on hit) and the EX version (+4) are already past the end state of the document and are not changed |
| "Chun-Li now goes through the mandatory one-standing-frame after wakeup, like every other character" | the wake up is the shared reaction code (`PLPCA.c`, `PLPNM.c`); the number of frames the character stands up in is the frame chain of the disc | needs-disc - a hook in the shared wake up would be needed |
| "Increase the height of her crouching head hurtbox (vulnerable box) by 2-3 pixels upward and 3-4 pixels backward in all applicable animations" | the hurtboxes are the entries of the attack table of the disc | needs-disc |

### The observation about the extra attack table of Chun-Li

`PLPAT.c:plxx_extra_attack_table` has no entry for the character index 15:
the list is `pl00_extra_attack` to `pl14_extra_attack` and then `pl16_extra_attack`
to `pl20_extra_attack`, so Chun-Li (15) is dispatched to the table of Makoto
and the four characters behind her are shifted by one. The file of the
character that the retail game uses for Chun-Li is not in the repository
(`PLPAT15.c` does not exist), and the preprocessed source of `PLPAT.c` is
part of the retail build, so this has not been touched here. It is reported
because it is the reason why the state machine of the special moves of
Chun-Li is the one of Makoto in this decompilation, and it may be a fault of
the decompilation rather than of the retail game.
