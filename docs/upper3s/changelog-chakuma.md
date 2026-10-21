# Third Strike Upper (3s') - Akuma

Akuma is the character 14 of the game (AGENTS.md section 3.1). The retail
sources are `pass14.c` and `plpat14.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl13.c`.

Retail base data (`PLCNT.c:super_arts_data[14]`):

| slot | command | alternative | gauge length | stocks | move |
|:--|:--|:--|:--|:--|:---|
| SA I | 20 | 38 | 112 | 2 | the 236+P+P super |
| SA II | 21 | - | 112 | 2 | the multi hit super ("Messatsu Gou Rasen") |
| SA III | 22 | 39 | 112 | 2 | the 623+P super |

The slots of the document and the slots of the retail table agree: the SA II
is the multi hit super, which is the "Messatsu Gou Rasen" of the document
(the routine of the command set 21 is the number 20, which
`plpat14.c:pl14_exatt_table` maps to `Att_SHOURYUUREPPA`).

The hidden super (the KKZ) is the alternative command of the SA I
(`nmsa_a_ix` 38). The alternative commands use the group 2 of the assignment
table of Akuma (`bin2obj/asstbl.c:_assadr_lv_9900[14][2]`), so the KKZ is
the char_ix 48, 49, 50 and 51. The four entries share one routine number
(r_no 82), so they are the four strength entries of one single move, and the
command set of the alternative (`cmd_data.c:pE_cmd_38`) asks for the routine
25, which `plpat14.c:pl14_exatt_table` maps to `Att_PL14_AT2` - a routine of
Akuma alone, which is what a hidden super needs. The other alternative
(`nmsa_a_ix` 39, the char_ix 76 to 79) uses the kick buttons and a shared
routine, so it is not the KKZ.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Stun bar increased to 'regular' size instead of 'short' size" | `PLCNT.c:pl_piyo_tbl[14]` is 56, the short size; the regular size is the one of the majority of the cast (64) | done - `upper3s/upper3s_sys.c:upper3s_sys_stun_genkai()` |
| "Messatsu Gou Rasen (SAII) bar length is now shorter than SAI or SAIII" | the three supers of the retail table share the same bar length of 112 | done - `upper3s/pl13.c`: the bar of the SA II is 0x60 (96), the other two keep 112 |
| "KKZ (hidden super) motion changed from D,D,D+PPP to D,D+PP" | the motion of a command is the chain of lever records of the command set in `cmd_data.c` (`pE_cmd_38`); the command tables are a shared file | needs-disc - the hook is the record chain of `cmd_data.c:pE_cmd_38` |
| "KKZ (hidden super) damage reduced from 107 to 95" | the base damage of a hit is the index of `POW_DATA.c:Power_Data`, so it can only be scaled at runtime (`Pow_Pow.c:upper3s_power`); the document gives the retail value and the wanted value, so the change is the ratio between them: 95 / 107 = 888 permille | done - `upper3s/pl13.c`: the four entries of the KKZ are scaled to 888 permille |
| "Towards+Strong is still invincible to throws" | nothing to change | drop - the document asks to keep it |

### The correction of the index of the KKZ

The override of the KKZ used the index 57 before. The index 57 is not one of
the moves of Akuma: it is in none of the four groups of
`_assadr_lv_9900[14]`, so the override patched an attack data that Akuma
never uses. The KKZ is the char_ix 48 to 51, and the damage is now a scale of
the retail value (888 permille) instead of an absolute value, so that the
relation of the document is kept whatever the retail base damage of the move
is.

### The tests

`tests/upper3s/test_upper3s_ch_akuma.c` checks that the four entries of the
KKZ are scaled to 888 permille, that the other supers of Akuma keep their
damage, that the index 57 is not one of them, and that the bar of the
"Messatsu Gou Rasen" is the shortest one of the three.
