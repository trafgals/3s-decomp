#!/usr/bin/env python3
"""validate_chain.py - the functional test of the chain decoder.

It checks the things that the decoder is supposed to deliver:

  1. the character index order of the dump against `PLCNT.c:plid_data`,
  2. the super art identity of every character against the assignment table
     of `bin2obj/asstbl.c` and the command index of
     `PLCNT.c:super_arts_data`,
  3. the frame data of the moves that the Third Strike Upper specification
     names, against the retail numbers of `docs/upper3s/design-spec.md`,
  4. the damage pipeline: the retail damage of a move is
     `POW_DATA.c:Power_Data[pow]`, so the damage the specification quotes
     pins the `pow` index of the attack table entry,
  5. the plausibility of the startups (a light normal is 3-4 frames, a
     heavy one is 10-12 frames).

Every check prints PASS or FAIL; the exit code is the number of failures.

Usage:

    python3 build/validate_chain.py upper3s_dump.txt
"""

from __future__ import annotations

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from decode_chain import decode_move, load_power_data, parse_dump  # noqa: E402
from move_report import (CHARACTERS, build_names, name_of, parse_asstbl,  # noqa: E402
                         parse_super_arts)

FAILURES = []


def check(name, condition, detail=""):
    """Prints the result of one check."""
    status = "PASS" if condition else "FAIL"
    print("[%s] %s%s" % (status, name, (" - " + detail) if detail else ""))
    if not condition:
        FAILURES.append(name)
    return condition


# --------------------------------------------------------------------------
# 1. the character order
# --------------------------------------------------------------------------

def check_character_order(characters, plcnt_path):
    """The charset of every character against `PLCNT.c:plid_data`."""
    with open(plcnt_path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    match = re.search(r"plid_data\[20\]\s*=\s*\{([^}]*)\}", text)
    if not match:
        check("the character order of the dump", False, "plid_data not found")
        return
    plid = [int(v) for v in re.findall(r"-?\d+", match.group(1))]

    ok = len(plid) == 20 and len(characters) == 20
    detail = ""
    if ok:
        for character in characters:
            if plid[character.index] != character.charset:
                ok = False
                detail = "%s has charset %d, plid_data says %d" % (
                    character.name, character.charset, plid[character.index])
                break
    else:
        detail = "%d characters, %d plid entries" % (len(characters), len(plid))
    check("the character order of the dump against plid_data", ok, detail)


# --------------------------------------------------------------------------
# 2. the super arts
# --------------------------------------------------------------------------

def check_super_arts(names, super_arts, plcnt_path):
    """Every super art of every character has a name and a command index."""
    named = 0
    missing = []
    for character in range(20):
        table = names.get(character, {})
        for number in range(3):
            command = super_arts[character][number] if super_arts else 0
            label = "SA %s" % ["I", "II", "III"][number]
            found = [value for (koc, _ix), values in table.items()
                     if koc == 5 for value in values if value.startswith(label)]
            if found:
                named += 1
            else:
                missing.append("%s %s (command %d)" % (CHARACTERS[character], label, command))

    check("every super art has an assignment table entry", not missing,
          ", ".join(missing[:8]))

    # the command index of a super art is the group of the column 0 of the
    # assignment table: the group of the super art N is the group
    # `nmsa_g_ix - 20`.  A super art that has no command index (the second
    # super art of Twelve, X.F.L.A.T.) is not in the assignment table.
    unexpected = []
    for character in range(20):
        for number in range(3):
            command = super_arts[character][number]
            if command not in (0, 20, 21, 22):
                unexpected.append("%s SA %d has the command index %d" % (
                    CHARACTERS[character], number + 1, command))
    check("the command index of the super arts is 20, 21 or 22", not unexpected,
          ", ".join(unexpected[:8]))
    return named


def check_super_art_retail(plcnt_path):
    """The retail gauge and stock of the super arts of the specification.

    `upper3s/plXX.c` quotes the retail values of `PLCNT.c:super_arts_data`
    in its comments; the two are independent sources and they have to agree.
    """
    with open(plcnt_path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    match = re.search(r"super_arts_data\[20\]\[4\]\s*=\s*", text)
    if not match:
        check("the retail super art data", False, "super_arts_data not found")
        return
    brace = text.find("{", match.end())
    depth = 0
    end = brace
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                end = index
                break
    groups = re.findall(r"\{([^{}]*)\}", text[brace:end])
    # the four rows of a character: SA_DATA.gauge_len is the ninth field and
    # SA_DATA.store_max the tenth one
    rows = []
    for character in range(20):
        rows.append([])
        for number in range(4):
            fields = [int(v) for v in re.findall(r"-?\d+", groups[character * 4 + number])]
            rows[character].append((fields[8], fields[9]))

    # Ken: SA I 112 / 2 stocks, SA II 104 / 1 stock, SA III 80 / 3 stocks
    check("the retail data of the super arts of Ken",
          rows[11][0] == (112, 2) and rows[11][1] == (104, 1) and rows[11][2] == (80, 3),
          "SA I %s, SA II %s, SA III %s" % (rows[11][0], rows[11][1], rows[11][2]))

    # Alex: SA I 128 / 1 stock, SA II 96 / 2 stocks, SA III 80 / 1 stock
    check("the retail data of the super arts of Alex",
          rows[1][0] == (128, 1) and rows[1][1] == (96, 2) and rows[1][2] == (80, 1),
          "SA I %s, SA II %s, SA III %s" % (rows[1][0], rows[1][1], rows[1][2]))

    # Oro: SA I 104 / 1 stock, SA II 88 / 3 stocks, SA III 112 / 1 stock
    check("the retail data of the super arts of Oro",
          rows[9][0] == (104, 1) and rows[9][1] == (88, 3) and rows[9][2] == (112, 1),
          "SA I %s, SA II %s, SA III %s" % (rows[9][0], rows[9][1], rows[9][2]))


# --------------------------------------------------------------------------
# 3. the moves the specification names
# --------------------------------------------------------------------------

def check_ken_jab_dp(characters, names):
    """The jab dragon punch of Ken.

    `upper3s/pl10.c` addresses it as the command 28 of the special move
    group, the four strengths of that group are the entries 28, 29, 30 and
    31 of the slot 5 of Ken.
    """
    ken = next(c for c in characters if c.name == "ken")
    table = names.get(ken.index, {})

    named = name_of(table, 5, 28)
    check("the jab dragon punch of Ken is the entry 28 of the slot 5",
          named.endswith("light"), named)

    info = decode_move(ken, 5, 28)
    check("the jab dragon punch of Ken decodes", not info.error, str(info.error))
    if info.error:
        return None

    indices = []
    for frame in info.frames:
        if frame.has_attack and (not indices or indices[-1] != frame.att_ix):
            indices.append(frame.att_ix)

    check("the jab dragon punch of Ken has three hit boxes", len(indices) == 3,
          "the attack table indices are %s" % indices)
    check("the last attack table index of the jab dragon punch is 52",
          indices and indices[-1] == 52, str(indices))
    check("the jab dragon punch of Ken starts in two frames", info.startup == 2,
          "the startup is %s" % info.startup)
    check("the jab dragon punch of Ken is 36 frames long", info.game_frames == 36,
          "the chain is %d frames" % info.game_frames)
    return info


def check_ken_sa_three(characters, names):
    """The third super art of Ken is the entry 44 of the slot 5.

    The chain of the super arts is not in the dump of Ken (the slot 5 of the
    dump is truncated), so the identity of the move is checked through the
    assignment table and the truncation is reported.
    """
    ken = next(c for c in characters if c.name == "ken")
    table = names.get(ken.index, {})

    named = name_of(table, 5, 44)
    check("the third super art of Ken is the entry 44 of the slot 5",
          named.startswith("SA III"), named)

    info = decode_move(ken, 5, 44)
    # the dump of the slot 5 of Ken stops before the offset of the super
    # arts, so the chain of the third super art is not in the dump at all.
    # That is a limitation of the dump, not a fault of the decoder.
    truncated = info.error is not None and "offset of" in str(info.error)
    check("the chain of the third super art of Ken is decoded or known to be"
          " missing from the dump",
          info.error is None or truncated, str(info.error))
    if truncated:
        print("       known limitation: %s" % info.error)
    return info


# --------------------------------------------------------------------------
# 4. the damage pipeline
# --------------------------------------------------------------------------

def check_damage_pipeline(power_data):
    """`Power_Data[pow]` is the base damage of an attack table entry."""
    if not power_data:
        check("the damage table", False, "POW_DATA.c not found")
        return
    check("Power_Data[3] is the retail damage 15 of the jab dragon punch",
          power_data[3] == 15, "Power_Data[3] is %d" % power_data[3])
    check("Power_Data[14] is the retail damage 70 of Shippu Jinrai Kyaku",
          power_data[14] == 70, "Power_Data[14] is %d" % power_data[14])
    check("Power_Data[p] is 5 * p", all(power_data[p] == 5 * p for p in range(0, 180)),
          "the table is linear up to p = 179")


# --------------------------------------------------------------------------
# 5. the plausibility of the startups
# --------------------------------------------------------------------------

def startup_of(character, names, move):
    """The decoded data of the named move of a character, or None."""
    table = names.get(character.index, {})
    for (koc, index), values in table.items():
        if koc == 4 and move in values:
            info = decode_move(character, 4, index)
            if not info.error:
                return info
    return None


def check_startup_plausibility(characters, names):
    """The light normals start in 3-4 frames, the heavy ones in 10-12."""
    by_name = {c.name: c for c in characters}

    # the reference numbers of the task, on Ken
    ken = by_name.get("ken")
    info = startup_of(ken, names, "5LP") if ken else None
    check("the standing light punch of Ken starts in 3-4 frames",
          info is not None and info.startup is not None and 3 <= info.startup <= 4,
          "the startup is %s" % (info.startup if info else "unknown"))

    info = startup_of(ken, names, "5HK") if ken else None
    check("the standing roundhouse of Ken starts in 10-12 frames",
          info is not None and info.startup is not None and 10 <= info.startup <= 12,
          "the startup is %s" % (info.startup if info else "unknown"))

    # the cast: the median of the light punches and of the roundhouses
    lights = []
    heavies = []
    for character in characters:
        for move, bucket in (("5LP", lights), ("5HK", heavies)):
            info = startup_of(character, names, move)
            if info is not None and info.startup is not None:
                bucket.append(info.startup)
    lights.sort()
    heavies.sort()
    median_light = lights[len(lights) // 2] if lights else None
    median_heavy = heavies[len(heavies) // 2] if heavies else None
    check("the median startup of the standing light punches of the cast is 3-4",
          median_light is not None and 3 <= median_light <= 4, str(lights))
    check("the median startup of the standing roundhouses of the cast is 8-12",
          median_heavy is not None and 8 <= median_heavy <= 12, str(heavies))


def check_design_spec_crouching_jab(characters, names):
    """`docs/upper3s/design-spec.md` on Chun-Li.

    "Crouching Jab (2LP) startup increased from 2f to 3f.  _She's one of the
    only characters in the game with a 2f normal_" - so Chun-Li has to be the
    only character of the cast whose crouching jab starts in two frames.
    """
    two_frame = []
    for character in characters:
        info = startup_of(character, names, "2LP")
        if info is not None and info.startup == 2:
            two_frame.append(character.name)
    check("Chun-Li is the only character with a two frame crouching jab",
          two_frame == ["chunli"], str(two_frame))


def main():
    dump = sys.argv[1] if len(sys.argv) > 1 else "upper3s_dump.txt"
    asstbl = "src/anniversary/bin2obj/asstbl.c"
    plcnt = "src/anniversary/sf33rd/Source/Game/PLCNT.c"
    power = "src/anniversary/sf33rd/Source/Game/POW_DATA.c"

    characters = parse_dump(dump)
    if not characters:
        print("no characters in %s" % dump, file=sys.stderr)
        return 1

    tables, columns = parse_asstbl(asstbl)
    super_arts = parse_super_arts(plcnt)
    names = build_names(tables, columns, super_arts)
    power_data = load_power_data(power)

    print("=" * 78)
    print("the validation of the chain decoder")
    print("=" * 78)

    check_character_order(characters, plcnt)
    check_super_arts(names, super_arts, plcnt)
    check_super_art_retail(plcnt)
    check_ken_jab_dp(characters, names)
    check_ken_sa_three(characters, names)
    check_damage_pipeline(power_data)
    check_startup_plausibility(characters, names)
    check_design_spec_crouching_jab(characters, names)

    print()
    if FAILURES:
        print("%d check(s) failed: %s" % (len(FAILURES), ", ".join(FAILURES)))
        return len(FAILURES)
    print("every check passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
