# Third Strike Upper (3s') - Dudley

Dudley is the character 4 of the game (AGENTS.md section 3.1). The retail
sources are `PLPAT04.c` and `PASS04.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl03.c`.

Retail base data (`PLCNT.c:super_arts_data[4]`):

| slot | command | gauge length | stocks | move |
|:--|:--|:--|:--|:---|
| SA I | 20 | 96 | 2 | "Rocket Uppercut" (623+P) |
| SA II | 21 | 112 | 1 | "Rolling Thunder" (236+P+P) |
| SA III | 22 | 80 | 3 | the third super (236+P+P+P) |

The slots of the document and the slots of the retail table agree. The
routine of each group is the one the command set of the group asks for
(`cmd_data.c:p4_cmd_20`, `p4_cmd_21`, `p4_cmd_22`; the headers hold the
routine numbers 19, 20 and 21) and `PLPAT04.c:pl04_exatt_table` maps them to
`Att_SHOURYUUREPPA` (the upper cut), `Att_HADOUKEN2` (the multi hit super)
and `Att_CHOUCHUURENGEKI`. The four strengths of a super are four
consecutive entries of the group 0 of the assignment table of Dudley
(`bin2obj/asstbl.c:_assadr_lv_9900[4][0]`):

```
SA I  (Rocket Uppercut)  char_ix 48 (light), 49 (medium), 50 (heavy / EX)
SA II (Rolling Thunder)  char_ix 32 (light), 33 (medium), 34 (heavy / EX)
SA III                   char_ix 28 (light), 29 (medium), 30 (heavy / EX)
```

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Reduce stun damage on Rocket Uppercut (SAI)" | the stun of a move is `UNK_7.piyo`, an index into the per player table `_add_piyo_gauge[player][16]` of the disc, so the absolute value cannot be read from the sources | done - the stun of the three versions of the super is scaled to 750 permille (`upper3s/pl03.c`) |
| "Increase stun damage on Rolling Thunder (SAII)" | the same | done - the stun of the three versions of the super is scaled to 1250 permille (`upper3s/pl03.c`) |
| "Fix Rocket Uppercut's 'character point' position to not knock opponent backwards as often in juggles" | the character point is the anchor of the move, a property of the frame chain of the disc | needs-disc |
| "Fix Corkscrew Blow to vacuum the opponent in on hit" | the vacuum is the movement data of the frame chain (`wk->wu.mvxy`) | needs-disc |
| "Fix Rolling Thunder (SAII) to turn around if the player makes him dash again but he goes underneath the opponent" | the direction of a move and the dash of the super are read from the frame chain and the command table; the turn around itself is the `wk->wu.rl_flag` of the move, which the shared state machine owns | needs-disc |
| "See if there is some way of making Cross Counter connect against invincible moves" | the invincibility of a move is the throw invulnerability and the super invincibility of the shared code (`HITCHECK.c`, `upper3s/upper3s_throw.c`); the Cross Counter is a shared special move (`PLPATUNI.c`), not a move of the file of Dudley | needs-disc - a hook in the shared invincibility handling would be needed, it is not in the file of the character |

### Correction of the index of the Rolling Thunder

The override of the Rolling Thunder used the index 44 before. The index 44 is
not one of the super arts of Dudley: it is the waza 33 of the command table
of Dudley, a special move (`Att_CHOUCHUURENGEKI`). The index of the Rolling
Thunder is 32, 33 and 34. The override now points at the three versions of
the super, and the test `tests/upper3s/test_upper3s_ch_dudley.c` checks that
the index 44 keeps its retail stun.
