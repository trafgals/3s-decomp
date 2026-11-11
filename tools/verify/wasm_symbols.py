#!/usr/bin/env python3
"""Turns the indices of a wasm stack trace into the names of the C functions.

The browser reports the stack of an error as a list of wasm function indices
("wasm-function[3983]"). The symbol map that emcc writes next to the module
maps those indices back to the names of the functions, which is the only way
to find the source of a crash in the browser build.

Usage:
    tools/verify/wasm_symbols.py build/web/sf33rd.js.symbols <index> ...
    tools/verify/wasm_symbols.py build/web/sf33rd.js.symbols < stack.txt
"""

from __future__ import annotations

import re
import sys

# The stack of the browser looks like this:
#   at wasm-function[3983] (http://127.0.0.1:8080/sf33rd.js:1234)
INDEX_PATTERN = re.compile(r"wasm-function\[(\d+)\]")


def load_symbols(path: str) -> dict[int, str]:
    symbols: dict[int, str] = {}

    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            index, _, name = line.strip().partition(":")
            if name:
                symbols[int(index)] = name

    return symbols


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2

    symbols = load_symbols(sys.argv[1])

    indices: list[int] = []
    for argument in sys.argv[2:]:
        indices.append(int(argument))

    if len(indices) == 0:
        # The stack trace comes from the standard input.
        for match in INDEX_PATTERN.finditer(sys.stdin.read()):
            index = int(match.group(1))
            if index not in indices:
                indices.append(index)

    if not indices:
        print("no wasm function indices given")
        return 1

    for index in indices:
        print(f"{index}: {symbols.get(index, '<unknown>')}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
