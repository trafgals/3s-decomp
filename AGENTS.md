# AGENTS.md — Street Fighter III: 3rd Strike Decompilation & 3S Upper Protocol

This document governs autonomous and pair-programming agents (Antigravity, MiniMax `mcode`, local LLMs on Strata) working on the **trafgals/3s-decomp** codebase. All agents must adhere to these directives.

---

## 1. Mission & Purpose

`trafgals/3s-decomp` is a fork of [`crowded-street/3s-decomp`](https://github.com/crowded-street/3s-decomp), which decompiled the entire game logic of *Street Fighter III: 3rd Strike* (PlayStation 2 `THIRD_U.BIN`) into matching C source code.

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

---

## 3. Codebase Architecture & Subsystem Mapping

The game logic resides in `src/anniversary/sf33rd/Source/Game/`. Capcom organized fighters by a 2-digit index (`00` to `19`):

### 3.1 Character Index Map
| ID | Character | ID | Character | ID | Character | ID | Character |
|:---:|:---|:---:|:---|:---:|:---|:---:|:---|
| `00` | Alex | `05` | Hugo | `10` | Ken | `15` | Makoto |
| `01` | Ryu | `06` | Ibuki | `11` | Sean | `16` | Q |
| `02` | Yun | `07` | Elena | `12` | Urien | `17` | Twelve |
| `03` | Dudley | `08` | Oro | `13` | Gouki (Akuma) | `18` | Remy |
| `04` | Necro | `09` | Yang | `14` | Chun-Li | `19` | Gill |

### 3.2 Core Combat Files
- **Player State & Movesets (`PLSxx.c`)**: State machine handling normals, specials, supers, frame timers, and cancel transitions for character `xx`.
- **Parameter Data (`passxx.c` / `PASSxx.c`)**: Frame data tables, hitbox coordinates, damage values, pushbox dimensions, and velocity vectors.
- **Combat & Collision (`HITCHECK.c`, `HITEFPL.c`, `HITPLPL.c`)**:
  - `check_blocking_flag()`: Parrying ("Blocking") detection, input direction checks, high/low/air validation.
  - `make_red_blocking_time()`: Red parry timing window during hitstop.
- **Super Art Gauge Engine (`spgauge.c`)**: Super meter lengths, stock counts, meter build multipliers on hit/whiff/block.
- **Throws & Throws Interlocks (`Com_Sub.c`, `HITPLPL.c`)**: Throw startup, range, tech windows, and throw invulnerability states.
- **Vitality & Stun (`VITAL.c`, `stun.c`)**: Health points, stun gauge capacities, stun recovery rates.

---

## 4. "Third Strike Upper (3s')" Balance Specification

Every change below must be cleanly implemented in the corresponding C source file:

### 4.1 System-Wide Mechanics & Bugfixes
1. **Universal Throw Invulnerability (`Com_Sub.c`, `HITPLPL.c`)**:
   - Make all successful throw animations 100% invulnerable throughout their duration to eliminate legacy glitches (specifically the **Remy/Urien glitch** and **Q throw bug**).
2. **Whiffed Normal Meter Building (`spgauge.c`)**:
   - Standardize whiff meter gain to prevent abusive out-of-range meter farming while preserving zoning incentives.
3. **Charge Partitioning Determinism**:
   - Eliminate random charge loss timers so charge partitioning behaves with 100% deterministic precision.

### 4.2 Top-Tier Rebalancing
1. **Yun (`PLS02.c`, `spgauge.c`)**:
   - **Genei Jin (SAIII)**: Remove the automatic Super priority flag on normals (normals during Genei Jin no longer crush opponent normals/specials on trade).
   - **Meter Length**: Increase Genei Jin meter length to match *Sourai Rengeki* (SAI) while keeping active timer duration intact.
2. **Chun-Li (`PLS14.c`, `spgauge.c`)**:
   - **Houyoku-sen (SAII)**: Reduce stock count from **2 stocks down to 1 stock**.
   - **Crouching Forward (c.MK)**: Tighten the late super-cancel buffer window to prevent super-late hit-confirms after hitstop finishes.
3. **Ken (`PLS10.c`, `spgauge.c`)**:
   - **Shippu Jinrai Kyaku (SAIII)**: Reduce stocks from **3 down to 2**; reduce base damage by 15%.
   - **Jab Dragon Punch (`623LP`)**: Lower damage from 15 to 11.5 (prevents double Jab DP combo from eclipsing Strong DP).
4. **Makoto (`PLS15.c`)**:
   - **Abare Tosanami (SAII)**: Reduce stun damage by 50%.
   - **Fukiage (`623P`)**: Tone down stun on Light, Medium, and Heavy Fukiage to eliminate degenerate 100% stun loops.

### 4.3 Low-Tier & Cast Viability Buffs
1. **Sean (`PLS11.c`, `PASS11.c`)**:
   - **Normals on Hit**: Increase hitstun on close/far Fierce, c.HP, s.MK, far HK, c.MK, and 6HP so they are no longer negative on hit.
   - **Sean Roll (`214P`)**: Allow Super-cancelling midway through the roll animation.
   - **Ryuubi Kyaku (`236K`)**: Decrease startup frames (Light: 17f, Medium: 19f, Heavy: 21f); Roundhouse version causes hard knockdown.
2. **Twelve (`PLS17.c`, `spgauge.c`)**:
   - **X.C.O.P.Y. (SAIII)**: Startup reduced from 12 frames to **1 frame**; allow spending remaining super gauge to execute EX moves; damage taken during recovery reduced to 66% (remove damage amplification penalty).
3. **Remy (`PLS18.c`, `stun.c`)**:
   - **Stun Bar**: Upgrade stun bar from "Short" to "Regular".
   - **Crouching Strong (c.MP)**: Increase base damage by +6 to match standard shoto damage.
   - **Close Fierce (s.HP)**: Fix early active frames so they properly knock down airborne opponents.
4. **Alex (`PLS00.c`)**:
   - **Air Knee Smash (`623K`)**: Standard versions (Light, Medium, Heavy) made **un-air-parryable** (restoring their role as functional anti-air specials).
   - **Boomerang Raid (SAI)**: Increase forward tracking on middle hits and extend final grab reach to prevent whiffing on hit.
5. **Oro (`PLS08.c`, `Com_Sub.c`)**:
   - **Forward Throw**: Add standard forward throw (`6 + LP + LK`) mirroring back-throw animation.
   - **Chicken Kick Loop**: Normalize pushboxes so `[s.MP xx Chicken Kick]` loops connect consistently across the entire cast.
   - **Oniyama (`[2]8P`)**: Increase hitstop and juggle window to make super-cancelling practical.

---

## 5. Build & Verification Gates

### 5.1 Verification Checklist
Before committing any changes to `main`:
1. **Mandatory Functional Test (Per User Request)**:
   - An automated test specifically verifying the requested functionality must be added/executed and pass 100%.
2. **Compilation Check**:
   - Code must compile cleanly with `-Werror` compatibility against CodeWarrior and GCC targets without syntax errors or mismatched prototypes.
3. **Data Structure Integrity**:
   - Ensure `sizeof(PLW)`, `sizeof(WORK)`, and `sizeof(GAUGE)` offsets remain 100% aligned with PS2 memory maps.
4. **Feature Cleanliness**:
   - Wrap modifications with `#ifdef UPPER_3S` / `#ifndef UPPER_3S` guards where appropriate so vanilla matching builds remain reproducible.
