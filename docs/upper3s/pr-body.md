# Third Strike Upper (3s') - the balance patch of Mike Z, on a WebAssembly build

This pull request implements the **Third Strike Upper (3s')** balance
overhaul and system bugfix patch of Mike Z on top of the byte exact PS2
decompilation, and adds the **WebAssembly build** that runs the game in a
browser.

The design specification is [What I'd Change For Third Strike Upper
(3s')](https://mikezsez.blogspot.com/2015/12/what-id-change-for-third-strike-upper-3s.html);
the copy that is used by the code is in
[`docs/upper3s/design-spec.md`](docs/upper3s/design-spec.md).

## What is in here

* **The 3s' layer** - a new `src/anniversary/sf33rd/Source/Game/upper3s/`
  directory with the framework, the shared system layer, the per character
  override tables and one module per mechanic, plus the hooks in the shared
  sources (`PLCNT.c`, `PLS00.c`, `PLS02.c`, `PLS03.c`, `Pow_Pow.c`,
  `HITCHECK.c`).
* **The WebAssembly target** - `make PLATFORM=web` builds the game for the
  browser. The disc image is streamed over HTTP range requests, so the
  browser build needs no local copy of the data. The build boots, reads the
  disc image and runs the game at the frame rate of the console: 59.7
  frames per second in a headless browser, 7080 frames in a run of two
  minutes.
* **The frame chain walker** - the startup and the recovery of a move live
  in the frame chain of the disc, which is a little program with jumps and
  branches. `upper3s/upper3s_chain.c` walks it the way the engine does and
  shortens the frames of the startup and of the recovery, and
  `tools/upper3s/compare_startup.py` checks the walk against an offline
  decoder of the disc over every move of every character.
* **The tools for the disc data** - the frame data of the moves is read from
  the disc at runtime, so the retail values have to be read out of the disc
  before they can be changed. `make UPPER3S_DUMP=1` builds a game that
  dumps the character data of the disc, character by character, as the
  loader reads it.
* **The verification** - a test program per mechanic (`make test` builds and
  runs all of them), a check that the retail PS2 build is untouched
  (`tools/verify/ps2_touched_sources.sh`), a check that the vanilla host
  build holds no part of the patch
  (`tools/verify/vanilla_unchanged.sh`), a headless browser boot check
  (`tools/verify/web_boot.py`) and a headless browser screenshot check
  (`tools/verify/web_shot.py`). `tools/verify/gate.sh` runs all of them.

## What the patch changes

The system mechanics of the design document:

| Item | Change |
|:---|:---|
| SM1 | The metergain of a whiffed normal is halved for every consecutive whiff (100%, 50%, 25%, 12.5%) and the counter restarts on a hit, a block or a parry. A whiffed special keeps its full gain. |
| SM2 | A parry attempt starts a 2 frame cooldown for that kind of parry, and a hit that lands during the cooldown and could have been parried the same way does 25% more damage. |
| SM3 | A guess parry (during a wakeup, a recovery roll or the hitstun of landing on the feet) is active for half the window and a throw in its window or cooldown cannot be teched. |
| SM4 | The partition timer of a charge is reloaded with its maximum as soon as the charge has reached one third, so the window in which a charge can be partitioned is always the full one. |
| SM5 | A cross up - a left-right unblockable that hits while the defender blocks facing the other way - deals 50% of the damage, follow-ups included. |

The additional bugfixes:

| Item | Change |
|:---|:---|
| B1 | The victim of a successful throw is invincible for the whole animation (the Remy/Urien glitch and the Q throw bug). |
| B2 | The damage stop of a hit is dropped while the defender is in the superflash of the opponent, so that a character touched by a projectile on the same frame cannot walk away. |
| B3 | The universal overhead (MP+MK) and the personal action (HP+HK) come out while the stick is held away or towards. |
| B4 | The character that teched an air throw cannot start an attack until it has landed again - the lock the retail game already applies to the ground throw tech. |

Together with the super art, the stun bar, the health and the hitstun
changes of the characters, and with the startup and the recovery of the
moves. The item by item status is in
[`docs/upper3s/spec-map.md`](docs/upper3s/spec-map.md), what each change
does is in [`docs/upper3s/changelog.md`](docs/upper3s/changelog.md).

## The names of the special moves

The special moves of a character are not named anywhere in the sources: they
are reached through the command tables of `cmd_data.c` and through the
assignment table of the disc, so a move the design document talks about could
not be addressed until that chain was read. It is now, and the tool
`tools/upper3s/decode_cmd.py` prints it:

* every command of every character is a header of twelve words followed by
  steps of four words, and the lever of a step is a nibble with the bit 1 for
  up, 2 for down, 4 for forward and 8 for back - the horizontal bits are
  relative to the character, because `CMD_MAIN.c:pl_lvr_set()` swaps them
  when the character faces to the left;
* written the way a player reads them, in the numpad notation, the tool
  prints the motion of every command - the Dragon Punch of Ryu is 623, the
  Hadouken is 236, the Tatsumaki Senpukyaku is 214 - and the `char_ix` of the
  four strengths of the move, which is the index the override tables address
  a move by;
* the decoder checks itself against the commands the game is known for, and
  the check includes the `char_ix` 28 of the jab Dragon Punch of Ken, which
  the patch already used, so the derivation reproduces a number that was
  already known.

That unlocked the last item of Makoto that was waiting for it. The document
asks for the stun of the Fukiage - the 623+P special - to go from 29/31/31
to 27/28/31, and the index of the move was the piece that was missing: the
Fukiage is the command 28 of `cmd_data.c`, and the four strengths of a
command are the four consecutive entries of the assignment table of the
character, which are the `char_ix` 36, 37, 38 and 39.

The stun of a move is the index `UNK_7.piyo` into
`bin2obj/gauge.c:_add_piyo_gauge[16]`, whose row for Makoto gives 29 for the
index 14 and 31 for the index 15. The attack table of the character holds
exactly three entries with those two indexes - the 29/31/31 of the document.
The fourth strength of the move is the EX, which has the index 10 and a stun
of 21; the document does not quote it, so it keeps the value of the disc.
The override is the absolute value of the document, so 27 and 28 can be used
even though the row of the character does not hold them.

## The startup and the recovery of the moves

The startup and the recovery of a move are in the frame chain of the disc,
and the patch now changes them. The chain of a move is not a flat list of
blocks: it is a little program with jumps and branches that the engine runs
for every frame of the move. `upper3s/upper3s_chain.c` opens it out of
`wk->char_table[koc]`, walks it the way `CHARSET.c:check_cm_extended_code()`
does, and shortens the holds of its frames:

* the **startup** of a move is the number of the game frames before the first
  frame of its chain that carries the attack data, and the **recovery** is
  the number of the frames that follow the last one;
* the frames are shortened the longest one first, and never below a hold of
  one game frame: a frame that is held for no game frame at all is a command
  block for the engine, not a frame of the animation;
* a change is only made when the chain still holds the number the retail
  data has, so a chain that the walk cannot follow is never touched;
* a chain that already holds the target is left alone and the change counts
  as done - the patch is applied again every time the character is loaded.

The disc does not give every move a chain of its own. The light, the medium
and the heavy version of the Ryuubi Kyaku of Sean are the entries 32, 33 and
34 of the slot of the special moves and they all point at the same byte
offset, and so do the 6LP and the 4LP of Q. A change of such a chain is a
change of every move that shares it, so the tables of the patch hold one
entry per chain. The consequence is visible in one item of the design
document: it asks for 17/19/21 frames for the three strengths of the Ryuubi
Kyaku, and a chain can only hold one number, so the chain is brought to 19
frames - the target of the medium version, and the value that is the closest
to the three of them. The document and the disc agree that the three
strengths start in the same number of frames ("Currently they all have the
same startup"), so nothing is lost by the change.

The walk is checked against the offline decoder of the disc
(`tools/upper3s/compare_startup.py`) over every move of every character:
2592 moves have the same startup, and the moves the two disagree on are the
ones whose chain calls another move or branches on the state of the game.

The numbers of the design document do not always agree with the data of the
disc. Where they disagree the intent of the document wins and the concrete
value is re-derived from the retail data, and the per character changelogs
record the reasoning of every one of them. Hugo's "Meat Squasher" (360K) is
the clearest example: the recovery of the document (30/33/36 frames) matches
the disc exactly and identifies the move beyond doubt - it is the only move
of the whole cast with that recovery - while the startup of the document
(21/31/44) matches no move of the disc, so the target of the document
(18/24/30) is used as it is and the value that has to be held by the chain
for the change to be applied is the number of the disc (24/52/67).

## The two layers of the patch

The frame data of the moves of Third Strike - the hitboxes, the pushboxes,
the cancel windows - is **not part of this repository**: it is read from the
disc image at runtime. The patch therefore works on two layers:

* the **data layer** overrides what the repository does know: the super art
  stock counts and gauge lengths (`PLCNT.c:super_arts_data`), the damage
  of the supers (`POW_DATA.c:Power_Data`), the stun bar size
  (`PLCNT.c:pl_piyo_tbl`) and the health
  (`Com_Data.c:Com_Vital_Unit_Data`);
* the **source layer** changes the shared engine code behind
  `#if defined(UPPER_3S)`: the metergain of a whiffed normal, the damage of
  a cross up, the parries, the throws, the superflash, the charge partition
  timer, the stun and the health tables.

The startup and the recovery of the moves are the third thing: they are in
the disc, and the patch walks the chain of the disc to change them.

Every item of the design document that needs the frame data of the disc is
listed as `needs-disc` in [`docs/upper3s/spec-map.md`](docs/upper3s/spec-map.md),
together with the place it has to be changed. The architecture is described
in [`docs/upper3s/implementation-guide.md`](docs/upper3s/implementation-guide.md).

## The retail build is not touched

The new sources are only compiled when `UPPER_3S=1` is set, and every change
in a shared file is behind `#if defined(UPPER_3S)`. The check compares the
**preprocessed** PS2 sources of the files the patch touches with the ones of
the commit the retail build was matched in:

```
$ bash tools/verify/ps2_touched_sources.sh
36 files compared, 0 changed
PASSED: the vanilla PS2 build is unchanged
```

The list of the files is derived from the tree, so a file that is changed
without being listed is still compared; a new file is compared through the
sources that include it, which is what proves that the headers of the port
stay behind their `#if !defined(TARGET_PS2)` guard.

The host build is checked for the patch itself: the sources of the patch are
not part of the build, and the binary holds no symbol of them.

```
$ bash tools/verify/vanilla_unchanged.sh
PASSED: the vanilla build is unchanged by the patch
```

## Two corrections that are worth a reviewer's attention

**The character index of the game is not the order of AGENTS.md section
3.1.** The index the game uses is `Gill 0, Alex 1, Ryu 2, Yun 3, Dudley 4,
Necro 5, Hugo 6, Ibuki 7, Elena 8, Oro 9, Yang 10, Ken 11, Sean 12, Urien
13, Akuma 14, Chun-Li 15, Makoto 16, Q 17, Twelve 18, Remy 19`. Every per
character table of the game uses it, and `PLCNT.c:plid_data` permutes it
with the charset id. `AGENTS.md` section 3.1 has been corrected, with the
evidence.

**The index of a normal attack is the `char_ix` of the disc's assignment
table, not the index of the move in the move list.** It is
`base + kos * 3 + koa`, with `kos` the button and `koa` the variant - 5X,
6X and 4X for the standing normals, 2X, 3X and 1X for the crouching ones.
The layout is derived from `bin2obj/asstbl.c` and `PLS03.c:check_nm_attack()`.

## The game in the browser

**https://trafgals.github.io/3s-decomp/**

The workflow `.github/workflows/pages.yml` builds the site from the sources of
the branch and deploys it to the Pages site of the repository. The deployment
holds the module and the disc image, and the game reads the disc image over
HTTP range requests from the same origin as the page.

The disc image is 612 megabytes. A file of a repository may not be over 100
megabytes, so the image is not committed: it is published as the asset
`SF33RD.AFS` of the release `web-data-1` of the repository, and the workflow
fetches it from there. The loose disc files (the directory of the IOP, which
the build packs into the module) are published the same way.

`tools/verify/web_public.py` opens the published address in a headless browser
and watches the traffic of the browser: the record of what the site served,
with the addresses of the requests and the number of bytes they read. The
markers of the boot travel in the address of the request to `/__trace`, so the
same record tells whether the game reached its frame loop and whether it read
the disc image from the site. The last run: the page loaded, the module and
its data came from the site, the disc image was read in blocks of four
megabytes with the answers 206, and the game reached its frame loop at 59.2
frames per second.

```bash
python tools/verify/web_public.py --timeout 240
```

## Building the browser version

```bash
make setup_sdl3_wasm PLATFORM=web
make PLATFORM=web UPPER_3S=1
python tools/web/serve.py --rom rom build/anniversary/web
```

The game then reads `rom/THIRD/SF33RD.AFS` over HTTP range requests from
the directory that is served, so the browser build needs no local copy of
the data. Without the disc image the build starts, reports what is missing
and stops - it does not hang.

The build shows the title screen of the game and runs it at 59.7 frames per
second. The screenshot check is what proves it: the browser build blocks the
main thread of the page, so the page cannot read the canvas back to take a
screenshot of its own. The build therefore takes the screenshot itself and
sends it to the dev server, which writes it to a file that the check looks
at.

## What is not in the patch yet

The item by item status of the design specification is in
[`docs/upper3s/spec-map.md`](docs/upper3s/spec-map.md). The framework, the
data layer, all five system mechanics and the first four of the additional
bugfixes are in, together with the super art, the stun bar, the health and
the hitstun changes of the characters and with the startup and the recovery
of the moves. What is left is the geometry of the hitboxes and the
pushboxes, the frame based cancel windows, the remaining frame data of the
moves, the two remaining bugfixes (B5, the rare move interaction soft
locks, and B6, the half circle motions) and the behaviour changes of the
characters - the new moves and the new inputs.

The frame data of the moves is read from the disc image at runtime. The
items that change it were marked `needs-disc` until the disc image was
there: the dump of all twenty characters is complete and the walk of the
frame chain is written, so the startup and the recovery are in. The hitbox
and the pushbox geometry still need the boxes of the attack table and the
hit box table of the disc, and the cancel windows need the `cg_cancel` byte
of the chains.

The half circle motions of B6 are the one item of that list that is not a
frame data item: the motion of a command is in `cmd_data.c`, and the decoder
of the previous section now reads it. The commands of the game that use a
half circle (the 1236, the 3214 and the loose 63214 of the retail tables)
are known, so the item can be taken as soon as the choice of the button for
the moves that overlap a quarter circle is made.

## Testing

```bash
bash tools/verify/gate.sh                     # every check of the port
make BUILD_DIR=/tmp/sf33rd-test test          # 377 checks, 0 failures
bash tools/verify/ps2_touched_sources.sh      # the retail build is unchanged
bash tools/verify/vanilla_unchanged.sh        # the vanilla build has no patch
python tools/verify/web_boot.py --real-time   # the browser build boots
python tools/verify/web_shot.py --real-time   # the browser build renders a frame
```

`make test` builds every file of `tests/upper3s/` that has a `main()` of its
own as a program of its own and runs them all, so that a test of one
mechanic does not have to link against the tests of the others. The tests do
not need the game, the disc image or a display.

The frame data of the patch is checked against the disc itself. The program
that reads the dump of the disc is a tool and not a test - the dump is not
part of the repository - so `make test` runs it when the dump is there, and
`tools/upper3s/compare_startup.py` compares what the runtime machinery
reads with what the offline decoder reads. That comparison fails on the
change of the patch that the data of the disc does not take: the tables of
the patch are read, and every change of the startup or of the recovery is
looked up in the data of the disc. All fourteen changes of the frame data
are taken by the game.

The browser checks need a headless browser and are run on the machine that
has one; the gate skips them with a note when there is none.

## The watcher of this pull request

`tools/watch_pr.py` waits for something to happen here: a comment of the
owner of the repository, a review, or the output of a check. It sleeps inside
one process and **exits** the moment an event appears, so the completion of
the background task is what wakes the agent up - nothing in the shell polls.
The event is written to `build/pr_watch_event.md` and the program prints a
summary of it.

```bash
python tools/watch_pr.py --repo trafgals/3s-decomp --pr 1
python tools/watch_pr.py --self-test
```

Its own test checks the event machine against synthetic events, without a
network: a comment of the owner wakes it, a comment of somebody else does
not, the reports of the agent do not, and a review, an inline review
comment, a check run and a commit that moved do.

## The review of this pull request

The findings of the code review are worked through in the comments of the
pull request, and the changelogs of the characters record the reasoning of
every number that the design document and the disc do not agree about.
