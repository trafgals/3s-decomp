# Third Strike Upper (3s')

This fork implements the **Third Strike Upper (3s')** balance and bugfix patch
designed by Mike Z. The normative design document lives in
[`docs/upper3s/design-spec.md`](upper3s/design-spec.md) and the per-item
implementation status is tracked in [`docs/upper3s/spec-map.md`](upper3s/spec-map.md).

## The `UPPER_3S` build flag

Every 3s' change is compiled behind `UPPER_3S`:

```bash
make PLATFORM=ps2 UPPER_3S=1   # 3s' PS2 build
make PLATFORM=linux UPPER_3S=1 # 3s' native build
make PLATFORM=web UPPER_3S=1   # 3s' browser build
```

With the flag off (the default) the tree is the unmodified decompilation:

* the `ps2` platform still byte-matches the retail `THIRD_U.BIN`
  (`tools/compare_bytes.py` over `0x80..0x478A00`),
* the `linux`/`windows`/`web` platforms build the same unmodified game logic.

With the flag on the build intentionally **no longer** byte-matches: the
balance data and the system-mechanics changes alter both code and data.

## Rules for 3s' changes

1. **Never break the vanilla build.** A change must compile with
   `UPPER_3S` both on and off, and the vanilla `ps2` build must keep
   byte-matching. Where a change has to alter shared code, put the original
   code under `#if !defined(UPPER_3S)` and the 3s' behaviour under
   `#if defined(UPPER_3S)`, so the vanilla path stays textually identical.
2. **Follow the intent, re-derive the numbers.** The design document was
   written from arcade play and its literal numbers do not always match the
   retail data (see the stock-count discussion in the spec map). When they
   disagree, implement what the designer intended and derive the concrete
   value from the retail tables, recording the reasoning in the spec map.
3. **Keep the decompilation comments.** The Japanese Capcom comments and the
   existing naming conventions are the documentation for the data tables. Add
   to them; do not replace them.
4. **One concern per change.** Data-table changes, frame-data changes and
   behaviour changes are separate commits, so that a reviewer can see exactly
   which numbers moved.

## Where the 3s' data lives

The retail game keeps most of its tunable data in tables rather than code, and
**the per-character move data is not part of this repository at all**: startup
frames, active/recovery frames, hitboxes, pushboxes, damage, stun, hitstun and
hit advantage are read from the disc image (`SF33RD.AFS`) at runtime and
interpreted by `CHARSET.c` / `HITCHECK.c`.

The patch therefore has two layers:

* **Source layer** - everything that is expressed in the C sources (system
  mechanics, cancel rules, the super art stock/gauge table, the stun bar
  table, the health table) is patched in place behind `#ifdef UPPER_3S`.
* **Data layer** - the numbers that live on the disc are patched at runtime,
  right after the character data has been loaded, through the override tables
  in `src/anniversary/sf33rd/Source/Game/upper3s/`.

[`docs/upper3s/implementation-guide.md`](upper3s/implementation-guide.md)
describes both layers in detail, including how a move is addressed
(`koc` + `index`), where the attack data lives (`att_ix_table`), and why
damage and stun have to be applied at runtime.

| Data | Location |
|:---|:---|
| Super art gauge length, stock count, damage per hit | `src/anniversary/sf33rd/Source/Game/PLCNT.c` - `super_arts_data[20][4]` and `super_arts_DATA[20][4]`, typed by `SA_DATA` in `include/sf33rd/Source/Game/PLCNT.h`; the 3s' values go into `upper3s_super_art_<name>[4]` in `upper3s/plXX.c` |
| Super art gauge length/stock option modifiers | `src/anniversary/sf33rd/Source/Game/SysDir.c` - `sag_length_omake`, `sag_stock_omake` |
| Per-character stun gauge size | `src/anniversary/sf33rd/Source/Game/PLCNT.c` - `pl_piyo_tbl[20]` |
| Per-character health | `src/anniversary/sf33rd/Source/Game/Com_Data.c` - `Com_Vital_Unit_Data[20][4][12]` and the base health table |
| Super meter gain (whiff, hit, parry, combo scaling) | `src/anniversary/sf33rd/Source/Game/PLS02.c` - `add_sp_arts_gauge_*` |
| Character state machines (specials, supers, cancels) | `src/anniversary/sf33rd/Source/Game/passXX.c` / `PASSXX.c` |
| Per-character attack behaviour | `src/anniversary/sf33rd/Source/Game/plpatXX.c` / `PLPATXX.c` |
| Per-character move data, frame data, hitboxes | **on the disc**; patched at runtime through `upper3s_moves_<name>[]` in `upper3s/plXX.c` |
| Blocking, parry and hit resolution | `src/anniversary/sf33rd/Source/Game/HITCHECK.c`, `src/anniversary/sf33rd/Source/Game/Com_Sub.c` |

Note that `PLSxx.c` are **shared** modules (normal moves, system checks, super
gauge, command dispatch) - they are not per-character files. The per-character
move logic lives in `passXX.c` / `plpatXX.c`.

## Building the browser version

```bash
# 1. SDL3 for wasm (once)
make setup_sdl3_wasm

# 2. Build
make PLATFORM=web

# 3. Serve it (the server understands HTTP range requests, which the game
#    needs in order to stream the disc image)
python3 tools/web/serve.py --root build/anniversary/web
```

The game data is not redistributed. Point the build at your own copy of the
disc image with `rom/THIRD/SF33RD.AFS`, or override it at runtime with the
`?afs=<url>` query parameter.

The loose files of the disc (`rom/THIRD/IOP/*.TM2`) are preloaded into the
browser build when that directory exists - the game reads the debug font
from there while it starts up.

### Checking the build

```bash
make test                                  # the data layer tests
tools/verify/ps2_sources_unchanged.sh      # the retail PS2 build is untouched
tools/verify/vanilla_unchanged.sh          # the host build is untouched
tools/verify/web_boot.py                   # the browser build boots
```

`tools/verify/web_boot.py` starts the dev server, loads the page in a
headless browser and checks that the game reaches its initialisation. It
also expects the game to *report* the missing disc data: the disc is not
part of the repository, so a build without it has to fail with a message
instead of spinning in the fatal-error trap.
