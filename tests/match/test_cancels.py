"""The cancel items (AL6, AL7, Q1, Q2, TW5, TW7, TW9): a normal that hits, is blocked or is parried,
then a special move or a super art entered during the hitstop. The cancel worked when the special starts
before the normal would have ended."""

import unittest

from common import F, skip_without_game


def setUpModule():
    skip_without_game()


def expand(lines):
    """The leading @ directives of a script, and one p1 input per frame for the rest."""
    directives, inputs = [], []
    for line in lines:
        if line.startswith("@"):
            directives.append(line)
            continue
        n, a = line.split()[:2]
        inputs += [a] * int(n)
    return directives, inputs


def play(char, p1_lines, p2, sa, d, build):
    directives, p1 = expand(p1_lines)
    script = directives + [f"1 {a} {p2.get(i, '5')}" for i, a in enumerate(p1, 1)]
    return F.run(script, p1=char, p2="ken", sa1=sa, pos=(400, 400 + d), build=build)


# HITCHECK.c puts a parrying character in normal state 31-34 (see test_system.py).
def contact(rows, defend, start=1):
    if defend == "parry":
        return F.first(rows, lambda r: r["p"][1]["rno"][1] == 0 and r["p"][1]["rno"][2] in (31, 32, 33, 34), start)
    return F.first_hit(rows, start=start)  # a hit or a block: the damage state group


def cancels(char, normal, follow, build, d=40, sa=0, stocks=False, lead=(), defend="hit"):
    """True when `follow` (script lines entered on the frame after `normal` makes contact) starts a
    special move or super art (slot 5) before the normal would have ended. `lead` comes before the
    normal (a jump, for an air normal). `defend` is what Ken does: nothing ("hit"), block
    (down-back against a crouching normal, back otherwise) or parry (a tap 6 frames before the hit, down
    against a crouching kick, 4 frames before against an air normal)."""
    pre = (["@stocks 1 3"] if stocks else []) + list(lead)
    alone = pre + [f"1 {normal}"] + F.wait(80)
    h0 = F.first_hit(play(char, alone, {}, sa, d, build))
    assert h0 is not None, f"{char} {normal} does not hit"
    p2 = {}
    if defend == "block":
        guard = "1" if normal.startswith("2+") else "4"  # far normals whiff over a crouching block
        p2 = {i: guard for i in range(max(1, h0 - 6), h0 + 60)}
    elif defend == "parry":
        tap = h0 - (4 if lead else 6)
        p2 = {tap: "2" if normal.startswith("2+") and normal.endswith("K") else "6"}
    rows = play(char, alone, p2, sa, d, build)
    h = contact(rows, defend)
    assert h is not None, f"{char} {normal}: no {defend}"
    end = F.free(rows, 0, h)
    if lead:  # an air normal ends when the character lands
        end = F.first(rows, lambda r: r["p"][0]["y"] == 0, h)
    press = F.press_frame(rows, 0)
    rows = play(char, pre + [f"1 {normal}", f"{h - press} 5 5"] + follow + F.wait(80), p2, sa, d, build)
    started = F.first(rows, lambda r: r["p"][0]["koc"] == 5, h)
    return started is not None and started < end


DEFENCES = ("hit", "block", "parry")


SPECIAL = {"alex": F.motion("236", "LP"), "q": F.motion("214", "LP"), "twelve": F.motion("236", "LP")}
SUPER = F.motion("23623", "LP")

# (item, character, normal, distance, special cancel in 3s', super cancel in 3s', super art slot)
GROUND = [
    ("AL6 Alex c.MP", "alex", "2+MP", 40, True, True, 1),
    ("AL7 Alex c.MK", "alex", "2+MK", 40, False, True, 1),
    ("Q1 Q c.LP", "q", "2+LP", 40, True, True, 0),
    ("Q1 Q s.LK", "q", "5+LK", 40, True, True, 0),
    ("Q1 Q c.LK", "q", "2+LK", 40, True, True, 0),
    ("Q2 Q c.MK", "q", "2+MK", 40, False, True, 0),
    ("TW5 Twelve far s.MP", "twelve", "5+MP", 140, True, True, 0),
    ("TW5 Twelve c.MP", "twelve", "2+MP", 40, True, True, 0),
    ("TW7 Twelve c.MK", "twelve", "2+MK", 40, False, True, 0),
]


class Cancels(unittest.TestCase):
    def test_retail_controls(self):
        # Ryu's c.MK cancels into a special in the retail game, his s.HK does not.
        hadoken = F.motion("236", "LP")
        for build in ("vanilla", "upper3s"):
            for defend in DEFENCES:
                with self.subTest(build=build, defend=defend):
                    self.assertTrue(cancels("ryu", "2+MK", hadoken, build, defend=defend))
                    self.assertFalse(cancels("ryu", "5+HK", hadoken, build, defend=defend))

    def test_ground_normals(self):
        # "special- and super-cancellable on hit/block/parry"
        for item, char, normal, d, special, sup, sa in GROUND:
            for defend in DEFENCES:
                with self.subTest(item, defend=defend):
                    self.assertFalse(cancels(char, normal, SPECIAL[char], "vanilla", d, defend=defend), "vanilla special")
                    self.assertFalse(cancels(char, normal, SUPER, "vanilla", d, sa, True, defend=defend), "vanilla super")
                    self.assertEqual(cancels(char, normal, SPECIAL[char], "upper3s", d, defend=defend), special, "3s' special")
                    self.assertEqual(cancels(char, normal, SUPER, "upper3s", d, sa, True, defend=defend), sup, "3s' super")

    def test_twelve_air_normals(self):
        # TW9 "Allow all air normals to be special-cancelled on hit/block into A.X.E. (j.214P) or
        # D.R.A. (j.214K) and super-cancelled into X.F.L.A.T (SAII)." A jumping Roundhouse that
        # hits a standing opponent on the way down.
        jump = ["4 9", "14 5"]
        for name, follow, stocks in (("A.X.E.", F.motion("214", "LP"), False), ("D.R.A.", F.motion("214", "LK"), False),
                                     ("X.F.L.A.T.", F.motion("23623", "LK"), True)):
            for defend in ("hit", "block"):
                with self.subTest(name, defend=defend):
                    self.assertFalse(cancels("twelve", "5+HK", follow, "vanilla", 120, 1, stocks, jump, defend))
                    self.assertTrue(cancels("twelve", "5+HK", follow, "upper3s", 120, 1, stocks, jump, defend))


if __name__ == "__main__":
    unittest.main()
