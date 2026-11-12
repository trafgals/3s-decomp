# Third Strike Upper (3s') - Ryu

Ryu is the character 2 of the game (AGENTS.md section 3.1). The retail sources
are `PLPAT02.c` and `PASS02.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl01.c` (the file of the 3s' layer
is numbered in the order of the character select, not in the order of the
game - the mapping is listed in the report).

Retail base data (`PLCNT.c:super_arts_data[2]`):

| slot | command | gauge length | stocks | move |
|:--|:--|:--|:--|:---|
| SA I | 20 | 112 | 2 | Shinkuu Hadoken (236+P+P) |
| SA II | 22 | 128 | 1 | Shin Shoryuken (623+P) |
| SA III | 21 | 96 | 1 | Denjin Hadoken |

The design document does not ask for a stock or gauge length change on Ryu, so
nothing is overridden in `upper3s/pl01.c`.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Decrease opponent knockback from F+Strong (6MP) on hit, to make links after it more reliable" | the knockback of a move is the movement data of the frame chain (`wk->wu.mvxy`, `setup_mvxy_data()` in `PLPAT.c`) and the hitbox entries of the attack table - both are read from the disc at runtime | partial - `upper3s/pl01.c`: the second hit's knockback (attack entry 51) impact 18 -> 17; Ken leaves hitstun 101 apart instead of 113. 6MP is +1 on hit and nothing of Ryu's links after it in this version, so the links cannot be checked (`tests/match/test_hitstun.py`) |
| "Increase forward movement on 2nd hit of Target Combo (far s.Fierce->s.RH)" | the forward movement of a move is the same `wk->wu.mvxy` data of the frame chain | needs-disc |
| "Increase hitstun on far standing Forward, standing Roundhouse, close Fierce, crouching Fierce, crouching Short, and crouching Forward so they are not negative on hit" | the hitstun of a move is the hold of the defender, the field `hs_you` of the attack data (`UNK_7.hs_you`): `HITCHECK.c:dm_status_copy()` puts it into `dm_stop`, `PLS02.c:set_hit_stop_hit_quake()` moves it into `hit_stop` and `PLMAIN.c:check_hit_stop()` holds the defender for it.  The field `impact` is the knockback of the hit, not its length | the hold of the defender goes up until the hit is not negative on hit | done - the values are derived by `tools/upper3s/hit_advantage.py` out of the data of the disc and are in `upper3s/pl01.c`: the close Fierce and the crouching Fierce go from -1 on hit to 0 and the crouching Forward from -2 to 0.  The far standing Forward (+1 on hit), the standing Roundhouse (0) and the crouching Short (+2) are already at zero or better in the retail game and are not changed |
| "Expand hitbox on LK Joudan Sokutou Geri (41236LK) downward" | the hitboxes are the entries of the attack table of the disc; the LK, MK and HK kicks share the same boxes (attack boxes 30 and 31) | verified - `upper3s/upper3s_boxes.c`: on the LK kick's animation (60) the four boxes' bottom edges go 20 lower (50 -> 30, 72 -> 52), on a copy, so the MK and HK kicks keep theirs. After close s.HP on a crouching opponent the LK kick hit 5 of the 20 characters and now hits all 20; after c.HP it already hit everyone (`tests/match/test_cancels.py:RyuJoudan`) |
| "Lower opponent's knockup height from Jab and Strong DPs (623LP/MP), and reduce knockup height on the middle hits of Shinkuu Hadoken" | the knockup height is the vertical component of the movement data of the frame chain | needs-disc |

The mechanism for all five items is the same: the value is in the frame chain
of the disc, and the 3s' layer can only change the fields of the attack table
that are already loaded (`UNK_7.impact`, `UNK_7.hs_me`, `UNK_7.hs_you` and the
power index), and a move's attack boxes (`upper3s/upper3s_box.c`), not the movement.
