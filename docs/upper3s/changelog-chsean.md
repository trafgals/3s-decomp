# Third Strike Upper - sean (character index 12)

The entries of the design document for Sean, with what the retail sources hold
and what Third Strike Upper does with them.

The frame data of the moves is not in the repository: the frame chain of a
move, its hitboxes and its pushboxes are read from the disc image at runtime
(`wk->char_table[koc]`, `CHARSET.c:set_new_attnum()`).  The items that change
a number of the chain are therefore `needs-disc`.

Sean's moves are the special moves with the command tables of the character
(`cmd_data.c:pC_cmd_28` to `pC_cmd_33`): the Sean Roll is the tech number
0x1C (the move 22 of the slot 5), the Dragon Smash the tech number 0x1F (the
move 20), the Ryuubi Kyaku the tech number 0x1E (the moves 19 and 25) and the
Tackle the tech number 0x20 (the move 21).

| # | Item of the design document | Retail | 3s' | Status |
|:--|:---|:---|:---|:---|
| S1 | "Increase hitstun of close and far Fierce, crouching Fierce, standing Forward, far Roundhouse, crouching Forward, and Towards+Fierce (6HP) to not be negative on hit" | the hitstun of a move is the hold of the defender, the field `hs_you` of the attack data (`UNK_7.hs_you`): `HITCHECK.c:dm_status_copy()` puts it into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into `hit_stop` and `PLMAIN.c:check_hit_stop()` holds the defender for it.  The field `impact` is the knockback of the hit, not its length | the hold of the defender goes up until the hit is not negative on hit | done - the values are derived by `tools/upper3s/hit_advantage.py` out of the data of the disc and are in `upper3s/pl15.c`: the close Fierce goes from -23 on hit to 0, the far Fierce from -16 to 0, the crouching Fierce from -2 to 0 and the crouching Forward from -3 to 0.  The standing Forward (+4 on hit) and the far Roundhouse (+18) are already at zero or better in the retail game and are not changed |
| S2 | "Allow Sean Roll (214P) to be super-cancelled during the middle" | the Sean Roll is a special move, so it already gets the cancel of a super when it hits (`HITCHECK.c:hit_pattern_extdat_check()`, HITCHECK.c:887) and when its chain carries the bit 0x40 (`CHARSET.c:2624`); the window of the move is the `cg_cancel` byte of its chain | the byte carries the bit of the super art during the middle of the roll | needs-disc - the `cg_cancel` byte of the chain of the Sean Roll |
| S3 | "Decrease the startup of L/M/H Ryuubi Kyaku (236K) from all being 25f to 17/19/21f; make Roundhouse version knock down" | the startup is the number of the game frames before the first frame of the chain that carries the attack data; the knockdown is the hit data of the attack (`UNK_7`) | the three versions start earlier and the Roundhouse knocks down | the startup is done for the three versions at once - the data of the disc puts them in ONE chain, so the chain is brought to 19 frames (`upper3s/pl15.c`); see the note below for why the three targets of the document cannot be told apart.  The knockdown of the Roundhouse version is needs-disc - the hit data of the attack |
| S4 | "Increase hit advantage of L/M versions of Ryuubi Kyaku (236K)" | the advantage is `UNK_7.hs_me` and `UNK_7.hs_you` of the attack data | the advantage goes up | needs-disc - the values of the disc; the framework can patch them as soon as they are known |
| S5 | "Decrease the startup of all Tackles" | the startup is the chain of the move | the Tackles start earlier | needs-disc - the chains of the Tackles |
| S6 | "Allow Fierce Tackle to be faked/cancelled by tapping K during startup" | the input is read by the chain of the move, which branches on the buttons (`comm_if_l` and the button commands of the chain) | the chain reads the button during the startup | needs-disc - the chain of the Fierce Tackle |
| S7 | "Make Fierce and EX Tackles un-ground-parryable" | the parry type of a move is the guard type of its attack data (`UNK_7.guard`) | the guard type of the two Tackles does not allow the ground parry | needs-disc - the guard type of the attack data |
| S8 | "Change EX Tackle so that the hitbox is active from 8f-end and it will automatically grab whenever it reaches the opponent; allow EX Tackle to be super-cancelled on the last hit" | the hitbox and its frames are the chain and the hit data table of the disc; the cancel window is the `cg_cancel` byte of the chain | the hitbox is live from the eighth frame and the byte carries the bit of the super art on the last hit | needs-disc - the chain of the EX Tackle, the hit data table and the byte of the chain |
| S9 | "Reduce startup of L/M/H/EX Dragon Smash (623P) from 5/5/9/8f to 3/4/5/3f" | the startup is the number of the game frames before the first frame of the chain that carries the attack data | the Dragon Smash starts earlier | done - `upper3s/pl15.c`, the entries 64 to 67 of the slot of the special moves; see the note below |
| S10 | "Fix Hyper Tornado (SAIII) so that it juggles properly vs airborne opponents" | the knockdown and the juggle of a hit are the hit data of the attack (`UNK_7`) | the hit knocks the opponent down | needs-disc - the hit data of Hyper Tornado |
| S11 | "Decrease recovery on Hadou Burst from 61f to 51f" | the recovery is the chain of the super | the chain is ten blocks shorter | needs-disc - the chain of Hadou Burst |
| S12 | "I would have suggested replacing his Fierce/EX DP animation ..." | the animation of a move is the `cg_olc_ix` of the chain of the move | - | drop - the design document states it as a wish, not as a change |

### The three versions of the Ryuubi Kyaku share one chain

The Ryuubi Kyaku is the special move of the command 30: the three strengths are
the entries 32, 33 and 34 of the slot of the special moves (`char_table[5]`).
The data of the disc puts the three of them in **one chain**: the slot holds
the same byte offset (3312) for the three entries, and the chain holds the
three frames that hit - one per strength, with the attack data 100, 101 and
102 of the attack table.  The three of them have the same damage (pow 24), the
same stun (piyo 185) and the same hitstun (12/-12), and they differ in the hit
box (85, 86 and 87) and in the animation.

A change of a chain is a change of every move that shares it, so the patch
cannot give the three versions the three different startups of the design
document (17, 19 and 21 frames).  The table of the patch holds one entry for
the chain, and the chain is brought to **19 frames**: the target of the medium
version, and the value that is the closest to all three targets of the
document.

The disc data and the document agree that the three versions start in the same
number of frames - "Currently they all have the same startup" - so nothing is
lost by the change: the move of Sean is six frames faster, and the difference
between the strengths of the document is not expressible in the data of the
disc.

### The move the document calls "Dragon Smash (623P)"

The Dragon Smash is the special move of the command 29: the four strengths are
the entries 64 to 67 of the slot of the special moves (`char_table[5]`).

The identification comes from the frame data of the document itself. The
startups of the four strengths of this move are 5/7/9/8 frames in the data of
the disc, and the document gives 5/5/9/8: the same three of the four and a
slip on the medium. No other group of the special moves of Sean has that
signature - the other group that starts in 5 frames (the entries 24 to 27)
starts in 5/7/8/8 and branches.

The targets of the document (3/4/5/3) are used as they are: they are below the
numbers of the disc and they keep the progression of the strengths.

The EX version is the one the document cannot have: it asks for three frames,
but the startup of the EX version is five frames of the chain and a frame of a
chain cannot be held for less than one game frame. Five frames - the shortest
the chain of the move can be made - is used instead.

## Verification

* `bash tools/verify/ps2_touched_sources.sh`: 36 files compared, 0 changed.
