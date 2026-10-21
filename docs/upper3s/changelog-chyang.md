# Third Strike Upper (3s') - Yang

Yang is the character 10 of the game (AGENTS.md section 3.1). The retail
sources are `pass10.c` and `plpat10.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl09.c`.

Retail base data (`PLCNT.c:super_arts_data[10]`):

| slot | command | gauge length | stocks | dtm | move |
|:--|:--|:--|:--|:--|:---|
| SA I | 21 | 120 | 1 | 65536 | the 236+P+P super |
| SA II | 22 | 104 | 2 | 65536 | the second super |
| SA III | 20 | 64 | 1 | 13107 | "Sei'ei Enbu" (a sixth of the damage) |

The four strengths of a super are four consecutive entries of the group 0 of
the assignment table of Yang (`bin2obj/asstbl.c:_assadr_lv_9900[10][0]`):
the SA I is 48 to 51, the SA II is 44 to 46 and the SA III is 61 to 63.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Alter the trajectory of the divekick in Target Combo 3 (jump towards MK->DF+MK) so that it hits more often; alter the trajectory of bug Target Combo 3 (jump backward MK->DF+MK) so that it lands faster" | the trajectory of a move is the movement data of the frame chain of the disc (`wk->wu.mvxy`) | needs-disc |
| "Fix bug where close standing Forward (MK) will sometimes not launch the opponent" | the hitboxes and the movement of the frame chain of the disc | needs-disc |
| "Fix bug where Yun/Yang can end up behind Yang after being hit by Fierce Mantis Slashes (236HPx3) with a certain timing" | the pushbox of a character is the pushbox of the frame chain of the disc | needs-disc |
| "I'm fine leaving Sei'ei Enbu with the shortest bar in the game, because it is not anywhere near as dominant as Genei Jin, and costs him the use of EX slashes" | the SA III of Yang already owns the shortest bar of the retail table (0x40) | drop - a deliberate no-op; `upper3s/pl09.c` only restates the retail value to make the intent explicit |

### The observation about the last item

The document justifies the shortest bar of "Sei'ei Enbu" by the bar of
"Genei Jin", and the bar of "Genei Jin" is the one of the SA III of Yun
(`PLCNT.c:super_arts_data[3][2].gauge_len` is 72, and the item of the
document raises it to 88). The bar of Yang is 64, which is shorter than both
of them, so the intent of the document is already the retail behaviour and
nothing has to be changed.
