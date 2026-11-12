# Third Strike Upper (3s') - Urien

Urien is the character 13 of the game (AGENTS.md section 3.1). The retail
sources are `pass13.c` and `PLPAT13.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl12.c`.

Retail base data (`PLCNT.c:super_arts_data[13]`):

| slot | command | gauge length | stocks | move |
|:--|:--|:--|:--|:---|
| SA I | 20 | 104 | 2 | "Tyrant Slaughter" (236+P+P) |
| SA II | 21 | 80 | 2 | "Tyrant Punish" (623+P) |
| SA III | 22 | 88 | 2 | the third super |

The four strengths of a super are four consecutive entries of the group 0 of
the assignment table of Urien (`bin2obj/asstbl.c:_assadr_lv_9900[13][0]`):
the SA I is 70, 71, 72 and 73, the SA II is 66 to 69 and the SA III is 62 to
65.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Fix Tyrant Slaughter/Tyrant Punish (SAI) to fully connect for maximum damage vs all characters when crouching or in most juggle situations" | the connectivity of a super is the hitboxes and the movement of the frame chain of the disc | needs-disc |
| "increase damage by 25% to match Shin Shoryuken" | the base damage of a hit is the index of `POW_DATA.c:Power_Data`, so it can only be scaled at runtime (`Pow_Pow.c:upper3s_power`) | done - `upper3s/pl12.c`: the four versions of the SA I are scaled to 1250 permille |
| "limit to 1 stock instead of 2" | the retail SA I costs two stocks (`PLCNT.c:super_arts_data[13][0].store_max`) | done - `upper3s/pl12.c`: the stock count is 1 |
| "increase bar length slightly" | the retail bar of the SA I is 104 (`gauge_len`); the document does not give a value, only "slightly", so the value 0x70 (112) is used - it is the length of the bar of the other supers of the cast and stays inside the range of the retail table (0x40 - 0x80) | done - `upper3s/pl12.c`: the gauge length is 0x70 |

The unblockables and the charge partitioning of Urien are items of the system
mechanics and are not repeated here.
