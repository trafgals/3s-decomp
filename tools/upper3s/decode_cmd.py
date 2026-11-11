#!/usr/bin/env python3
"""Decode the command records of the game into the motion of every special.

The game reads the inputs of a player through the command tables of
`src/anniversary/sf33rd/Source/Game/cmd_data.c`. Every command is a header of
twelve words followed by a list of steps of four words each; a step is
`(type, int, free, lever)`. The `lever` word of a step holds the direction the
step waits for in its low nibble, and the bit 0x8000 says that the lever has to
be *exactly* that direction instead of only containing it.

The lever of the game is a nibble with these bits:

    1 = up      2 = down      4 = forward      8 = back

The horizontal bits are relative to the character: `CMD_MAIN.c:pl_lvr_set()`
swaps the two horizontal bits when the character faces to the left, so "4" is
always "towards the opponent" and "8" is always "away from it". The directions
are therefore written the way a player reads them, in the numpad notation:

    1 = down-back   2 = down   3 = down-forward   4 = back
    5 = neutral     6 = forward   7 = up-back   8 = up   9 = up-forward

Usage:
    python3 tools/upper3s/decode_cmd.py            # every command of every character
    python3 tools/upper3s/decode_cmd.py --half     # only the half circle motions
    python3 tools/upper3s/decode_cmd.py --char 6   # only Hugo (the character index 6)
    python3 tools/upper3s/decode_cmd.py --char makoto  # only Makoto (by name)
"""

import argparse
import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CMD_DATA = os.path.join(REPO_ROOT, "src/anniversary/sf33rd/Source/Game/cmd_data.c")
ASSTBL = os.path.join(REPO_ROOT, "src/anniversary/bin2obj/asstbl.c")
PLS03 = os.path.join(REPO_ROOT, "src/anniversary/sf33rd/Source/Game/PLS03.c")

# The character index of the game (Gill 0, Alex 1, Ryu 2, ... Remy 19) and the
# name of the character. The command tables of cmd_data.c are indexed by the
# character index, not by the charset id (PLCNT.c:plid_data permutes the two).
CHARACTERS = [
    "Gill", "Alex", "Ryu", "Yun", "Dudley", "Necro", "Hugo", "Ibuki", "Elena", "Oro",
    "Yang", "Ken", "Sean", "Urien", "Gouki", "Chun-Li", "Makoto", "Q", "Twelve", "Remy",
]

# The nibble of the lever and the direction it means, in the numpad notation.
NUMPAD = {
    0x0: "5", 0x1: "8", 0x2: "2", 0x4: "6", 0x5: "9", 0x6: "3",
    0x8: "4", 0x9: "7", 0xA: "1",
}

# The direction of the lever, as a word, for the documentation.
DIRECTION = {
    0x0: "neutral", 0x1: "up", 0x2: "down", 0x4: "forward", 0x8: "back",
    0x5: "up-forward", 0x6: "down-forward", 0x9: "up-back", 0xA: "down-back",
}

# The check function of a step type. The type of the data is the index of
# `chk_move_jp` in CMD_MAIN.c, and the table starts with check_init, so the
# type of the data is the number of the check function plus one.
CHECK_FUNCTION = {
    1: "check_0", 2: "check_1", 3: "check_2", 4: "check_3", 5: "check_4",
    6: "check_5", 7: "check_6", 8: "check_7", 9: "check_7", 10: "check_9",
    11: "check_10", 12: "check_11", 13: "check_12", 14: "check_13",
    15: "check_14", 16: "check_15", 17: "check_16", 18: "check_16",
    19: "check_18", 20: "check_19", 21: "check_20", 22: "check_21",
    23: "check_22", 24: "check_23", 25: "check_24", 26: "check_25",
    27: "check_26",
}

# The steps that wait for a direction. The others wait for a button.
DIRECTION_STEPS = {1, 2, 3, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                    22, 23, 24, 25, 26, 27}
BUTTON_STEPS = {4, 5, 8, 9}

# The buttons of the game (CMD_MAIN.c:chk_pl->shot_up is masked with 0x770).
BUTTONS = {0x10: "LP", 0x20: "MP", 0x40: "HP", 0x100: "LK", 0x200: "MK", 0x400: "HK"}

HEADER_WORDS = 12
STEP_WORDS = 4


def parse_source(path):
    """Read the command tables out of cmd_data.c."""
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    # The command records themselves.
    arrays = {}
    for match in re.finditer(r"const s16 (\w+)\[(\d+)\]\s*=\s*\{([^}]*)\}\s*;", text, re.S):
        name = match.group(1)
        words = [int(value) for value in re.findall(r"-?\d+", match.group(3))]
        arrays[name] = words

    # The list of the 56 commands of a character.
    lists = {}
    for match in re.finditer(r"const_s16_arr (\w+)\[56\]\s*=\s*\{([^}]*)\}\s*;", text, re.S):
        lists[match.group(1)] = [name.strip() for name in match.group(2).split(",") if name.strip()]

    # The character index -> the list of the commands.
    player = []
    match = re.search(r"void\* pl_cmd\[20\]\s*=\s*\{([^}]*)\}\s*;", text, re.S)
    if not match:
        raise SystemExit("pl_cmd not found in " + path)
    for name in match.group(1).split(","):
        name = name.strip()
        if name:
            player.append(name)

    # The second set of the commands, the one the game uses when the command
    # select of the player is on (CMD_MAIN.c:cmd_sel).
    player_md = []
    match = re.search(r"void\* pl_CMD\[20\]\s*=\s*\{([^}]*)\}\s*;", text, re.S)
    if match:
        for name in match.group(1).split(","):
            name = name.strip()
            if name:
                player_md.append(name)

    return arrays, lists, player, player_md


def decode_steps(words):
    """Cut the steps of a command out of its words."""
    steps = []
    index = HEADER_WORDS
    while index + STEP_WORDS <= len(words):
        step_type = words[index]
        if step_type == 28:
            break
        steps.append({
            "type": step_type,
            "int": words[index + 1],
            "free": words[index + 2],
            "lever": words[index + 3] & 0xFFFF,
        })
        index += STEP_WORDS
    return steps


def step_motion(step):
    """The motion a single step waits for."""
    lever = step["lever"]
    nibble = lever & 0xF
    exact = bool(lever & 0x8000)
    kind = CHECK_FUNCTION.get(step["type"], "check_?")

    if step["type"] in BUTTON_STEPS:
        if step["type"] == 4:
            return "button x%d" % (lever & 0xFFFF)
        if step["type"] == 5:
            if lever == 0x10:
                return "tap punch"
            return "tap kick"
        if step["type"] == 8:
            return "hold punch"
        return "hold kick"

    if step["type"] == 7:
        return "the four directions"
    if step["type"] == 23:
        return "the eight directions"

    if nibble not in NUMPAD:
        return "%s ?" % kind
    mark = "" if exact else "+"
    return NUMPAD[nibble] + mark


def brace_block(text, start):
    """The text of the brace delimited block that starts at `start`."""
    open_brace = text.index("{", start)
    depth = 0

    for index in range(open_brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace:index + 1]

    raise SystemExit("the block that starts at %d is not closed" % start)


def parse_assignment():
    """Read the assignment table of the moves out of asstbl.c and PLS03.c.

    The assignment table `_assadr_lv_9900[character][column]` holds the
    entries of the moves of a character.  A special move is the command `i`
    of the character, and its four strengths are the four consecutive
    entries that start at the offset `(i - 28) * 4` of the column
    `cmdixconv(i)` of the table.
    """
    with open(ASSTBL, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    # The entries of the assignment table: the char_ix of every move.
    entries = {}
    for match in re.finditer(r"DATA_SECTION AS (D_\w+)\[\d+\]\s*=", text):
        block = brace_block(text, match.end())
        char_ix = [int(value) for value in re.findall(r"\.char_ix\s*=\s*(\d+)", block)]
        entries[match.group(1)] = char_ix

    # The table itself: the character index and the column of the pointer.
    table = []
    start = text.find("_assadr_lv_9900[20][4]")
    if start < 0:
        raise SystemExit("_assadr_lv_9900 not found in " + ASSTBL)
    body = brace_block(text, start)
    for row in re.findall(r"\{([^{}]*)\}", body):
        names = [name.strip() for name in row.split(",") if name.strip()]
        if names:
            table.append(names)

    # The column of a command index.
    with open(PLS03, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    match = re.search(r"const s16 cmdixconv_table\[\d+\]\s*=\s*\{([^}]*)\}\s*;", text, re.S)
    if not match:
        raise SystemExit("cmdixconv_table not found in " + PLS03)
    conv = [int(value) for value in re.findall(r"\d+", match.group(1))]

    return entries, table, conv


def char_ix_of(entries, table, conv, character, command):
    """The char_ix of the four strengths of a special move."""
    if command < 28 or command > 45:
        return None
    column = conv[command - 20]
    name = table[character][column]
    char_ix = entries.get(name, [])
    start = (command - 28) * 4
    return char_ix[start:start + 4]


def describe(words):
    """Describe one command."""
    steps = decode_steps(words)
    motion = "".join(step_motion(step) for step in steps)
    header = {
        "reset": words[0],
        "w_dead": words[1],
        "w_dead2": words[2],
        "waza_r": words[3:7],
        "btix": words[7],
        "exdt": words[8:12],
    }
    return steps, motion, header


def print_assignment(entries, table, conv, character, command):
    """Print the char_ix of the four strengths of a special move."""
    char_ix = char_ix_of(entries, table, conv, character, command)
    if not char_ix:
        return
    print("            char_ix %s" % ", ".join(str(value) for value in char_ix))


def is_placeholder(array_name):
    """The placeholder of a command slot that the character does not use.

    The record of the placeholder holds the same words as a real command,
    so it decodes as a motion and would be reported as a move that the
    character does not have.
    """
    return array_name == "dm_cmd_xx"


def missing_lists(lists, player):
    """The command lists that the source does not have.

    A character whose list is not there has no command at all, and a run
    that leaves it out without saying so is a run that says less than it
    does.
    """
    return [list_name for list_name in player if list_name not in lists]


def command_entries(arrays, lists, player, character=None, minimum=0):
    """Every command of every character, without the placeholders.

    Yields the index of the character, its name, the number of the command,
    the name of the array of the command and the words of that array.
    """
    for index, list_name in enumerate(player):
        if character is not None and index != character:
            continue
        command_list = lists.get(list_name)
        if command_list is None:
            continue
        for number, array_name in enumerate(command_list):
            if number < minimum:
                continue
            if is_placeholder(array_name):
                continue
            words = arrays.get(array_name)
            if words is None:
                continue
            yield index, CHARACTERS[index], number, array_name, words


def resolve_character(value):
    """The character index of a name or of an index.

    The name is matched without the punctuation, so that "chun-li", "chunli"
    and "Chun-Li" are the same character.
    """
    if value is None:
        return None

    text = value.strip()

    if text.isdigit():
        index = int(text)
        if 0 <= index < len(CHARACTERS):
            return index
        raise SystemExit(
            "the character index %d is out of range (0..%d)" % (index, len(CHARACTERS) - 1)
        )

    wanted = re.sub(r"[^a-z0-9]", "", text.lower())

    for index, name in enumerate(CHARACTERS):
        if re.sub(r"[^a-z0-9]", "", name.lower()) == wanted:
            return index

    raise SystemExit("there is no character named %r" % value)


def self_test(arrays, lists, player, entries, table, conv):
    """Check the decoder against the commands the game is known for."""
    checks = [
        # The character index, the command index, the motion and the char_ix.
        (2, 28, "623", None),    # Ryu: the Dragon Punch
        (2, 29, "236", None),    # Ryu: the Hadouken
        (2, 30, "214", None),    # Ryu: the Tatsumaki Senpukyaku
        (11, 28, "623", [28, 29, 30, 31]),  # Ken: the Dragon Punch, the jab is 28
        (16, 28, "623", [36, 37, 38, 39]),  # Makoto: the Fukiage
    ]

    failures = 0
    total = len(checks)

    for character, command, motion, char_ix in checks:
        name = CHARACTERS[character]
        array_name = lists[player[character]][command]
        steps, decoded, header = describe(arrays[array_name])

        if decoded != motion:
            failures += 1
            print("  FAIL  %s command %d is %s, not %s" % (name, command, decoded, motion))
        else:
            print("  ok    %s command %d is %s" % (name, command, decoded))

        if char_ix is not None:
            found = char_ix_of(entries, table, conv, character, command)
            if found != char_ix:
                failures += 1
                print("  FAIL  %s command %d is char_ix %s, not %s" %
                      (name, command, found, char_ix))
            else:
                print("  ok    %s command %d is char_ix %s" % (name, command, found))

    # The character of a name and of an index.
    for value, expected in (("makoto", 16), ("16", 16), ("chun-li", 15),
                             ("Gouki", 14), ("twelve", 18)):
        total += 1
        found = resolve_character(value)
        if found != expected:
            failures += 1
            print("  FAIL  the character %r is %s, not %s" % (value, found, expected))
        else:
            print("  ok    the character %r is %s" % (value, CHARACTERS[expected]))

    # The placeholder of an unused command slot is not a move.
    total += 1
    unused = [
        character
        for character in range(len(CHARACTERS))
        if "dm_cmd_xx" in lists[player[character]]
    ]
    if not unused:
        failures += 1
        print("  FAIL  no character uses the placeholder of an unused command")
    else:
        print("  ok    %d characters use the placeholder of an unused command" % len(unused))

    # The listing does not report the placeholder as a move.
    total += 1
    listed = list(command_entries(arrays, lists, player))
    raw = sum(
        1
        for character in range(len(CHARACTERS))
        for array_name in lists[player[character]]
        if array_name != "dm_cmd_xx"
    )
    if any(is_placeholder(entry[3]) for entry in listed):
        failures += 1
        print("  FAIL  the listing reports a placeholder as a move")
    elif len(listed) != raw:
        failures += 1
        print("  FAIL  the listing reports %d commands, not %d" % (len(listed), raw))
    else:
        print("  ok    the listing reports %d commands and no placeholder" % len(listed))

    print("the command decoder: %d checks, %d failures" % (total, failures))

    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--char", default=None, metavar="INDEX|NAME",
                        help="only this character, by index or by name")
    parser.add_argument("--half", action="store_true",
                        help="only the commands with a half circle motion")
    parser.add_argument("--motion", default=None,
                        help="only the commands whose motion contains this string")
    parser.add_argument("--brief", action="store_true",
                        help="one line per command, without the steps")
    parser.add_argument("--md", action="store_true",
                        help="the second set of the commands (the one of the command select)")
    parser.add_argument("--assignment", action="store_true",
                        help="also print the char_ix of the four strengths of the move")
    parser.add_argument("--self-test", action="store_true",
                        help="check the decoder against the known motions of the game")
    parser.add_argument("--min", type=int, default=0,
                        help="the first command index to show")
    args = parser.parse_args()

    arrays, lists, player, player_md = parse_source(CMD_DATA)

    if args.md:
        player = player_md

    entries, table, conv = parse_assignment()

    if args.self_test:
        return self_test(arrays, lists, player, entries, table, conv)

    half_circle = []
    character = resolve_character(args.char)

    missing = missing_lists(lists, player)
    for list_name in missing:
        print("the command list %s is not there" % list_name)
    if missing:
        return 1

    for index, name, number, array_name, words in command_entries(
        arrays, lists, player, character, args.min
    ):
        steps, motion, header = describe(words)
        if not steps:
            continue
        if args.motion and args.motion not in motion:
            continue
        if args.half:
            if motion not in ("41236", "63214", "41236+", "63214+"):
                continue
            half_circle.append((name, number, array_name, motion))
            continue
        if args.brief:
            print("%-8s cmd %2d %-12s %s" % (name, number, array_name, motion))
            if args.assignment:
                print_assignment(entries, table, conv, index, number)
            continue
        print("%-8s cmd %2d %-12s %-10s reset=%-3d waza_r=%s exdt=%s" %
              (name, number, array_name, motion, header["reset"],
               header["waza_r"], header["exdt"]))
        if args.assignment:
            print_assignment(entries, table, conv, index, number)
        for step in steps:
            lever = step["lever"]
            print("            %-10s type %2d int %-3d free %-4d lever 0x%04X %s" %
                  (CHECK_FUNCTION.get(step["type"], "check_?"), step["type"],
                   step["int"], step["free"], lever,
                   DIRECTION.get(lever & 0xF, "?")))

    if args.half:
        if not half_circle:
            print("no half circle motion found")
            return 1
        for name, number, array_name, motion in half_circle:
            print("%-8s cmd %2d %-12s %s" % (name, number, array_name, motion))
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
