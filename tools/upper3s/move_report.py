#!/usr/bin/env python3
"""move_report.py - the frame data of the moves of the Third Strike Upper
characters, with the moves named by the assignment tables of the disc.

The chain program of the disc is decoded by `decode_chain.py`; this script
adds the names and prints the result as a table.

Where the names come from
-------------------------

`bin2obj/asstbl.c` holds the assignment tables of the game.  They map the
command that the player made to the move of the character data that plays
it:

  * `_assadr_lv_9900[character][0]` are the super arts.  The group of four
    entries that belongs to the command index of a super art holds its four
    strengths.  The command index of a super art is `SA_DATA.nmsa_g_ix` of
    `PLCNT.c:super_arts_data`, so the group of the super art number N is the
    group `nmsa_g_ix - 20` of the column 0.

  * `_assadr_lv_9900[character][1]` are the special moves.  The group of
    four entries that belongs to the command index of a special move holds
    its four strengths; the command index of the group is `28 + group`.

  * `_asstbl_lv_0010`, `_asstbl_lv_1010`, `_asstbl_lv_2010`,
    `_asstbl_lv_3010` and `_asstbl_lv_4010` are the normal attacks.  They
    are indexed by the player, by the button (`kos`) and by the variant
    (`koa`); the `char_ix` of the entry is the index of the move in the
    normal move slot (`char_table[4]`).

Usage:

    python3 build/move_report.py upper3s_dump.txt --char ken
    python3 build/move_report.py upper3s_dump.txt --char ken --slot 5
    python3 build/move_report.py upper3s_dump.txt --check
"""

from __future__ import annotations

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from decode_chain import (attack_table_of, decode_slot, load_attack_table,  # noqa: E402
                          load_power_data, parse_dump)

# the character index the game uses, the order of `PLCNT.c:plid_data`
CHARACTERS = ["gill", "alex", "ryu", "yun", "dudley", "necro", "hugo", "ibuki", "elena", "oro",
              "yang", "ken", "sean", "urien", "akuma", "chunli", "makoto", "q", "twelve", "remy"]

# the kind of character data, `include/sf33rd/Source/Game/upper3s.h`
KOC_REACTION = 0
KOC_NORMAL = 4      # atca - the normal attacks
KOC_ATTACK = 5      # saca - the special moves and the super arts

# the buttons of the normal move tables
BUTTONS = ["LP", "MP", "HP", "LK", "MK", "HK"]

# the variants of the normal move tables; the first two tables are the
# standing and the crouching ones, the comment of `upper3s/pl00.c` names the
# inputs of their three variants
STANDING_VARIANTS = ["5%s", "6%s", "4%s"]
CROUCHING_VARIANTS = ["2%s", "3%s", "1%s"]

# the name of the normal move tables and the base of their `char_ix`
NORMAL_TABLES = [("_asstbl_lv_0010", 0, "standing"),
                 ("_asstbl_lv_1010", 18, "crouching"),
                 ("_asstbl_lv_2010", 36, "pat 22/16/28"),
                 ("_asstbl_lv_3010", 48, "pat 20/14/26"),
                 ("_asstbl_lv_4010", 60, "pat 24/18/30")]

STRENGTHS = ["light", "medium", "heavy", "EX"]
SA_NUMBERS = ["SA I", "SA II", "SA III", "SA IV"]


# --------------------------------------------------------------------------
# the assignment tables of `bin2obj/asstbl.c`
# --------------------------------------------------------------------------

AS_TABLE_RE = re.compile(r"DATA_SECTION AS\*?\s*(\w+)((?:\[\d+\])*)\s*=\s*")
AS_ENTRY_RE = re.compile(r"\{\s*\.r_no\s*=\s*(-?\d+)\s*,\s*\.char_ix\s*=\s*(-?\d+)\s*,"
                         r"\s*\.data_ix\s*=\s*(-?\d+)\s*\}")
ASSADR_RE = re.compile(r"_assadr_lv_9900\[20\]\[4\]\s*=\s*")


def body_of(text, start):
    """The text of a brace block that starts at `start` (the opening brace)."""
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:index]
    return ""


def parse_asstbl(path):
    """Reads the assignment tables of `bin2obj/asstbl.c`.

    Returns the tables by name and the four columns of every character.
    """
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    tables = {}
    for match in AS_TABLE_RE.finditer(text):
        brace = text.find("{", match.end())
        if brace < 0:
            continue
        body = body_of(text, brace)
        entries = AS_ENTRY_RE.findall(body)
        tables[match.group(1)] = [(int(r), int(c), int(d)) for r, c, d in entries]

    # the four columns of every character
    columns = {}
    match = ASSADR_RE.search(text)
    if match:
        brace = text.find("{", match.end())
        body = body_of(text, brace)
        # every row of the table is a group of four table names
        rows = re.findall(r"\{([^{}]*)\}", body)
        for index, row in enumerate(rows):
            # the self reference of the table is written as a cast
            names = re.findall(r"\w+", row.replace("(AS*)", ""))
            if len(names) == 4:
                columns[index] = names

    return tables, columns


def parse_super_arts(path):
    """Reads `PLCNT.c:super_arts_data`, the command index of every super art.

    Returns a list of four lists: the command index of the super art number
    N of every character.
    """
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    match = re.search(r"super_arts_data\[20\]\[4\]\s*=\s*", text)
    if not match:
        return []
    brace = text.find("{", match.end())
    body = body_of(text, brace)

    groups = re.findall(r"\{([^{}]*)\}", body)
    result = []
    for character in range(20):
        row = []
        for number in range(4):
            fields = [int(v) for v in re.findall(r"-?\d+", groups[character * 4 + number])]
            row.append(fields[0] if fields else 0)     # SA_DATA.nmsa_g_ix
        result.append(row)
    return result


# --------------------------------------------------------------------------
# the names of the moves
# --------------------------------------------------------------------------

def build_names(tables, columns, super_arts):
    """Builds the name of every move of every character.

    The result is a dictionary: character index -> (koc, char_ix) -> list of
    names.  A list, because several moves can share the same entry of the
    character data (Gill, for instance, plays his three super arts with the
    same chain).
    """
    names = {}

    for character in range(20):
        table = {}

        def add(koc, char_ix, text, zero_is_a_move=False):
            # the index 0 of the normal move table is a real move, the index 0
            # of the super art and of the special move table is "no move"
            if char_ix < 0 or (not zero_is_a_move and not char_ix):
                return
            table.setdefault((koc, char_ix), []).append(text)

        # -- the super arts: the group of the command index of the super art
        column = columns.get(character, [None] * 4)[0]
        entries = tables.get(column, []) if column else []
        if entries and super_arts:
            for number, command in enumerate(super_arts[character]):
                if command < 20:
                    continue
                group = command - 20
                for strength in range(4):
                    position = group * 4 + strength
                    if position < len(entries):
                        add(KOC_ATTACK, entries[position][1],
                            "%s %s" % (SA_NUMBERS[number] if number < len(SA_NUMBERS)
                                       else "SA ?", STRENGTHS[strength]))

        # -- the special moves: the group of the command index of the move
        column = columns.get(character, [None] * 4)[1]
        entries = tables.get(column, []) if column else []
        for group in range(len(entries) // 4):
            for strength in range(4):
                add(KOC_ATTACK, entries[group * 4 + strength][1],
                    "special %d %s" % (28 + group, STRENGTHS[strength]))

        # -- the normal attacks
        for name, base, label in NORMAL_TABLES:
            entries = tables.get(name, [])
            for kos in range(6):
                for koa in range(3):
                    position = character * 18 + kos * 3 + koa
                    if position >= len(entries):
                        continue
                    char_ix = entries[position][1]
                    if char_ix < 0:
                        continue
                    if label == "standing":
                        text = STANDING_VARIANTS[koa] % BUTTONS[kos]
                    elif label == "crouching":
                        text = CROUCHING_VARIANTS[koa] % BUTTONS[kos]
                    else:
                        # the second and the third variant of a button share
                        # the move of the first one, only the last variant has
                        # a move of its own
                        text = "%s %s" % (label, BUTTONS[kos])
                    key = (KOC_NORMAL, char_ix)
                    if text not in table.get(key, []):
                        add(KOC_NORMAL, char_ix, text, True)

        names[character] = table

    return names


def name_of(table, koc, index):
    """The name of a move, the names of a shared move are joined."""
    entries = table.get((koc, index))
    if not entries:
        return ""
    return " / ".join(entries)


# --------------------------------------------------------------------------
# the report
# --------------------------------------------------------------------------

def move_note(info):
    """The note of a move: the things that are not certain."""
    note = []
    if info.branches:
        note.append("%d branch(es)" % len(info.branches))
    if info.cycle:
        note.append("loops")
    if info.terminal:
        note.append("0xFF end")
    elif info.wait:
        note.append("hold end")
    else:
        note.append("no end marker")
    return " ".join(note)


def first_hit_frame(info):
    """The game frame the attack data is loaded on, counted from 1.

    `CHARSET.c:check_cgd_patdat2()` loads the attack data (`set_new_attnum`)
    when the block carries an attack table index, so that frame is the first
    frame on which the move can hit.
    """
    if info.attack_frame is None:
        return None
    return 1 + sum(frame.hold for frame in info.frames[:info.frames.index(info.attack_frame)])


def attack_indices(info):
    """The attack table indices of the move, in the order they appear."""
    indices = []
    for frame in info.frames:
        if frame.has_attack and (not indices or indices[-1] != frame.att_ix):
            indices.append(frame.att_ix)
    return indices


def print_character(character, names, slot_filter=None, show_all=False, table=None,
                    power_data=None):
    """Prints the table of the decoded moves of one character."""
    names_of_character = names.get(character.index, {})
    print("=" * 104)
    print("%s - character index %d, charset %d" % (character.name, character.index,
                                                   character.charset))
    print("=" * 104)

    header = "%-4s %-5s %-22s %6s %6s %6s %6s %-16s %6s %5s %5s %5s %5s %s" % (
        "slot", "idx", "name", "frames", "start", "active", "act", "attack table",
        "damage", "stun", "hs_me", "hs_you", "impact", "note")
    print(header)
    print("-" * len(header))

    decoded = 0
    failed = 0
    for koc in sorted(character.moves):
        if slot_filter is not None and koc != slot_filter:
            continue
        for info in decode_slot(character, koc):
            name = name_of(names_of_character, koc, info.index)
            if info.error:
                failed += 1
                print("%-4d %-5d %-22s %s" % (koc, info.index, name, "ERROR: " + info.error))
                continue
            decoded += 1
            if not show_all and not name:
                continue
            indices = attack_indices(info)
            attack = info.attack(table)
            damage = info.damage(table, power_data)
            print("%-4d %-5d %-22s %6d %6s %6d %6s %-16s %6s %5s %5s %5s %5s %s" % (
                koc,
                info.index,
                name,
                info.game_frames,
                "-" if info.startup is None else info.startup,
                info.active,
                "-" if first_hit_frame(info) is None else first_hit_frame(info),
                ",".join(str(a) for a in indices) if indices else "-",
                "-" if damage is None else damage,
                "-" if attack is None else attack["piyo"],
                "-" if attack is None else attack["hs_me"],
                "-" if attack is None else attack["hs_you"],
                "-" if attack is None else attack["impact"],
                move_note(info)))
    print()
    print("  %d moves decoded, %d could not be decoded" % (decoded, failed))
    print()


def write_csv(path, characters, names, table=None, power_data=None):
    """Writes the decoded moves of every character into a CSV file."""
    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write("character,koc,index,name,game_frames,startup,active,first_active_frame,"
                     "attack_indices,damage,stun,branches,loops,error\n")
        for character in characters:
            names_of_character = names.get(character.index, {})
            table_of_character = attack_table_of(table, character)
            for koc in sorted(character.moves):
                for info in decode_slot(character, koc):
                    attack = info.attack(table_of_character)
                    damage = info.damage(table_of_character, power_data)
                    handle.write("%s,%d,%d,%s,%d,%s,%d,%s,%s,%s,%s,%d,%d,%s\n" % (
                        character.name, koc, info.index,
                        name_of(names_of_character, koc, info.index),
                        info.game_frames,
                        "" if info.startup is None else info.startup,
                        info.active,
                        "" if first_hit_frame(info) is None else first_hit_frame(info),
                        " ".join(str(a) for a in attack_indices(info)),
                        "" if damage is None else damage,
                        "" if attack is None else attack["piyo"],
                        len(info.branches),
                        1 if info.cycle else 0,
                        info.error or ""))


def check(character, koc, index):
    """The decoded data of one move, for the validation."""
    for info in decode_slot(character, koc):
        if info.index == index:
            return info
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("dump", nargs="?", default="upper3s_dump.txt",
                        help="the dump of the character data of the disc")
    parser.add_argument("--char", action="append", default=[],
                        help="a character, may be given more than once")
    parser.add_argument("--slot", type=int, default=None,
                        help="only the moves of this kind of character data")
    parser.add_argument("--all", action="store_true", help="every move, not only the named ones")
    parser.add_argument("--asstbl", default="src/anniversary/bin2obj/asstbl.c")
    parser.add_argument("--plcnt", default="src/anniversary/sf33rd/Source/Game/PLCNT.c")
    parser.add_argument("--power-data", default="src/anniversary/sf33rd/Source/Game/POW_DATA.c")
    parser.add_argument("--att-table", default=None,
                        help="the attack table of the characters (see parse_att_table)")
    parser.add_argument("--startup", action="store_true",
                        help="print the startup of every move of the character")
    parser.add_argument("--csv", help="write the table of every move into a CSV file")
    args = parser.parse_args()

    characters = parse_dump(args.dump)
    if not characters:
        print("no characters in %s" % args.dump, file=sys.stderr)
        return 1

    tables, columns = parse_asstbl(args.asstbl)
    super_arts = parse_super_arts(args.plcnt)
    names = build_names(tables, columns, super_arts)

    if args.csv:
        write_csv(args.csv, characters, names,
                  load_attack_table(args.att_table), load_power_data(args.power_data))
        print("wrote %s" % args.csv)
        if not args.char:
            return 0

    wanted = args.char or CHARACTERS
    tables = load_attack_table(args.att_table)
    power_data = load_power_data(args.power_data)
    if args.att_table and tables is None:
        print("the attack table %s does not exist" % args.att_table, file=sys.stderr)
    for character in characters:
        if character.name not in wanted:
            continue
        if args.startup:
            print("--- %s: the startup of every move of the named groups ---" % character.name)
            table = names.get(character.index, {})
            for koc in sorted(character.moves):
                for info in decode_slot(character, koc):
                    name = name_of(table, koc, info.index)
                    if not name or info.error:
                        continue
                    print("%-4d %-5d %-22s startup %-4s frames %-4d att %d" % (
                        koc, info.index, name,
                        "-" if info.startup is None else info.startup,
                        info.game_frames, info.att_ix))
            print()
            continue
        print_character(character, names, args.slot, args.all,
                        attack_table_of(tables, character), power_data)

    return 0


if __name__ == "__main__":
    sys.exit(main())
