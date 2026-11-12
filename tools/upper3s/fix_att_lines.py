#!/usr/bin/env python3
"""Joins the entries of the ATT lines of the dump into one line each.

The dump tool writes the entries of the attack table of a character on one
line, the way it writes the data of a slot.  This script fixes a dump that
was written by the older version of the tool, which wrote one entry per line.
"""
import sys

PATH = sys.argv[1] if len(sys.argv) > 1 else "upper3s_dump.txt"

with open(PATH, encoding="utf-8", errors="replace") as handle:
    lines = handle.read().split("\n")

out = []
i = 0
joined = 0

while i < len(lines):
    line = lines[i]
    out.append(line)

    if line.startswith("ATT "):
        parts = line.split()
        count = int(parts[3])
        entries = []

        while len(entries) < count and i + 1 < len(lines):
            nxt = lines[i + 1]
            if not nxt or len(nxt) != 32:
                break
            entries.append(nxt)
            i += 1

        if entries:
            out[-1] = line
            out.append("".join(entries))
            joined += 1

    i += 1

with open(PATH, "w", encoding="utf-8", newline="\n") as handle:
    handle.write("\n".join(out))

print(f"joined {joined} attack tables")
