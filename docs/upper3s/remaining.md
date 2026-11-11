# Third Strike Upper (3s') — itemised remaining work

As of 2026-11-11. Spec: [What I'd Change For Third Strike Upper (3s')](https://mikezsez.blogspot.com/2015/12/what-id-change-for-third-strike-upper-3s.html). Where things live in the source: [`AGENTS.md` §3](../../AGENTS.md).

<!-- summary:start -->
**134 work items** (plus 5 "no change" items). On `main`: all 134 open. On this branch: 28 verified in a fight test, 18 done but not yet measured, 8 partial, 80 not done.
<!-- summary:end -->

## How to read this

- **On `main`, nothing is implemented.** Every item below is open on `main`.
- **Branch** is the state of this branch (`ccr-579fa5e1-azpt7p`), which has PR #1 (`feat/auto-20261020-abd7ca61`, commit `ea1d880`) merged in, with the fixes listed under [Defects in PR #1](#defects-in-pr-1).
  - `verified`: implemented, and a fight test (`tests/match/`) measures the spec's change in the real engine.
  - `done`: implemented, not yet measured in a fight test. PR #1's own tests run on synthetic data only, and a fight test has already shown that synthetic tests can pass on a change that does not reach the game.
  - `partial`: part of the item is implemented.
  - `—`: not implemented.
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
| I4 | A way to change disc data | **On this branch** (from PR #1): `upper3s/upper3s_dump.c`, `upper3s/upper3s_chain.c`, `tools/upper3s/`. The fight tests found and this branch fixed that the hitstun/knockback overrides wrote to the wrong attack entry. The chain walker's startups agree with the spec's retail numbers on every move measured so far. |
| I5 | Specials are unnamed | **On this branch** (from PR #1): `tools/upper3s/decode_cmd.py` reads `cmd_data.c` and `bin2obj/asstbl.c`. The fight tests identify a move by its effect (`koc` and `cix` in the log). |
| I6 | Guarding the vanilla build without the game file | **On this branch** (from PR #1): `tools/verify/ps2_touched_sources.sh` compares the preprocessed PS2 sources with those of the retail-matched commit. |
| I7 | Leftover assembly (not a blocker) | Two game functions (`MTRANS` `mlt_obj_trans_rgb_ext`, `end_20` `sea_write`) and `EFFXX`'s two effect jump tables (`effmovejptbl`, `effinitjptbl`) are still included from assembly, and 6 CRI functions are not decompiled. They link as they are. |

## System mechanics

| # | Item | Kind | Where | Branch |
|:--|:--|:--|:--|:--|
| SM1 | Each successive whiffed normal halves the whiff meter gain (100%, 50%, 25%, 12.5%…); whiffed specials unchanged. The scaling resets when a move touches the opponent (hit/block/parry), or when you are hit, block or parry. Blocked projectiles reset nothing, projectiles that hit reset it, and parried projectiles reset it for the thrower only. | logic | `PLS02.c:add_sp_arts_gauge_init()` (the whiff gain); resets at the hit/block/parry gain functions | done |
| SM2 | +2f on the parry cooldown. Being hit during the cooldown by an attack parried the same way deals 25% extra damage (the crouching damage bonus). No bonus when thrown during the cooldown. Successful parries unchanged. | logic | `CMD_MAIN.c` parry input; `HITCHECK.c:defense_ground()`/`defense_sky()`; `Pow_Pow.c` | done |
| SM3 | "Guess parries" (input during a wakeup, a recovery roll or an air land-on-feet hitstun) are active half as long, with the same cooldown. A throw during that window or cooldown can't be teched. Parries during throw-tech animations unchanged. | logic | `HITCHECK.c`; `PLPCA.c:check_nagenuke()` | done |
| SM4 | Charge partitioning: as soon as a charged direction (4 or 2) reaches 1/3 charge, the partition timer resets to maximum. | logic | `CMD_MAIN.c` charge (`check_1()`/`check_2()`, `waza_work[][].w_int`) | done |
| SM5 | Left-right unblockables stay. A hit on a character in block animation who faces the other way scales that hit and its follow-up combo to 50%. | logic | `HITCHECK.c`; `Pow_Pow.c:cal_damage_vitality()` | done |

## Additional bugfixes

| # | Item | Kind | Where | Branch |
|:--|:--|:--|:--|:--|
| B1 | Successful throws are invincible for the whole animation (fixes the Remy/Urien glitch and the Q throw bug). | logic | `HITCHECK.c:catch_hit_check()`/`set_caught_status()`; `PLPCU.c` | done |
| B2 | Characters can move during the opponent's superflash if they touch a projectile on the frame it starts. | logic | `PLS01.c:sa_stop_check()`; `HITCHECK.c` | done |
| B3 | Universal overheads (MP+MK) and taunts (HP+HK) work while holding away or towards (today only throws do). | logic | `PLS03.c:check_leap_attack()`, `check_chouhatsu()` | done |
| B4 | Some characters (Makoto, etc.) can attack immediately after teching an air throw. | logic | `PLS00.c:nm_49000()`; `PLPCA.c:check_nagenuke()` | done |
| B5 | Rare move-interaction soft locks: X.F.L.A.T. vs EX Yagyou Dama / Aegis Reflector / Tengu Stone; Critical Combo Attack / Deadly Double Combination vs Magnetic Storm. (OTG on Dudley/Hugo after some throws is to stay.) | find | — | — |
| B6 | Wishlist: half-circle motions (41236, 63214) become quarter-circles, on the same button unless another special overlaps. Akuma's red fireball may stay. | table | `cmd_data.c` command tables | — |
| B7 | Characters passing through each other: the post says this is not fixable. | none | — | n/a |

## Characters

The post's order. The number after each name is the character index.

### Chun-Li (15)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| CL1 | Houyoku-sen (SA II): 1 stock instead of 2; bar the longest possible, as Shin-Shoryuken. `super_arts_data[15][1]` 104/2 → 128/1 | table | verified |
| CL2 | c.MK: cancel window no longer allows super-late cancels. | disc | — |
| CL3 | c.LP startup 2f → 3f. | disc | verified |
| CL4 | Tensei Ranka (SA III): startup hitbox reaches the floor (can't be crouched); pulls in and down so all hits connect vs airborne opponents; enough knockdown advantage for a crossup; 2 stocks; bar as long as Shippu's. `super_arts_data[15][2]` 72/3 → 80/2 | table + disc | partial (stocks and bar verified) |
| CL5 | Houyoku-sen's middle hits no longer un-combo vs crouching Makoto etc. | disc | — |
| CL6 | Goes through the mandatory standing frame after wakeup, like every other character. | find | — |
| CL7 | Crouching head hurtbox 2–3px higher and 3–4px further back, in every crouching animation (neutral, crouch-block, non-low-profile attacks). | disc | — |

### Ken (11)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| KN1 | Shippu Jinrai Kyaku (SA III): 2 stocks instead of 3; damage −15%. `super_arts_data[11][2]` 80/3 → 80/2 | table | verified |
| KN2 | Jab DP (623LP) damage 15 → 11.5. | disc | verified |
| KN3 | Target combo MP→HP: less knockback on grounded opponents from the 2nd hit, so DPs miss less after it. | disc | — |
| KN4 | Fix the crash when killing Makoto with a neutral throw (may not be present in this version). | find | — |

### Yun (3)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| YU1 | Genei Jin (SA III) no longer makes all his normals count as super hits for priority. | find | — (PR #1 says the flag is in the disc data) |
| YU2 | Genei Jin bar as long as Sourai Rengeki's; the super's timer is unchanged. `super_arts_data[3][2]` 72 → 88 | table | verified |
| YU3 | Sourai Rengeki (SA II): 2 stocks instead of 3. `super_arts_data[3][1]` 88/3 → 88/2 | table | verified |
| YU4 | Upkicks (623K) hitbox reaches further down. | disc | — |

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
| AK3 | KKZ (hidden super) motion D,D,D+PPP → D,D+PP. | table | — |
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
| RY1 | 6MP: less knockback on hit, so links after it are reliable. | disc | — |
| RY2 | Target combo far s.HP→s.HK: more forward movement on the 2nd hit. | disc | — |
| RY3 | More hitstun on far s.MK, s.HK, close s.HP, c.HP, c.LK and c.MK, so none is negative on hit. | disc | verified |
| RY4 | LK Joudan Sokutou Geri (41236LK): hitbox extends down so it connects after close s.HP / c.HP on everyone. | disc | — |
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
| EL1 | c.LK chains into itself. | disc | — |
| EL2 | Mallet Smash: startup frames unthrowable (7f are throwable today). | disc | — |
| EL3 | s.MP cancellable from the 2nd hit, not just the 1st. | disc | — |
| EL4 | Brave Dance juggles properly vs airborne opponents outside the corner. | disc | — |
| EL5 | Healing (SA III): faster cancel animation; faster health recovery. | disc + logic | partial (recovery rate, `PLPAT08.c`) |
| EL6 | Spinning Beat keeps its 3 stocks. | none | n/a |

### Ibuki (7)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| IB1 | Health 1035 → 1050 (Twelve's). `Com_Vital_Unit_Data[7]` | table | verified |
| IB2 | Yami Shigure (SA III): 2 stocks instead of 1. `super_arts_data[7][2]` 80/1 → 80/2 | table | verified |
| IB3 | Tsuiji Goe (623P): special- and super-cancellable almost straight after takeoff; a kunai thrown from it faces the right way if she hasn't passed over the opponent. | disc | — |
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
| AL4 | Target combo c.LK→c.MK: cancel window longer than 1f. | disc | — |
| AL5 | Juggles allowed after Flash Chops that hit an airborne opponent. | disc | — |
| AL6 | c.MP special- and super-cancellable on hit/block/parry. | logic | done |
| AL7 | c.MK super-cancellable on hit/block/parry, including in the target combo. | logic | done |
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
| Q1 | c.LP, s.LK and c.LK special- and super-cancellable on hit/block/parry. | logic | done |
| Q2 | c.MK super-cancellable on hit/block/parry. | logic | done |
| Q3 | All overhead/low rush punches ([4]6P/K, [P]) super-cancellable on hit/block/parry. | logic | — |
| Q4 | Far s.LP startup 6f → 4f. | disc | verified |
| Q5 | 4HK: cancel window extends past the first active frame. | disc | — |
| Q6 | More hitstun on c.LK, c.MK and s.HK (s.HK is −10 on hit). | disc | verified |
| Q7 | High Speed Barrage (slaps), all versions: change the opponent's movement on the middle hits so every hit connects. | disc | — |
| Q8 | EX slaps: first hit's hitbox extends down, to connect on crouchers. | disc | — |
| Q9 | New EX (PP) Total Destruction (SA III) that grabs immediately on activation. | new | — |
| Q10 | Punch Total Destruction can't trade hits. | disc | — |
| Q11 | QCT+P deactivates Total Destruction. | new | — |
| Q12 | Taunt health bonus for 1–3 taunts: 1200 base/1400/1705/2050 → 1200/1400/1600/1800. | logic | done |

### Remy (19)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| RM1 | Stun bar short → regular. `pl_piyo_tbl[19]` 56 → 64 | table | verified |
| RM2 | c.MP damage +6 (nearly Ryu's). | disc | verified |
| RM3 | Close s.HP: the earlier hitboxes knock airborne opponents down like the later ones, with juggles after. | disc | — |
| RM4 | More hitstun on far s.MK, far s.HK, c.MK and 6MK, so none is negative on hit. | disc | verified |
| RM5 | s.MK: less knockback on grounded opponents. Target combo s.MK→s.HK: bigger hitboxes on the 2nd hit. | disc | — |
| RM6 | Chains c.LP→c.LK and c.LK→c.LP. | disc | — |
| RM7 | Taunt length 131f → about 80f. | disc | — |
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
| TW4 | 4MK: activate the earlier low hitboxes that exist but can't connect. | disc | — |
| TW5 | Far s.MP, s.MP and c.MP special- and super-cancellable on hit/block/parry. | logic | done |
| TW6 | c.MP superjump-cancellable on hit/block, like s.MK. | logic | — |
| TW7 | c.MK super-cancellable on hit/block/parry. | logic | done |
| TW8 | Jab A.X.E. (214LP): more hit and block advantage, at most −1/−2 on block. | disc | — |
| TW9 | All air normals special-cancellable on hit/block into A.X.E. (j.214P) or D.R.A. (j.214K), and super-cancellable into X.F.L.A.T. | logic | done |
| TW10 | X.N.D.L.: drags juggled opponents down so more hits connect. | disc | — |
| TW11 | X.F.L.A.T. (SA II): remove the low-damage air-to-air version (always full damage); 2 stocks. `super_arts_data[18][1]` 112/1 → 112/2 | table + disc | partial (2 stocks verified) |
| TW12 | X.C.O.P.Y. (SA III): EX moves (maybe even the opponent's super) by spending chunks of the super's timer; startup 12f → 1f; when hit in its recovery he takes 66% damage instead of extra damage. | new + disc | — (startup in progress; its chain loops) |

### Sean (12)

| # | Item | Kind | Branch |
|:--|:--|:--|:--|
| SN1 | More hitstun on close and far s.HP, c.HP, s.MK, far s.HK, c.MK and 6HP, so none is negative on hit. | disc | verified |
| SN2 | Sean Roll (214P) super-cancellable in the middle. | disc | — |
| SN3 | Ryuubi Kyaku (236K) startup 25f (all) → 17/19/21f; the HK version knocks down. | disc | partial (all three 25 → 19 measured; one shared chain, and no knockdown) |
| SN4 | Ryuubi Kyaku L/M: more hit advantage, so supers link after a hit. | disc | — |
| SN5 | Faster startup on all Tackles: c.MK xx LP Tackle and s.HP/c.HP xx MP Tackle combo, and HP Tackle is 1f from comboing. | disc | — |
| SN6 | HP Tackle can be faked/cancelled by tapping K during its startup. | new | — |
| SN7 | HP and EX Tackle not ground-parryable. | disc | — |
| SN8 | EX Tackle (236PP~P): hitbox active from 8f to the end, grabs automatically on contact; super-cancellable on the last hit. | disc + new | — |
| SN9 | Dragon Smash (623P) L/M/H/EX startup 5/5/9/8 → 3/4/5/3f. | disc | partial (3/4/5 measured; EX 8 → 5, not 3) |
| SN10 | Hyper Tornado (SA III) juggles properly vs airborne opponents. | disc | — |
| SN11 | Hadou Burst recovery 61f → 51f. | disc | — |

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

### Still open

1. **HU5: check the move.** PR #1 reads the Meat Squasher's startup from the disc as 24/52/67, against the post's 21/31/44; light is close, medium and heavy are far off.
2. **SN3 and SN9 are partial.** The three Ryuubi Kyaku share one chain, so all three are 19 frames (the spec asks 17/19/21) and the Roundhouse knockdown is not done. EX Dragon Smash is 5 frames (the spec asks 3), because the chain cannot be held shorter.
3. **Documentation errors.** Issue #2 says `plpat16.c` is Makoto (it is Chun-Li; Makoto is `plpat17.c`, AGENTS.md §3.2). PR #1's `spec-map.md` says the whiff gain is `add_sp_arts_gauge_guard()` (that is the block gain; the code hooks `add_sp_arts_gauge_init()`). Chun-Li's table header calls SA I "Houyoku-sen" (it is Kikoushou). PR #1's table for Q labels entry 1 as "6LP"; in the game it is the far standing Jab.
