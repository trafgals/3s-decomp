# AGENTS.md — Street Fighter III: 3rd Strike Decompilation & 3S Upper Protocol

This document governs autonomous and pair-programming agents (Antigravity, MiniMax `mcode`, local LLMs on Strata) working on the **trafgals/3s-decomp** codebase. All agents must adhere to these directives.

---

## 1. Mission & Purpose

`trafgals/3s-decomp` is a fork of [`crowded-street/3s-decomp`](https://github.com/crowded-street/3s-decomp), which decompiled the game logic of *Street Fighter III: 3rd Strike* (PlayStation 2 `THIRD_U.BIN`) into matching C source code. What is left: two game functions are still assembly (`INCLUDE_ASM`: `MTRANS` `mlt_obj_trans_rgb_ext`, `end_20` `sea_write`), the two effect jump tables of `EFFXX` are still included as data (`INCLUDE_RODATA`: `effmovejptbl`, `effinitjptbl`), and six CRI middleware functions are not decompiled (`cri-progress.md`).

The primary objective of this fork is to implement the definitive **"Third Strike Upper (3s')"** balance overhaul and system bugfix patch engineered by fighting game designer **Mike Zaimont (Mike Z)**:
> 🔗 **Design Specification**: [What I'd Change For Third Strike Upper (3s')](https://mikezsez.blogspot.com/2015/12/what-id-change-for-third-strike-upper-3s.html)

The design philosophy is: **change the game as little as possible** while eliminating legacy engine bugs, leveling dominant top-tier outliers, and making every character and Super Art viable in competitive tournament play.

---

## 2. Multi-Agent Hierarchy & Delegation Contract

### 2.1 The Mandatory Subagent Audit Rule
The parent agent **MUST NEVER** blindly accept or skim a subagent's return message.
When a subagent reports back:
1. **Analyze the Report**: Scrutinize the subagent's summary for:
   - Any bugs, edge cases, or traps the subagent claimed to have uncovered or resolved.
   - Any unhandled frame data, hitbox boundaries, or register side effects.
   - Any deviations from the target branch.
2. **Inspect the Code Diff**: Audit the actual changes on disk (`git diff`, `git status`, or reading the file):
   - Confirm whether claimed balance changes actually exist in C source.
   - Ensure the fixes adhere to Capcom's original data structures and do not introduce memory corruption.
3. **Remediate and Rework**: If any partial implementations or regressions remain, the parent agent **MUST immediately rework and fix them** before proceeding.
4. **Merge and Push to `main`**: All completed and verified work must land directly on `main` and be pushed to `origin/main`.

### 2.2 The Mandatory Functional Verification Rule (Every User Request)
For **every single user request**, the agent **MUST** write and execute an automated functional test verifying the exact feature or fix requested before concluding the turn or presenting results to the user.
- **Zero Assumptions**: Never assume code or configuration works based solely on static edits, syntax checks, or build passes.
- **Automated Verification**: Implement and run a test script or harness that exercises the exact functionality end-to-end.
- **Failure Remediation**: If the test fails or uncovers regressions, diagnose the root cause immediately, fix the implementation, and re-run verification until 100% passing.

### 2.3 Execution Directives
- **Zero Background Polling**: Never run busy-wait loops, sleep loops, or status-polling scripts in shells or subagents. Rely on reactive system notifications when commands complete.
- **Always Ship on `main`**: Working trees must be left clean and synchronized with `origin/main`.
- **Preserve Documentation & Comments**: Maintain existing Japanese Capcom comments and naming conventions. Do not delete explanatory context.
- **Comment What You Work Out**: When you establish something non-obvious about the code (what a table is indexed by, which function does what, a numbering quirk, what a packed field holds), write it as a short comment where it applies in the source, as well as in the docs. Say what is confirmed and what is a guess. Comments do not change the matching build; `tools/check_comments_only.py` proves an edit is comment-only.

---

## 3. Codebase Architecture & Subsystem Mapping

Every file, symbol and value named in this section is checked against the source by `tools/check_agents_md.py`. Run it after editing this section.

- Game logic: `src/anniversary/sf33rd/Source/Game/`
- Tables converted from binary data: `src/anniversary/bin2obj/`
- Shared structs: `include/structs.h`; per-file headers: `include/sf33rd/Source/Game/`

### 3.1 Character Index Map

This is the index the game uses: `PLW.player_number`, `My_char[]`, and every `[20]` per-character table (`PLCNT.c:super_arts_data`, `PLCNT.c:pl_piyo_tbl`, `Com_Data.c:Com_Vital_Unit_Data`, `bin2obj/gauge.c:_add_arts_gauge`, …). Two tables in the source give the order: the CPU table `Ck_Pass.c:Passive_AS_tbl` (`VS_GILL_AS`, `VS_ALEX_AS`, `VS_RYU_AS`, …; Yang, 10, reuses `VS_YUN_AS`) and the stage names in `Eff99.c` (`"ALEX STAGE"` … `"REMY STAGE"`; Gill and Q are blank). The retail values agree with it: `super_arts_data[11]` gives Ken's Shippu Jinrai Kyaku its 3 stocks, and `pl_piyo_tbl[14]` (Akuma) and `pl_piyo_tbl[19]` (Remy) are the two short stun bars.

| ID | Character | ID | Character | ID | Character | ID | Character |
|:---:|:---|:---:|:---|:---:|:---|:---:|:---|
| `00` | Gill | `05` | Necro | `10` | Yang | `15` | Chun-Li |
| `01` | Alex | `06` | Hugo | `11` | Ken | `16` | Makoto |
| `02` | Ryu | `07` | Ibuki | `12` | Sean | `17` | Q |
| `03` | Yun | `08` | Elena | `13` | Urien | `18` | Twelve |
| `04` | Dudley | `09` | Oro | `14` | Gouki (Akuma) | `19` | Remy |

The character data loaded from disc is keyed by a different number, the charset id: `charset_id = PLCNT.c:plid_data[index]`.

### 3.2 Per-character files and their numbering

There are **no per-character `PLSxx.c` files**. `PLS00.c`–`PLS03.c` are shared engine code (3.3). The per-character sources are the three families below, and they do not all number the same way. File-name case varies (`PLPAT09.c` / `plpat10.c`, `PASS11.c` / `pass10.c`).

| Files | What they contain | Numbering |
|:---|:---|:---|
| `passNN.c` | CPU opponent AI ("Passive" patterns), dispatched by `Com_Pl.c:Com_Passive()`. No frame data, hitboxes or damage. | `NN` = index (`pass16.c` is Makoto) |
| `plpatNN.c` | Character-specific special and super behaviour: `plNN_extra_attack()` and the `Att_*` routines, dispatched by `PLPAT.c:plxx_extra_attack_table` | `NN` = index for 0–14, **index + 1 for 15–19** (no file is numbered 15): Chun-Li `plpat16.c`, Makoto `plpat17.c`, Q `plpat18.c`, Twelve `plpat19.c`, Remy `plpat20.c` |
| `pX_cmd` in `cmd_data.c` | Command (motion) tables, listed in index order in `cmd_data.c:pl_cmd[20]` | `p0`–`p9` and `pA`–`pE` for 0–14, `p10`–`p14` for 15–19 (Makoto is `p11_cmd`) |

### 3.3 Where things live

**Per-character values that are in the source**

| What | Where |
|:---|:---|
| Super art stocks and bar length | `PLCNT.c:super_arts_data[20][4]`: `SA_DATA.store_max` (stocks) and `SA_DATA.gauge_len`. `super_arts_DATA` is read instead when `cmd_sel[ix]` or `no_sa[ix]` is set. |
| Stun bar size | `PLCNT.c:pl_piyo_tbl[20]` (56 short, 64 regular, 72 long) |
| Stun recovery | `PLCNT.c:pl_nr_piyo_tbl[20]` |
| Health | `Com_Data.c:Com_Vital_Unit_Data[20][4][12]`. It is read by `PLS02.c:setup_vitality()` for every player, despite the `Com` prefix. |
| Meter gain | `bin2obj/gauge.c:_add_arts_gauge[20][16][4]`, row = the attack's `add_arts_point`. Column 0 is the gain when an attack starts, whether or not it connects (the whiff gain): `PLS02.c:add_sp_arts_gauge_init()`, called from `CHARSET.c` as the attack frame loads. Column 1 is on block (`add_sp_arts_gauge_guard()`), 2 on hit (`add_sp_arts_gauge_hit_dm()`), 3 on parry (`add_sp_arts_gauge_paring()`). |
| Stun per hit | `bin2obj/gauge.c:_add_piyo_gauge[20][16]`, indexed by the attack's `att.piyo` (`HITCHECK.c:set_damage_and_piyo()`) |
| Damage | `POW_DATA.c:Power_Data[256]`, indexed by the attack data; the calculation and scaling are in `Pow_Pow.c:cal_damage_vitality()` |

**Shared player engine**

| File | Role |
|:---|:---|
| `PLCNT.c` | Player control (`Player_control()`) and the per-character tables above |
| `PLMAIN.c`, `PLMAIN2.c` | Per-frame player update (`Player_move()`) |
| `PLS00.c` | Normal-state machine (`nm_*`, e.g. the air-throw-tech state `nm_49000()`), lever checks, cancel checks (`check_cg_cancel_data()`) |
| `PLS01.c` | Superflash stop (`sa_stop_check()`), tech-roll (`ukemi`) checks |
| `PLS02.c` | Movement data (`setup_mvxy_data()`), meter gain (`add_sp_arts_gauge_*()`), health setup (`setup_vitality()`) |
| `PLS03.c` | Special and super command checks (`hissatsu_setup_union()`, `check_super_arts_attack()`); universal overhead and taunt input (`check_leap_attack()`, `check_chouhatsu()`) |
| `PLPNM.c`, `PLPDM.c`, `PLPCA.c`, `PLPCU.c` | Player state routines: normal, damage, throwing (`Player_catch()`, and the throw tech `check_nagenuke()`), being thrown (`Player_caught()`) |
| `PLPAT.c`, `PLPATUNI.c` | Attack-state dispatch, and special-move routines shared by several characters (`Att_HADOUKEN`, …) |
| `CMD_MAIN.c` | Command interpreter: motion checks (`check_0()`, …); charge (`tame`) in `check_1()`/`check_2()`; input timers `waza_work[][].w_int` (declared in `cmd_data.c`); the parry input `check_10()` (parries are the shared command slots 3–6 in `cmd_data.c`) |

**Combat**

| File | Role |
|:---|:---|
| `HITCHECK.c` | Hit detection. Parry: `check_blocking_flag()`, `make_red_blocking_time()`, `defense_ground()`, `defense_sky()`. Throws: `catch_hit_check()`, `set_caught_status()`. Damage and stun: `set_damage_and_piyo()`. Damage dispatch: `plef_at_vs_player_damage_union()`. |
| `HITPLPL.c`, `HITPLEF.c`, `HITEFPL.c`, `HITEFEF.c` | Hit resolution for each pairing: player/player, player/effect, effect/player, effect/effect |

**Display only (no gameplay values):** `spgauge.c` (super meter HUD, struct `SPG_DAT`), `VITAL.c` (health bar), `stun.c` (stun bar).

**CPU opponent only:** `Com_Pl.c`, `Com_Sub.c`, `Ck_Pass.c`, `passNN.c`. (`Com_Data.c` also holds the player health table.)

### 3.4 Frame data is not in the repository

The per-move data lives in each character's data on the **disc**, and is loaded at runtime. That covers animation frames and their timing (startup, active, recovery), hit, hurt, throw and push boxes, cancel windows, and each attack's damage, stun and meter indexes.

- `CHARID.c:set_char_base_data()` points `WORK` at it through `char_init_data[charset_id]`. That includes `char_table[]` (the animation chains), `hit_ix_table`, `body_adrs`, `hand_adrs`, `catch_adrs`, `caught_adrs`, `attack_adrs`, `hosei_adrs` and `att_ix_table`.
- `CHARSET.c` (`setupCharTableData()`, `check_cm_extended_code()`) runs the chains frame by frame. Each frame sets fields such as `WORK.cg_hit_ix` and `WORK.cg_cancel`, and the frame's attack fields (`att.piyo`, `add_arts_point`, …) are copied out as it loads.

Changing any of this needs a patch applied after the character loads (or a patched disc), not an edit to a C table. Most character items in the spec are of this kind. Specials are not named in the source: a move is found through its command in `cmd_data.c` and the assignment table in `bin2obj/asstbl.c`.

---

## 4. "Third Strike Upper (3s')" Specification

The specification is Mike Z's post (linked in §1). It is the only source for *what* changes.

- **The itemised list is [`docs/upper3s/remaining.md`](docs/upper3s/remaining.md).** It covers every item of the post (five system mechanics, the character items for 19 characters, and seven additional bugfixes), with where each change has to be made and its status. Work from that list, and update its status columns in the same commit as the change.
- **Read the post's own wording before implementing an item.** An earlier version of this section paraphrased about a third of the post and got items wrong. The post makes *all* versions of Alex's Air Knee Smash un-air-parryable (EX stays ground-parryable). X.C.O.P.Y.'s EX moves spend the super's *timer*, not gauge. Houyoku-sen also gets the longest bar in the game. Yun's Sourai Rengeki is SA II (and goes from 3 stocks to 2).
- The post is organised as system mechanics, characters, and additional bugfixes. It does not group characters by tier.
- Where a number in the post disagrees with the retail data, record both values and the reason for the one used, next to the change.

---

## 5. Build & Verification Gates

### 5.1 What the build is today
- `make build` (the default, `PLATFORM=ps2`) produces a PS2 ELF and **fails unless it matches the retail `THIRD_U.BIN` byte for byte** (`tools/compare_bytes.py`). It needs your own `THIRD_U.BIN` in the repository root, `make setup_tools`, `make split`, and initialised submodules (`git submodule update --init`). See `docs/builde-guide.md`. CRI middleware is compiled with EE GCC 2.96, everything else with CodeWarrior (`mwccps2`, run through `wibo` and `mwccgap`).
- `UPPER_3S=1` compiles the 3s' patch in (`-DUPPER_3S` and the sources of `src/anniversary/sf33rd/Source/Game/upper3s/`). The PS2 rule always byte-compares, so a 3s' build is a host build:
  - `PLATFORM=linux`: the SDL3 port (needs SDL3 and the ffmpeg libraries through `pkg-config`).
  - `PLATFORM=web`: the WebAssembly port (emscripten and an SDL3 WebAssembly build).
  - `PLATFORM=headless`: no window, sound or pacing, no dependencies; for tests (`src/anniversary/port/headless/`).

  All of them read the disc's files from `rom/THIRD/` (`SF33RD.AFS` and the `IOP` directory) in the working directory.

### 5.2 Checklist before committing
1. **Functional test (§2.2):** an automated test for the change must exist and pass. There are two suites:
   - `make test`: unit tests of the patch layer on synthetic characters (`tests/upper3s/`), no game data needed.
   - `make match-test`: fight tests (`tests/match/`) that play scripted fights in the vanilla and the 3s' headless builds and measure startup, advantage, damage, stun and meter in the real engine. They need `SF33_GAME_DATA` pointing at the disc's files. A balance change is verified only when a fight test measures it.
2. **The vanilla build stays matching:** with `UPPER_3S` off, `make build` must still print `Files match ✅`. Every change to a shared file goes behind `#if defined(UPPER_3S)` / `#if !defined(UPPER_3S)`. Without the game file, `bash tools/verify/ps2_touched_sources.sh` compares the preprocessed PS2 sources with those of the retail-matched commit, and for a comment-only edit `python3 tools/check_comments_only.py` compares what the compiler sees before and after.
3. **Struct layout:** `PLW` is 0x46C bytes, `WORK` 0x388, `SA_DATA` 0x10 and `SPG_DAT` 0x34; the field offsets are commented in `include/structs.h` and the headers. Do not change them. Keep 3s' state in separate structures. (There is no `GAUGE` struct.)
4. **Compiler:** game code must compile under `mwccps2` with the Makefile's flags (the PS2 build) and under clang with `-Werror` (the host builds).
5. **This file:** after editing §3, run `python3 tools/check_agents_md.py`.
