# Third Strike Upper - remy (character index 19)

The entries of the design document for Remy, with what the retail sources hold
and what Third Strike Upper does with them.

The frame data of the moves is not in the repository: the frame chain of a
move, its hitboxes and its pushboxes are read from the disc image at runtime
(`wk->char_table[koc]`, `CHARSET.c:set_new_attnum()`).  The items that change
a number of the chain are therefore `needs-disc`.

| # | Item of the design document | Retail | 3s' | Status |
|:--|:---|:---|:---|:---|
| R1 | "Stun bar increased to "regular" size instead of "short" size" | the size of the stun bar of a character is `PLCNT.c:pl_piyo_tbl` | regular | done - `upper3s/upper3s_sys.c` |
| R2 | "Increase damage of crouching Strong by 6pt" | the damage of a move is the power of the attack data (`UNK_7.pow`), an index into `POW_DATA.c:Power_Data`, so the damage itself cannot be patched in place; the framework scales it instead (`Pow_Pow.c:upper3s_power()`) | the damage of the crouching Strong goes up by six | done - the retail damage of the crouching Strong is 55, so the table of `upper3s/pl18.c` scales it by 1110 permille, which makes it 61.  The remark of the design document that the move should "almost match Ryu's" does not hold for the retail data: the crouching Strong of Ryu does 90, so the gap between the two characters is 35 points and not six |
| R3 | "Fix the earlier hitboxes on close Fierce to knock down vs airborne opponents ... allow juggles afterward" | the knockdown of a hit and the juggle are the hit data of the attack (`UNK_7`) | the earlier hitboxes knock down | needs-disc - the hit data of the close Fierce |
| R4 | "Increase hitstun of far Forward, far Roundhouse, crouching Forward, and Towards+Forward (6MK)" | the hitstun of a move is the hold of the defender, the field `hs_you` of the attack data (`UNK_7.hs_you`): `HITCHECK.c:dm_status_copy()` puts it into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into `hit_stop` and `PLMAIN.c:check_hit_stop()` holds the defender for it.  The field `impact` is the knockback of the hit, not its length | the hold of the defender goes up until the hit is not negative on hit | done - the values are derived by `tools/upper3s/hit_advantage.py` out of the data of the disc and are in `upper3s/pl18.c`: the far Forward goes from -15 on hit to 0, the far Roundhouse from -17 to 0, the crouching Forward from -24 to 0 and the Towards+Forward from -21 to 0 |
| R5 | "Decrease grounded opponent knockback on standing Forward (MK), enlarge hitboxes on 2nd hit of Target Combo" | the knockback is the velocity of the hit (`UNK_7`), the hitbox is the hit data table of the disc | the knockback goes down and the hitbox grows | needs-disc - the velocity of the attack data and the hit data table |
| R6 | "Allow chaining from crouching Jab->crouching Short and crouching Short->crouching Jab" | the chain (renda) is the bit 0x10 of the cancel mask of the move, tested at `PLS00.c:1888`; the target of the chain is the table of the character `wk->cp->sw_now`; `plpat_rno_filter` (PLPAT.c) only lets the standing normals 0 to 15 chain | the two crouching attacks chain into each other | needs-disc - the chain table of the character and the chain filter of the shared file |
| R7 | "Decrease total length of Personal Action (taunt) from 131f to ~80f" | the length of the personal action is the chain of the move | the chain is about fifty blocks shorter | needs-disc - the chain of the personal action |
| R8 | "Increase meter gain from all Light of Virtues on startup/hit/block from 0/1/1 to 1/3/2" | the meter that a move gives is the table of the disc that `PLS02.c:add_sp_arts_gauge_*()` reads | the three values go up | needs-disc - the table of the meter of the disc |
| R9 | "Allow all strengths of Rising Rage Flash to be super-cancelled on hit/block/parry; allow juggles after all versions" | the Rising Rage Flash is a special move, so it already gets the cancel of a super when it hits (`HITCHECK.c:hit_pattern_extdat_check()`, HITCHECK.c:887) and when its chain carries the bit 0x40 (`CHARSET.c:2624`); the window of the move is the `cg_cancel` byte of its chain; the juggle is the hit data of the attack (`UNK_7`) | the byte carries the bit of the super art during the move and the hit knocks the opponent down | needs-disc - the `cg_cancel` byte of the chain of the three versions and the hit data |
| R10 | "Increase hit advantage of Short/Forward Cold Blue Kicks (63214K)" | the advantage is the hold of the defender, the field `hs_you` of the attack data, against the recovery of the attacker | the advantage goes up | done - the move is the Cold Blue Kick of **Chun-Li**, not a move of Remy (the design document lists the item in the section of Remy because it was written while thinking about the matchup against him), so the change is in `upper3s/pl14.c`; see `changelog-chchunli.md` |
| R11 | "Remove superflash on activation of Blue Nocturne (SAIII) pose, only show the superflash after it successfully counters something; still always spends the meter ... increase bar length by a bit; make it properly juggle/knock down vs air" | the superflash is started by the shared code of the super arts (`PLS03.c:execute_super_arts()`); the pose of Blue Nocturne is the extra attack of Remy (`plpat19.c:Att_SA__D_R_A`); the bar length is `PLCNT.c:super_arts_data`; the juggle is the hit data of the attack (`UNK_7`) | the bar is longer (done); the superflash only comes after a successful counter (needs a hook in the shared file); the juggle needs the disc | the bar is done - `upper3s/pl18.c`; the superflash needs a hook in `PLS03.c:execute_super_arts()`; the juggle is needs-disc |

### The hook that R11 needs

The superflash of a super art is started by the shared function
`PLS03.c:execute_super_arts()`, which is the only place that knows whether the
super art is the one of the character and whether it is a counter.  The hook
would be a test of the character index and of the kind of the super art before
the flash is started; the logic belongs in a module of the character, the
shared file is not touched here.

## Verification

* `bash tools/verify/ps2_touched_sources.sh`: 36 files compared, 0 changed.
