#!/usr/bin/env python3
"""Normalises preprocessed C source for comparison.

Whitespace outside of string literals carries no meaning for the compiler,
so it is collapsed before two preprocessed sources are compared. String
literals are left alone: their contents end up in the binary.
"""

import sys


def normalise(source: str) -> str:
    out = []
    index = 0
    length = len(source)
    in_string = False

    while index < length:
        char = source[index]

        if in_string:
            out.append(char)
            if char == "\\" and index + 1 < length:
                out.append(source[index + 1])
                index += 2
                continue
            if char == '"':
                in_string = False
            index += 1
            continue

        if char == '"':
            in_string = True
            out.append(char)
            index += 1
            continue

        if char.isspace():
            if out and out[-1] != " ":
                out.append(" ")
            index += 1
            continue

        out.append(char)
        index += 1

    return "".join(out)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: normalize_pp.py <input> <output>", file=sys.stderr)
        return 2

    with open(sys.argv[1], encoding="utf-8", errors="replace") as handle:
        source = handle.read()

    with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as handle:
        handle.write(normalise(source))

    return 0


if __name__ == "__main__":
    sys.exit(main())
