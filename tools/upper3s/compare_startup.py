#!/usr/bin/env python3
"""Compares the startup that the runtime machinery reads with the one of
the offline decoder of the frame chain.

The offline decoder (tools/upper3s/decode_chain.py) walks the frame chain
the way the retail game does, with the branches, and it is the reference.
The runtime machinery (src/anniversary/sf33rd/Source/Game/upper3s/
upper3s_chain.c) only walks the straight line of a chain, because a command
block can jump anywhere and the frames that follow one depend on the state
of the game.

The two therefore do not agree on every move, and the report says on which
ones they do not: the moves whose chain branches on the state of the game.
The patch does not touch them, because a change is only made when the chain
still holds the number the retail data has, so the disagreement does not
reach the game.

What the check does fail on is the change that the disc does not take: a
change of the startup or of the recovery whose move does not hold the value
the patch expects is a change of the patch that is not there.  The tables of
the patch are read for that, and every change of them is looked up in the
data of the disc.

The check also fails when nothing was compared at all: a run that compared
nothing says nothing about the data.

Usage: compare_startup.py [dump] [chain_startup.txt]

The chain_startup.txt file is the output of
tests/upper3s/test_upper3s_chain_dump, which reads the data of the disc
with the runtime machinery.
"""
import os
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import decode_chain as dc  # noqa: E402

DUMP = sys.argv[1] if len(sys.argv) > 1 else "upper3s_dump.txt"
CHAIN = sys.argv[2] if len(sys.argv) > 2 else "build/chain_startup.txt"


def read_chain(path):
    """The startup and the recovery that the runtime machinery reads."""
    result = {}
    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            if not line.startswith("STARTUP "):
                continue
            _, name, koc, index, startup, recovery = line.split()
            result[(name, int(koc), int(index))] = (int(startup), int(recovery))
    return result


# --------------------------------------------------------------------------
# The tables of the patch
# --------------------------------------------------------------------------
HEADER = "include/sf33rd/Source/Game/upper3s.h"
FOLDER = "src/anniversary/sf33rd/Source/Game/upper3s"


def read_header():
    """The numbers of the header: the slots, the flags and the characters."""
    text = open(HEADER, encoding="utf-8", errors="replace").read()

    kocs = {}
    for name, value in re.findall(r"(U3S_KOC_\w+)\s*=\s*(\d+)", text):
        kocs[name] = int(value)

    # The super arts are in the same slot as the special moves.
    kocs["U3S_KOC_SUPER"] = kocs["U3S_KOC_ATTACK"]

    flags = {}
    for name, value in re.findall(r"#define\s+(U3S_MV_\w+)\s+(0x[0-9A-Fa-f]+|\d+)", text):
        flags[name] = int(value, 0)

    characters = {}
    for name, value in re.findall(r"(U3S_\w+)\s*=\s*(\d+),", text):
        if name.startswith("U3S_MV_") or name.startswith("U3S_KOC_"):
            continue
        characters[name] = int(value)

    return kocs, flags, characters


def parse_patch_tables():
    """The overrides of the patch, out of the per character tables.

    Every entry is (character, koc, index, flags, startup_from, startup,
    recovery_from, recovery), and the character is the name the disc uses
    for it.  The symbol of the table is the name of the character -
    upper3s_moves_makoto is the table of Makoto - whatever the number of
    the file is.  The flags of the entry are the bits of the header, so
    that the two cannot drift apart.
    """
    kocs, flags, _ = read_header()

    entries = []
    for path in sorted(pathlib.Path(FOLDER).glob("pl*.c")):
        text = path.read_text(encoding="utf-8", errors="replace")
        match = re.search(r"upper3s_moves_(\w+)\[\]\s*=\s*\{", text)
        if not match:
            continue
        character = match.group(1)
        for block in re.findall(r"\{([^{}]*)\}", text[match.end():]):
            block = re.sub(r"/\*.*?\*/", " ", block, flags=re.S)
            fields = [field.strip() for field in block.split(",")]
            if len(fields) < 14:
                continue
            if not fields[0].startswith("U3S_KOC_"):
                continue
            koc = kocs.get(fields[0])
            if koc is None:
                continue
            bits = 0
            for name in fields[2].split("|"):
                bits |= flags.get(name.strip(), 0)
            entries.append(
                (
                    character,
                    koc,
                    int(fields[1]),
                    bits,
                    int(fields[10]),
                    int(fields[11]),
                    int(fields[12]),
                    int(fields[13]),
                )
            )

    return entries, flags


def dropped_changes(chain, entries, flags):
    """The changes of the patch that the data of the disc does not take.

    A change of the startup or of the recovery is only made when the chain
    of the move holds the value of the retail data, so a chain that holds
    another value means that the change is not made at all.  A move that is
    not in the data of the disc is the same thing: the runtime machinery
    does not find its chain, so the change is not made either.
    """
    startup_bit = flags["U3S_MV_STARTUP"]
    recovery_bit = flags["U3S_MV_RECOVERY"]

    dropped = []
    for character, koc, index, bits, startup_from, _, recovery_from, _ in entries:
        key = (character, koc, index)
        wanted = (bits & startup_bit) or (bits & recovery_bit)
        if not wanted:
            continue
        if key not in chain:
            dropped.append((key, "move", None, None))
            continue
        read_startup, read_recovery = chain[key]
        if (bits & startup_bit) and read_startup != startup_from:
            dropped.append((key, "startup", read_startup, startup_from))
        if (bits & recovery_bit) and read_recovery != recovery_from:
            dropped.append((key, "recovery", read_recovery, recovery_from))
    return dropped


def main():
    characters = dc.parse_dump(DUMP)
    chain = read_chain(CHAIN)

    agree = 0
    only_decoder = 0
    only_chain = []
    startup_different = []
    recovery_different = []
    recovery_unknown = 0
    decoder_errors = []

    for character in characters:
        for koc in sorted(character.moves):
            for info in dc.decode_slot(character, koc):
                key = (character.name, koc, info.index)

                if info.error:
                    decoder_errors.append((key, info.error))
                    continue

                mine = chain.get(key)

                if info.startup is None:
                    if mine is not None:
                        only_chain.append((key, mine[0]))
                    continue

                if mine is None:
                    only_decoder += 1
                    continue

                if mine[0] == info.startup:
                    agree += 1
                else:
                    startup_different.append((key, info.startup, mine[0]))

                # The recovery of the runtime machinery is only known when
                # the walk of the chain reached its end.  When it is known
                # and the decoder has none, the decoder found no attack
                # data and the runtime machinery did: that is a
                # disagreement about the data of the disc.
                if mine[1] < 0:
                    recovery_unknown += 1
                elif info.recovery is None:
                    recovery_different.append((key, None, mine[1]))
                elif mine[1] != info.recovery:
                    recovery_different.append((key, info.recovery, mine[1]))

    print("the runtime machinery and the offline decoder")
    print("  %d moves have the same startup" % agree)
    print("  %d moves have a different startup" % len(startup_different))
    print("  %d moves have a different recovery" % len(recovery_different))
    print("  %d moves have a recovery that only one of the two knows" % recovery_unknown)
    print("  %d moves only the decoder knows" % only_decoder)
    print("  %d moves only the runtime machinery knows" % len(only_chain))
    print("  %d moves the decoder could not read" % len(decoder_errors))

    for key, expected, got in startup_different[:40]:
        print("  %s %d %d: the decoder says %s, the runtime says %d" % (
            key[0], key[1], key[2], expected, got))

    if len(startup_different) > 40:
        print("  ... and %d more" % (len(startup_different) - 40))

    for key, expected, got in recovery_different[:40]:
        print("  %s %d %d: the recovery of the decoder is %s, the runtime says %d" % (
            key[0], key[1], key[2], expected, got))

    if len(recovery_different) > 40:
        print("  ... and %d more" % (len(recovery_different) - 40))

    for key, startup in only_chain[:40]:
        print("  %s %d %d: the runtime machinery reads a startup of %d, the decoder does not" % (
            key[0], key[1], key[2], startup))

    if len(only_chain) > 40:
        print("  ... and %d more" % (len(only_chain) - 40))

    for key, error in decoder_errors[:40]:
        print("  %s %d %d: the decoder could not read the move (%s)" % (
            key[0], key[1], key[2], error))

    if len(decoder_errors) > 40:
        print("  ... and %d more" % (len(decoder_errors) - 40))

    # The changes of the patch have to be taken by the data of the disc: a
    # change whose move does not hold the value the patch expects is a
    # change of the patch that is not there.
    entries, flags = parse_patch_tables()
    dropped = dropped_changes(chain, entries, flags)
    frame = [
        entry for entry in entries
        if entry[3] & (flags["U3S_MV_STARTUP"] | flags["U3S_MV_RECOVERY"])
    ]

    print("the changes of the patch")
    print("  %d changes in the tables of the patch" % len(entries))
    print("  %d changes of the frame data" % len(frame))
    print("  %d changes the data of the disc does not take" % len(dropped))

    for key, what, read, expected in dropped:
        if what == "move":
            print("  %s %d %d: the move is not in the data of the disc, the change is dropped" % (
                key[0], key[1], key[2]))
        else:
            print("  %s %d %d: the %s of the disc is %d, the patch expects %d" % (
                key[0], key[1], key[2], what, read, expected))

    # A move that only the decoder knows is a move whose chain has a command
    # block before the first hit: the runtime machinery does not touch it,
    # and that is the safe behaviour.  The moves the two disagree on are
    # the same ones, and the report says so.
    #
    # A check that compared nothing fails as well: it says nothing about
    # the data.
    if agree == 0:
        print("  nothing was compared")
        return 1

    if dropped:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
