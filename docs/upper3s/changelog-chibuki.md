# Third Strike Upper (3s') - Ibuki

Ibuki is the character 7 of the game (AGENTS.md section 3.1). The retail
sources are `PLPAT07.c` and `PASS07.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl06.c`, and the source layer of
the personal action is
`src/anniversary/sf33rd/Source/Game/upper3s/upper3s_ch_ibuki.c`.

Retail base data (`PLCNT.c:super_arts_data[7]`):

| slot | command | gauge length | stocks | move |
|:--|:--|:--|:--|:---|
| SA I | 38 (the alternative of the slot) | 88 | 3 | the first super |
| SA II | 21 | 120 | 1 | the second super |
| SA III | 20 | 80 | 1 | "Yami Shigure" (236+P+P) |

The health of Ibuki is in `Com_Data.c:Com_Vital_Unit_Data[7]`, the stun bar
in `PLCNT.c:pl_piyo_tbl[7]`.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Health increased from 1035 to 1050, same as Twelve's health" | the health table of the retail game does not have a 1035 for Ibuki and no 1050 for Twelve in the same cell, and the order of the document is the opposite of the retail one | done - `upper3s/upper3s_sys.c:upper3s_sys_health_source()`: Ibuki takes the row of the standard cast, scaled by 937 permille (1050 / 1120) |
| "Yami Shigure (SAIII) now has 2 stocks instead of 1" | `PLCNT.c:super_arts_data[7][2].store_max` is 1 | done - `upper3s/pl06.c`: `{ U3S_SA_STORE_MAX, 2, 0, 0 }` on the SA III |
| "Allow Tsuiji Goe (623P, command jump) to be special- and super-cancelled much earlier in the animation, almost directly after takeoff; make Kunai cancelled from it face the correct direction if it hasn't passed over the opponent yet" | the cancel window is the `cg_cancel` bits of the frame chain of the disc, and the direction of the cancelled move is the `rl_flag` of the state machine of the shared code | needs-disc |
| "Fix the unique-animation standing Fierce in her Target Combos (B+Strong->Fierce and Jab->Strong(1)->Fierce) to never knock down" | the two simultaneous hitboxes and the knockdown flag of one of them are entries of the attack table of the disc | needs-disc |
| "Remove the ability to throw-tech her Personal Action (taunt), and allow Ibuki to parry or attack in the air once the opponent recovers" | the personal action opens a throw tech window: `PLPAT07.c:Att_PL07_TOKUSHUKOUDOU` adds 14 to `PLW.tk_dageki` and to `PLW.tk_nage` and clamps both of them to 28 | done - the window is not opened any more (`upper3s/upper3s_ch_ibuki.c`, hooked in `PLPAT07.c` behind `#if defined(UPPER_3S)`) |

### The second half of the personal action item

"allow Ibuki to parry or attack in the air once the opponent recovers" is the
other half of the same item of the document. The personal action of Ibuki is
a jump (`PLPAT07.c:Att_PL07_TOKUSHUKOUDOU` calls `jumping_union_process()`),
and the lock out of the input while the character is in the air is the
shared one: `PLPNM.c` (the normal state machine) and `PLPCA.c` (the reaction
state machine) decide whether a character may attack while it is in the air.
The hook that the item needs is therefore in the shared state machine, not in
the file of Ibuki: a per character flag that releases the air lock of the
personal action of Ibuki. It is reported here and not implemented, because
`PLPNM.c` and `PLPCA.c` are shared files.

### The tests

`tests/upper3s/test_upper3s_ch_ibuki.c` checks that the window of the
personal action is not opened and that a window that is still open when the
personal action starts is closed. The retail reference of the test (a window
of 14 frames, clamped to 28) is kept next to it.
