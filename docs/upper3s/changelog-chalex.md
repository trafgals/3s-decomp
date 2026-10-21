# Third Strike Upper - alex (character index 1)

The entries of the design document for Alex, with what the retail sources hold
and what Third Strike Upper does with them.

The frame data of the moves is not in the repository: the frame chain of a
move, its hitboxes and its pushboxes are read from the disc image at runtime
(`wk->char_table[koc]`, `CHARSET.c:set_new_attnum()`).  The items that change
a number of the chain are therefore `needs-disc`.

| # | Item of the design document | Retail | 3s' | Status |
|:--|:---|:---|:---|:---|
| A1 | "Change Hyper Bomb (SAI) startup from 0+1 to 1+0" | the startup is the number of blocks of the chain of the super that run before the block that carries the hit data; the chain of the disc is walked by `CHARSET.c:check_cm_extended_code()` | the block that carries the hit data runs one frame earlier | needs-disc - the chain of the disc has to be walked and the block that carries the hit data moved one block up |
| A2 | "Increase forward movement on Boomerang Raid's middle hits, and increase the range on the final grab" | the movement of a hit is the velocity of the attack data (`UNK_7`, the `mvx`/`mvy` of the hit) and the range of a grab is the reach of its hitbox | the middle hits carry the opponent further and the grab reaches further | needs-disc - the velocity of the attack data and the hitbox of the grab |
| A3 | "Make all versions of Air Knee Smash not air-parryable; EX version is still ground parryable" | the parry type of a move is the guard type of its attack data (`UNK_7.guard`), read in `HITCHECK.c` when the defender reacts | the guard type of the move does not allow the air parry | needs-disc - the guard type of the attack data of Air Knee Smash |
| A4 | "Increase the cancel window for his Target Combo (cr.Short->cr.Fwd)" | the window is the number of frames of the chain of the move that carry the cancel byte (`cg_cancel`), and the chain of the target combo is the chain of the crouching Short | the cancel byte is set during more frames of the chain | needs-disc - the frames of the chain of the crouching Short |
| A5 | "Allow juggles after Flash Chops that connect vs airborne opponents" | the knockdown and the juggle of a hit are the hit data of the attack (`UNK_7`) | the hit knocks the opponent down | needs-disc - the hit data of the Flash Chop |
| A6 | "Allow crouching Strong (2MP) to be special- and super-cancelled on hit/block/parry" | the cancel mask of a move is the `cg_cancel` byte of its chain, read every frame by `PLS00.c:check_cg_cancel_data()` (PLS00.c:1821): 0x20 is the cancel into a special move (PLS00.c:1877) and 0x40 the cancel into a super art (PLS00.c:1860) | the crouching Strong gets the two bits | implemented as a rule in `upper3s/upper3s_ch_alex.c`, the shared hook is reported below |
| A7 | "Allow crouching Forward (2MK) to be super-cancelled on hit/block/parry" | the same byte | the crouching Forward gets the bit of the super art | implemented as a rule in `upper3s/upper3s_ch_alex.c`, the shared hook is reported below |
| A8 | "Fix Air Stampede not knocking down if it hits a limb-hurtbox rather than a body hurtbox" | the knockdown of a hit is the hit data of the attack (`UNK_7`), the hurtbox that was hit is the one of the defender that the collision found | every hurtbox of the move knocks down | needs-disc - the hit data of Air Stampede |
| A9 | "Fix position of anchor point and pushbox for air knockdown animation" | the anchor point and the pushbox of a knockdown are in the hit data and the chain of the reaction move | the anchor point and the pushbox are at the right place | needs-disc - the hit data of the reaction and the chain of the knockdown |
| A10 | "Fix position of hitboxes on j.D+Fierce dive" | the hitboxes of a move are the hit data table of the disc (`wk->hit_ix_table`) | the hitboxes are at the right place | needs-disc - the hitbox table of the disc |

## The rule of A6 and A7

`upper3s/upper3s_ch_alex.c` holds the two rules, as a function of the index of
the move and of the cancel mask of the disc:

```c
u8 upper3s_ch_alex_cancel_mask(s16 char_ix, u8 cg_cancel);
```

* the crouching Strong (the move 21 of the slot 4) gets `0x20 | 0x40`,
* the crouching Forward (the move 30 of the slot 4) gets `0x40`.

The index of a normal attack is the `char_ix` of the assignment table of the
disc (`bin2obj/asstbl.c`): `char_ix = base + kos * 3 + koa`, with the base 18
for the crouching normals and the `koa` 0 for the version without a dash.

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

* `tests/upper3s/test_upper3s_ch_alex.c`: 9 checks, 0 failures.
* `tests/upper3s/test_upper3s_cancel.c`: 15 checks, 0 failures - the
  dispatcher sends the move of alex to the rule of alex, leaves the other
  characters alone and leaves the special moves alone.
* With the rule neutralised (`return cg_cancel;` at the top of the function):
  9 checks, 5 failures.
* `bash tools/verify/ps2_touched_sources.sh`: 36 files compared, 0 changed.
* `make PLATFORM=linux UPPER_3S=1 BUILD_DIR=build/chW -j4`: builds with
  `-Wall -Werror`.
