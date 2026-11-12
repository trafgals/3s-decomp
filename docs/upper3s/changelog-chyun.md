# Third Strike Upper (3s') - Yun

Yun is the character 3 of the game (AGENTS.md section 3.1). The retail sources
are `PLPAT03.c` and `PASS03.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl02.c`.

Retail base data (`PLCNT.c:super_arts_data[3]`):

| slot | command | gauge length | stocks | dtm | move |
|:--|:--|:--|:--|:--|:---|
| SA I | 22 | 120 | 1 | 65536 | Senpuukyaku super |
| SA II | 21 | 88 | 3 | 65536 | "Sourai Rengeki" |
| SA III | 20 | 72 | 1 | 16384 | "Genei Jin" (a quarter of the damage) |

The design document and the retail table agree on the slots: the SA II is the
one that costs three stocks, and the SA III is the one with the short bar and
the damage multiplier of a quarter.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Genei Jin (SAIII) no longer grants all his normals increased priority" | the priority of a normal is the attack level of the attack table (`UNK_7.level`, read in `CHARSET.c:2798`); the normals of Yun are not the only moves of the game that carry a super priority, so the value that has to be changed is the one of the attack data of the normals of Yun | needs-disc |
| "Genei-Jin bar length increased, now same length as Sourai Rengeki" | `PLCNT.c:super_arts_data[3][2].gauge_len` is 72, the SA II bar is 88 | done - `upper3s/pl02.c`: `{ U3S_SA_GAUGE_LEN, 0, 88, 0 }` on the SA III |
| "Sourai Rengeki (SAII) now a maximum of 2 stocks instead of 3" | `PLCNT.c:super_arts_data[3][1].store_max` is 3 | done - `upper3s/pl02.c`: `{ U3S_SA_STORE_MAX, 2, 0, 0 }` on the SA II |
| "Upkicks (623K) hitbox adjusted to reach further downward" | the hitboxes are the entries of the attack table of the disc | needs-disc |

The "duration of timer for the super is not changed" and "the damage scaling
during Genei Jin juggles has not been changed" of the document are respected:
the duration is a property of the frame chain and the damage scaling is the
`dtm` of the super art data (16384, a quarter), which is not overridden.
