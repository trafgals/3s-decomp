# Fight tests

These tests play scripted fights in the real game engine and measure them: startup,
hit advantage, damage, stun and meter, in the vanilla build and in the 3s' build.

- The engine runs headless (`make PLATFORM=headless`, `src/anniversary/port/headless/`):
  no window, sound or frame pacing, about 13,000 frames per second.
- `harness.c` sets up the attract-mode demo fight with the characters and super arts a
  test asks for, makes both players human, and feeds them the test's inputs.
- `tools/match/fight.py` writes the script, runs a build and reads back the per-frame
  log; `startup()`, `hit_advantage()`, `damage()` and friends measure it.

The measurements use the conventions of the design document, and `test_harness.py`
checks them against the retail numbers the document quotes (Q's standing Roundhouse is
-10 on hit; Chun-Li's crouching Jab starts in 2 frames; and so on).

## Running

The game data is not in the repository. Point `SF33_GAME_DATA` at a directory that holds
`rom/THIRD/SF33RD.AFS` and `rom/THIRD/IOP`, then:

```sh
make match-test
```

Without the data every test is skipped.
