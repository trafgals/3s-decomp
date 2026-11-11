#!/usr/bin/env python3
"""Apply the 3s' super art overrides to the retail table and print the result.

This is the end to end check of the data layer: it reads the retail super art
table out of PLCNT.c and the override tables out of upper3s/plXX.c and prints
what the patch actually does to every super art of every character.

    python3 tools/verify/upper3s_super_arts.py
"""
import os
import re
import sys

GAME = ["Gill", "Alex", "Ryu", "Yun", "Dudley", "Necro", "Hugo", "Ibuki",
        "Elena", "Oro", "Yang", "Ken", "Sean", "Urien", "Akuma", "Chun-Li",
        "Makoto", "Q", "Twelve", "Remy"]

# The retail file names of the per character tables, in the order of the
# character index of the game.
PL = {"Gill": "pl19", "Alex": "pl00", "Ryu": "pl01", "Yun": "pl02",
      "Dudley": "pl03", "Necro": "pl04", "Hugo": "pl05", "Ibuki": "pl06",
      "Elena": "pl07", "Oro": "pl08", "Yang": "pl09", "Ken": "pl10",
      "Sean": "pl15", "Urien": "pl12", "Akuma": "pl13", "Chun-Li": "pl14",
      "Makoto": "pl11", "Q": "pl16", "Twelve": "pl17", "Remy": "pl18"}

STORE_MAX = 0x01
GAUGE_LEN = 0x02
DTM = 0x04


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def retail_table(root):
    src = read(os.path.join(root, "src/anniversary/sf33rd/Source/Game/PLCNT.c"))
    m = re.search(r"super_arts_data\[20\]\[4\]\s*=\s*\{(.*?)\n\s*\};", src, re.S)
    rows = []
    for block in re.finditer(r"\{((?:\s*\{[^{}]*\}[^{}]*)+)\}", m.group(1)):
        entries = re.findall(r"\{([^{}]*)\}", block.group(1))
        if len(entries) != 4:
            continue
        vals = [[int(x) for x in e.replace("\n", " ").split(",") if x.strip()]
                for e in entries]
        rows.append(vals)
        if len(rows) == 20:
            break
    return rows


def overrides(root, name):
    """The three super art override rows of a character, in slot order."""
    path = os.path.join(root, "src/anniversary/sf33rd/Source/Game/upper3s", name + ".c")
    src = read(path)
    m = re.search(r"upper3s_super_art_\w+\[4\]\s*=\s*\{(.*?)\n\s*\};", src, re.S)
    if not m:
        return []
    out = []
    for row in re.findall(r"\{\s*([^,]+?)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\}",
                          m.group(1)):
        flags = 0
        for part in row[0].split("|"):
            part = part.strip()
            if part == "U3S_SA_STORE_MAX":
                flags |= STORE_MAX
            elif part == "U3S_SA_GAUGE_LEN":
                flags |= GAUGE_LEN
            elif part == "U3S_SA_DTM":
                flags |= DTM
        out.append((flags, int(row[1], 0), int(row[2], 0), int(row[3], 0)))
    return out


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    table = retail_table(root)
    if len(table) != 20:
        print("!! could not read the retail table (%d rows)" % len(table))
        return 1

    changed = 0
    for idx, char in enumerate(GAME):
        rows = overrides(root, PL[char])
        if len(rows) != 4:
            print("!! %s: %d override rows" % (char, len(rows)))
            continue
        parts = []
        for slot in range(3):
            flags, stock, length, dtm = rows[slot]
            retail = table[idx][slot]
            r_stock, r_length, r_dtm = retail[9], retail[8], retail[10]
            new_stock = stock if (flags & STORE_MAX) else r_stock
            new_length = length if (flags & GAUGE_LEN) else r_length
            new_dtm = dtm if (flags & DTM) else r_dtm
            mark = ""
            if (new_stock, new_length, new_dtm) != (r_stock, r_length, r_dtm):
                mark = "  <-"
                changed += 1
            parts.append("SA%d %d stocks %3d len%s" %
                         (slot + 1, new_stock, new_length, mark))
        print("%2d %-8s %s" % (idx, char, "   ".join(parts)))

    print("\n%d super art entries changed" % changed)
    return 0


if __name__ == "__main__":
    sys.exit(main())
