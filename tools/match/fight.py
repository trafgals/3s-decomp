"""Run scripted fights in the headless build and measure what happens in them.

The headless build (make PLATFORM=headless) plays one fight per process, driven by an
input script, and logs both players' state on every frame (see
src/anniversary/port/headless/harness.c). This module writes the script, runs the
build and reads the log back, so tests can measure startup, hitstun, advantage,
damage, stun and meter in the real engine.

The game data is not part of the repository: point SF33_GAME_DATA at a directory that
holds rom/THIRD/SF33RD.AFS and rom/THIRD/IOP (the disc's files).

    rows = run(["1 5+LP"], p1="ryu", p2="ken", pos=(420, 470))
    hit = first_hit(rows, attacker=0)

A row is {"f": frame, "p": [player0, player1]}, a player is the dict the harness logs.
Frame 1 is the first frame on which the players can act.
"""

import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent

BINARIES = {
    "upper3s": ROOT / "build/anniversary/headless/sf33rd-headless",
    "vanilla": ROOT / "build/anniversary/headless-vanilla/sf33rd-headless",
}

# The character index of the game (AGENTS.md 3.1).
CHARS = {
    "gill": 0, "alex": 1, "ryu": 2, "yun": 3, "dudley": 4, "necro": 5, "hugo": 6, "ibuki": 7,
    "elena": 8, "oro": 9, "yang": 10, "ken": 11, "sean": 12, "urien": 13, "akuma": 14,
    "chunli": 15, "makoto": 16, "q": 17, "twelve": 18, "remy": 19,
}

# WORK.routine_no[1], the state group of a player: 0 normal (free to act), 1 being hit,
# 4 attacking. Checked against the logs (tests/match/test_harness.py).
RNO_NORMAL = 0
RNO_DAMAGE = 1
RNO_ATTACK = 4


class FightError(RuntimeError):
    pass


def game_data():
    path = os.environ.get("SF33_GAME_DATA")
    if not path or not (Path(path) / "rom/THIRD/SF33RD.AFS").is_file():
        raise FightError("set SF33_GAME_DATA to a directory with rom/THIRD/SF33RD.AFS")
    return Path(path)


def char_index(c):
    return c if isinstance(c, int) else CHARS[c]


def run(script, p1="ryu", p2="ryu", sa1=0, sa2=0, build="upper3s", pos=None, frames=12000):
    """Play a fight and return its rows. script is a list of harness script lines; pos
    (x1, x2) places the players before the script starts."""
    binary = BINARIES[build]
    if not binary.is_file():
        raise FightError(f"{binary} is missing: make PLATFORM=headless ...")

    lines = list(script)
    if pos is not None:
        lines = ["1 5 5", f"@pos {pos[0]} {pos[1]}", "2 5 5"] + lines

    with tempfile.TemporaryDirectory(prefix="fight-") as tmp:
        script_path = Path(tmp) / "script.txt"
        log_path = Path(tmp) / "log.jsonl"
        script_path.write_text("\n".join(lines) + "\n")
        env = dict(os.environ)
        env.update({
            "SF33_P1": str(char_index(p1)), "SF33_P2": str(char_index(p2)),
            "SF33_SA1": str(sa1), "SF33_SA2": str(sa2),
            "SF33_SCRIPT": str(script_path), "SF33_LOG": str(log_path), "SF33_FRAMES": str(frames),
        })
        out = subprocess.run([str(binary)], cwd=game_data(), env=env, capture_output=True, text=True, timeout=300)
        if out.returncode != 0:
            raise FightError(f"{build} exited with {out.returncode}:\n{out.stderr[-2000:]}")
        rows = [json.loads(line) for line in log_path.read_text().splitlines()]

    for r in rows:
        for i, pl in enumerate(r["p"]):
            if pl["char"] != char_index((p1, p2)[i]):
                raise FightError(f"player {i + 1} is character {pl['char']}, not {(p1, p2)[i]}")
    return rows


def motion(dirs, buttons, p=1, hold=1):
    """Script lines for a command motion: each numpad direction of `dirs` for `hold`
    frames, the last one together with `buttons` (e.g. motion("236", "LK"),
    motion("63214", "LP+MP")). The other player stays neutral."""
    lines = []
    for k, d in enumerate(dirs):
        last = k == len(dirs) - 1
        inp = f"{d}+{buttons}" if last and buttons else d
        lines.append(f"{hold} {inp} 5" if p == 1 else f"{hold} 5 {inp}")
    return lines


def wait(frames):
    return [f"{frames} 5 5"]


# --- measurements -----------------------------------------------------------------


def player(rows, f, i):
    return rows[f - 1]["p"][i]


def first(rows, pred, start=1):
    """The first frame >= start for which pred(row) holds, or None."""
    for r in rows[start - 1:]:
        if pred(r):
            return r["f"]
    return None


def press_frame(rows, i, button_mask=0xFF0, start=1):
    """The first frame >= start on which player i's input has one of these buttons."""
    return first(rows, lambda r: r["p"][i]["in"] & button_mask, start)


def first_hit(rows, attacker=0, start=1):
    """The first frame >= start on which the attacker's attack lands on the other player
    (the defender goes into the damage state)."""
    d = 1 - attacker
    return first(rows, lambda r: r["p"][d]["rno"][1] == RNO_DAMAGE, start)


def free(rows, i, start):
    """The first frame >= start on which player i is back in the normal state group,
    i.e. can act again."""
    return first(rows, lambda r: r["p"][i]["rno"][1] == RNO_NORMAL, start)


def active_frame(rows, attacker=0, start=1):
    """The first frame on which the attack the attacker starts first is active: its
    attack_num changes (CHARSET.c:set_new_attnum() gives every new hit a new number),
    which does not depend on the distance to the opponent."""
    p = press_frame(rows, attacker, start=start)
    if p is None:
        return None
    before = player(rows, p, attacker)["anum"]
    return first(rows, lambda r: r["p"][attacker]["anum"] != before, p)


def startup(rows, attacker=0, start=1):
    """Startup in the convention of the 3s' design document: the frames before the first
    active frame, counted from the frame of the press (Chun-Li's crouching Jab is 2, an
    attack that is active on the frame after the press is 1). Checked against the
    document's retail numbers (tests/match/test_harness.py)."""
    p = press_frame(rows, attacker, start=start)
    a = active_frame(rows, attacker, start=start)
    return None if p is None or a is None else a - p


def startup_to_hit(rows, attacker=0, start=1):
    """Frames from the press to the first hit, in the same convention; differs from
    startup() for moves that travel or for opponents out of range."""
    p = press_frame(rows, attacker, start=start)
    h = first_hit(rows, attacker, start=p)
    return None if p is None or h is None else h - p


def hit_advantage(rows, attacker=0, start=1):
    """Frames the attacker can act before the defender after the first hit (negative when
    the defender recovers first)."""
    h = first_hit(rows, attacker, start)
    if h is None:
        return None
    a = free(rows, attacker, h)
    d = free(rows, 1 - attacker, h)
    return None if a is None or d is None else d - a


def damage(rows, attacker=0):
    """Health the defender lost over the whole fight, in health points."""
    d = 1 - attacker
    return rows[0]["p"][d]["ovit"] * (rows[0]["p"][d]["vit"] - rows[-1]["p"][d]["vit"]) / rows[0]["p"][d]["vit"]
