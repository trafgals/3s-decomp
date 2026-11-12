# Third Strike Upper (3s') — itemised remaining work

As of 2026-11-12. Open items are tracked as GitHub issues ([below](#github-issues)). Spec: [What I'd Change For Third Strike Upper (3s')](https://mikezsez.blogspot.com/2015/12/what-id-change-for-third-strike-upper-3s.html). Where things live in the source: [`AGENTS.md` §3](../../AGENTS.md).

<!-- summary:start -->
**134 work items** (plus 5 "no change" items). On `main`: all 134 open. On this branch: 64 verified in a fight test, 2 done but not yet measured, 9 partial, 58 not done, 1 not needed in this version.
<!-- summary:end -->

## How to read this

- **On `main`, nothing is implemented.** Every item below is open on `main`.
- **Branch** is the state of this branch (`ccr-579fa5e1-azpt7p`), which has PR #1 (`feat/auto-20261020-abd7ca61`, commit `ea1d880`) merged in, with the fixes listed under [Defects in PR #1](#defects-in-pr-1).
  - `verified`: implemented, and a fight test (`tests/match/`) measures the spec's change in the real engine.
  - `done`: implemented, not yet measured in a fight test. PR #1's own tests run on synthetic data only, and a fight test has already shown that synthetic tests can pass on a change that does not reach the game.
  - `partial`: part of the item is implemented.
  - `—`: not implemented.
  - `n/a`: the post asks for no change, or a fight test shows the bug it fixes is not in this version.
  - ⚠: a fight test shows the change does not do what the spec asks (see [Defects in PR #1](#defects-in-pr-1)).
- **Kind** says where the change has to be made:
  - `table`: a value in a C table (AGENTS.md §3.3). This can be done on `main` once I1 exists.
  - `logic`: engine code in the C source.
  - `disc`: per-move data loaded from the disc (AGENTS.md §3.4). Blocked on I4.
  - `new`: a new move, input or behaviour.
  - `find`: the cause is not known yet; investigate first.
  - `none`: the post asks for no change.

  Several kinds joined with `+` mean the item has parts of each.
- `super_arts_data[c][s]` is character `c` (AGENTS.md §3.1), super art `s` (0 = SA I). Values are written length/stocks.

## Infrastructure

| # | Issue | Status |
|:--|:--|:--|
| I1 | A 3s' build | **Done on this branch** for the host builds: `UPPER_3S=1` with `PLATFORM=linux`, `web` or `headless`. A 3s' PS2 ELF is not possible yet: the PS2 rule always byte-compares against `THIRD_U.BIN`. |
| I2 | Something to play it on | **On this branch**: the SDL3 port (`PLATFORM=linux`, needs SDL3 and ffmpeg) and the WebAssembly port from PR #1, and the headless port for tests. The published site (trafgals.github.io/3s-decomp) still runs PR #1's code, without the fixes of this branch. |
| I3 | A test harness | **Done on this branch**: `make test` (unit tests on synthetic characters) and `make match-test` (fight tests in the real engine, `tests/match/`). |
| I4 | A way to change disc data | **On this branch** (from PR #1): `upper3s/upper3s_dump.c`, `upper3s/upper3s_chain.c`, `tools/upper3s/`. The fight tests found and this branch fixed that the hitstun/knockback overrides wrote to the wrong attack entry. The chain walker's startups agree with the spec's retail numbers on every move measured so far. Patch kinds: damage, stun, knockback, hitstun, startup, recovery, the tail of a chain from a given frame (`U3S_MV_TAIL`, for moves whose chain has no attack data), a move's attack boxes (`upper3s/upper3s_box.c`: `upper3s_move_boxes[]` swaps a box of one move's frames for another, on a copy, so other moves sharing the box are unchanged), and one field of one frame (`U3S_MV_FRAME`: the attack index, which arms an attack or continues one; the on-hit jump `cg_extdat`; the cancel bits `cg_cancel`). Not yet: hurt, throw and push boxes, and new moves. Cancel windows per frame can now be set with `U3S_FRAME_CANCEL`, frame by frame; no item uses it yet. |
| I5 | Specials are unnamed | **On this branch** (from PR #1): `tools/upper3s/decode_cmd.py` reads `cmd_data.c` and `bin2obj/asstbl.c`. The fight tests identify a move by its effect (`koc` and `cix` in the log). |
| I6 | Guarding the vanilla build without the game file | **On this branch** (from PR #1): `tools/verify/ps2_touched_sources.sh` compares the preprocessed PS2 sources with those of the retail-matched commit. |
| I7 | Leftover assembly (not a blocker) | Two game functions (`MTRANS` `mlt_obj_trans_rgb_ext`, `end_20` `sea_write`) and `EFFXX`'s two effect jump tables (`effmovejptbl`, `effinitjptbl`) are still included from assembly, and 6 CRI functions are not decompiled. They link as they are. |

## System mechanics

| # | Item | Kind | Where | Branch |
|:--|:--|:--|:--|:--|
| SM1 | Each successive whiffed normal halves the whiff meter gain (100%, 50%, 25%, 12.5%…); whiffed specials unchanged. The scaling resets when a move touches the opponent (hit/block/parry), or when you are hit, block or parry. Blocked projectiles reset nothing, projectiles that hit reset it, and parried projectiles reset it for the thrower only. | logic | `PLS02.c:add_sp_arts_gauge_init()` (the whiff gain); resets at the hit/block/parry gain functions | verified |
| SM2 | +2f on the parry cooldown. Being hit during the cooldown by an attack parried the same way deals 25% extra damage (the crouching damage bonus). No bonus when thrown during the cooldown. Successful parries unchanged. | logic | `CMD_MAIN.c` parry input; `HITCHECK.c:defense_ground()`/`defense_sky()`; `Pow_Pow.c` | verified |
| SM3 | "Guess parries" (input during a wakeup, a recovery roll or an air land-on-feet hitstun) are active half as long, with the same cooldown. A throw during that window or cooldown can't be teched. Parries during throw-tech animations unchanged. | logic | `HITCHECK.c`; `PLPCA.c:check_nagenuke()` | verified |
| SM4 | Charge partitioning: as soon as a charged direction (4 or 2) reaches 1/3 charge, the partition timer resets to maximum. | logic | `CMD_MAIN.c` charge (`check_1()`/`check_2()`, `waza_work[][].w_int`) | verified |
| SM5 | Left-right unblockables stay. A hit on a character in block animation who faces the other way scales that hit and its follow-up combo to 50%. | logic | `HITCHECK.c`; `Pow_Pow.c:cal_damage_vitality()` | verified |

## Additional bugfixes

| # | Item | Kind | Where | Branch |
|:--|:--|:--|:--|:--|
| B1 | Successful throws are invincible for the whole animation (fixes the Remy/Urien glitch and the Q throw bug). | logic | `HITCHECK.c:catch_hit_check()`/`set_caught_status()`; `PLPCU.c` | verified |
| B2 | Characters can move during the opponent's superflash if they touch a projectile on the frame it starts. | logic | `PLS01.c:sa_stop_check()`; `HITCHECK.c` | done |
| B3 | Universal overheads (MP+MK) and taunts (HP+HK) work while holding away or towards (today only throws do). | logic | `PLS03.c:check_leap_attack()`, `check_chouhatsu()` | verified |
| B4 | Some characters (Makoto, etc.) can attack immediately after teching an air throw. | logic | `PLS00.c:nm_49000()`; `PLPCA.c:check_nagenuke()` | verified |
| B5 | Rare move-interaction soft locks: X.F.L.A.T. vs EX Yagyou Dama / Aegis Reflector / Tengu Stone; Critical Combo Attack / Deadly Double Combination vs Magnetic Storm. (OTG on Dudley/Hugo after some throws is to stay.) | find | — | — |
| B6 | Wishlist: half-circle motions (41236, 63214) become quarter-circles, on the same button unless another special overlaps. Akuma's red fireball may stay. | table | `cmd_data.c` command tables | — |
| B7 | Characters passing through each other: the post says this is not fixable. | none | — | n/a |

## Characters

The post's order. The number after each name is the character index.

### Chun-Li (15)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| CL1 | Houyoku-sen (SA II): 1 stock instead of 2; bar the longest possible, as Shin-Shoryuken. `super_arts_data[15][1]` 104/2 → 128/1 | table | verified |
| CL2 | c.MK: cancel window no longer allows super-late cancels. | disc | verified (`upper3s/pl14.c`: the c.MK had a second cancel window on block 6, after a frame without; its cancel bits are cleared, so one window is left, as on the c.MK of Ken, Ryu, Yun and Sean) |
| CL3 | c.LP startup 2f → 3f. | disc | verified |
| CL4 | Tensei Ranka (SA III): startup hitbox reaches the floor (can't be crouched); pulls in and down so all hits connect vs airborne opponents; enough knockdown advantage for a crossup; 2 stocks; bar as long as Shippu's. `super_arts_data[15][2]` 72/3 → 80/2 | table + disc | partial (stocks and bar verified; the startup hitbox verified: on its block only, the box reaches the floor, so the startup hit lands on all 20 crouching (retail: Hugo only). Not done: pulling airborne opponents in and down, and the knockdown advantage) |
| CL5 | Houyoku-sen's middle hits no longer un-combo vs crouching Makoto etc. | disc | — |
| CL6 | Goes through the mandatory standing frame after wakeup, like every other character. Not reproduced after a sweep: for Chun-Li and 11 others the last frame of the knockdown has no hurtbox and the character is free (standing, or crouching with down held) on the next. Other wakeups (throws, air knockdowns) not checked yet. | find | — |
| CL7 | Crouching head hurtbox 2–3px higher and 3–4px further back, in every crouching animation (neutral, crouch-block, non-low-profile attacks). | disc | — |

### Ken (11)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| KN1 | Shippu Jinrai Kyaku (SA III): 2 stocks instead of 3; damage −15%. `super_arts_data[11][2]` 80/3 → 80/2 | table | verified |
| KN2 | Jab DP (623LP) damage 15 → 11.5. | disc | verified |
| KN3 | Target combo MP→HP: less knockback on grounded opponents from the 2nd hit, so DPs miss less after it. The post's "slightly" (one or two steps) changes no DP outcome; the smallest knockback step makes the MP/HP DP connect after it, midscreen and in the corner, which is used (impact 19 → 16). | disc | verified |
| KN4 | Fix the crash when killing Makoto with a neutral throw (may not be present in this version). Not present: each of Ken's throws knocks Makoto out and the fight goes on (`tests/match/test_damage.py`). | find | n/a |

### Yun (3)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| YU1 | Genei Jin (SA III) no longer makes all his normals count as super hits for priority. | find | — (PR #1 says the flag is in the disc data) |
| YU2 | Genei Jin bar as long as Sourai Rengeki's; the super's timer is unchanged. `super_arts_data[3][2]` 72 → 88 | table | verified |
| YU3 | Sourai Rengeki (SA II): 2 stocks instead of 3. `super_arts_data[3][1]` 88/3 → 88/2 | table | verified |
| YU4 | Upkicks (623K) hitbox reaches further down. | disc | — (measured, not implemented: the four versions, special animations 32–35, use attack box sets 107 and 108, whose lowest edges are 66 and 69. Point-blank they miss every crouching character except Gill and Hugo; cancelled from s.LP, c.LP, c.MP, s.MP or close s.HP they miss 2 to 12 standing characters. With every box reaching further down: 20 lower hits all 20 crouching characters point-blank; 40 lower leaves one miss in the combos (close s.HP on crouching Chun-Li); 60 lower leaves none. The post says "misses less often", so how far is a choice) |

### Makoto (16)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| MK1 | Abare Tosanami (SA II) stun −50%. | disc | verified |
| MK2 | Fukiage L/M/H stun 29/31/31 → 27/28/31. | disc | verified |
| MK3 | Tanden Renki (SA III): 236K while active deactivates it. It counts as a special, uses her post-Hayate taunt animation, and can be cancelled into attacks (not block or movement) at any time. | new | — |
| MK4 | Karakusa's startup no longer auto throw-techs when she's thrown out of it. | find | — |
| MK5 | Seichusen Godanzuki (SA I): more vacuum and hitstun on the first hit. | disc | — |
| MK6 | EX Oroshi (214PP) knocks down when it hits a limb hurtbox. | disc | — |
| MK7 | Hayate (236[P]): one startup/hold frame has a throwable box too small for its pushbox. | disc | — |
| MK8 | No invincibility on the recovery after a successful Karakusa (63214K); the neutral throw is unchanged. | disc | — |
| MK9 | EX Oroshi xx Abare Tosanami on Hugo: no change. | none | n/a |

### Dudley (4)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| DD1 | Rocket Uppercut (SA I): less stun. | disc | verified |
| DD2 | Rolling Thunder (SA II): more stun. | disc | verified |
| DD3 | Rocket Uppercut's character point: knocks opponents backwards less often in juggles. | disc | — |
| DD4 | Corkscrew Blow vacuums the opponent in on hit. | disc | — |
| DD5 | Rolling Thunder turns around if he dashes again and passes under the opponent. | logic | — |
| DD6 | Cross Counter connects against invincible moves instead of whiffing after taking the damage. | find | — |

### Urien (13)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| UR1 | Tyrant Slaughter/Punish (SA I): fully connects vs crouching opponents and in most juggles; damage +25% (as Shin Shoryuken); 1 stock instead of 2; bar slightly longer. `super_arts_data[13][0]` 104/2 | table + disc | partial (stocks, bar and +25% damage verified; the connect fix is not done) |

### Akuma (14)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| AK1 | Stun bar short → regular. `pl_piyo_tbl[14]` 56 → 64 | table | verified |
| AK2 | Messatsu Gou Rasen (SA II) bar shorter than SA I and SA III. `super_arts_data[14][1]` 112; SA I and SA III are also 112 | table | verified |
| AK3 | KKZ (hidden super) motion D,D,D+PPP → D,D+PP. In this version the command already takes any two punches (D,D,D+PP gives the KKZ); only the third down is removed. | table | verified |
| AK4 | KKZ damage 107 → 95. | disc | verified |
| AK5 | 6MP stays throw-invincible. | none | n/a |

### Yang (10)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| YA1 | Target combo 3 (j.MK→3MK): the divekick's trajectory hits more often; the backward-jump version lands faster. | disc | — |
| YA2 | Close s.MK sometimes doesn't launch. | disc | — |
| YA3 | Yun/Yang can end up behind Yang after Mantis Slashes (236HP×3) with certain timing (probably his pushbox). | disc | — |
| YA4 | Sei'ei Enbu keeps the shortest bar in the game. | none | n/a |

### Ryu (2)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| RY1 | 6MP: less knockback on hit, so links after it are reliable. | disc | partial (the knockback is one step less: Ken leaves hitstun 101 apart instead of 113; but 6MP is +1 on hit and no normal or super of Ryu's links after it in this version, with this knockback or the smallest one, so the "links" part cannot be shown) |
| RY2 | Target combo far s.HP→s.HK: more forward movement on the 2nd hit. | disc | — |
| RY3 | More hitstun on far s.MK, s.HK, close s.HP, c.HP, c.LK and c.MK, so none is negative on hit. | disc | verified |
| RY4 | LK Joudan Sokutou Geri (41236LK): hitbox extends down so it connects after close s.HP / c.HP on everyone. | disc | verified (`upper3s/upper3s_boxes.c`: the four attack boxes of the LK kick's animation reach 20 lower; after close s.HP on a crouching opponent it hit 5 of the 20 characters and now hits all 20; after c.HP it already connected; MK and HK, which share the boxes, are unchanged) |
| RY5 | LP/MP DP knock opponents up less, and so do Shinkuu Hadoken's (SA I) middle hits, so the whole super connects after a DP. | disc | — |

### Oro (9)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| OR1 | Jinchu Nobori (chicken kick): pushboxes, so [s.MP xx chicken kick]×2 works on every character. | disc | — |
| OR2 | Oniyama ([2]8P): more hitstop and juggle time, so super cancels from it are useful. | disc | — |
| OR3 | New forward throw (6+LP+LK): the back throw's animation mirrored, about half the distance. | new | — |
| OR4 | Neutral throw can't be escaped before 3–4 hits, like other mashable grabs. | find | — |
| OR5 | Kishin Riki (SA I): bar as long as Tengu Stone's. `super_arts_data[9][0]` 104 → 112 | table | verified |
| OR6 | Kishin Riki: Human Pillar Driver (63214P) stays a hit-grab during it, so it combos; more damage on the air grab when it's combo'd into; QCT+P deactivates it with a shortened activation animation. | new | — |

### Elena (8)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| EL1 | c.LK chains into itself. | logic | verified |
| EL2 | Mallet Smash: startup frames unthrowable (7f are throwable today). | disc | — |
| EL3 | s.MP cancellable from the 2nd hit, not just the 1st. | logic | verified |
| EL4 | Brave Dance juggles properly vs airborne opponents outside the corner. | disc | — |
| EL5 | Healing (SA III): faster cancel animation; faster health recovery. | disc + logic | partial (recovery rate, `PLPAT08.c`) |
| EL6 | Spinning Beat keeps its 3 stocks. | none | n/a |

### Ibuki (7)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| IB1 | Health 1035 → 1050 (Twelve's). `Com_Vital_Unit_Data[7]` | table | verified |
| IB2 | Yami Shigure (SA III): 2 stocks instead of 1. `super_arts_data[7][2]` 80/1 → 80/2 | table | verified |
| IB3 | Tsuiji Goe (623P): special- and super-cancellable almost straight after takeoff; a kunai thrown from it faces the right way if she hasn't passed over the opponent. | disc | verified (`upper3s/pl06.c`: the cancel bits, from frame 28-33 in retail, now start on the second frame in the air, 2 frames after takeoff, for the four versions; `PLS03.c:hissatsu_setup_union()` turns her to the opponent when a move is cancelled out of it in the air, so a kunai thrown before she passes over flies at the opponent instead of away. With the test's 214 input the kunai does not come out on frames 11-12 after the press; not explained, the cancel bits are set there) |
| IB4 | The target-combo s.HP (4MP→HP and LP→MP→HP) never knocks down. Today one of its two hitboxes is marked knockdown and the other isn't. | disc | — |
| IB5 | Taunt: can't be throw-teched, and she can parry or attack in the air once the opponent recovers. | logic | partial (throw tech) |

### Necro (5)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| NE1 | Slam Dance (SA II) startup 0+2 → 2+0: same total, but it can't be jumped on reaction after the superflash. | disc | — |
| NE2 | Electric Shock (623P): fix its pushbox; slight vacuum on the middle hits. | disc | — |
| NE3 | New EX Electric Shock (possibly juggles after; safer on block than the HP version). | new | — |

### Alex (1)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| AL1 | Hyper Bomb (SA I) startup 0+1 → 1+0; possibly slightly less damage, except the back-turned Backdrop Bomb. | disc | — |
| AL2 | Boomerang Raid: more forward movement on the middle hits; longer range on the final grab. | disc | — |
| AL3 | Air Knee Smash: **all** versions not air-parryable; EX stays ground-parryable. | disc | — |
| AL4 | Target combo c.LK→c.MK: cancel window longer than 1f. | logic | verified |
| AL5 | Juggles allowed after Flash Chops that hit an airborne opponent. | disc | — |
| AL6 | c.MP special- and super-cancellable on hit/block/parry. | logic | verified |
| AL7 | c.MK super-cancellable on hit/block/parry, including in the target combo. | logic | verified |
| AL8 | Air Stampede ([2]8K) knocks down when it hits a limb hurtbox. | disc | — |
| AL9 | Air knockdown animation: fix the anchor point and pushbox so juggles work on him. | disc | — |
| AL10 | j.2HP: hitboxes on his arms, not his chest. | disc | — |

### Hugo (6)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| HU1 | Monster Lariat (236K) super-cancellable on hit, and supers juggle after it. | logic + disc | — |
| HU2 | EX Giant Palm Bomber (63214PP) startup 22f → 17f. | disc | verified |
| HU3 | c.LP and c.LK hitstun +1–2f. | disc | verified |
| HU4 | New EX Shootdown Backbreaker that allows juggles; distance chosen with LK+MK / MK+HK / LK+HK. | new | — |
| HU5 | Meat Squasher (360K): startup 21/31/44 → 18/24/30; whiff recovery 30/33/36 → 26/29/32. | disc | done ⚠ |
| HU6 | Frame 1 of landing from air hitstun: fix the offset hitboxes. | disc | — |

### Q (17)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| Q1 | c.LP, s.LK and c.LK special- and super-cancellable on hit/block/parry. | logic | verified |
| Q2 | c.MK super-cancellable on hit/block/parry. | logic | verified |
| Q3 | All overhead/low rush punches ([4]6P/K, [P]) super-cancellable on hit/block/parry. | logic | verified |
| Q4 | Far s.LP startup 6f → 4f. | disc | verified |
| Q5 | 4HK: cancel window extends past the first active frame. | logic | verified |
| Q6 | More hitstun on c.LK, c.MK and s.HK (s.HK is −10 on hit). | disc | verified |
| Q7 | High Speed Barrage (slaps), all versions: change the opponent's movement on the middle hits so every hit connects. | disc | — |
| Q8 | EX slaps: first hit's hitbox extends down, to connect on crouchers. | disc | verified (`upper3s/upper3s_boxes.c`: the slaps are 214+P; the EX one's first slap, box set 73, missed crouching or crouch-blocking Elena, Oro and Chun-Li at every distance, and Makoto in crouching hitstun after c.LK. Its three boxes reach 4 lower, the least that connects on all 20 in each case; the other versions and the later slaps are unchanged) |
| Q9 | New EX (PP) Total Destruction (SA III) that grabs immediately on activation. | new | — |
| Q10 | Punch Total Destruction can't trade hits. | disc | — |
| Q11 | QCT+P deactivates Total Destruction. **Conflicts with the retail game:** while Total Destruction is active, 236+P is already its punch attack (special 53; 236+K is the grab, 54; `cmd_data.c:p12_cmd_32`/`_33`), the "punch version" Q10 refers to. QCT is 236 in the post's own notation (MK3: "QCT+K (236K)"). Needs the designer's choice of another input. | new | — |
| Q12 | Taunt health bonus for 1–3 taunts: 1200 base/1400/1705/2050 → 1200/1400/1600/1800. | logic | verified |

### Remy (19)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| RM1 | Stun bar short → regular. `pl_piyo_tbl[19]` 56 → 64 | table | verified |
| RM2 | c.MP damage +6 (nearly Ryu's). | disc | verified |
| RM3 | Close s.HP: the earlier hitboxes knock airborne opponents down like the later ones, with juggles after. | disc | — |
| RM4 | More hitstun on far s.MK, far s.HK, c.MK and 6MK, so none is negative on hit. | disc | verified |
| RM5 | s.MK: less knockback on grounded opponents. Target combo s.MK→s.HK: bigger hitboxes on the 2nd hit. | disc | verified (`upper3s/pl18.c`: the close s.MK's knockback one step less, 17 → 16, so Ken slides 66 apart instead of 82; `upper3s/upper3s_boxes.c`: the s.HK of the target combo reaches 20 lower, so it hits everyone standing (retail missed Yun, Oro and Yang) and 17 of 20 crouching, all but Elena, Oro and Chun-Li (retail: Hugo only)) |
| RM6 | Chains c.LP→c.LK and c.LK→c.LP. | logic | verified |
| RM7 | Taunt length 131f → about 80f. | disc | verified |
| RM8 | Light of Virtue meter gain on startup/hit/block 0/1/1 → 1/3/2. | table + disc | — |
| RM9 | Rising Rage Flash: all strengths super-cancellable on hit/block/parry; juggles after all versions. | logic + disc | — |
| RM10 | Cold Blue Kick (63214K): LK and MK versions not negative on hit up close; MK version +2. | disc | verified |
| RM11 | Blue Nocturne (SA III): superflash only after a successful counter; meter spent on the counter, the recovery or being thrown, not on activation; bar a bit longer; juggles/knocks down airborne opponents. `super_arts_data[19][2]` 80/1 | table + logic + disc | partial (bar 80 → 96 verified) |

### Twelve (18)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| TW1 | Health 1050 → 1120. `Com_Vital_Unit_Data[18]` | table | verified |
| TW2 | More hitstun on s.MP, far s.MP, s.HP, c.HP, s.LK, s.MK, s.HK, c.MK and 4MK, so none is negative on hit. | disc | verified |
| TW3 | c.MP and s.MK parryable high only; c.HP and 4MK parryable low only. | disc | — |
| TW4 | 4MK: activate the earlier low hitboxes that exist but can't connect. | disc | verified (`upper3s/pl17.c`: frame 2 of the chain had low attack boxes but no attack data, so no attack was armed on it; it now starts the attack, frame 3 continues it, and a hit on frame 2 jumps on as a hit on frame 3 does. 4MK hits once, 2 frames earlier; its on-hit advantage (0) and whiff are unchanged) |
| TW5 | Far s.MP, s.MP and c.MP special- and super-cancellable on hit/block/parry. | logic | verified |
| TW6 | c.MP superjump-cancellable on hit/block, like s.MK. | logic | verified |
| TW7 | c.MK super-cancellable on hit/block/parry. | logic | verified |
| TW8 | Jab A.X.E. (214LP): more hit and block advantage, at most −1/−2 on block. | disc | verified |
| TW9 | All air normals special-cancellable on hit/block into A.X.E. (j.214P) or D.R.A. (j.214K), and super-cancellable into X.F.L.A.T. | logic | verified |
| TW10 | X.N.D.L.: drags juggled opponents down so more hits connect. | disc | — |
| TW11 | X.F.L.A.T. (SA II): remove the low-damage air-to-air version (always full damage); 2 stocks. `super_arts_data[18][1]` 112/1 → 112/2 | table + disc | partial (2 stocks verified) |
| TW12 | X.C.O.P.Y. (SA III): EX moves (maybe even the opponent's super) by spending chunks of the super's timer; startup 12f → 1f; when hit in its recovery he takes 66% damage instead of extra damage. | new + disc | — (startup in progress; its chain loops) |

### Sean (12)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| SN1 | More hitstun on close and far s.HP, c.HP, s.MK, far s.HK, c.MK and 6HP, so none is negative on hit. | disc | verified |
| SN2 | Sean Roll (214P) super-cancellable in the middle. | disc | verified (`upper3s/pl15.c`: no frame of the roll had cancel bits; the rolling blocks but the first and the last get the super bit, LP frames 6-13, MP 4-24, HP 4-23 and its replay; `upper3s/upper3s_ch_sean.c` arms the cancel there, since the roll does not arm it itself. A super comes out across the middle of all three, never in retail. With the test's input, a super with its punch on frame 13 after the press (and 25 in the HP roll) does not come out; not explained) |
| SN3 | Ryuubi Kyaku (236K) startup 25f (all) → 17/19/21f; the HK version knocks down. | disc | partial (all three 25 → 19 measured; one shared chain, and no knockdown) |
| SN4 | Ryuubi Kyaku L/M: more hit advantage, so supers link after a hit. Measured: L/M are −3 on hit in the retail game and −8/−6 with SN3's shorter startup, because the kick ends when Sean lands (the startup change moves the hit earlier but not the landing). The defender's hold (`hs_you`) does not reach these hits: their stops are synced to the attacker's (`HITCHECK.c:same_dm_stop()`, `att.dipsw & 1`). Needs a patch of the hit's level or of the sync. | disc | — |
| SN5 | Faster startup on all Tackles: c.MK xx LP Tackle and s.HP/c.HP xx MP Tackle combo, and HP Tackle is 1f from comboing. | disc | — |
| SN6 | HP Tackle can be faked/cancelled by tapping K during its startup. | new | verified |
| SN7 | HP and EX Tackle not ground-parryable. | disc | — |
| SN8 | EX Tackle (236PP~P): hitbox active from 8f to the end, grabs automatically on contact; super-cancellable on the last hit. | disc + new | — |
| SN9 | Dragon Smash (623P) L/M/H/EX startup 5/5/9/8 → 3/4/5/3f. | disc | partial (3/4/5 measured; EX 8 → 5, not 3) |
| SN10 | Hyper Tornado (SA III) juggles properly vs airborne opponents. | disc | — |
| SN11 | Hadou Burst recovery 61f → 51f. | disc | verified |

### Gill (0)

No changes in the post.

## Defects in PR #1

### Found by the fight tests and fixed on this branch

1. **The 3s' build crashed on the first frame of every fight.** `PLS00.c:check_cg_cancel_data()` read `wk->as->char_ix` while `wk->as` is still NULL (before the player's first move of the round). PR #1's checks only booted to the title screen. Fixed: the cancel rules are looked up only when there is a current move. Covered by `tests/match/test_harness.py`.
2. **The hitstun and knockback overrides wrote to the wrong attack entry.** `upper3s.c` took the attack data index from the first frame of the move (a startup frame, without the engine's shift by 6), so the change went to an unrelated entry: Ryu's standing Jab went from −7 to −12 frames of hold (+3 → +8 on hit), and the moves of the table did not change. The synthetic unit tests encoded the same misreading, so they passed. Fixed: the entries are read from the attacking frames of the move's chain (`upper3s_chain_attacks()`), and the unit tests encode the index the way the engine reads it. Covered by `tests/match/test_harness.py`.

3. **The hitstun values missed the spec's goal, and some moves were missing.** PR #1 derived the values from a model of the advantage; in the game several stayed negative and Remy's went far past it (far s.MK +19, c.MK +16, even the close s.HK, which the spec does not touch, +24). Some listed moves had no entry at all (Ryu's far s.MK and s.HK, Q's c.MK, Twelve's far s.MP and s.LK, Sean's close s.HP and far s.HK), and several entries were labelled with the wrong move (`koa` 1 is the far normal and 2 the command normal, not dash versions). Fixed: every value is set from the advantage measured in the real engine, to 0 on hit (Hugo's +2 frames and Remy's Cold Blue Kick +2 as the spec says), and `tests/match/test_hitstun.py` checks each move and its untouched neighbours.
4. **The chain walk ran past the end of a move.** It did not stop at the commands that end a move (`comm_end`, `comm_jmp`, `comm_ret`, ...), so a move of Remy's listed the attack entries of the moves after it. Fixed in `upper3s_chain.c`. A move that branches (Twelve's c.HP plays its second and third hits only after the first connects) is patched with rows that name the attack entry (`U3S_KOC_ENTRY`).
5. **RM10 was applied to Chun-Li.** Remy's Cold Blue Kick change was in Chun-Li's override table, on two of her specials. Fixed: removed there, and implemented for Remy (LK −4 → 0, MK −4 → +2 on hit up close).

6. **The KKZ's damage change missed the KKZ.** The rows scaled animations 48–51; every hit of the KKZ lands during animation 63. Fixed (AK4: the first hit 60 → 53 bar units, 95/107).
7. **Shippu lost 11%, not 15%.** Its last four kicks play in a finishing animation (52) that the rows did not include; damage is looked up by the animation playing when a hit lands. Fixed (KN1: 45 → 38 bar units).
8. **Remy's c.MP got about +1 point instead of +6.** The document's points are the attack's `pow` (`Power_Data[pow]` = 5 × pow): Remy's c.MP is 11, Ryu's 18, so "+6pt to almost match Ryu's" is pow 17; the rows scaled it by 1110 permille. Fixed (RM2: 8 → 13 bar units, Ryu's is 14).
9. **Abare Tosanami's stun rows were on the wrong animations.** They named animations 44 and 45, which have no attack frames, and missed 47, where three of its four hits land; only the first hit was halved. Fixed (MK1: 22 → 9 stun). The super only connects from near a wall, which the test sets up.
10. **Ibuki's health came from a misread column.** PR #1 read a column of `Com_Vital_Unit_Data` where Ibuki looks like the healthiest character; humans read column 2, where she has 950 and Twelve 997, and scaled Necro's row (950 → 951). Fixed: she takes Twelve's retail row (IB1: 950 → 997).

11. **SM1: a halved whiff gain was rounded down.** A whiffed normal gives a whole number of meter units, so 3 → 1.5 became 1 and 2 → 1 → 0.5 became 0 after the second whiff. Fixed: rounded to the nearest unit (`upper3s_sys.c`). Ryu's whiffed s.HP now gains 3, 2, 1, 0 (`tests/match/test_system.py`).
12. **SM2 did nothing in a fight, and cut red parries short.** PR #1's cooldown was the two frames after a tap, which are inside the 10-frame parry window, so a missed parry never took the bonus; and the cooldown check sat in the red-parry branch only, where it disabled the first two frames of a red parry. Fixed: the retail game already has a cooldown (the parry slot takes no new tap until its `free3` runs out, 22 frames after the tap, `CMD_MAIN.c:check_10()`); 3s' adds 2 frames to it, and a hit that lands after the window but in that cooldown, of the kind the tap would have parried, takes the crouching bonus (125%; not stacked on a defender who already has a retail bonus). Measured: a second tap counts 24 frames after the first in the retail game, 26 in 3s'; a missed forward tap 13–26 frames before Ryu's s.HP makes it 21 → 26; parries, red parries and throws are unchanged.
13. **SM3 did not recognise the guess states, and its halved window did not apply.** PR #1 took reaction 88 for the wakeup (it is the frame of an air hit), `my_roll_flag` for the recovery roll (a sprite rotation flag, `MTRANS.c`) and the frame of touching the ground for the air land-on-feet hitstun; and it halved only the red-parry window, which a waking character never uses. Fixed: a knockdown from the first frame lying on the ground (`pat_status` 38) to the end of the damage state, the quick stand (from `ukemi_success`), and air hit reaction 17; a tap made in one has its 10-frame window halved to 5, with the same cooldown, and locks the throw tech until the cooldown is over. Measured on a wakeup after a sweep, a quick stand after a Shoryuken and an air reset by a jab, and on a throw on wakeup.

14. **SM4 kept a charge alive for ever.** The partition timer was reset to its maximum on every frame once a third of the charge was done, including the frames the direction was released, so it never ran out: Remy's charge survived a 116-frame release. Fixed: it is reset once per charge, when the charge reaches a third (42 frames for every phase of the hidden timer, against 0–32 in the retail game).
15. **SM5 used the parry states as "block animation", and did not scale the combo or projectiles.** It took normal states 31–35 (parries) for blocking, missed the block posture (27–30), scaled only the first hit and never a projectile. Fixed: block posture or blockstun (reactions 4–11), the 50% carries to the rest of the combo, and projectile hits are scaled too. Measured with Ken blocking a Hadoken and hit from behind by c.MK xx Hadoken: 13, 9 → 6, 4.
16. **B1 protected the victim, not the thrower.** In the retail game a thrown character has no hurtbox, but the thrower has them on much of its throw (its recovery: 26 frames of Ryu's forward throw, 34 of Q's). Fixed: both are invincible until their part of the throw is over; measured on the forward and back throws of all 20 characters, with the throw's damage unchanged.
17. **Q12 was not implemented.** The docs said it was done in Q's table; nothing changed the taunt bonus. Implemented (see the item).

### Still open

1. **SN3 makes L/M Ryuubi Kyaku worse on hit.** With the startup 25 → 19 (PR #1), the hit comes 6 frames earlier but Sean still lands when he did, so L/M go from −3 to −8/−6 on hit. SN4 is meant to raise their advantage; it has to make up for this as well.
2. **B2 is not reproduced.** No fight makes a character move during the opponent's superflash: a hit, a block or a parry of a projectile on the first frame of the flash, and the opponent's fireball landing then, all leave the frozen character frozen in the retail game too (a hit on the frame the flash starts is delayed until after it). PR #1's guard drops the damage stop of a frozen character that is hit, on the theory that it replaces the flash's stop, but `PLMAIN.c:select_hit_stop()` keeps the longer of the two. The trigger is not known, so the item stays `done`, not verified.
3. **HU5: check the move.** PR #1 reads the Meat Squasher's startup from the disc as 24/52/67, against the post's 21/31/44; light is close, medium and heavy are far off.
4. **SN3 and SN9 are partial.** The three Ryuubi Kyaku share one chain, so all three are 19 frames (the spec asks 17/19/21) and the Roundhouse knockdown is not done. EX Dragon Smash is 5 frames (the spec asks 3), because the chain cannot be held shorter.
5. **Documentation errors.** Issue #2 says `plpat16.c` is Makoto (it is Chun-Li; Makoto is `plpat17.c`, AGENTS.md §3.2). Chun-Li's table header called SA I "Houyoku-sen" (it is Kikoushou) and gave the wrong commands (fixed: all three are 236236, P for SA I, K for SA II and III). PR #1's table for Q labelled entry 1 as "6LP"; in the game it is the far standing Jab (fixed, with the index comment of the five tables that read koa as a dash).

## GitHub issues

Every item that was open on 2026-11-12 has an issue (closed once the item is verified), under the tracking issue [#3](https://github.com/trafgals/3s-decomp/issues/3). The issues repeat the rows above, with notes on how to make the change; this file stays the source of each item's status.

- Infrastructure: missing disc patch kinds (I4) [#4](https://github.com/trafgals/3s-decomp/issues/4), the SDL port in GitHub Actions [#5](https://github.com/trafgals/3s-decomp/issues/5), a CI completeness check [#7](https://github.com/trafgals/3s-decomp/issues/7), a 3s' PS2 build and the published web build (I1/I2) [#8](https://github.com/trafgals/3s-decomp/issues/8), doc errors [#6](https://github.com/trafgals/3s-decomp/issues/6).
- Bugfix: B2 [#14](https://github.com/trafgals/3s-decomp/issues/14), B5 [#9](https://github.com/trafgals/3s-decomp/issues/9), B6 [#10](https://github.com/trafgals/3s-decomp/issues/10).
- Chun-Li: CL2 [#15](https://github.com/trafgals/3s-decomp/issues/15), CL4 [#16](https://github.com/trafgals/3s-decomp/issues/16), CL5 [#17](https://github.com/trafgals/3s-decomp/issues/17), CL6 [#18](https://github.com/trafgals/3s-decomp/issues/18), CL7 [#11](https://github.com/trafgals/3s-decomp/issues/11).
- Yun: YU1 [#19](https://github.com/trafgals/3s-decomp/issues/19), YU4 [#20](https://github.com/trafgals/3s-decomp/issues/20).
- Makoto: MK3 [#12](https://github.com/trafgals/3s-decomp/issues/12), MK4 [#21](https://github.com/trafgals/3s-decomp/issues/21), MK5 [#22](https://github.com/trafgals/3s-decomp/issues/22), MK6 [#23](https://github.com/trafgals/3s-decomp/issues/23), MK7 [#24](https://github.com/trafgals/3s-decomp/issues/24), MK8 [#25](https://github.com/trafgals/3s-decomp/issues/25).
- Dudley: DD3 [#26](https://github.com/trafgals/3s-decomp/issues/26), DD4 [#27](https://github.com/trafgals/3s-decomp/issues/27), DD5 [#28](https://github.com/trafgals/3s-decomp/issues/28), DD6 [#29](https://github.com/trafgals/3s-decomp/issues/29).
- Urien: UR1 [#30](https://github.com/trafgals/3s-decomp/issues/30).
- Yang: YA1 [#31](https://github.com/trafgals/3s-decomp/issues/31), YA2 [#32](https://github.com/trafgals/3s-decomp/issues/32), YA3 [#33](https://github.com/trafgals/3s-decomp/issues/33).
- Ryu: RY1 [#34](https://github.com/trafgals/3s-decomp/issues/34), RY2 [#35](https://github.com/trafgals/3s-decomp/issues/35), RY5 [#36](https://github.com/trafgals/3s-decomp/issues/36).
- Oro: OR1 [#37](https://github.com/trafgals/3s-decomp/issues/37), OR2 [#38](https://github.com/trafgals/3s-decomp/issues/38), OR3 [#39](https://github.com/trafgals/3s-decomp/issues/39), OR4 [#40](https://github.com/trafgals/3s-decomp/issues/40), OR6 [#41](https://github.com/trafgals/3s-decomp/issues/41).
- Elena: EL2 [#42](https://github.com/trafgals/3s-decomp/issues/42), EL4 [#43](https://github.com/trafgals/3s-decomp/issues/43), EL5 [#13](https://github.com/trafgals/3s-decomp/issues/13).
- Ibuki: IB3 [#44](https://github.com/trafgals/3s-decomp/issues/44), IB4 [#45](https://github.com/trafgals/3s-decomp/issues/45), IB5 [#46](https://github.com/trafgals/3s-decomp/issues/46).
- Necro: NE1 [#47](https://github.com/trafgals/3s-decomp/issues/47), NE2 [#48](https://github.com/trafgals/3s-decomp/issues/48), NE3 [#49](https://github.com/trafgals/3s-decomp/issues/49).
- Alex: AL1 [#50](https://github.com/trafgals/3s-decomp/issues/50), AL2 [#51](https://github.com/trafgals/3s-decomp/issues/51), AL3 [#52](https://github.com/trafgals/3s-decomp/issues/52), AL5 [#53](https://github.com/trafgals/3s-decomp/issues/53), AL8 [#54](https://github.com/trafgals/3s-decomp/issues/54), AL9 [#55](https://github.com/trafgals/3s-decomp/issues/55), AL10 [#56](https://github.com/trafgals/3s-decomp/issues/56).
- Hugo: HU1 [#57](https://github.com/trafgals/3s-decomp/issues/57), HU4 [#58](https://github.com/trafgals/3s-decomp/issues/58), HU5 [#59](https://github.com/trafgals/3s-decomp/issues/59), HU6 [#60](https://github.com/trafgals/3s-decomp/issues/60).
- Q: Q7 [#61](https://github.com/trafgals/3s-decomp/issues/61), Q8 [#62](https://github.com/trafgals/3s-decomp/issues/62), Q9 [#63](https://github.com/trafgals/3s-decomp/issues/63), Q10 [#64](https://github.com/trafgals/3s-decomp/issues/64), Q11 [#65](https://github.com/trafgals/3s-decomp/issues/65).
- Remy: RM3 [#66](https://github.com/trafgals/3s-decomp/issues/66), RM5 [#67](https://github.com/trafgals/3s-decomp/issues/67), RM8 [#68](https://github.com/trafgals/3s-decomp/issues/68), RM9 [#69](https://github.com/trafgals/3s-decomp/issues/69), RM11 [#70](https://github.com/trafgals/3s-decomp/issues/70).
- Twelve: TW3 [#71](https://github.com/trafgals/3s-decomp/issues/71), TW4 [#72](https://github.com/trafgals/3s-decomp/issues/72), TW10 [#73](https://github.com/trafgals/3s-decomp/issues/73), TW11 [#74](https://github.com/trafgals/3s-decomp/issues/74), TW12 [#75](https://github.com/trafgals/3s-decomp/issues/75).
- Sean: SN2 [#76](https://github.com/trafgals/3s-decomp/issues/76), SN3 [#77](https://github.com/trafgals/3s-decomp/issues/77), SN4 [#78](https://github.com/trafgals/3s-decomp/issues/78), SN5 [#79](https://github.com/trafgals/3s-decomp/issues/79), SN7 [#80](https://github.com/trafgals/3s-decomp/issues/80), SN8 [#81](https://github.com/trafgals/3s-decomp/issues/81), SN9 [#82](https://github.com/trafgals/3s-decomp/issues/82), SN10 [#83](https://github.com/trafgals/3s-decomp/issues/83).
