#!/usr/bin/env python3
"""Prints the assignment tables of bin2obj/asstbl.c.

The assignment tables tell which move of the character data a button and a
direction select.  Each entry has a routine number (r_no, the behaviour class
the state machine dispatches on) and the char_ix, the index of the move in the
slot 4 (the normal attacks) of the character data.

    python3 build/asstbl_dump.py [player ...]
"""
import re
import sys

SRC = "src/anniversary/bin2obj/asstbl.c"

# The five tables that hold the normal attacks, with the player count.
TABLES = [
    "_asstbl_lv_0010",  # standing
    "_asstbl_lv_1010",  # crouching
    "_asstbl_lv_2010",  # the neutral jump
    "_asstbl_lv_3010",  # the crouching jump
    "_asstbl_lv_4010",  # the jump that goes down
]

# The name of the button of a kos.
KOS = ["LP", "MP", "HP", "LK", "MK", "HK"]

ENTRY = re.compile(r"\{\s*\.r_no\s*=\s*(-?\d+)\s*,\s*\.char_ix\s*=\s*(-?\d+)\s*,\s*\.data_ix\s*=\s*(-?\d+)\s*\}")


def parse(text):
    """Returns {table: [[entry, ...], ...]} with one list per player."""
    out = {}
    for name in TABLES:
        m = re.search(r"AS\s+%s\[20\]\[6\]\[3\]\s*=\s*\{" % name, text)
        if not m:
            continue
        # The table ends at the "};" that closes it.
        end = text.index("};", m.end())
        body = text[m.end():end]
        entries = ENTRY.findall(body)
        if len(entries) != 20 * 6 * 3:
            raise SystemExit("%s: %d entries, expected %d" % (name, len(entries), 20 * 6 * 3))
        players = []
        for p in range(20):
            chunk = entries[p * 18:(p + 1) * 18]
            players.append([(int(r_no), int(char_ix), int(data_ix)) for r_no, char_ix, data_ix in chunk])
        out[name] = players
    return out


def main():
    players = [int(a) for a in sys.argv[1:]] or [0]
    with open(SRC, encoding="utf-8", errors="replace") as f:
        text = f.read()
    tables = parse(text)

    for p in players:
        print("=" * 78)
        print("player %d" % p)
        print("=" * 78)
        for name in TABLES:
            if name not in tables:
                continue
            print("  %s" % name)
            print("    %-4s %-4s %-5s %-7s %s" % ("kos", "koa", "r_no", "char_ix", "data_ix"))
            for kos in range(6):
                for koa in range(3):
                    r_no, char_ix, data_ix = tables[name][p][kos * 3 + koa]
                    print("    %-4s %-4d %-5d %-7d %d" % (KOS[kos], koa, r_no, char_ix, data_ix))
    print()


if __name__ == "__main__":
    main()
