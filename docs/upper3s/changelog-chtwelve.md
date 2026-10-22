# Third Strike Upper - twelve (character index 18)

The entries of the design document for Twelve, with what the retail sources
hold and what Third Strike Upper does with them.

The frame data of the moves is not in the repository: the frame chain of a
move, its hitboxes and its pushboxes are read from the disc image at runtime
(`wk->char_table[koc]`, `CHARSET.c:set_new_attnum()`).  The items that change
a number of the chain are therefore `needs-disc`.

| # | Item of the design document | Retail | 3s' | Status |
|:--|:---|:---|:---|:---|
| T1 | "Health increased from 1050 to 1120" | the health of a character is `Com_Data.c:Com_Vital_Unit_Data` | 1120 | done - `upper3s/upper3s_sys.c` |
| T2 | "Increase hitstun on standing Strong, far Strong, standing Fierce, crouching Fierce, standing Short, standing Forward, standing Roundhouse, crouching Forward, and Back+Forward (4MK)" | the hitstun of a move is the hold of the defender, the field `hs_you` of the attack data (`UNK_7.hs_you`): `HITCHECK.c:dm_status_copy()` puts it into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into `hit_stop` and `PLMAIN.c:check_hit_stop()` holds the defender for it.  The field `impact` is the knockback of the hit, not its length | the hold of the defender goes up until the hit is not negative on hit | done - the values are derived by `tools/upper3s/hit_advantage.py` out of the data of the disc and are in `upper3s/pl17.c`: the standing Strong goes from -1 on hit to 0, the crouching Fierce from -9 to 0, the standing Forward from -2 to 0, the standing Roundhouse from -5 to 0, the crouching Forward from -1 to 0 and the Back+Forward from -6 to 0.  The far Strong (0 on hit), the standing Fierce (+1) and the standing Short (+1) are already at zero or better in the retail game and are not changed |
| T3 | "Make crouching Strong and standing Forward only parryable high ... crouching Fierce and Back+Forward (4MK) only parryable low" | the parry type of a move is the guard type of its attack data (`UNK_7.guard`) | the guard type of the four moves changes | needs-disc - the guard type of the attack data |
| T4 | "Activate the earlier hitboxes in Back+Forward (4MK) that exist but cannot connect" | the hitboxes of a move are the hit data table of the disc (`wk->hit_ix_table`) and the chain decides which of them is live | the earlier hitboxes are live | needs-disc - the chain of the move and the hit data table |
| T5 | "Allow far Strong, standing Strong, and crouching Strong to be special- and super-cancelled on hit/block/parry" | the cancel mask of a move is the `cg_cancel` byte of its chain, read every frame by `PLS00.c:check_cg_cancel_data()` (PLS00.c:1821): 0x20 is the cancel into a special move (PLS00.c:1877) and 0x40 the cancel into a super art (PLS00.c:1860) | the three moves get the two bits | implemented as a rule in `upper3s/upper3s_ch_twelve.c`, the shared hook is reported below |
| T6 | "Allow crouching Strong to be superjump-cancelled on hit/block" | the super jump cancel is the bit 0x01 of the same byte, tested at PLS00.c:1908 | the crouching Strong gets the bit | implemented as a rule in `upper3s/upper3s_ch_twelve.c`, the shared hook is reported below |
| T7 | "Allow crouching Forward to be super-cancelled on hit/block/parry" | the same byte | the crouching Forward gets the bit of the super art | implemented as a rule in `upper3s/upper3s_ch_twelve.c`, the shared hook is reported below |
| T8 | "Increase hit and block advantage of Jab A.X.E. (214LP) so that it is at most -1 or -2 on block" | the advantage is `UNK_7.hs_me` and `UNK_7.hs_you` of the attack data, plain values that the framework patches in `att_ix_table` | the advantage goes up | needs-disc - the index of the move in the slot 5 and the values of the disc |
| T9 | "Allow all air normals to be special-cancelled on hit/block into A.X.E. or D.R.A. and super-cancelled into X.F.L.A.T" | the same byte; the air attacks are the moves 36 to 71 of the slot 4 (`bin2obj/asstbl.c`, the tables 2010, 3010 and 4010) | the air attacks get the two bits | implemented as a rule in `upper3s/upper3s_ch_twelve.c`, the shared hook is reported below |
| T10 | "Change opponent trajectory when juggled with X.N.D.L." | the movement of the opponent after a hit is the velocity of the hit (`UNK_7`) | the opponent is dragged down | needs-disc - the velocity of the attack data |
| T11 | "Remove low-damage air-to-air version of X.F.L.A.T., have it always do full damage even vs air; increase number of stocks to 2" | the stocks are `PLCNT.c:super_arts_data`, the damage is the power of the attack data (`POW_DATA.c:Power_Data` and `UNK_7.pow`) | the stocks are 2 and the damage is the same in the air | the stocks are done - `upper3s/pl17.c`; the damage is needs-disc (the power of the attack data of the disc) |
| T12 | "During X.C.O.P.Y., allow Twelve to do EX moves ... reduce X.C.O.P.Y. startup from 12f to 1f ... when hit during X.C.O.P.Y.'s recovery, Twelve now takes 66% damage" | the super of Twelve is `plpat18.c:Att_PL18_NINGENBAKUDAN` (the extra attack 7, the move 23 of the slot 5); its startup is the chain of the disc, the damage that Twelve takes is the damage scaling of the hit (`HITCHECK.c`) and the gauge that the super burns is `wk->sa->dtm * wk->sa->dtm_mul` (`PLS03.c`) | the startup is one frame, the damage taken is 66%, the EX moves are possible during the super | needs-disc - the chain of the super (the startup) and the damage scaling of the defender; the EX move during the super needs a hook in the shared command code |

## The rules of T5, T6, T7 and T9

`upper3s/upper3s_ch_twelve.c` holds the rules, as a function of the index of
the move and of the cancel mask of the disc:

```c
u8 upper3s_ch_twelve_cancel_mask(s16 char_ix, u8 cg_cancel);
```

* the standing Strong (the move 3 of the slot 4) and the far Strong (4 and 5)
  get `0x20 | 0x40`,
* the crouching Strong (21) gets `0x20 | 0x40 | 0x01`,
* the crouching Forward (30) gets `0x40`,
* every air attack (36 to 71) gets `0x20 | 0x40`.

The index of a normal attack is the `char_ix` of the assignment table of the
disc (`bin2obj/asstbl.c`): `char_ix = base + kos * 3 + koa`, with the base 0
for the standing normals, 18 for the crouching ones and 36 for the air ones.

The `char_ix` is the only thing that identifies the move. The routine number
of the state machine (`routine_no[2]`) cannot be used for it: the assignment
tables give it as `r_no`, which is the *behaviour class* of the normal - 0 for
most of the normals and 4 for the medium punches, so the standing and the
crouching Medium Punch share it.

### The far Strong

The "far Strong" of the design document is the only move of these three
characters whose `char_ix` is not obvious, because the selection tables of
the disc decide which variant of a button the game uses and the choice is
made **per move**.

For Twelve's standing Medium Punch the selection is

```c
u16 _asstbl_lv_0000[20][6][2] = {
    ...
    { { 0, 0x36 }, { 0x4008, 0 }, { 0x4008, 0x30 }, { 0, 0x30 }, { 0x3C, 0 }, { 0x8008, 0x44 } },
```

(the row of Twelve is the nineteenth one). The Medium Punch is the second
entry of the row: `{ 0x4008, 0 }`. The first condition is the lever that
selects the variant of the dash and the second one the condition of the
range, and the second one is zero, so the game only ever uses the neutral
punch (the `char_ix` 3) and the variant that the forward lever selects (the
`char_ix` 5). The `char_ix` 4 is in the tables, but the game never reaches
it.

Both of the extended variants are given the cancels, so that the rule does
not depend on which of them the design document calls the "far Strong".

### The hook that is needed

The rules are applied in the shared file `PLS00.c`, in `check_cg_cancel_data()`,
right at the top of the function, before the test that gives up when the mask
of the disc is empty (PLS00.c:1822). The dispatcher
(`upper3s/upper3s_cancel.c`) sends the move to the rule of its character, so
that the shared file needs a single hook:

```c
s32 check_cg_cancel_data(PLW* wk) {
#if defined(UPPER_3S)
    // Third Strike Upper: the cancels of the design document.
    wk->wu.cg_cancel = upper3s_cancel_mask(wk->player_number, wk->wu.now_koc, wk->as->char_ix, wk->wu.cg_cancel);
#endif
    if (wk->wu.cg_cancel == 0) {
        return 0;
    }
```

The hook has to be placed before that test, because a move whose mask of the
disc is empty never reaches the tests of the cancels.

The kind of character data has to be passed as well, because the special moves
and the super arts are in the slot 5 and their numbering overlaps the one of
the normals: the rules of the design document are about the normal attacks,
which are the slot 4.

The hook gives the cancel for the whole move; the retail gives it for the
frames of the chain of the move that carry the byte, so the exact timing of
the design document ("on hit/block/parry") is a change of the disc data.

## Verification

* `tests/upper3s/test_upper3s_ch_twelve.c`: 14 checks, 0 failures.
* With the rule neutralised (`return cg_cancel;` at the top of the function):
  14 checks, 10 failures.
* `bash tools/verify/ps2_touched_sources.sh`: 36 files compared, 0 changed.
* `make PLATFORM=linux UPPER_3S=1 BUILD_DIR=build/chW -j4`: builds with
  `-Wall -Werror`.
