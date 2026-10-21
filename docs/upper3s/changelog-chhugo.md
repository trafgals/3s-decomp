# Third Strike Upper (3s') - Hugo

Hugo is the character 6 of the game (AGENTS.md section 3.1). The retail
sources are `PLPAT06.c` and `PASS06.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl05.c`.

Retail base data (`PLCNT.c:super_arts_data[6]`):

| slot | command | gauge length | stocks | move |
|:--|:--|:--|:--|:---|
| SA I | 20 | 128 | 1 | "Megaton Press" (236+P+P) |
| SA II | 21 | 112 | 2 | "Hammer Mountain Frenzy" (214214K) |
| SA III | 22 | 88 | 2 | the third super |

The design document does not ask for a stock or gauge length change on Hugo,
so nothing is overridden in `upper3s/pl05.c`.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Allow Monster Lariat (236+K) to be super-cancelled on hit, and allow supers to juggle after it" | the cancel window of a move is the `cg_cancel` bits of the frame chain of the disc, read by `PLS00.c:check_cg_cancel_data()`; the juggle after a super is the same kind of data | needs-disc |
| "Decrease startup of EX Giant Palm Bomber (63214PP) from 22f to 17f" | the startup is the frame chain of the disc: the startup of a move is the number of the game frames before the first frame of its chain that carries the attack data | done - `upper3s/pl05.c`, the entry 27 of the slot of the special moves starts in 22 frames in the data of the disc and is brought to 17 frames |
| "Increase hitstun of crouching .Jab (2LP) and crouching Short (2LK) by 1-2f" | the hitstun of a move is the hold of the defender, the field `hs_you` of the attack data (`UNK_7.hs_you`): `HITCHECK.c:dm_status_copy()` puts it into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into `hit_stop` and `PLMAIN.c:check_hit_stop()` holds the defender for it | the hold of the defender goes up by two frames | done - `upper3s/pl05.c`; the upper end of the range of the document is used, so that the link to the EX Monster Lariat is reliable |
| "Add EX Shootdown Backbreaker, which allows juggles afterwards like his 2i version; distance can be controlled with LK+MK / MK+HK / LK+HK" | a new move needs a new entry in the command table of `cmd_data.c` and a new move in the frame chain of the disc | needs-disc |
| "Reduce startup of L/M/H Meat Squasher (360K) from 21/31/44 to 18/24/30; reduce recovery on whiff from 30/33/36 to 26/29/32" | the startup and the recovery are the frame chain of the disc | done - `upper3s/pl05.c`, the entries 64, 65 and 66 of the slot of the special moves; the numbers of the document do not agree with the disc - see the note below |
| "Fix the wrongly-offset hitboxes on frame 1 of his landing-from-air-hitstun" | the hitboxes are the entries of the attack table of the disc | needs-disc |

### The move the document calls "Meat Squasher (360K)"

The move is the special move of the command 29: the three strengths are the
entries 64, 65 and 66 of the slot of the special moves (`char_table[5]`).

The identification does not come from the name but from the frame data. The
recovery on whiff of the document (30/33/36) is the number of the game frames
from the first frame of the chain that hits to the end of the move. A scan of
the whole disc (`tools/upper3s/move_report.py` and the search of
`build/find_rec.py`) shows that the three strengths of this move are the only
ones of the whole cast that have that recovery.

The startup of the document does not agree with the disc:

| | the document | the disc |
|:---|:---|:---|
| light | 21 | 24 |
| medium | 31 | 52 |
| heavy | 44 | 67 |

No move of the disc has the startups of the document (a scan of every run of
three consecutive strengths of every slot of every character finds none), so
the numbers of the document are a reading of the move that the frame data of
the disc does not reproduce.

The startup of the patch is the target of the document, 18/24/30: the change
is an absolute target and not the difference the document asks for applied to
the numbers of the disc, which would have given 21/45/53 - a number that is
neither the retail one nor the one of the document.  The value that has to be
held by the chain for the change to be applied is the number of the disc, so
that the change only reaches the real move.

| | the startup of the disc | the startup of the patch | the recovery of the disc | the recovery of the patch |
|:---|:---|:---|:---|:---|
| light | 24 | 18 | 29 | 25 |
| medium | 52 | 24 | 32 | 28 |
| heavy | 67 | 30 | 35 | 31 |

The recovery of the document is the number of the game frames from the first
frame that hits to the end of the move, which is one more than the number of
the frames that follow the last frame that hits (29/32/35 in the data of the
disc). The targets of the document (26/29/32) are therefore 25/28/31 frames
after the last hit.

The frames of the chain are shortened, the longest one first, and never below
a hold of one game frame: a frame that is held for no game frame at all is a
command block for the engine, not a frame of the animation. The frames that
hit are never touched.

### The move the document calls "Monster Lariat (236+K)"

The 236+K of the document could not be matched to a move of Hugo with the
sources alone. The special moves of Hugo are the command sets 28 to 33 of
`cmd_data.c:p6_cmd`, and their routine numbers (the header of the command
set) are 17, 24, 20, 16, 18 and 19, which
`PLPAT06.c:pl06_exatt_table` maps to `Att_HADOUKEN2`,
`Att_PL06_HASHIRI_NAGE` (the shoulder throw), `Att_SHOURYUUKEN`, the
hadouken, `Att_CHOUCHUURENGEKI` (the Palm Bomber) and `Att_HADOUKEN2` again.
The 236+K of the document is therefore one of the entries of that list, but
the motion of a command set is stored as a chain of lever records in
`cmd_data.c` whose decoding is not documented in the repository, so the move
could not be named with certainty.

The hook that the item needs is in the cancel window of the frame chain of
the disc (`wk->wu.cg_cancel`, the bits `0x20` and `0x40` that
`PLS00.c:check_cg_cancel_data()` reads), plus the juggle data of the super
arts of the disc. The name of the move is not needed for that: once the
`char_ix` of the move is known, the override goes into
`upper3s_moves_hugo[]`.
