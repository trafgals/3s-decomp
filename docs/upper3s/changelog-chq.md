# Third Strike Upper - q (character index 17)

The entries of the design document for Q, with what the retail sources hold
and what Third Strike Upper does with them.

The frame data of the moves is not in the repository: the frame chain of a
move, its hitboxes and its pushboxes are read from the disc image at runtime
(`wk->char_table[koc]`, `CHARSET.c:set_new_attnum()`).  The items that change
a number of the chain are therefore `needs-disc`.

| # | Item of the design document | Retail | 3s' | Status |
|:--|:---|:---|:---|:---|
| Q1 | "Allow crouching Jab, standing Short, and crouching Short to be special- and super-cancelled on hit/block/parry" | the cancel mask of a move is the `cg_cancel` byte of its chain, read every frame by `PLS00.c:check_cg_cancel_data()` (PLS00.c:1821): 0x20 is the cancel into a special move (PLS00.c:1877) and 0x40 the cancel into a super art (PLS00.c:1860) | the three moves get the two bits | implemented as a rule in `upper3s/upper3s_ch_q.c`, the shared hook is reported below |
| Q2 | "Allow crouching Forward to be super-cancelled on hit/block/parry" | the same byte | the crouching Forward gets the bit of the super art | implemented as a rule in `upper3s/upper3s_ch_q.c`, the shared hook is reported below |
| Q3 | "Allow all overhead/low rushpunches ([4]6P/K/[P]) to be super-cancelled on hit/block/parry" | the rush punches are special moves (slot 5), so they get the cancel of a special move when they hit (`HITCHECK.c:hit_pattern_extdat_check()`, HITCHECK.c:891) and the byte of their chain decides the rest | the byte of the chain carries the bit of the super art during the active frames | needs-disc - the `cg_cancel` byte of the chains of the rush punches |
| Q4 | "Decrease the startup of far standing Jab from 6f to 4f" | the startup is the number of the game frames before the first frame of the chain that carries the attack data | the frame that carries the hit data runs two game frames earlier | done - `upper3s/pl16.c`, the entries 1 and 2 of the slot of the normal attacks - the dash forward Jab (6LP) and the dash back Jab (4LP), which share their chain - start in 6 frames in the data of the disc and are brought to 4 frames.  The Jab of the document is the one that starts in 6 frames, and that is the version with a dash: the plain standing Jab (5LP, the entry 0) is a chain of its own and already starts in 4 frames, so it is not changed.  The table holds an entry for each of the two moves; the second one finds the chain that already holds the startup of the target and leaves it alone |
| Q5 | "Extend the cancel window on B+RH (4HK) past the first active frame" | the window is the number of frames of the chain that carry the cancel byte (`cg_cancel`) | the byte is set during more frames | needs-disc - the frames of the chain of B+RH |
| Q6 | "Increase hitstun on crouching Short, crouching Forward, and standing Roundhouse so they are not negative on hit" | the hitstun of a move is the hold of the defender, the field `hs_you` of the attack data (`UNK_7.hs_you`): `HITCHECK.c:dm_status_copy()` puts it into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into `hit_stop` and `PLMAIN.c:check_hit_stop()` holds the defender for it.  The field `impact` is the knockback of the hit, not its length | the hold of the defender goes up until the hit is not negative on hit | done - the values are derived by `tools/upper3s/hit_advantage.py` out of the data of the disc and are in `upper3s/pl16.c`: the standing Roundhouse goes from -10 on hit to 0 and the crouching Short from -3 to 0.  The crouching Forward is already at +1 on hit and is not changed |
| Q7 | "Change opponent's movement when juggled with the middle hits of all High Speed Barrage versions" | the movement of the opponent after a hit is the velocity of the hit (`UNK_7`) | the opponent is dragged down | needs-disc - the velocity of the attack data |
| Q8 | "Extend hitbox of first hit of EX High Speed Barrage downward" | the hitbox of a move is the hit data table of the disc (`wk->hit_ix_table`) | the hitbox reaches lower | needs-disc - the hitbox table of the disc |
| Q9 | "Add EX (PP) version of Total Destruction (SAIII) which immediately does the grab part upon activation" | a new move needs a new command (the command tables of the character are in the shared file `cmd_data.c`, the chain of the super is `p12_cmd_22`), a new entry in the state machine of the character (`pass17.c`) and a new entry in the move list of the disc | the EX version grabs at once | needs-disc - the new chain of the command and the new entry of the move list; the state machine of the character is in `pass17.c` and can take the new pattern |
| Q10 | "Do not allow the punch version of Total Destruction to trade hits" | the trade of two attacks is decided by the level of the attack (`UNK_7.level`) and by `HITCHECK.c:hit_check_main_process()` | the punch version does not trade | needs-disc - the level of the attack data of the super |
| Q11 | "Allow Q to deactivate Total Destruction by entering QCT+P while the super is active" | the deactivation of a super is a branch of the frame chain of the super that reads the lever and the buttons (`comm_if_l` in the chain) | the branch is added to the chain | needs-disc - the chain of the super |
| Q12 | "Decrease health bonus for having 1-3 Personal Actions from 1200/1400/1705/2050 to 1200/1400/1600/1800" | the bonus of the personal actions is a table of the disc that the framework reads (`Com_Data.c:Com_Vital_Unit_Data` and the personal action table of the character) | the third and the fourth bonus go down | done - the table of Q is in `upper3s/pl16.c` |

## The rule of Q1 and Q2

`upper3s/upper3s_ch_q.c` holds the two rules, as a function of the index of
the move and of the cancel mask of the disc:

```c
u8 upper3s_ch_q_cancel_mask(s16 char_ix, u8 cg_cancel);
```

* the standing Short (the move 9 of the slot 4), the crouching Jab (18) and
  the crouching Short (27) get `0x20 | 0x40`,
* the crouching Forward (30) gets `0x40`.

The index of a normal attack is the `char_ix` of the assignment table of the
disc (`bin2obj/asstbl.c`): `char_ix = base + kos * 3 + koa`, with the base 0
for the standing normals and 18 for the crouching ones, and the `koa` 0 for
the version without a dash.

The `char_ix` is the only thing that identifies the move. The routine number
of the state machine (`routine_no[2]`) cannot be used for it: the assignment
tables give it as `r_no`, which is the *behaviour class* of the normal - 0 for
most of the normals and 4 for the medium punches, so the standing and the
crouching Medium Punch share it.

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

* `tests/upper3s/test_upper3s_ch_q.c`: 12 checks, 0 failures.
* With the rule neutralised (`return cg_cancel;` at the top of the function):
  12 checks, 8 failures.
* `bash tools/verify/ps2_touched_sources.sh`: 36 files compared, 0 changed.
* `make PLATFORM=linux UPPER_3S=1 BUILD_DIR=build/chW -j4`: builds with
  `-Wall -Werror`.
