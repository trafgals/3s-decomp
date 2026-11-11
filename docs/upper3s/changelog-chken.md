# Third Strike Upper (3s') - Ken

Ken is the character 11 of the game (AGENTS.md section 3.1). The retail
source is `PASS11.c` and `PLPAT11.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl10.c`.

Retail base data (`PLCNT.c:super_arts_data[11]`):

| slot | command | gauge length | stocks | move |
|:--|:--|:--|:--|:---|
| SA I | 20 | 112 | 2 | Shinkuu Hadoken (236+P+P) |
| SA II | 21 | 104 | 1 | Shin Shoryuken (623+P) |
| SA III | 22 | 80 | 3 | "Shippu Jinrai Kyaku" (multi hit) |

The dragon punch of Ken is the command set 28 of `cmd_data.c:pB_cmd`; the
header of that command set holds the routine number 17, which
`PLPAT11.c:pl11_exatt_table` maps to `Att_SHOURYUUKEN`, and the four
strengths of the command are the four consecutive entries 28 (light), 29
(medium), 30 (heavy) and 31 (EX) of the group 1 of the assignment table of
Ken (`bin2obj/asstbl.c:_assadr_lv_9900[11][1]`). The light one is the jab
dragon punch of the document, 623+LP.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Shippu Jinrai Kyaku (SAIII) now has a maximum of 2 stocks instead of 3; damage reduced by 15%" | the retail SA III costs three stocks (`PLCNT.c:super_arts_data[11][2].store_max`); the base damage of a hit is the index of `POW_DATA.c:Power_Data`, so it can only be scaled at runtime (`Pow_Pow.c:upper3s_power`) | done - `upper3s/pl10.c`: the stock count is 2 and the three versions of the super are scaled to 850 permille |
| "Jab DP (623LP) damage reduced from 15 to 11.5" | the same pipeline; the document gives the retail value and the wanted value, so the change is the ratio between them: 11.5 / 15 = 767 permille | done - `upper3s/pl10.c`: `{ U3S_KOC_ATTACK, 28, U3S_MV_DAMAGE_PERMILLE, 0, 767, 0, 0, 0, 0 }` |
| "Grounded opponent knockback on 2nd hit of Target Combo Strong->Fierce decreased slightly" | the knockback is the movement data of the frame chain of the disc | needs-disc |
| "Fix the crash when killing Makoto with a neutral throw" | the throw is the shared code (`Com_Sub.c`, `HITPLPL.c`); the crash is a fault of the shared throw handling, not of a file of Ken | needs-disc - a hook in the shared throw code would be needed |

### The unit of the damage of the document

The base damage of a move is the index of the power table
(`POW_DATA.c:Power_Data`, whose values are multiples of 5), and the vitality
damage is `(power * Pow_Control_Data_1[0][Round_Level]) / 100`
(`Pow_Pow.c:cal_damage_vitality`). The value 11.5 of the document is
therefore not reachable in the integer pipeline of the retail game. The
override scales the base damage by the ratio of the document (767 permille),
so that the relation between the jab and the strong dragon punch is the one
the document asks for whatever the retail base damage of the move is; the
integer pipeline then truncates the result to the reachable value that is the
closest to 11.5.

### The tests

`tests/upper3s/test_upper3s_ch_ken.c` checks that the jab dragon punch is
scaled to 767 permille, that the other versions of the dragon punch and the
other moves of Ken keep their retail damage, and that Shippu costs two
stocks and does 15% less damage.
