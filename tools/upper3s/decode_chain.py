#!/usr/bin/env python3
"""decode_chain.py - an offline decoder for the frame chain of Third Strike.

The per move frame data of the game (startup / active / recovery frames, the
hit box, the push box and the attack data that carries the damage, the stun
and the hit stun) is not part of this repository: it is read from the disc
image at runtime and interpreted by `CHARSET.c`.

`upper3s_dump.txt` is a dump of that data.  The decoder that comes with the
dump (`src/anniversary/sf33rd/Source/Game/upper3s/upper3s_dump.c`) walks the
frame chain as if it were a flat list of blocks, which is wrong: the chain is
a little program.  This script walks it the way the retail game does.

------------------------------------------------------------------ the model

`CHARSET.c` keeps the frame chain of a move in `wk->char_table[koc]`.  The
entry `char_table[koc][index]` is a byte offset; the header of the move is at
word `offset / 4` of that slot and the size of one frame block is
`header[-2] & 0xFFFF` (`cgd_type`).

A block is `cgd_type` u32 words:

    word 0   cg_type | cg_ctr << 8 | cg_se << 16
    word 1   cg_olc_ix | cg_number << 16
    word 2   the hit and the attack data (see below)

The engine reads a block through `UNK11` (`CHARSET.c:check_cm_extended_code`):

    typedef struct { u16 code; s16 koc; s16 ix; s16 pat; } UNK11;

so `code` is the low u16 of word 0 - that is the *command code* in the low
byte and the *frame counter* `cg_ctr` in the second byte - and `koc` is
`cg_se`, the sound effect id.

    * a block with `code < 0x100` has `cg_ctr == 0`: it is a *command* block,
      it is dispatched through `decode_chcmd[125]` and the walk continues;
    * a block with `code >= 0x100` has `cg_ctr >= 1`: it is an *animation
      frame*.  The walk stops there, the block is copied into the work
      structure and `cg_ctr` becomes the number of game frames the frame is
      held for.

`char_move()` is called once per game frame; it only advances the chain when
the frame counter of the current animation frame has run out.  A move is
therefore a sequence of animation frames, each with its own hold time.

The attack data of a frame lives in word 2:

    bits  0..12   the "meoshi" flags           -> wk->cg_meoshi
    bits 13..21   the hit box index            -> wk->cg_hit_ix
    bits 22..31   the attack table index       -> wk->cg_att_ix

`CHARSET.c:check_cgd_patdat()` extracts them with a bit trick: it shifts the
whole word left by three and reads the hit box index out of the high half.
The attack table index is `((s16)(word2 >> 16)) >> 6`; a negative value means
"this attack gives arts points" (`CHARSET.c:set_new_attnum()`), the table
index is then the absolute value.

------------------------------------------------------------------ the walk

`check_cm_extended_code()` is the walker:

    cg_ix += cgd_type                       (or jump to cg_next_ix)
    loop:
        read the block at cg_ix
        if block.code >= 0x100:  it is an animation frame -> stop
        if handler(block) == 0:   the handler moved to another move -> stop
        cg_ix += cgd_type

The handlers that move to another move are `comm_roa`, `comm_jmp`,
`comm_jpss`, `comm_jsr`, `comm_ret`, `comm_nex`, `comm_nex2`, `comm_uja*`,
`comm_umja`, `comm_back`, `comm_abbak` and `comm_retmj`.  They call
`set_char_move_init2()`, which re-arms the chain at block `pat - 1` of the
new move.

The handlers that branch (`comm_if_l`, `comm_djmp`, `comm_if_s`, ...) depend
on the state of the game, which does not exist offline.  This decoder uses a
fixed, documented default state (no lever held, no button held, no hit, full
health, ...) and reports every branch it had to take, so that a reader can
tell which numbers are certain and which depend on the branch.

Usage:

    python3 build/decode_chain.py upper3s_dump.txt --char ken
    python3 build/decode_chain.py upper3s_dump.txt --all --csv out.csv
"""

from __future__ import annotations

import argparse
import collections
import os
import re
import sys

# --------------------------------------------------------------------------
# the dump
# --------------------------------------------------------------------------

MOVE_RE = re.compile(
    r"^MOVE koc=(\d+) idx=(\d+) att=(-?\d+) hit=(-?\d+) frames=(\d+) "
    r"pow=(-?\d+) dmg=(-?\d+) impact=(-?\d+) piyo=(-?\d+) "
    r"hs_me=(-?\d+) hs_you=(-?\d+) dipsw=(-?\d+) guard=(-?\d+) dir=(-?\d+) "
    r"cgd=(\d+) off=(\d+) raw=([0-9a-f,]+) codes=(\S*)$")
SLOT_RE = re.compile(r"^SLOT koc=(\d+) len=(\d+) size=(\d+) data=([0-9a-f]*)$")
CHAR_RE = re.compile(r"^CHAR (\d+) (\w+) charset=(\d+)$")
ATT_RE = re.compile(r"^ATT (\d+) (\w+) (\d+)\s*$")
HEX_RE = re.compile(r"^[0-9a-f]+$")


def s16(value: int) -> int:
    """The C `s16` view of a 16 bit value."""
    return value - 0x10000 if value >= 0x8000 else value


def s8(value: int) -> int:
    """The C `s8` view of an 8 bit value."""
    return value - 0x100 if value >= 0x80 else value


class Character:
    """The data of one character, as it is in the dump."""

    def __init__(self, index: int, name: str, charset: int):
        self.index = index
        self.name = name
        self.charset = charset
        self.slots = {}          # koc -> list of u32 words
        self.slot_len = {}       # koc -> the number of moves in the slot
        self.moves = {}          # koc -> list of (index, off, cgd)
        self.dump_moves = {}     # koc -> the MOVE lines of the dump
        self.att = None          # list of UNK_7 entries, if the table is known

    def move_list(self, koc):
        return self.moves.get(koc, [])


def parse_dump(path: str):
    """Reads the dump and returns the list of characters.

    The MOVE lines of the dump are only a subset of the moves: the dump tool
    stops the list of a slot at the first offset that does not grow any more
    and it skips the moves whose block size is out of range.  The SLOT lines
    hold the whole slot, and the first words of a slot are the byte offsets
    of its moves, so the list of the moves is taken from there.
    """
    characters = []
    current = None

    with open(path, encoding="utf-8", errors="replace") as handle:
        for line in handle:
            line = line.rstrip("\n")

            match = CHAR_RE.match(line)
            if match:
                current = Character(int(match.group(1)), match.group(2), int(match.group(3)))
                characters.append(current)
                continue

            if current is None:
                continue

            match = MOVE_RE.match(line)
            if match:
                current.dump_moves.setdefault(int(match.group(1)), []).append(
                    (int(match.group(2)), int(match.group(16)), int(match.group(15))))
                continue

            match = SLOT_RE.match(line)
            if match:
                koc = int(match.group(1))
                length = int(match.group(2))
                hexdata = match.group(4)
                words = [int(hexdata[i:i + 8], 16) for i in range(0, len(hexdata), 8)]
                current.slots[koc] = words
                current.slot_len[koc] = length

                # The move table is at the start of the slot: `table[index]` is
                # the byte offset of the header of move `index`.  The moves
                # themselves follow the table, so the offset of the first
                # move is four times the size of the table.  The last three
                # words of the table are a trailer, so the slot holds
                # `table[0] / 4 - 3` moves.
                #
                # The dump tool derives the number of the moves from the order
                # of the offsets instead, which stops at the first offset that
                # does not grow: the table is a list of groups and the groups
                # repeat, so the dump only sees a part of the moves.
                count = (words[0] // 4 - 3) if words and words[0] else length
                count = max(count, 0)
                moves = []
                for index in range(count):
                    if index >= len(words):
                        break
                    off = words[index]
                    base = off // 4
                    cgd = (words[base - 2] & 0xFFFF) if 2 <= base <= len(words) else 0
                    moves.append((index, off, cgd))
                current.moves[koc] = moves
                continue

    return characters


def parse_att_table(path: str):
    """Reads the optional attack table.

    The format is the same as the SLOT lines of the dump:

        ATT <character index> <name> <number of entries>
        <the entries, 16 bytes each, as hex>

    The entries follow the header line.  The dump tool writes one entry per
    line, but the reader does not care: it collects the hex until it has the
    number of entries the header announces, so a table that was joined into a
    single line is read just as well.

    The table itself is not in `upper3s_dump.txt` - it is `wk->att_ix_table`,
    a separate table of the character data of the disc.  See the report.
    """
    tables = {}
    with open(path, encoding="utf-8", errors="replace") as handle:
        lines = handle.read().split("\n")
    index = 0
    while index < len(lines):
        match = ATT_RE.match(lines[index].strip())
        if not match:
            index += 1
            continue
        char_index = int(match.group(1))
        count = int(match.group(3))

        # Collect the entries: every line that is made of hex only is part of
        # the table, until the table holds the announced number of entries.
        blob = ""
        index += 1
        while index < len(lines) and len(blob) < count * 32:
            chunk = lines[index].strip()
            index += 1
            if not chunk or not HEX_RE.match(chunk):
                index -= 1
                break
            blob += chunk

        entries = []
        for entry in range(count):
            raw = blob[entry * 32:(entry + 1) * 32]
            if len(raw) < 32:
                break
            data = bytes.fromhex(raw)
            entries.append({
                "reaction": data[0], "level": data[1], "mkh_ix": data[2],
                "but_ix": data[3], "dipsw": data[4], "guard": data[5],
                "dir": data[6], "free": data[7], "pow": data[8],
                "impact": data[9], "piyo": data[10], "ng_type": data[11],
                "hs_me": s8(data[12]), "hs_you": s8(data[13]),
                "hit_mark": data[14], "dmg_mark": data[15],
            })
        tables[char_index] = entries
    return tables


AS_RE = re.compile(r"DATA_SECTION AS (\w+)\[(\d+)\]\s*=\s*\{(.*?)\};", re.S)
AS_ENTRY_RE = re.compile(r"\{\s*\.r_no\s*=\s*(-?\d+)\s*,\s*\.char_ix\s*=\s*(-?\d+)\s*,"
                         r"\s*\.data_ix\s*=\s*(-?\d+)\s*\}")
ASSTBL_ROW_RE = re.compile(r"_assadr_lv_9900\[20\]\[4\]\s*=\s*\{(.*?)\};", re.S)


def parse_asstbl(path: str):
    """Reads the assignment tables of `bin2obj/asstbl.c`.

    The `AS` entries carry the routine that plays a move (`r_no`) and the
    index of the move in the slot of the character (`char_ix`); they are what
    names the moves of the frame chain.
    """
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    tables = {}
    for name, _count, body in AS_RE.findall(text):
        tables[name] = [(int(r), int(c), int(d)) for r, c, d in AS_ENTRY_RE.findall(body)]

    rows = []
    match = ASSTBL_ROW_RE.search(text)
    if match:
        for row in re.findall(r"\{([^}]*)\}", match.group(1)):
            names = re.findall(r"\w+", row)
            if len(names) == 4:
                rows.append(names)

    return tables, rows


def load_attack_table(path: str):
    """Loads the attack tables of every character, if the file exists."""
    if not path or not os.path.exists(path):
        return None
    return parse_att_table(path)


def attack_table_of(tables, character):
    """The attack table of one character."""
    if not tables:
        return None
    return tables.get(character.index)


# --------------------------------------------------------------------------
# the attack table of the game
# --------------------------------------------------------------------------

POWER_RE = re.compile(r"Power_Data\[\d+\]\s*=\s*\{(.*?)\};", re.S)


def load_power_data(path: str):
    """Reads `POW_DATA.c:Power_Data`, the table the damage index points into."""
    if not path or not os.path.exists(path):
        return None
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    match = POWER_RE.search(text)
    if not match:
        return None
    values = [int(v) for v in re.findall(r"\d+", match.group(1))]
    return values


# --------------------------------------------------------------------------
# the block and its fields
# --------------------------------------------------------------------------

class Block:
    """One block of the frame chain.

    The engine reads a block through `UNK11`:

        typedef struct { u16 code; s16 koc; s16 ix; s16 pat; } UNK11;

    `code` is the low u16 of word 0 - the command code in the low byte and
    the frame counter in the second byte - and `koc` is `cg_se`, the sound
    effect id, which the jump handlers use as the slot they jump to.
    """

    __slots__ = ("src_koc", "src_index", "cg_ix", "code", "hold", "se", "koc",
                 "ix", "pat", "word2", "hit_raw", "att_raw", "meoshi",
                 "hit_ix", "att_ix", "att_ok", "raw_code")

    def __init__(self, src_koc, src_index, cg_ix, words, cgd):
        self.src_koc = src_koc
        self.src_index = src_index
        self.cg_ix = cg_ix
        word0 = words[0]
        word1 = words[1] if len(words) > 1 else 0
        self.word2 = words[2] if len(words) > 2 else 0

        self.raw_code = word0 & 0xFFFF          # UNK11.code
        self.code = word0 & 0xFF                # the command code
        self.hold = (word0 >> 8) & 0xFF         # cg_ctr, the hold time
        self.se = (word0 >> 16) & 0xFFFF        # cg_se, the sound effect
        self.koc = s16(self.se)                 # UNK11.koc
        self.ix = s16(word1 & 0xFFFF)           # UNK11.ix
        self.pat = s16((word1 >> 16) & 0xFFFF)  # UNK11.pat

        self.hit_raw = self.word2 & 0xFFFF
        self.att_raw = (self.word2 >> 16) & 0xFFFF

        # CHARSET.c:check_cgd_patdat()
        self.meoshi = self.hit_raw & 0x1FFF
        shifted = (((s16(self.att_raw) << 16) | self.hit_raw) & 0xFFFFFFFF) * 8 & 0xFFFFFFFF
        self.hit_ix = (shifted >> 16) & 0x1FF
        att = s16(self.att_raw) >> 6            # arithmetic shift, as in C
        self.att_ok = att < 0                   # "this attack gives arts points"
        self.att_ix = -att if att < 0 else att

    @property
    def is_frame(self):
        """A block with a hold time is an animation frame, not a command."""
        return self.raw_code >= 0x100

    @property
    def has_hitbox(self):
        """The retail game looks the hit box up with `wk->cg_hit_ix`."""
        return self.hit_ix != 0

    @property
    def has_attack(self):
        """`CHARSET.c` only loads the attack data when the index is not zero."""
        return self.att_ix != 0


# --------------------------------------------------------------------------
# the walker
# --------------------------------------------------------------------------

class DecodeError(Exception):
    pass


class Branch:
    """One branch the decoder had to take."""

    __slots__ = ("code", "taken", "target", "game_frame")

    def __init__(self, code, taken, target, game_frame):
        self.code = code
        self.taken = taken
        self.target = target
        self.game_frame = game_frame

    def __str__(self):
        return "code %d -> %s (frame %d)" % (self.code, self.taken, self.game_frame)


class Registers:
    """The registers of the chain walker.

    They are part of the work structure of the game, so they survive from one
    move to the next: a move that ends with a `comm_jsr` leaves the return
    address in `cmsw` and the move that comes next can return from it.  A
    decoder that looks at every move on its own would miss that, so the
    registers are shared by all the moves of a slot.
    """

    def __init__(self):
        self.cmja = None
        self.cmj = [None] * 7
        self.cmms = None
        self.cmsw = None
        self.cmbk = None
        self.cmb3 = None
        self.cmlp = [0, 0, 0, 0]
        self.cml2 = [0, 0, 0, 0]
        self.cmwk = [0] * 16

class Timeline:
    """The frame chain of one move, walked the way `CHARSET.c` walks it."""

    # the default state of the game; every branch of the chain is resolved
    # against it.  See the module docstring.
    LEVER = 0            # no direction is held
    SHOT = 0             # no button is held
    HIT_FLAG = 0         # the move has not connected
    WAZA_FLAG = 0        # no special move is active
    RL_FLAG = 0          # not mirrored
    RL_WAZA = 0
    OPERATOR = 0         # not in operator mode
    VITAL = 100          # full health, in percent
    MVXY = 0             # no movement
    RANDOM = 0           # the random source of `random_32()`

    MAX_GAME_FRAMES = 512
    MAX_STEPS = 20000

    # a block that is held for that many frames is the "the animation is
    # over" marker of a move, not a part of it
    WAIT_HOLD = 250

    def __init__(self, character: Character, koc: int, index: int, registers=None):
        self.char = character
        self.koc = koc
        self.index = index
        self.game_frame = 0

        # the registers of the engine; they are shared by the moves of a slot
        self.reg = registers if registers is not None else Registers()

        # the move the chain started in
        self.cmoa = (koc, index, 1)

        # the state of the walker
        self.cg_ix = 0
        self.cgd = 0
        self.base = 0
        self.cg_ctr = 1
        self.cg_next_ix = 0
        self.cg_wca_ix = 0
        self.current = None

        self.frames = []                # the animation frames, in order
        self.branches = []              # the branches that were taken
        self.steps = 0
        self.error = None
        self.debug = False
        self.cycle = False
        self.terminal = False
        self.wait = False

        self.set_char_move_init2(koc, index, 1, False)

    # -- the engine primitives -------------------------------------------

    def slot(self, koc):
        return self.char.slots.get(koc)

    def move_offset(self, koc, index):
        """The byte offset of the header of a move inside its slot."""
        for (idx, off, cgd) in self.char.move_list(koc):
            if idx == index:
                return off, cgd
        return None, None

    def set_char_move_init2(self, koc, index, ip, scf=False):
        """`CHARSET.c:set_char_move_init2()`."""
        words = self.slot(koc)
        if words is None:
            raise DecodeError("slot %d is not in the dump" % koc)
        off, cgd = self.move_offset(koc, index)
        if off is None:
            raise DecodeError("move %d/%d is not in the dump" % (koc, index))

        self.koc = koc
        self.index = index
        self.base = off // 4
        if self.base < 2 or self.base > len(words):
            raise DecodeError("move %d/%d has an offset of %d" % (koc, index, off))
        self.cgd = words[self.base - 2] & 0xFFFF
        if self.cgd < 2 or self.cgd > 32:
            raise DecodeError("move %d/%d has a block size of %d" % (koc, index, self.cgd))
        self.cg_ix = (ip - 1) * self.cgd - self.cgd
        self.cg_ctr = 1
        self.cg_next_ix = 0
        self.cg_wca_ix = 0
        if self.cmoa[2] == 0:
            self.cmoa = (koc, index, 1)

    def setup_comm_back(self):
        """`CHARSET.c:setup_comm_back()`."""
        self.reg.cmbk = (self.koc, self.index, self.cg_ix // self.cgd + 2)

    def block_at(self, cg_ix):
        """The block at a word offset of the current slot."""
        words = self.slot(self.koc)
        if words is None:
            raise DecodeError("slot %d is not in the dump" % self.koc)
        start = self.base + cg_ix
        if start < 0 or start + self.cgd > len(words):
            raise DecodeError("block at %d is outside of slot %d" % (start, self.koc))
        return Block(self.koc, self.index, cg_ix, words[start:start + self.cgd], self.cgd)

    # -- the walker -------------------------------------------------------

    def run(self):
        """Walks the chain until the animation loops or the budget runs out."""
        try:
            self._run()
        except DecodeError as error:
            self.error = str(error)
        return self.frames

    def _run(self):
        seen = set()
        last = None
        while self.game_frame < self.MAX_GAME_FRAMES:
            self.game_frame += 1
            self._game_frame()
            if self.error:
                return
            if self.current is None:
                return
            if self.current is last:
                # the frame is held for another game frame, nothing new
                continue
            last = self.current
            key = (self.current.src_koc, self.current.src_index, self.current.cg_ix)
            if key in seen:
                # the animation repeats: the move has come back to a frame it
                # has already shown.  This is the end of the timeline.
                self.cycle = True
                return
            seen.add(key)
            if self.terminal or self.wait:
                # `cg_type == 0xFF` is the end of the animation and a block
                # that is held for 250 frames is the "hold the last pose"
                # marker: the game reads them in appear.c and in the
                # character state machines to know that the chain is over.
                # They are sentinels, not a part of the move, so the walk
                # stops here.
                return
            self.frames.append(self.current)

    def _game_frame(self):
        """`CHARSET.c:char_move()`."""
        self.cg_ctr -= 1
        if self.cg_ctr != 0:
            return
        self._check_cm_extended_code()

    def _check_cm_extended_code(self):
        """`CHARSET.c:check_cm_extended_code()`.

        The recursion of `set_char_move_init2()` is turned into a loop.
        """
        while True:
            self.steps += 1
            if self.steps > self.MAX_STEPS:
                raise DecodeError("the chain does not terminate")

            if self.cg_next_ix:
                self.cg_ix = (self.cg_next_ix - 1) * self.cgd
            else:
                self.cg_ix += self.cgd

            switched = False
            while True:
                self.steps += 1
                if self.steps > self.MAX_STEPS:
                    raise DecodeError("the chain does not terminate")

                block = self.block_at(self.cg_ix)
                if self.debug:
                    print("    step %5d: koc=%d idx=%d block=%d code=%d hold=%d "
                          "koc_field=%d ix=%d pat=%d" %
                          (self.steps, self.koc, self.index, self.cg_ix // self.cgd,
                           block.code, block.hold, block.koc, block.ix, block.pat))
                if block.is_frame:
                    # an animation frame: the work structure is filled with it
                    self.current = block
                    self.cg_ctr = block.hold
                    # `CHARSET.c:check_cgd_patdat()` - the low byte of the
                    # block is the animation type, not a command
                    if block.code == 0xFF:
                        # the sentinel that ends the animation
                        self.terminal = True
                    elif (block.code & 0x80):
                        self.cg_wca_ix = block.code & 0x7F
                    if block.hold >= self.WAIT_HOLD:
                        # a block that is held for 250 frames is the "the
                        # animation is over, hold the last pose" marker: the
                        # character state machine decides when the move ends,
                        # the chain only waits here.
                        self.wait = True
                    return
                if self.handle(block) == 0:
                    switched = True
                    break
                self.cg_ix += self.cgd

            if not switched:
                return

    # -- the handlers -----------------------------------------------------

    def handle(self, block: Block) -> int:
        """Runs one command block.  Returns what the engine returns."""
        code = block.code

        handler = HANDLERS.get(code)
        if handler is None:
            return 1
        return handler(self, block)

    # -- the branches -----------------------------------------------------

    def branch(self, block: Block, taken: str, target: int) -> int:
        """Records a branch and jumps to it."""
        self.branches.append(Branch(block.code, taken, target, self.game_frame))
        return self.decord_if_jump(block, target)

    def decord_if_jump(self, block: Block, ix: int) -> int:
        """`CHARSET.c:decord_if_jump()`."""
        mode = ix & 0xE000

        if mode == 0x4000:
            self.cg_ix += ((ix & 0xFF) - 1) * self.cgd
            return 1

        if mode == 0x8000:
            self.cg_ix -= ((ix & 0xFF) + 1) * self.cgd
            return 1

        if mode == 0x2000:
            return self.decode_if_lever(block, ix & 0xFF)

        self.cg_ix = (ix - 2) * self.cgd
        return 1

    def decode_if_lever(self, block: Block, index: int) -> int:
        """`CHARSET.c:decode_if_lever[]`."""
        if index == 0:            # comm_dummy
            return 1
        if index == 1:            # comm_ret
            if self.reg.cmsw is None:
                raise DecodeError("comm_ret without a subroutine")
            self.set_char_move_init2(*self.reg.cmsw)
            return 0
        if 2 <= index <= 8:        # comm_uja .. comm_uja7
            register = self.reg.cmja if index == 2 else self.reg.cmj[index - 3]
            if register is None:
                raise DecodeError("comm_uja%d without a target" % (index - 1))
            self.setup_comm_back()
            self.set_char_move_init2(*register)
            return 0
        if index == 9:            # comm_umja
            if self.reg.cmms is None:
                raise DecodeError("comm_umja without a target")
            self.setup_comm_back()
            self.set_char_move_init2(*self.reg.cmms)
            return 0
        if index == 10:           # comm_back
            if self.reg.cmbk is None:
                raise DecodeError("comm_back without a saved position")
            self.set_char_move_init2(*self.reg.cmbk)
            return 0
        if index == 11:           # comm_nex
            return self.comm_nex(self.reg.cmlp)
        if index == 12:           # comm_nex2
            return self.comm_nex(self.reg.cml2)
        if index == 13:           # comm_wca
            self.cg_ix = (self.cg_wca_ix - 1) * self.cgd - self.cgd
            self.cg_ctr = 1
            return 1
        if index == 14:           # comm_retmj
            return 0
        if index == 15:           # comm_abbak
            if self.reg.cmb3 is None:
                raise DecodeError("comm_abbak without a saved position")
            self.set_char_move_init2(*self.reg.cmb3)
            return 0
        raise DecodeError("unknown decode_if_lever index %d" % index)

    def comm_nex(self, loop) -> int:
        """`CHARSET.c:comm_nex()` and `comm_nex2()`."""
        if loop is None or not loop[0]:
            return 1
        loop[0] -= 1
        if loop[0] > 0:
            self.set_char_move_init2(loop[1], loop[2], loop[3], True)
            return 0
        return 1


# --------------------------------------------------------------------------
# the command handlers
# --------------------------------------------------------------------------

def _dummy(tl, blk):
    return 1


def _roa(tl, blk):
    """`comm_roa()` - the move starts over."""
    tl.set_char_move_init2(*tl.cmoa)
    return 0


def _end(tl, blk):
    """`comm_end()` - the animation loops back to block `pat - 1`."""
    tl.cg_ix = (blk.pat - 2) * tl.cgd
    return 1


def _jmp(tl, blk):
    """`comm_jmp()` / `comm_jpss()` - another move takes over."""
    tl.setup_comm_back()
    tl.set_char_move_init2(blk.koc, blk.ix, blk.pat, False)
    return 0


def _jsr(tl, blk):
    """`comm_jsr()` - call a subroutine."""
    tl.reg.cmsw = (tl.koc, tl.index, tl.cg_ix // tl.cgd + 2)
    tl.set_char_move_init2(blk.koc, blk.ix, blk.pat, False)
    return 0


def _ret(tl, blk):
    """`comm_ret()` - return from a subroutine."""
    if tl.reg.cmsw is None:
        raise DecodeError("comm_ret without a subroutine")
    tl.set_char_move_init2(*tl.reg.cmsw)
    return 0


def _for(tl, blk, register):
    """`comm_for()` / `comm_for2()`."""
    code = blk.pat & 0xF if (blk.pat & 0x4000) else blk.pat
    register[:] = [code, tl.koc, tl.index, tl.cg_ix // tl.cgd + 2]
    return 1


def _for1(tl, blk):
    return _for(tl, blk, tl.reg.cmlp)


def _for2(tl, blk):
    return _for(tl, blk, tl.reg.cml2)


def _rja(tl, blk, which):
    """`comm_rja()` .. `comm_rja7()` - remember a jump target."""
    target = (blk.koc, blk.ix, blk.pat)
    if which == 0:
        tl.reg.cmja = target
    else:
        tl.reg.cmj[which - 1] = target
    return 1


def _uja(tl, blk, which):
    """`comm_uja()` .. `comm_uja7()` - jump to the remembered target."""
    target = tl.reg.cmja if which == 0 else tl.reg.cmj[which - 1]
    if target is None:
        raise DecodeError("comm_uja without a target")
    tl.setup_comm_back()
    tl.set_char_move_init2(*target)
    return 0


def _rmja(tl, blk):
    tl.reg.cmms = (blk.koc, blk.ix, blk.pat)
    return 1


def _umja(tl, blk):
    if tl.reg.cmms is None:
        raise DecodeError("comm_umja without a target")
    tl.setup_comm_back()
    tl.set_char_move_init2(*tl.reg.cmms)
    return 0


def _back(tl, blk):
    if tl.reg.cmbk is None:
        raise DecodeError("comm_back without a saved position")
    tl.set_char_move_init2(*tl.reg.cmbk)
    return 0


def _abbak(tl, blk):
    if tl.reg.cmb3 is None:
        raise DecodeError("comm_abbak without a saved position")
    tl.set_char_move_init2(*tl.reg.cmb3)
    return 0


def _ixfw(tl, blk):
    """`comm_ixfw()` - jump forward."""
    tl.cg_ix += (blk.pat - 1) * tl.cgd
    return 1


def _ixbw(tl, blk):
    """`comm_ixbw()` - jump back."""
    tl.cg_ix -= (blk.pat + 1) * tl.cgd
    return 1


# -- the branches ----------------------------------------------------------

def _if_l(tl, blk):
    """`comm_if_l()` - the direction that is held."""
    if blk.koc & 0x4000:
        my_lvdat = tl.reg.cmwk[blk.koc & 0xF]
    else:
        my_lvdat = blk.koc

    lvdat = tl.LEVER

    if not (my_lvdat & 0x7FFF):
        if lvdat == 0:
            return tl.branch(blk, "if", blk.ix)
        return tl.branch(blk, "else", blk.pat)

    if my_lvdat & 0x8000:
        if lvdat == (my_lvdat & 0xF):
            return tl.branch(blk, "if", blk.ix)
        return tl.branch(blk, "else", blk.pat)

    if lvdat & my_lvdat:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _djmp(tl, blk):
    """`comm_djmp()` - the direction that is held."""
    ldir = tl.LEVER
    if ldir:
        if ldir == 1:
            return tl.branch(blk, "if", blk.ix)
        return tl.branch(blk, "else", blk.pat)
    return tl.branch(blk, "no-lever", blk.koc)


def _rngc(tl, blk):
    """`comm_rngc()` - the range of the body of the opponent."""
    rngdat = 0
    if rngdat > blk.koc:
        return tl.branch(blk, "else", blk.pat)
    return tl.branch(blk, "if", blk.ix)


def _pjmp(tl, blk):
    """`comm_pjmp()` - a random branch."""
    if tl.RANDOM < blk.koc:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _hjmp(tl, blk):
    """`comm_hjmp()` - the move has connected."""
    if tl.HIT_FLAG:
        if tl.HIT_FLAG & 0x303:
            return tl.branch(blk, "hit", blk.koc)
        if tl.HIT_FLAG & 0x3030:
            return tl.branch(blk, "hit", blk.ix)
        if tl.HIT_FLAG & 0xC0C0:
            return tl.branch(blk, "hit", blk.pat)
    return 1


def _if_s(tl, blk):
    """`comm_if_s()` - the buttons that are held."""
    if blk.koc & 0x4000:
        my_shdat = tl.reg.cmwk[blk.koc & 0xF]
    else:
        my_shdat = blk.koc

    if my_shdat == tl.SHOT:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _ifs2(tl, blk):
    """`comm_ifs2()` - the buttons that are held."""
    if blk.koc & 0x4000:
        my_shdat = tl.reg.cmwk[blk.koc & 0xF]
    else:
        my_shdat = blk.koc

    if my_shdat & tl.SHOT:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _ifs3(tl, blk):
    """`comm_ifs3()` - the buttons that are held."""
    if blk.koc & 0x4000:
        my_shdat = tl.reg.cmwk[blk.koc & 0xF]
    else:
        my_shdat = blk.koc

    if my_shdat & tl.SHOT:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _s_chg(tl, blk):
    """`comm_s_chg()` - the buttons that were released."""
    if blk.koc & 0x4000:
        my_shdat = tl.reg.cmwk[blk.koc & 0xF]
    else:
        my_shdat = blk.koc

    if my_shdat == tl.SHOT:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _schg2(tl, blk):
    """`comm_schg2()` - the buttons that were released."""
    if blk.koc & 0x4000:
        my_shdat = tl.reg.cmwk[blk.koc & 0xF]
    else:
        my_shdat = blk.koc

    if my_shdat & tl.SHOT:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _iflb(tl, blk):
    """`comm_iflb()` - the level of the player."""
    if blk.koc & 0x4000:
        my_shdat = tl.reg.cmwk[blk.koc & 0xF]
    else:
        my_shdat = blk.koc

    if my_shdat == 0:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _iflg(tl, blk):
    """`comm_iflg()` - the state of the opponent."""
    if blk.koc == 0:
        if tl.reg.cmwk[11] < blk.ix:
            return 1
        return tl.branch(blk, "else", blk.pat)

    if tl.reg.cmwk[11] < blk.ix:
        return 1
    return tl.branch(blk, "else", blk.pat)


def _mpcy(tl, blk):
    """`comm_mpcy()` - the height of the player."""
    if blk.ix == 1:
        ans = tl.MVXY > blk.koc
    elif blk.ix == 2:
        ans = tl.MVXY < blk.koc
    else:
        ans = tl.MVXY == blk.koc
    if not ans:
        return 1
    return tl.branch(blk, "if", blk.pat)


def _epcy(tl, blk):
    """`comm_epcy()` - the height of the opponent."""
    if blk.ix == 1:
        ans = tl.MVXY > blk.koc
    elif blk.ix == 2:
        ans = tl.MVXY < blk.koc
    else:
        ans = tl.MVXY == blk.koc
    if not ans:
        return 1
    return tl.branch(blk, "if", blk.pat)


def _myhp(tl, blk):
    """`comm_myhp()` - the health of the player."""
    if blk.koc == 1:
        num = tl.VITAL > blk.ix
    elif blk.koc == 2:
        num = tl.VITAL < blk.ix
    else:
        num = tl.VITAL == blk.ix
    if num:
        return tl.branch(blk, "if", blk.pat)
    return 1


def _emhp(tl, blk):
    """`comm_emhp()` - the health of the opponent."""
    if blk.koc == 1:
        num = tl.VITAL > blk.ix
    elif blk.koc == 2:
        num = tl.VITAL < blk.ix
    else:
        num = tl.VITAL == blk.ix
    if num:
        return tl.branch(blk, "if", blk.pat)
    return 1


def _chkwf(tl, blk):
    """`comm_chkwf()` - the special move that is active."""
    if tl.WAZA_FLAG == 0:
        return tl.branch(blk, "else", blk.pat)
    return tl.branch(blk, "if", blk.ix)


def _ifrlf(tl, blk):
    """`comm_ifrlf()` - the facing of the player."""
    if blk.koc:
        if tl.RL_FLAG == tl.RL_WAZA:
            return tl.branch(blk, "else", blk.pat)
        return tl.branch(blk, "if", blk.ix)

    if tl.RL_FLAG == tl.RL_WAZA:
        return tl.branch(blk, "if", blk.ix)
    return tl.branch(blk, "else", blk.pat)


def _bgrlf(tl, blk):
    """`comm_bgrlf()` - the position of the player."""
    if tl.RL_FLAG:
        if tl.MVXY > 0:
            return tl.branch(blk, "else", blk.pat)
        return tl.branch(blk, "if", blk.ix)

    if tl.MVXY < 0:
        return tl.branch(blk, "else", blk.pat)
    return tl.branch(blk, "if", blk.ix)


def _rljmp(tl, blk):
    """`comm_rljmp()` - the facing of the player."""
    if tl.RL_FLAG:
        return tl.branch(blk, "else", blk.pat)
    return tl.branch(blk, "if", blk.ix)


def _ifcom(tl, blk):
    """`comm_ifcom()` - the operator mode."""
    if tl.OPERATOR:
        return tl.branch(blk, "else", blk.pat)
    return tl.branch(blk, "if", blk.ix)


def _axjmp(tl, blk):
    """`comm_axjmp()` - the movement of the player."""
    if tl.MVXY > 2:
        return tl.branch(blk, "if", blk.koc)
    if tl.MVXY < -2:
        return tl.branch(blk, "else", blk.pat)
    return tl.branch(blk, "if", blk.ix)


def _ayjmp(tl, blk):
    """`comm_ayjmp()` - the movement of the player."""
    if tl.MVXY > 0:
        return tl.branch(blk, "if", blk.koc)
    if tl.MVXY < 0:
        return tl.branch(blk, "else", blk.pat)
    return tl.branch(blk, "if", blk.ix)


def _retmj(tl, blk):
    """`comm_retmj()` - the chain of the effect ends here."""
    return 0


def _uhsja(tl, blk):
    """`comm_uhsja()` - the chain moves to the effect work."""
    return 0


# the command table of `CHARSET.c:decode_chcmd[125]`
HANDLERS = {
    0: _dummy,
    1: _roa,
    2: _end,
    3: _jmp,
    4: _jmp,
    5: _jsr,
    6: _ret,
    10: _if_l,
    11: _djmp,
    12: _for1,
    13: lambda tl, blk: tl.comm_nex(tl.reg.cmlp),
    14: _for2,
    15: lambda tl, blk: tl.comm_nex(tl.reg.cml2),
    16: lambda tl, blk: _rja(tl, blk, 0),
    17: lambda tl, blk: _uja(tl, blk, 0),
    18: lambda tl, blk: _rja(tl, blk, 1),
    19: lambda tl, blk: _uja(tl, blk, 1),
    20: lambda tl, blk: _rja(tl, blk, 2),
    21: lambda tl, blk: _uja(tl, blk, 2),
    22: lambda tl, blk: _rja(tl, blk, 3),
    23: lambda tl, blk: _uja(tl, blk, 3),
    24: lambda tl, blk: _rja(tl, blk, 4),
    25: lambda tl, blk: _uja(tl, blk, 4),
    26: lambda tl, blk: _rja(tl, blk, 5),
    27: lambda tl, blk: _uja(tl, blk, 5),
    28: lambda tl, blk: _rja(tl, blk, 6),
    29: lambda tl, blk: _uja(tl, blk, 6),
    30: _rmja,
    31: _umja,
    44: _rngc,
    46: _pjmp,
    47: _hjmp,
    49: _ixfw,
    50: _ixbw,
    53: _if_s,
    65: _iflb,
    69: _back,
    87: _iflg,
    88: _mpcy,
    89: _epcy,
    96: _myhp,
    97: _emhp,
    101: _chkwf,
    102: _retmj,
    109: _ifrlf,
    111: _bgrlf,
    113: _rljmp,
    114: _ifs2,
    115: _abbak,
    117: _s_chg,
    118: _schg2,
    120: _uhsja,
    121: _ifcom,
    122: _axjmp,
    123: _ayjmp,
    124: _ifs3,
}


# --------------------------------------------------------------------------
# the result
# --------------------------------------------------------------------------

class MoveInfo:
    """What the decoder found out about one move."""

    def __init__(self, character, koc, index):
        self.character = character
        self.koc = koc
        self.index = index
        self.frames = []           # the animation frames
        self.game_frames = 0
        self.startup = None        # the game frames before the first hit box
        self.active = 0            # the game frames with a hit box
        self.recovery = None        # the game frames after the last hit box
        self.hit_frame = None      # the frame with the first hit box
        self.attack_frame = None   # the frame with the first attack data
        self.att_ix = 0            # the attack table index of the active frame
        self.att_ok = False        # the attack gives arts points
        self.error = None
        self.branches = []
        self.cycle = False
        self.terminal = False   # the chain ends with the 0xFF sentinel
        self.wait = False       # the chain ends with a "hold the pose" block

    @property
    def name(self):
        return "%s %d/%d" % (self.character.name, self.koc, self.index)

    def attack(self, table):
        """The attack data of the active frame."""
        if not table or self.att_ix >= len(table):
            return None
        return table[self.att_ix]

    def damage(self, table, power_data):
        """The base damage of the active frame."""
        entry = self.attack(table)
        if not entry or not power_data:
            return None
        pow_index = entry["pow"]
        if pow_index >= len(power_data):
            return None
        return power_data[pow_index]


def decode_move(character: Character, koc: int, index: int, registers=None) -> MoveInfo:
    """Decodes the frame chain of one move."""
    info = MoveInfo(character, koc, index)
    try:
        timeline = Timeline(character, koc, index, registers)
    except DecodeError as error:
        info.error = str(error)
        return info
    timeline.run()

    info.frames = timeline.frames
    info.error = timeline.error
    info.branches = timeline.branches
    info.cycle = timeline.cycle
    info.terminal = timeline.terminal
    info.wait = timeline.wait

    game_frame = 0
    idle_hit = info.frames[0].hit_ix if info.frames else None
    first_att = None
    last_att = None
    for number, frame in enumerate(info.frames):
        game_frame += frame.hold
        if frame.hit_ix != idle_hit and info.hit_frame is None:
            # the move does not use the hit box it starts with: this is the
            # first frame with a live hit box
            info.hit_frame = frame
        if frame.has_attack:
            if first_att is None:
                first_att = number
                info.attack_frame = frame
            last_att = number
            info.att_ix = frame.att_ix
            info.att_ok = frame.att_ok

    if first_att is not None:
        # the startup are the game frames before the first frame that carries
        # the attack data, the active frames are the ones from there to the
        # last frame that carries new attack data
        info.startup = sum(frame.hold for frame in info.frames[:first_att])
        info.active = sum(frame.hold for frame in info.frames[first_att:last_att + 1])
        # the recovery are the game frames that follow the last frame that
        # carries the attack data
        info.recovery = sum(frame.hold for frame in info.frames[last_att + 1:])
    elif info.hit_frame is not None:
        info.startup = sum(frame.hold for frame in info.frames[:info.frames.index(info.hit_frame)])

    info.game_frames = game_frame
    return info


# --------------------------------------------------------------------------
# the report
# --------------------------------------------------------------------------

def decode_slot(character: Character, koc: int):
    """Decodes every move of a slot.

    The registers of the chain walker survive from one move to the next, so
    the moves are decoded in the order of their index with the registers
    shared.
    """
    registers = Registers()
    result = []
    for (index, _off, _cgd) in character.moves.get(koc, []):
        result.append(decode_move(character, koc, index, registers))
    return result


def format_table(character, moves, table, power_data, show_branches=False):
    """A table of the decoded moves of one character."""
    lines = []
    lines.append("=== %s (character %d, charset %d) ===" %
                 (character.name, character.index, character.charset))
    header = ("koc idx  frames  start  active   hit  att  damage  stun  hitstun  hs_me hs_you  note")
    lines.append(header)
    lines.append("-" * len(header))

    for info in moves:
        if info.error:
            lines.append("%3d %3d  %s" % (info.koc, info.index, "ERROR: " + info.error))
            continue

        attack = info.attack(table)
        damage = info.damage(table, power_data)
        note = ""
        if info.branches:
            note = "%d branch(es)" % len(info.branches)
        if info.cycle:
            note = (note + " " if note else "") + "loops"

        lines.append("%3d %3d %6d %6s %6d %5s %4s %8s %6s %8s %6s %6s  %s" % (
            info.koc,
            info.index,
            info.game_frames,
            "-" if info.startup is None else info.startup,
            info.active,
            "-" if info.hit_frame is None else info.frames.index(info.hit_frame),
            info.att_ix if info.att_ix else "-",
            "-" if damage is None else damage,
            "-" if attack is None else attack["piyo"],
            "-" if attack is None else attack["impact"],
            "-" if attack is None else attack["hs_me"],
            "-" if attack is None else attack["hs_you"],
            note,
        ))

        if show_branches and info.branches:
            for branch in info.branches:
                lines.append("        branch: %s" % branch)

    return "\n".join(lines)


def write_csv(path, characters, tables, power_data):
    """Writes the decoded moves of every character into a CSV file."""
    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write("character,koc,index,game_frames,startup,active,hit_frame,"
                     "attack_frame,att_ix,att_ok,damage,stun,hitstun,hs_me,hs_you,"
                     "branches,error\n")
        for character in characters:
            table = attack_table_of(tables, character)
            for koc in sorted(character.moves):
                for info in decode_slot(character, koc):
                    attack = info.attack(table)
                    damage = info.damage(table, power_data)
                    hit_frame = (info.frames.index(info.hit_frame)
                                 if info.hit_frame is not None else "")
                    attack_frame = (info.frames.index(info.attack_frame)
                                    if info.attack_frame is not None else "")
                    handle.write("%s,%d,%d,%d,%s,%d,%s,%s,%d,%d,%s,%s,%s,%s,%s,%d,%s\n" % (
                        character.name, info.koc, info.index, info.game_frames,
                        "" if info.startup is None else info.startup,
                        info.active, hit_frame, attack_frame,
                        info.att_ix, 1 if info.att_ok else 0,
                        "" if damage is None else damage,
                        "" if attack is None else attack["piyo"],
                        "" if attack is None else attack["impact"],
                        "" if attack is None else attack["hs_me"],
                        "" if attack is None else attack["hs_you"],
                        len(info.branches),
                        info.error or ""))


def trace_move(character, koc, index, table=None, power_data=None):
    """Prints the frame chain of one move, block by block."""
    info = decode_move(character, koc, index)
    print("=== %s koc=%d idx=%d ===" % (character.name, koc, index))
    if info.error:
        print("  error: %s" % info.error)
        return info

    print("  %-4s %-5s %-5s %-6s %-6s %-6s %-6s %s" %
          ("frm", "hold", "code", "hit_ix", "att_ix", "meoshi", "game", "note"))
    game = 0
    for number, frame in enumerate(info.frames):
        game += frame.hold
        print("  %-4d %-5d %-5d %-6d %-6d %-6d %-6d %s" % (
            number, frame.hold, frame.code, frame.hit_ix, frame.att_ix,
            frame.meoshi, game, "(%d/%d)" % (frame.src_koc, frame.src_index)))
    print("  total: %d game frames, startup %s, active %d, att index %d%s" % (
        info.game_frames, info.startup, info.active, info.att_ix,
        " (arts points)" if info.att_ok else ""))
    for branch in info.branches:
        print("  branch: %s" % branch)
    attack = info.attack(table)
    if attack:
        damage = info.damage(table, power_data)
        print("  attack: pow=%d damage=%s impact=%d piyo=%d hs_me=%d hs_you=%d" % (
            attack["pow"], damage, attack["impact"], attack["piyo"],
            attack["hs_me"], attack["hs_you"]))
    return info


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("dump", nargs="?", default="upper3s_dump.txt",
                        help="the dump of the character data of the disc")
    parser.add_argument("--char", help="the name of a character, e.g. ken")
    parser.add_argument("--all", action="store_true", help="every character")
    parser.add_argument("--trace", help="trace one move, e.g. ken:5:44")
    parser.add_argument("--att-table", help="the attack table (see parse_att_table)")
    parser.add_argument("--power-data", default="src/anniversary/sf33rd/Source/Game/POW_DATA.c",
                        help="POW_DATA.c, for the damage table")
    parser.add_argument("--csv", help="write the whole table into a CSV file")
    parser.add_argument("--branches", action="store_true",
                        help="show the branches that were taken")
    parser.add_argument("--stats", action="store_true",
                        help="a summary of the decoded data")
    args = parser.parse_args()

    characters = parse_dump(args.dump)
    if not characters:
        print("no characters in %s" % args.dump, file=sys.stderr)
        return 1

    tables = load_attack_table(args.att_table)
    if args.att_table and tables is None:
        print("the attack table %s does not exist" % args.att_table, file=sys.stderr)

    power_data = load_power_data(args.power_data)

    if args.csv:
        write_csv(args.csv, characters, tables, power_data)
        print("wrote %s" % args.csv)

    if args.stats:
        print_stats(characters, tables, power_data)
        return 0

    if args.trace:
        name, koc, index = args.trace.split(":")
        for character in characters:
            if character.name == name:
                table = attack_table_of(tables, character)
                trace_move(character, int(koc), int(index), table, power_data)
        return 0

    wanted = [args.char] if args.char else []
    for character in characters:
        if wanted and character.name not in wanted:
            continue
        table = attack_table_of(tables, character)
        moves = []
        for koc in sorted(character.moves):
            moves.extend(decode_slot(character, koc))
        print(format_table(character, moves, table, power_data, args.branches))
        print()

    return 0


def print_stats(characters, tables, power_data):
    """A summary of the decoded data."""
    total = 0
    errors = 0
    branched = 0
    frames_hist = collections.Counter()
    att_hist = collections.Counter()
    startup_hist = collections.Counter()

    for character in characters:
        table = attack_table_of(tables, character)
        for koc in sorted(character.moves):
            for info in decode_slot(character, koc):
                total += 1
                if info.error:
                    errors += 1
                    continue
                if info.branches:
                    branched += 1
                frames_hist[info.game_frames] += 1
                att_hist[info.att_ix] += 1
                if info.startup is not None:
                    startup_hist[info.startup] += 1

    print("moves: %d, errors: %d, moves with branches: %d" % (total, errors, branched))
    print("game frames per move:", sorted(frames_hist.items())[:20])
    print("startup frames:", sorted(startup_hist.items())[:20])
    print("attack table indices used:", len(att_hist),
          "max", max(att_hist) if att_hist else 0)


if __name__ == "__main__":
    sys.exit(main())
