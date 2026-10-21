# Third Strike Upper tests

## The data layer tests

```
make test
```

builds every test file that has a `main()` of its own as a program of its own
and runs them all: `tests/upper3s/test_upper3s.c` for the data layer, and one
program per mechanic (`test_upper3s_input.c`, `test_upper3s_throwtech.c`, ...).
A test of one mechanic does not have to link against the tests of the others.

The tests do not need the game, the disc image or a display: they build
a synthetic character - a fake `char_table`, a fake `att_ix_table` and a
fake move header - and check that the override machinery rewrites exactly
the numbers it is supposed to rewrite, and nothing else.

That is the only way to test the data layer, because the real per
character data is read from the disc image at runtime.

## The vanilla build checks

```
tools/verify/ps2_sources_unchanged.sh [base-commit]
```

checks the property that matters for the retail build: the PS2 build has to
keep byte matching `THIRD_U.BIN`, so every 3s' change has to be invisible
when `UPPER_3S` is not defined. CodeWarrior and the retail binary are not
available here, so the check compares the *preprocessed* source of every C
file that goes into the PS2 build, with the PS2 flags and without
`UPPER_3S`. If the preprocessed source is the same, the compiler sees the
same code.

```
tools/verify/vanilla_unchanged.sh [base-commit]
```

does the same for the host build, by building it twice and comparing the
binaries.

## The browser boot check

```
make PLATFORM=web
tools/verify/web_boot.py
```

starts the dev server, loads the page in a headless browser and checks that
the game reaches its initialisation. The progress markers are sent by the
game itself (`port/web/web_trace.c`) to the dev server, which logs them as
ordinary requests.

The check also expects the game to *report* the missing disc data: the
loose disc files are not part of the repository, so a build without them
has to fail with a message instead of spinning in the fatal-error trap.
