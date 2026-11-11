# Third Strike Upper - changelog

Every change of the 3s' layer, and what it does. The item by item status of
the design spec is in [`spec-map.md`](spec-map.md); the architecture is in
[`implementation-guide.md`](implementation-guide.md).

The patch is behind `UPPER_3S`. The vanilla build is left untouched: the
new sources are only compiled when the flag is set, and every change in a
shared file is behind `#if defined(UPPER_3S)`.

---

## The framework

### The data layer

The per character move data of Third Strike - startup frames, active and
recovery frames, hitboxes, pushboxes, damage, stun, hitstun and hit
advantage - is **not part of this repository**. It is read from the disc
image at runtime and interpreted by `CHARSET.c` and `HITCHECK.c`.

The 3s' layer therefore patches those numbers at runtime, right after they
have been loaded:

* `CHARID.c` / `PLCNT.c` - the hook that applies the per character
  overrides once the character index is known.
* `PLCNT.c` - the hook that replaces the base super art data before the
  in game option modifiers are applied to it.
* `Pow_Pow.c` - the hook that applies the damage overrides.
* `HITCHECK.c` - the hook that applies the stun overrides.

`src/anniversary/sf33rd/Source/Game/upper3s/` holds the framework:

| file | content |
|:---|:---|
| `upper3s.c` | the framework itself |
| `upper3s_sys.c` | the shared state of the system mechanics |
| `plXX.c` | the per character override tables |

`include/sf33rd/Source/Game/upper3s.h` is the interface, and
`include/sf33rd/Source/Game/upper3s_sys.h` the one of the system
mechanics.

### Two numbers for a character

The **character index** (0 - 19) and the **charset id** (the index into the
disc data) are not the same; `PLCNT.c:plid_data` permutes them. The override
tables are keyed by the character index.

The character index is *not* the order in which the characters were
introduced. The order the game uses is:

| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|:--|:--|:--|:--|:--|:--|:--|:--|:--|:--|
| Gill | Alex | Ryu | Yun | Dudley | Necro | Hugo | Ibuki | Elena | Oro |

| 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
|:--|:--|:--|:--|:--|:--|:--|:--|:--|:--|
| Yang | Ken | Sean | Urien | Akuma | Chun-Li | Makoto | Q | Twelve | Remy |

The evidence is in `AGENTS.md` section 3.1. Two independent confirmations are
worth spelling out because they also pin the *super art* slots:

* the stock counts of the design document match the retail table only in this
  order - Ken has three stocks in his SA III, Chun-Li two in her SA II,
  Twelve one in his SA II, Remy the shortest SA III bar and Akuma and Remy
  the short stun gauge;
* the per-character sources are named after it, `PLPAT00.c` holds Gill's
  `Att_RESURRECTION`, `PLPAT08.c` Elena's healing super and `PLPAT09.c`
  Oro's `TENGU`/`KISHI`.

---

## The changes

<!-- Every entry below is one commit's worth of changes. -->

### System mechanics

| Item | Change | Where |
|:---|:---|:---|
| SM1 | The metergain of a whiffed normal is halved for every consecutive whiff (100%, 50%, 25%, 12.5%). The counter restarts on a hit, on a block, on a parry and when the character is hit, blocks or parries. A whiffed special keeps its full gain. | `upper3s/upper3s_sys.c` (`upper3s_sys_whiff_start()`, `upper3s_sys_whiff_contact()`), hooked in `PLS02.c` |
| SM2 | A parry attempt starts a cooldown of 2 frames for that kind of parry, in which the parry is not available and the character falls back to the normal block. A hit that lands during the cooldown and could have been parried the same way does 25% more damage. A successful parry, a hit of the other kind and a throw are not affected. | `upper3s/upper3s_throw.c` (`upper3s_parry_cooling()`, `upper3s_parry_damage_permille()`), hooked in `HITCHECK.c` (`defense_ground`, `defense_sky`, `plef_at_vs_player_damage_union`) |
| SM3 | A parry that is attempted in a wakeup animation, in a recovery roll or in the hitstun of landing on the feet is a guess parry: its window is half as long (2 frames become 1), the cooldown is the same, and a throw that lands in the window or in the cooldown cannot be teched. | `upper3s/upper3s_throw.c` (`upper3s_parry_is_guess_state()`, `upper3s_parry_threshold()`, `upper3s_parry_guess_locked()`), hooked in `HITCHECK.c` |
| SM4 | The partition timer of a charge is reloaded with its maximum as soon as the charge has reached one third (14 of the 42 frames), so the window in which the charge can be partitioned is always the full one instead of depending on where the hidden timer happened to be. | `upper3s/upper3s_sm4.c` (`upper3s_sm4_charge_partition()`), hooked in `PLCNT.c` (`Player_control()`) |
| SM5 | A cross up - a left-right unblockable that hits while the defender blocks facing the other way - deals 50% of the damage, follow-ups included. | `upper3s/upper3s_sys.c` (`upper3s_sys_crossup_permille()`), hooked in `Pow_Pow.c` |
| Stun bar | The stun gauge of Akuma and of Remy goes from the "short" size (56) to the "regular" one (64) that the majority of the cast uses. | `upper3s/upper3s_sys.c` (`upper3s_sys_stun_genkai()`), hooked in `PLCNT.c` |
| Health | Twelve takes the health of the standard cast (the retail row that Yun, Necro, Makoto and Q share, 1466 instead of 1530). Ibuki takes the same row scaled by 937 permille, which is the 1050/1120 ratio of the design document. | `upper3s/upper3s_sys.c` (`upper3s_sys_health_source()`), hooked in `PLS02.c` (`setup_vitality()`) |
| Stun scaling | The stun of a move can now be scaled relatively instead of being replaced by an absolute value. The retail stun is read from the disc at runtime, so an absolute number could only be a guess. | `upper3s.h` (`U3S_MV_STUN_PERMILLE`), `upper3s.c` (`upper3s_stun_permille()`), hooked in `HITCHECK.c` |

### Bugfixes

| Item | Change | Where |
|:---|:---|:---|
| B1 | The victim of a successful throw is invincible for the whole animation, so that it cannot be hit out of it (the Remy/Urien glitch and the Q throw bug). The retail's own invincible flag is used, so nothing can hit the character while it is thrown. | `upper3s/upper3s_throw.c` (`upper3s_throw_invuln_start()`, `upper3s_throw_frame()`), hooked in `HITCHECK.c` (`set_caught_status`, `hit_check_main_process`) |
| B2 | The damage stop of a hit is dropped while the defender is in the superflash of the opponent. In the retail game the damage stop replaces the hit stop of the superflash, so a character that is touched by a projectile on the same frame can walk away while the superflash is still running. | `upper3s/upper3s_throw.c` (`upper3s_superflash_hit()`), hooked in `HITCHECK.c` (`plef_at_vs_player_damage_union`, `set_guard_status`) |
| B3 | The universal overhead (MP+MK) and the personal action (HP+HK) come out while the stick is held away or towards. The retail game throws the input away when the stick is not neutral, and the command turns into a normal attack. The up and the down direction keep blocking the two commands, so that the jump and the crouch of the retail game stay the same. | `upper3s/upper3s_input.c` (`upper3s_input_combo_direction_ok()`), hooked in `PLS03.c` (`check_leap_attack()`, `check_chouhatsu()`) |
| B4 | The character that teched an air throw cannot start an attack until it has landed again - the lock that the retail game already applies to the ground throw tech. | `upper3s/upper3s_throwtech.c` (`upper3s_throwtech_attack_locked()`), hooked in `PLS00.c` (`nm_49000()`) |

### Per character

The changes of the characters are in the table below. Every character has a
file of its own with the item by item status of the design document, the
retail data the items are built on, and the exact mechanism of every item
that still needs the disc:

| file | character |
|:---|:---|
| [`changelog-chakuma.md`](changelog-chakuma.md) | Akuma |
| [`changelog-chalex.md`](changelog-chalex.md) | Alex |
| [`changelog-chchunli.md`](changelog-chchunli.md) | Chun-Li |
| [`changelog-chdudley.md`](changelog-chdudley.md) | Dudley |
| [`changelog-chelena.md`](changelog-chelena.md) | Elena |
| [`changelog-chhugo.md`](changelog-chhugo.md) | Hugo |
| [`changelog-chibuki.md`](changelog-chibuki.md) | Ibuki |
| [`changelog-chken.md`](changelog-chken.md) | Ken |
| [`changelog-chmakoto.md`](changelog-chmakoto.md) | Makoto |
| [`changelog-chnecro.md`](changelog-chnecro.md) | Necro |
| [`changelog-choro.md`](changelog-choro.md) | Oro |
| [`changelog-chq.md`](changelog-chq.md) | Q |
| [`changelog-chremy.md`](changelog-chremy.md) | Remy |
| [`changelog-chryu.md`](changelog-chryu.md) | Ryu |
| [`changelog-chsean.md`](changelog-chsean.md) | Sean |
| [`changelog-chtwelve.md`](changelog-chtwelve.md) | Twelve |
| [`changelog-churien.md`](changelog-churien.md) | Urien |
| [`changelog-chyang.md`](changelog-chyang.md) | Yang |
| [`changelog-chyun.md`](changelog-chyun.md) | Yun |

### The cancel rules

The cancel permission of a move is not in the sources: it is the `cg_cancel`
byte of the frame chain of the move, which the disc holds. The design document
grants a few cancels to a few normals of a few characters, so the rules are
kept in the 3s' layer:

* `upper3s/upper3s_ch_alex.c`, `upper3s/upper3s_ch_q.c` and
  `upper3s/upper3s_ch_twelve.c` - the rule of each character, as a pure
  function of the move.
* `upper3s/upper3s_cancel.c` - the dispatcher that routes a move to the rule
  of its character, hooked in `PLS00.c` (`check_cg_cancel_data()`), right at
  the top of the function, before the test that gives up when the mask of the
  disc is empty.

A move is addressed by the **kind of character data** it lives in (the slot 4
holds the normal attacks, the slot 5 the special moves and the super arts) and
by its **char_ix**, the index of the move in that kind of data.

The char_ix is the only thing that identifies the move. The routine number of
the state machine (`routine_no[2]`) cannot be used: it is the *behaviour class*
of the normal, and the assignment tables of the disc (`bin2obj/asstbl.c`) give
it as `r_no`, which is 0 for most of the normals and 4 for the medium
punches - the standing and the crouching Medium Punch share it. The rules
that are keyed on the routine number would have been applied to the wrong
moves.

| Character | Change | Where |
|:---|:---|:---|
| Alex | The crouching Strong (2MP) can be cancelled into a special move and into a super art. The crouching Forward (2MK) can be cancelled into a super art. | `upper3s/upper3s_ch_alex.c` |
| Q | The crouching Jab (2LP), the standing Short (5LK) and the crouching Short (2LK) can be cancelled into a special move and into a super art. The crouching Forward (2MK) can be cancelled into a super art. | `upper3s/upper3s_ch_q.c` |
| Twelve | The standing Strong (5MP) and the far Strong can be cancelled into a special move and into a super art. The crouching Strong (2MP) can also be cancelled into a super jump. The crouching Forward (2MK) can be cancelled into a super art. Every air attack can be cancelled into a special move and into a super art. | `upper3s/upper3s_ch_twelve.c` |

The "far Strong" of Twelve is the only one of these moves whose char_ix is
not obvious: the selection tables of the disc decide which variant of a
button the game uses, and the choice is per move. For Twelve's standing Medium
Punch the selection is `{ 0x4008, 0 }`, so the game only ever uses the neutral
punch (char_ix 3) and the variant that the forward lever selects (char_ix 5);
char_ix 4 is in the tables but the game never reaches it. Both of the
extended variants are given the cancels, so that the rule does not depend on
which of them the design document calls the "far Strong".

| Character | Change | Where |
|:---|:---|:---|
| Akuma | The bar of "Messatsu Gou Rasen" (SA II) becomes shorter than the bars of his other two supers (96 instead of 112). The stun bar goes from the short to the regular size. | `upper3s/pl13.c`, `upper3s/upper3s_sys.c` |
| Chun-Li | "Houyoku-sen" (SA II) costs one stock instead of two and gets the longest bar of the game (128). "Tensei Ranka" (SA III) costs two stocks instead of three and its bar grows to the length of "Shippu" (80, the length of Ken's SA III). The light and the medium version of the Cold Blue Kick hold the defender longer, so that the hit is not negative on hit. | `upper3s/pl14.c` |
| Dudley | The stun of "Rocket Uppercut" goes down to 75% and the stun of "Rolling Thunder" goes up to 125% of the retail value, which differentiates the two supers. | `upper3s/pl03.c` |
| Elena | "Spinning Beat" keeps the three stocks it costs in the retail game - the document calls it SA III, but the retail SA III of Elena is the healing super, so the stock count of the retail SA I is the one the document asks for. | `upper3s/pl07.c` |
| Ibuki | "Yami Shigure" (SA III) costs two stocks instead of one. The health goes from the highest of the whole game to 937 permille of the health of the standard cast. | `upper3s/pl06.c`, `upper3s/upper3s_sys.c` |
| Ken | "Shippu Jinrai Kyaku" (SA III) costs two stocks instead of three and its base damage is 15% lower. | `upper3s/pl10.c` |
| Makoto | The stun of "Abare Tosanami" (SA II) is halved. The stun of the three "Fukiage" goes from 29/31/31 to 27/28/31. | `upper3s/pl11.c` |
| Oro | The bar of "Kishi Riki" (SA I) grows to the length of the bar of "Tengu Stone" (112). | `upper3s/pl08.c` |
| Q | The health bonus for one to three personal actions drops from 1705 and 2050 to 1600 and 1800. The hitstun of the crouching Short and of the standing Roundhouse is raised, so that the hit is not negative on hit. | `upper3s/pl16.c` |
| Remy | The bar of "Blue Nocturne" (SA III) grows from 80 to 96. The crouching Strong deals six points more damage. The hitstun of the far Forward, of the far Roundhouse, of the crouching Forward and of the Towards+Forward is raised, so that they are no longer negative on hit. The stun bar goes from the short to the regular size. | `upper3s/pl18.c`, `upper3s/upper3s_sys.c` |
| Ryu | The hitstun of the close Fierce, of the crouching Fierce and of the crouching Forward is raised, so that the hit is not negative on hit. | `upper3s/pl01.c` |
| Sean | The hitstun of the close and far Fierce, of the crouching Heavy and of the crouching Forward is raised, so that they are no longer negative on hit. | `upper3s/pl15.c` |
| Twelve | "X.F.L.A.T." (SA II) costs two stocks instead of one. The hitstun of the standing and crouching normals is raised, so that the hit is not negative on hit. The health goes from 1530 to the health of the standard cast. | `upper3s/pl17.c`, `upper3s/upper3s_sys.c` |
| Hugo | The hitstun of the crouching Jab and of the crouching Short grows by two frames, so that the link to the EX Monster Lariat is reliable. | `upper3s/pl05.c` |
| Urien | "Tyrant Slaughter" (SA I) costs one stock instead of two, its bar grows from 104 to 112 and its damage grows by 25%. | `upper3s/pl12.c` |
| Yun | "Sourai Rengeki" (SA II) costs two stocks instead of three and the bar of "Genei Jin" (SA III) grows to the length of "Sourai Rengeki" (88). | `upper3s/pl02.c` |

### The startup and the recovery of the moves

The startup and the recovery of a move are in the frame chain of the disc, and
the patch now changes them: `upper3s/upper3s_chain.c` walks the chain of a
move, reads the hold of every frame and shortens the frames of the startup and
of the recovery.

The **startup** of a move is the number of the game frames before the first
frame of its chain that carries the attack data, and the **recovery** is the
number of the game frames that follow the last frame that carries it. A change
is only made when the chain still holds the number the retail data has, so a
chain that the walk cannot follow is never touched. A chain that already holds
the target is left alone and the change counts as done: the patch is applied
again every time the character is loaded, and the disc puts some moves of a
character in one chain - the light, the medium and the heavy version of the
Ryuubi Kyaku of Sean, and the 6LP and the 4LP of Q - so a change of such a
chain is a change of every move that shares it.

| Character | Change | Where |
|:---|:---|:---|
| Chun-Li | The crouching Jab (2LP) starts in three frames instead of two. It is the only normal of the cast that starts in two frames. | `upper3s/pl14.c` |
| Hugo | The EX Giant Palm Bomber starts in 17 frames instead of 22 - the number of the Jab version. The "Meat Squasher" (360K) starts in 18/24/30 frames instead of 24/52/67 and recovers in 25/28/31 frames instead of 29/32/35. | `upper3s/pl05.c` |
| Q | The dash standing Jab starts in four frames instead of six. | `upper3s/pl16.c` |
| Sean | The Ryuubi Kyaku starts in 19 frames instead of 25 - the three strengths of the move share one chain in the data of the disc, so the change is a change of the three of them; the design document asks for 17/19/21 frames and the chain can only hold one number. The Dragon Smash starts in 3/4/5 frames instead of 5/7/9, and the EX version in 5 frames instead of 8. | `upper3s/pl15.c` |

The numbers of the design document do not always agree with the data of the
disc. Where they disagree, the intent of the document wins and the concrete
value is re-derived from the retail data; the reasoning is recorded in the
per character changelogs.

The walk of the game is cross-checked against the offline decoder of the disc
(`tools/upper3s/compare_startup.py`) over every move of every character: 2592
moves have the same startup, and the moves the two disagree on are the ones
whose chain branches on the state of the game - the patch does not touch them.

### The browser build

* `src/anniversary/sf33rd/AcrSDK/ps2/flps2debug.c` - the fatal-error trap
  of `flPS2DebugInit()` spins forever when the debug font cannot be
  loaded. On the host builds there is no disc to read it from, so the game
  reported the missing file and stopped instead of hanging the browser tab.
* `src/anniversary/port/utils.c` - the fatal error is also reported to the
  dev server, so the reason survives the page being torn down.
* `tools/web/make_web_shell.py` - the browser shell shows the reported
  reason instead of an empty abort message.
* `src/anniversary/port/sdk/sdk_libmc.c` - the port reported that there was
  no memory card, so the game stopped at its start with the question
  whether it should be started without saving. The port now provides a
  virtual card that reports a formatted card with enough free space and no
  save file on it, so the game boots on its own.
* `src/anniversary/port/web/web_shot.c` - the browser build blocks the main
  thread, so the page cannot read the canvas back to take a screenshot of
  the running game. The build takes the screenshot itself and sends it to
  the dev server, which writes it to a file.
* `tools/verify/web_shot.py` - the check that loads the page in a headless
  browser and looks at the frame the build sent.
* `src/anniversary/port/web/web_co.c` - the browser build is linked with
  ASYNCIFY, so that the blocking game loop can let the page breathe, and the
  coroutines of the port cannot use libco: libco switches the stack with
  `setjmp`/`longjmp`, which does not survive ASYNCIFY. The browser build
  therefore uses the fiber API of emscripten, which does the same job on top
  of ASYNCIFY, and provides the four functions of libco that the port uses.
* `include/port/port_fixups.h` - a few functions of the game and of the CRI
  middleware are called with arguments that their implementation ignores, and
  the implementation does not take them. On the desktop the call is a direct
  one and the extra argument is simply ignored, but WebAssembly resolves the
  call through the function table, where there is only one signature per
  function - a call that does not match the signature of the function is an
  error there. The arguments are dropped at the call site for the port
  builds; the retail build is compiled without the header and stays as it is.
* `src/anniversary/port/port_trace.h` - the progress markers of the port.
  The browser build reports them to the dev server with a synchronous
  request, because the beacons of the browser are dropped when the page
  switches the stack of a fiber, and a trace that arrives half of the time
  is worse than no trace at all. The markers are gated by a level, because
  a marker of the browser build costs a synchronous request and the game
  loop produces thousands of them per second.
* `Makefile` - the default goal of make is the first target of the first
  makefile it reads. The dependency files of the objects are included with
  `-include`, and the first one of them defined a target, which became the
  default goal: `make` built a single object file instead of the program,
  and the program was never linked again. `.DEFAULT_GOAL` now names the
  program explicitly. Every build that relied on the default goal before
  this fix produced a stale binary, which is worth remembering: a build
  that reports "is up to date" for an object that was just edited is not a
  build that did anything.
* `tools/verify/browser.py` - the browser of the checks runs with a
  profile directory of its own and is killed with its whole process tree. A
  fixed profile directory is locked by the browser that is still alive, and
  the next launch hands its address to that browser and quits: the page
  under test is never loaded and the check reports the state of an older
  run.

### The deployment of the game

The game is played in the browser at <https://trafgals.github.io/3s-decomp/>.

* `.github/workflows/pages.yml` - the workflow builds the module and deploys
  it, together with the disc image, to the Pages site of the repository. The
  toolchain is installed by the emsdk itself and not by an action: the action
  downloads a copy of the emsdk that can be older than the newest release of
  the emscripten.
* The disc image is not part of the repository - it is the retail data of the
  disc, and it is 612 megabytes, which is over the limit of a file of a
  repository. It is published as the asset `SF33RD.AFS` of the release
  `web-data-1`, and the workflow fetches it from there. The loose disc files
  (the directory of the IOP) are published the same way, packed into
  `IOP.tar.gz`, because the build packs them into the module.
* The disc image has to be served from the same origin as the page: the
  browser reads it with a cross origin request, and the asset of a release is
  served without the header that allows it. That is the reason the disc image
  is part of the deployment and not only a link in the page.
* `tools/verify/web_public.py` - the check of the published site. It opens
  the public address in a headless browser and watches the traffic of the
  browser, which is the record of what the site served: the addresses of the
  requests and the number of bytes they read. The markers of the boot travel
  in the address of the request to `/__trace`, so the same record tells
  whether the game reached its frame loop and whether it read the disc image
  from the site. The answers of the server are asked for directly, one
  request per file, because the netlog of this browser does not hold them.
  The published site has no dev server, so the requests of the markers answer
  404, and that is not a failure.
  The last run: the module and its data came from the site, the disc image
  was read in blocks of four megabytes with the answers 206, and the game
  reached the frame 480 of its loop. The wait ends as soon as the game
  reports a frame, and not after a fixed time: the disc image is 612
  megabytes, and the first read of it after a deployment is served by a
  network that has to fetch it from the origin first.

### The tools for the disc data

The frame data of the moves - the startup, the active and the recovery
frames, the hitboxes - is read from the disc image at runtime. To change
those numbers, the retail values have to be read out of the disc first.

* `src/anniversary/sf33rd/Source/Game/upper3s/upper3s_dump.c` - dumps the
  character data of the disc: the frame chain and the attack data of every
  move of every character. The dump follows the loader of the game, which
  reads the data of a character on demand.
* `src/anniversary/sf33rd/Source/Game/texgroup.c` - the hook of the loader
  that hands the data of a character to the dump.
* `src/anniversary/sf33rd/Source/Game/GD3rd.c` - the number of free slots
  in the request queue of the loader, so that the dump knows when there is
  room for the next character.
* `make UPPER3S_DUMP=1` - builds the game with the dump. The result is
  `upper3s_dump.txt`.

### The browser build boots and renders

The browser build boots, streams the disc image over HTTP range requests and
runs the game at the frame rate of the console (59.4 frames per second in a
headless browser, 8640 frames in a run of two and a half minutes).

Three defects had to be fixed for that, and none of them was in the game:

* `src/anniversary/port/web/web_cd.c` - the disc reader wrote into the
  buffers of the game through a pointer that Emscripten hands to JavaScript
  as a plain number. `out[i] = value` assigns a property to a number in
  JavaScript, which is silently ignored, so neither the query parameter
  nor a single byte of the disc image ever reached the heap. The memory is
  reached through the heap view (`HEAPU8[out + i] = value`) now.
* `src/anniversary/port/web/web_cd.c` - the address of the disc image was
  taken from a local variable of `web_cd_init()` and kept after the function
  had returned. The address pointed at a stack frame that was reused
  immediately afterwards, so the range request asked for a URL made of the
  text of the last trace marker. The override is kept in a static buffer.
* `Makefile` - the default goal of make was hijacked by the dependency
  files of the objects, so the program was never linked again after the
  dependency tracking was added. Every symptom that was diagnosed before
  that was fixed was measured against a binary that was months old.

The screenshot check (`tools/verify/web_shot.py`) looks at a frame that the
build sends to the dev server, and finds the title screen of the game.

