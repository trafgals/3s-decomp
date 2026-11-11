# Third Strike Upper (3s') — itemised remaining work

As of 2026-11-11. Spec: [What I'd Change For Third Strike Upper (3s')](https://mikezsez.blogspot.com/2015/12/what-id-change-for-third-strike-upper-3s.html). Where things live in the source: [`AGENTS.md` §3](../../AGENTS.md).

<!-- summary:start -->
**134 work items** (plus 5 "no change" items). On `main`: all 134 open. On PR #1: 45 done, 8 partial, 81 not done.
<!-- summary:end -->

## How to read this

- **On `main`, nothing is implemented.** Every item below is open on `main`.
- **PR #1** is the unmerged branch `feat/auto-20261020-abd7ca61` (one commit, `ea1d880`). This column shows what that branch implements, taken from its `docs/upper3s/spec-map.md` and code. Its 392 unit tests pass, but they run on synthetic data; nothing has been checked in a match.
  - `done`: implemented there.
  - `partial`: part of the item is implemented.
  - `—`: not implemented there.
  - ⚠: implemented wrongly, or needs checking (see [Defects in PR #1](#defects-in-pr-1)).

  If the branch is deleted, the commit stays reachable as `refs/pull/1/head` while the pull request exists.
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

These come first: most items below can't be built, tested or played without them.

| # | Issue | Notes |
|:--|:--|:--|
| I1 | No 3s' build | Nothing in the Makefile defines `UPPER_3S`, and `make build` always byte-compares against `THIRD_U.BIN`. This needs a target (e.g. `make build UPPER_3S=1`) that adds `-DUPPER_3S` and skips the compare. Its output is a PS2 ELF. |
| I2 | Nothing to play it on | Upstream removed the PC port (#303), so a 3s' ELF only runs in a PS2 emulator such as PCSX2. PR #1 carries its own SDL/WebAssembly port in the same commit as the patch. |
| I3 | No test harness | AGENTS.md §2.2 requires a test per change; `main` has none. PR #1 has `tests/upper3s/` and `make test`. |
| I4 | No way to change disc data | Most character items change frame data loaded from the disc. This needs three things:<br>1. A dump of the retail values per move.<br>2. A hook that patches a character's data after it loads.<br>3. A walker for the animation chains. The chains branch and call each other, and some moves share one (Sean's L/M/H Ryuubi Kyaku are a single chain).<br>PR #1 has all three: `upper3s/upper3s_dump.c`, `upper3s/upper3s_chain.c` and `tools/upper3s/`. |
| I5 | Specials are unnamed | The source doesn't say which special is which. A move is found from its command in `cmd_data.c` and the assignment table in `bin2obj/asstbl.c`. PR #1 has a decoder, `tools/upper3s/decode_cmd.py`. |
| I6 | Guarding the vanilla build without the game file | `make build` proves the vanilla build still matches, but needs the retail `THIRD_U.BIN`, so it can't run in CI. PR #1 has a check that compares preprocessed sources instead (`tools/verify/ps2_touched_sources.sh`). |
| I7 | Leftover assembly (not a blocker) | Two game functions (`MTRANS` `mlt_obj_trans_rgb_ext`, `end_20` `sea_write`) and `EFFXX`'s two effect jump tables (`effmovejptbl`, `effinitjptbl`) are still included from assembly, and 6 CRI functions are not decompiled. They link as they are. |

## System mechanics

| # | Item | Kind | Where | PR #1 |
|:--|:--|:--|:--|:--|
| SM1 | Each successive whiffed normal halves the whiff meter gain (100%, 50%, 25%, 12.5%…); whiffed specials unchanged. The scaling resets when a move touches the opponent (hit/block/parry), or when you are hit, block or parry. Blocked projectiles reset nothing, projectiles that hit reset it, and parried projectiles reset it for the thrower only. | logic | `PLS02.c:add_sp_arts_gauge_init()` (the whiff gain); resets at the hit/block/parry gain functions | done |
| SM2 | +2f on the parry cooldown. Being hit during the cooldown by an attack parried the same way deals 25% extra damage (the crouching damage bonus). No bonus when thrown during the cooldown. Successful parries unchanged. | logic | `CMD_MAIN.c` parry input; `HITCHECK.c:defense_ground()`/`defense_sky()`; `Pow_Pow.c` | done |
| SM3 | "Guess parries" (input during a wakeup, a recovery roll or an air land-on-feet hitstun) are active half as long, with the same cooldown. A throw during that window or cooldown can't be teched. Parries during throw-tech animations unchanged. | logic | `HITCHECK.c`; `PLPCA.c:check_nagenuke()` | done |
| SM4 | Charge partitioning: as soon as a charged direction (4 or 2) reaches 1/3 charge, the partition timer resets to maximum. | logic | `CMD_MAIN.c` charge (`check_1()`/`check_2()`, `waza_work[][].w_int`) | done |
| SM5 | Left-right unblockables stay. A hit on a character in block animation who faces the other way scales that hit and its follow-up combo to 50%. | logic | `HITCHECK.c`; `Pow_Pow.c:cal_damage_vitality()` | done |

## Additional bugfixes

| # | Item | Kind | Where | PR #1 |
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

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| CL1 | Houyoku-sen (SA II): 1 stock instead of 2; bar the longest possible, as Shin-Shoryuken. `super_arts_data[15][1]` 104/2 → 128/1 | table | done |
| CL2 | c.MK: cancel window no longer allows super-late cancels. | disc | — |
| CL3 | c.LP startup 2f → 3f. | disc | done |
| CL4 | Tensei Ranka (SA III): startup hitbox reaches the floor (can't be crouched); pulls in and down so all hits connect vs airborne opponents; enough knockdown advantage for a crossup; 2 stocks; bar as long as Shippu's. `super_arts_data[15][2]` 72/3 → 80/2 | table + disc | partial (stocks and bar) |
| CL5 | Houyoku-sen's middle hits no longer un-combo vs crouching Makoto etc. | disc | — |
| CL6 | Goes through the mandatory standing frame after wakeup, like every other character. | find | — |
| CL7 | Crouching head hurtbox 2–3px higher and 3–4px further back, in every crouching animation (neutral, crouch-block, non-low-profile attacks). | disc | — |

### Ken (11)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| KN1 | Shippu Jinrai Kyaku (SA III): 2 stocks instead of 3; damage −15%. `super_arts_data[11][2]` 80/3 → 80/2 | table | done |
| KN2 | Jab DP (623LP) damage 15 → 11.5. | disc | done |
| KN3 | Target combo MP→HP: less knockback on grounded opponents from the 2nd hit, so DPs miss less after it. | disc | — |
| KN4 | Fix the crash when killing Makoto with a neutral throw (may not be present in this version). | find | — |

### Yun (3)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| YU1 | Genei Jin (SA III) no longer makes all his normals count as super hits for priority. | find | — (PR #1 says the flag is in the disc data) |
| YU2 | Genei Jin bar as long as Sourai Rengeki's; the super's timer is unchanged. `super_arts_data[3][2]` 72 → 88 | table | done |
| YU3 | Sourai Rengeki (SA II): 2 stocks instead of 3. `super_arts_data[3][1]` 88/3 → 88/2 | table | done |
| YU4 | Upkicks (623K) hitbox reaches further down. | disc | — |

### Makoto (16)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| MK1 | Abare Tosanami (SA II) stun −50%. | disc | done |
| MK2 | Fukiage L/M/H stun 29/31/31 → 27/28/31. | disc | done |
| MK3 | Tanden Renki (SA III): 236K while active deactivates it. It counts as a special, uses her post-Hayate taunt animation, and can be cancelled into attacks (not block or movement) at any time. | new | — |
| MK4 | Karakusa's startup no longer auto throw-techs when she's thrown out of it. | find | — |
| MK5 | Seichusen Godanzuki (SA I): more vacuum and hitstun on the first hit. | disc | — |
| MK6 | EX Oroshi (214PP) knocks down when it hits a limb hurtbox. | disc | — |
| MK7 | Hayate (236[P]): one startup/hold frame has a throwable box too small for its pushbox. | disc | — |
| MK8 | No invincibility on the recovery after a successful Karakusa (63214K); the neutral throw is unchanged. | disc | — |
| MK9 | EX Oroshi xx Abare Tosanami on Hugo: no change. | none | n/a |

### Dudley (4)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| DD1 | Rocket Uppercut (SA I): less stun. | disc | done |
| DD2 | Rolling Thunder (SA II): more stun. | disc | done |
| DD3 | Rocket Uppercut's character point: knocks opponents backwards less often in juggles. | disc | — |
| DD4 | Corkscrew Blow vacuums the opponent in on hit. | disc | — |
| DD5 | Rolling Thunder turns around if he dashes again and passes under the opponent. | logic | — |
| DD6 | Cross Counter connects against invincible moves instead of whiffing after taking the damage. | find | — |

### Urien (13)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| UR1 | Tyrant Slaughter/Punish (SA I): fully connects vs crouching opponents and in most juggles; damage +25% (as Shin Shoryuken); 1 stock instead of 2; bar slightly longer. `super_arts_data[13][0]` 104/2 | table + disc | partial (stocks, bar, damage) |

### Akuma (14)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| AK1 | Stun bar short → regular. `pl_piyo_tbl[14]` 56 → 64 | table | done |
| AK2 | Messatsu Gou Rasen (SA II) bar shorter than SA I and SA III. `super_arts_data[14][1]` 112; SA I and SA III are also 112 | table | done |
| AK3 | KKZ (hidden super) motion D,D,D+PPP → D,D+PP. | table | — |
| AK4 | KKZ damage 107 → 95. | disc | done |
| AK5 | 6MP stays throw-invincible. | none | n/a |

### Yang (10)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| YA1 | Target combo 3 (j.MK→3MK): the divekick's trajectory hits more often; the backward-jump version lands faster. | disc | — |
| YA2 | Close s.MK sometimes doesn't launch. | disc | — |
| YA3 | Yun/Yang can end up behind Yang after Mantis Slashes (236HP×3) with certain timing (probably his pushbox). | disc | — |
| YA4 | Sei'ei Enbu keeps the shortest bar in the game. | none | n/a |

### Ryu (2)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| RY1 | 6MP: less knockback on hit, so links after it are reliable. | disc | — |
| RY2 | Target combo far s.HP→s.HK: more forward movement on the 2nd hit. | disc | — |
| RY3 | More hitstun on far s.MK, s.HK, close s.HP, c.HP, c.LK and c.MK, so none is negative on hit. | disc | done |
| RY4 | LK Joudan Sokutou Geri (41236LK): hitbox extends down so it connects after close s.HP / c.HP on everyone. | disc | — |
| RY5 | LP/MP DP knock opponents up less, and so do Shinkuu Hadoken's (SA I) middle hits, so the whole super connects after a DP. | disc | — |

### Oro (9)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| OR1 | Jinchu Nobori (chicken kick): pushboxes, so [s.MP xx chicken kick]×2 works on every character. | disc | — |
| OR2 | Oniyama ([2]8P): more hitstop and juggle time, so super cancels from it are useful. | disc | — |
| OR3 | New forward throw (6+LP+LK): the back throw's animation mirrored, about half the distance. | new | — |
| OR4 | Neutral throw can't be escaped before 3–4 hits, like other mashable grabs. | find | — |
| OR5 | Kishin Riki (SA I): bar as long as Tengu Stone's. `super_arts_data[9][0]` 104 → 112 | table | done |
| OR6 | Kishin Riki: Human Pillar Driver (63214P) stays a hit-grab during it, so it combos; more damage on the air grab when it's combo'd into; QCT+P deactivates it with a shortened activation animation. | new | — |

### Elena (8)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| EL1 | c.LK chains into itself. | disc | — |
| EL2 | Mallet Smash: startup frames unthrowable (7f are throwable today). | disc | — |
| EL3 | s.MP cancellable from the 2nd hit, not just the 1st. | disc | — |
| EL4 | Brave Dance juggles properly vs airborne opponents outside the corner. | disc | — |
| EL5 | Healing (SA III): faster cancel animation; faster health recovery. | disc + logic | partial (recovery rate, `PLPAT08.c`) |
| EL6 | Spinning Beat keeps its 3 stocks. | none | n/a |

### Ibuki (7)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| IB1 | Health 1035 → 1050 (Twelve's). `Com_Vital_Unit_Data[7]` | table | done |
| IB2 | Yami Shigure (SA III): 2 stocks instead of 1. `super_arts_data[7][2]` 80/1 → 80/2 | table | done |
| IB3 | Tsuiji Goe (623P): special- and super-cancellable almost straight after takeoff; a kunai thrown from it faces the right way if she hasn't passed over the opponent. | disc | — |
| IB4 | The target-combo s.HP (4MP→HP and LP→MP→HP) never knocks down. Today one of its two hitboxes is marked knockdown and the other isn't. | disc | — |
| IB5 | Taunt: can't be throw-teched, and she can parry or attack in the air once the opponent recovers. | logic | partial (throw tech) |

### Necro (5)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| NE1 | Slam Dance (SA II) startup 0+2 → 2+0: same total, but it can't be jumped on reaction after the superflash. | disc | — |
| NE2 | Electric Shock (623P): fix its pushbox; slight vacuum on the middle hits. | disc | — |
| NE3 | New EX Electric Shock (possibly juggles after; safer on block than the HP version). | new | — |

### Alex (1)

| # | Item | Kind | PR #1 |
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

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| HU1 | Monster Lariat (236K) super-cancellable on hit, and supers juggle after it. | logic + disc | — |
| HU2 | EX Giant Palm Bomber (63214PP) startup 22f → 17f. | disc | done |
| HU3 | c.LP and c.LK hitstun +1–2f. | disc | done |
| HU4 | New EX Shootdown Backbreaker that allows juggles; distance chosen with LK+MK / MK+HK / LK+HK. | new | — |
| HU5 | Meat Squasher (360K): startup 21/31/44 → 18/24/30; whiff recovery 30/33/36 → 26/29/32. | disc | done ⚠ |
| HU6 | Frame 1 of landing from air hitstun: fix the offset hitboxes. | disc | — |

### Q (17)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| Q1 | c.LP, s.LK and c.LK special- and super-cancellable on hit/block/parry. | logic | done |
| Q2 | c.MK super-cancellable on hit/block/parry. | logic | done |
| Q3 | All overhead/low rush punches ([4]6P/K, [P]) super-cancellable on hit/block/parry. | logic | — |
| Q4 | Far s.LP startup 6f → 4f. | disc | done ⚠ |
| Q5 | 4HK: cancel window extends past the first active frame. | disc | — |
| Q6 | More hitstun on c.LK, c.MK and s.HK (s.HK is −10 on hit). | disc | done |
| Q7 | High Speed Barrage (slaps), all versions: change the opponent's movement on the middle hits so every hit connects. | disc | — |
| Q8 | EX slaps: first hit's hitbox extends down, to connect on crouchers. | disc | — |
| Q9 | New EX (PP) Total Destruction (SA III) that grabs immediately on activation. | new | — |
| Q10 | Punch Total Destruction can't trade hits. | disc | — |
| Q11 | QCT+P deactivates Total Destruction. | new | — |
| Q12 | Taunt health bonus for 1–3 taunts: 1200 base/1400/1705/2050 → 1200/1400/1600/1800. | logic | done |

### Remy (19)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| RM1 | Stun bar short → regular. `pl_piyo_tbl[19]` 56 → 64 | table | done |
| RM2 | c.MP damage +6 (nearly Ryu's). | disc | done |
| RM3 | Close s.HP: the earlier hitboxes knock airborne opponents down like the later ones, with juggles after. | disc | — |
| RM4 | More hitstun on far s.MK, far s.HK, c.MK and 6MK, so none is negative on hit. | disc | done |
| RM5 | s.MK: less knockback on grounded opponents. Target combo s.MK→s.HK: bigger hitboxes on the 2nd hit. | disc | — |
| RM6 | Chains c.LP→c.LK and c.LK→c.LP. | disc | — |
| RM7 | Taunt length 131f → about 80f. | disc | — |
| RM8 | Light of Virtue meter gain on startup/hit/block 0/1/1 → 1/3/2. | table + disc | — |
| RM9 | Rising Rage Flash: all strengths super-cancellable on hit/block/parry; juggles after all versions. | logic + disc | — |
| RM10 | Cold Blue Kick (63214K): LK and MK versions not negative on hit up close; MK version +2. | disc | — ⚠ |
| RM11 | Blue Nocturne (SA III): superflash only after a successful counter; meter spent on the counter, the recovery or being thrown, not on activation; bar a bit longer; juggles/knocks down airborne opponents. `super_arts_data[19][2]` 80/1 | table + logic + disc | partial (bar, 80 → 96) |

### Twelve (18)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| TW1 | Health 1050 → 1120. `Com_Vital_Unit_Data[18]` | table | done |
| TW2 | More hitstun on s.MP, far s.MP, s.HP, c.HP, s.LK, s.MK, s.HK, c.MK and 4MK, so none is negative on hit. | disc | done |
| TW3 | c.MP and s.MK parryable high only; c.HP and 4MK parryable low only. | disc | — |
| TW4 | 4MK: activate the earlier low hitboxes that exist but can't connect. | disc | — |
| TW5 | Far s.MP, s.MP and c.MP special- and super-cancellable on hit/block/parry. | logic | done |
| TW6 | c.MP superjump-cancellable on hit/block, like s.MK. | logic | — |
| TW7 | c.MK super-cancellable on hit/block/parry. | logic | done |
| TW8 | Jab A.X.E. (214LP): more hit and block advantage, at most −1/−2 on block. | disc | — |
| TW9 | All air normals special-cancellable on hit/block into A.X.E. (j.214P) or D.R.A. (j.214K), and super-cancellable into X.F.L.A.T. | logic | done |
| TW10 | X.N.D.L.: drags juggled opponents down so more hits connect. | disc | — |
| TW11 | X.F.L.A.T. (SA II): remove the low-damage air-to-air version (always full damage); 2 stocks. `super_arts_data[18][1]` 112/1 → 112/2 | table + disc | partial (stocks) |
| TW12 | X.C.O.P.Y. (SA III): EX moves (maybe even the opponent's super) by spending chunks of the super's timer; startup 12f → 1f; when hit in its recovery he takes 66% damage instead of extra damage. | new + disc | — (startup in progress; its chain loops) |

### Sean (12)

| # | Item | Kind | PR #1 |
|:--|:--|:--|:--|
| SN1 | More hitstun on close and far s.HP, c.HP, s.MK, far s.HK, c.MK and 6HP, so none is negative on hit. | disc | done |
| SN2 | Sean Roll (214P) super-cancellable in the middle. | disc | — |
| SN3 | Ryuubi Kyaku (236K) startup 25f (all) → 17/19/21f; the HK version knocks down. | disc | partial ⚠ |
| SN4 | Ryuubi Kyaku L/M: more hit advantage, so supers link after a hit. | disc | — |
| SN5 | Faster startup on all Tackles: c.MK xx LP Tackle and s.HP/c.HP xx MP Tackle combo, and HP Tackle is 1f from comboing. | disc | — |
| SN6 | HP Tackle can be faked/cancelled by tapping K during its startup. | new | — |
| SN7 | HP and EX Tackle not ground-parryable. | disc | — |
| SN8 | EX Tackle (236PP~P): hitbox active from 8f to the end, grabs automatically on contact; super-cancellable on the last hit. | disc + new | — |
| SN9 | Dragon Smash (623P) L/M/H/EX startup 5/5/9/8 → 3/4/5/3f. | disc | partial ⚠ |
| SN10 | Hyper Tornado (SA III) juggles properly vs airborne opponents. | disc | — |
| SN11 | Hadou Burst recovery 61f → 51f. | disc | — |

### Gill (0)

No changes in the post.

## Defects in PR #1

These matter if any of PR #1 is kept.

1. **One commit holds both the balance patch and the web port.** To keep the patch without the browser build, the commit has to be split. Deleting the branch deletes the patch too.
2. **RM10 was applied to Chun-Li.** Remy's Cold Blue Kick change is in Chun-Li's override table (`upper3s/pl14.c`, entries 28 and 29 of her attack slot, labelled "Cold Blue Kick"). `tests/upper3s/test_upper3s_ch_hitstun.c:141` asserts it. So Chun-Li gets a hitstun change the post never asks for, and Remy's table (`upper3s/pl18.c`) still lists the item as a TODO.
3. **HU5: check the move.** PR #1 reads the Meat Squasher's startup from the disc as 24/52/67, against the post's 21/31/44. Light is close, but medium and heavy are far off. Check in a match that the change landed on the right move.
4. **Interpretations to confirm:**
   - Q4: PR #1 changed 6LP/4LP (which share one chain) as the "far standing Jab".
   - SN3: the three Ryuubi Kyaku share one chain, so all three are set to 19f, and the knockdown isn't done.
   - SN9: EX Dragon Smash is set to 5f, not 3f, because PR #1's chain walker can't go shorter.
5. **Documentation errors:**
   - Issue #2 says `plpat16.c` is Makoto. It's Chun-Li; Makoto is `plpat17.c` (AGENTS.md §3.2).
   - Its `spec-map.md` says the whiff gain is `add_sp_arts_gauge_guard()`. That function is the *block* gain; the code correctly hooks `add_sp_arts_gauge_init()`.
   - Its AGENTS.md still names `PLS11.c`, `PLS15.c`, `PLS18.c` and `passxx.c`.
   - Chun-Li's table header calls SA I "Houyoku-sen". SA I is Kikoushou.
