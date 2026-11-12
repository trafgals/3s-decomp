#!/usr/bin/env python3
"""Computes the advantage of a hit from the retail data of the disc.

The design document of Third Strike Upper asks for a number of moves to have
"more hitstun, so they are not negative on hit".  The advantage of a hit is
not a number that is written down anywhere: it is the difference between how
long the defender is held and how long the attacker is locked.

This script computes it from the data of the disc, so that the moves the
design document names can be given a value that is derived and not guessed.

----------------------------------------------------------------- the model

The hit lands on the first frame the attack data is loaded on.  From that
moment on:

  * the attacker is frozen for `hs_me` frames (`HITCHECK.c:dm_status_copy()`
    puts the value of the attack data into `hit_stop`), and the rest of its
    animation is the recovery, so the attacker is locked for
    `hs_me + recovery` frames, where `recovery` is the number of frames of
    the move that follow the frame of the hit;

  * the defender is held in the hit reaction.  `HITCHECK.c:dm_status_copy()`
    puts `hs_you` into `dm_stop`, and `PLS02.c:set_hit_stop_hit_quake()` moves
    it into `hit_stop`, so the defender is held for `|hs_you|` frames.  After
    that the reaction itself runs: the reaction animation of the character
    (`PLPDM.c:Damage_12000()` plays the move `dm_reaction_table[react].char_ix
    + level` of the reaction slot) and the pause of the reaction
    (`_damage_pause_table[level]` frames).

    The sign of `hs_you` says how the defender is held: a positive value
    freezes the defender, a negative one lets the reaction animation run
    while the hit stop counts down.

  * the field `impact` of the attack data is *not* the hitstun: it selects
    the step table of the reaction (`PLPDM.c:add_dm_step_tbl()`), which is the
    knockback of the hit, not its length.  The hitstun of a move is the hold
    of the defender, and that is `hs_you`.

The advantage is therefore

    advantage = |hs_you| + reaction + pause - recovery

which is the number of frames the defender is still in the hit reaction when
the attacker can act again.  It is negative when the attacker is punished for
the hit.
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)

from decode_chain import (attack_table_of, decode_slot,  # noqa: E402
                           load_attack_table, parse_dump)
from move_report import (attack_indices, build_names, first_hit_frame,  # noqa: E402
                         name_of, parse_asstbl, parse_super_arts)

REPO = "src/anniversary"


def read(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        return handle.read()


def parse_c_table(text, name):
    """Reads a C table of numbers out of a source file."""
    match = re.search(r"\b%s\s*\[[^=]*=\s*" % re.escape(name), text)
    if not match:
        raise SystemExit("the table %s is not in the file" % name)

    brace = text.find("{", match.end())
    if brace < 0:
        raise SystemExit("the table %s has no body" % name)

    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                body = text[brace:index + 1]
                break
    else:
        raise SystemExit("the table %s is not closed" % name)

    rows = []
    for row in re.findall(r"\{([^{}]*)\}", body):
        values = [int(value) for value in re.findall(r"-?\d+", row)]
        if values:
            rows.append(values)

    return rows


def parse_reaction_table(text):
    """Reads `dm_reaction_table`, the table of the damage reactions."""
    match = re.search(r"dm_reaction_table\s*\[[^\]]*\]\s*=\s*\{", text)
    if not match:
        raise SystemExit("dm_reaction_table is not in the file")

    body = text[match.end():]
    rows = re.findall(r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", body)

    return [(int(a), int(b), int(c)) for a, b, c in rows]


class Tables(object):
    """The retail tables that decide how long a hit holds the defender."""

    def __init__(self, root):
        etc = read(os.path.join(root, REPO, "bin2obj", "etc.c"))
        pldm = read(os.path.join(root, REPO, "sf33rd", "Source", "Game", "PLPDM.c"))

        self.select_hit = parse_c_table(etc, "_select_hit_dsd")
        self.select_grd = parse_c_table(etc, "_select_grd_dsd")
        self.dm_step = parse_c_table(etc, "_dm_step_data")
        self.damage_pause = parse_c_table(etc, "_damage_pause_table")
        self.guard_pause = parse_c_table(etc, "_guard_pause_table")
        self.reactions = parse_reaction_table(pldm)

    def reaction_move(self, reaction, level):
        """The move of the reaction slot that the reaction of a level plays."""
        if reaction >= len(self.reactions):
            return None
        return self.reactions[reaction][1] + level

    def step_frames(self, impact, weight_point=3):
        """The number of frames the step table of the reaction holds."""
        if impact >= len(self.select_hit):
            return 0
        row = self.select_hit[impact]
        if weight_point >= len(row):
            return 0
        index = row[weight_point]
        if index >= len(self.dm_step):
            return 0
        return sum(self.dm_step[index])


def advantage(attack, info, tables, reactions):
    """The advantage of the hit of a move, in frames.

    `reactions` maps the index of a move of the reaction slot to the number
    of frames of that move.
    """
    if attack is None or info is None:
        return None

    hit_frame = first_hit_frame(info)
    if hit_frame is None:
        return None

    level = attack["level"] & 7
    if level >= len(tables.damage_pause[0]):
        # The level of the attack data is out of range for a few entries of
        # the retail table; the last column of the table is the closest one.
        level = len(tables.damage_pause[0]) - 1

    hold = abs(attack["hs_you"])
    pause = tables.damage_pause[0][level]

    reaction_move = tables.reaction_move(attack["reaction"], level)
    reaction = reactions.get(reaction_move, 0) if reaction_move is not None else 0

    recovery = info.game_frames - hit_frame

    return hold + reaction + pause - recovery


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("dump", nargs="?", default="upper3s_dump.txt")
    parser.add_argument("--char", action="append", default=[])
    parser.add_argument("--slot", type=int, default=4)
    parser.add_argument("--root", default=ROOT)
    parser.add_argument("--att-table", default=None)
    parser.add_argument("--no-step", action="store_true",
                        help="kept for compatibility; the step table is the knockback")
    parser.add_argument("--csv", default=None)
    args = parser.parse_args()

    characters = parse_dump(args.dump)
    if not characters:
        print("no characters in %s" % args.dump, file=sys.stderr)
        return 1

    tables = Tables(args.root)
    tables_by_character = load_attack_table(args.att_table)

    tables_c, columns = parse_asstbl(os.path.join(args.root, REPO, "bin2obj", "asstbl.c"))
    super_arts = parse_super_arts(os.path.join(args.root, REPO, "sf33rd", "Source", "Game", "PLCNT.c"))
    names = build_names(tables_c, columns, super_arts)

    # The reaction animations of every character: the moves of the reaction
    # slot are the hit reactions, and their length is part of the hitstun.
    reactions = {}

    rows = []
    for character in characters:
        table = attack_table_of(tables_by_character, character)
        names_of_character = names.get(character.index, {})

        for info in decode_slot(character, 1):
            reactions[(character.index, info.index)] = info.game_frames

        if args.char and character.name not in args.char:
            continue

        for info in decode_slot(character, args.slot):
            name = name_of(names_of_character, args.slot, info.index)
            attack = info.attack(table)
            value = advantage(attack, info, tables, {
                key[1]: value for key, value in reactions.items() if key[0] == character.index
            })

            rows.append((character.name, args.slot, info.index, name, info.game_frames,
                         first_hit_frame(info), attack, value))

    if args.csv:
        with open(args.csv, "w", encoding="utf-8", newline="") as handle:
            handle.write("character,koc,index,name,game_frames,hit_frame,damage,stun,hs_me,hs_you,"
                         "impact,level,advantage\n")
            for row in rows:
                attack = row[6]
                handle.write("%s,%d,%d,%s,%d,%s,%s,%s,%s,%s,%s,%s,%s\n" % (
                    row[0], row[1], row[2], row[3], row[4],
                    "" if row[5] is None else row[5],
                    "" if attack is None else attack["pow"],
                    "" if attack is None else attack["piyo"],
                    "" if attack is None else attack["hs_me"],
                    "" if attack is None else attack["hs_you"],
                    "" if attack is None else attack["impact"],
                    "" if attack is None else attack["level"],
                    "" if row[7] is None else row[7]))
        print("wrote %s" % args.csv)
        return 0

    header = "%-8s %-5s %-22s %6s %5s %6s %6s %6s %6s %8s" % (
        "char", "idx", "name", "frames", "hit", "hs_me", "hs_you", "impact", "level", "advantage")
    print(header)
    print("-" * len(header))

    for row in rows:
        attack = row[6]
        print("%-8s %-5d %-22s %6d %5s %6s %6s %6s %6s %8s" % (
            row[0], row[2], row[3], row[4],
            "-" if row[5] is None else row[5],
            "-" if attack is None else attack["hs_me"],
            "-" if attack is None else attack["hs_you"],
            "-" if attack is None else attack["impact"],
            "-" if attack is None else attack["level"],
            "-" if row[7] is None else row[7]))

    return 0


if __name__ == "__main__":
    sys.exit(main())
