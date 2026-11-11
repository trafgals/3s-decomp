"""The hitstun items: normals made "not negative on hit" (RY3, Q6, RM4, TW2, SN1), Hugo's +2
frames (HU3) and Remy's Cold Blue Kick (RM10), measured in the real engine."""

import unittest

from common import F, both, skip_without_game


def setUpModule():
    skip_without_game()


# (character, label, script, distance, vanilla advantage on hit, 3s' advantage on hit)
ITEMS = [
    # RY3 "Increase hitstun on far standing Forward, standing Roundhouse, close Fierce, crouching
    # Fierce, crouching Short, and crouching Forward so they are not negative on hit."
    ("ryu", "far s.MK", ["1 5+MK"], 100, -4, 0),
    ("ryu", "s.HK", ["1 5+HK"], 60, -3, 0),
    ("ryu", "close s.HP", ["1 5+HP"], 40, -2, 0),
    ("ryu", "c.HP", ["1 2+HP"], 40, -6, 0),
    ("ryu", "c.LK", ["1 2+LK"], 40, 1, 1),  # already +1 in the retail data: nothing to do
    ("ryu", "c.MK", ["1 2+MK"], 40, -2, 0),
    # Q6 "Increase hitstun on crouching Short, crouching Forward, and standing Roundhouse so they
    # are not negative on hit. Standing Roundhouse is -10 on hit."
    ("q", "c.LK", ["1 2+LK"], 40, -2, 0),
    ("q", "c.MK", ["1 2+MK"], 40, -2, 0),
    ("q", "s.HK", ["1 5+HK"], 60, -10, 0),
    ("q", "far s.HK", ["1 5+HK"], 100, -10, 0),
    # RM4 "Increase hitstun of far Forward, far Roundhouse, crouching Forward, and Towards+Forward
    # (6MK) so that they are not negative on hit."
    ("remy", "far s.MK", ["1 5+MK"], 100, -2, 0),
    ("remy", "far s.HK", ["1 5+HK"], 100, -7, 0),
    ("remy", "c.MK", ["1 2+MK"], 40, -8, 0),
    ("remy", "6MK", ["1 6+MK"], 40, -6, 0),
    # RM10 "Increase hit advantage of Short/Forward Cold Blue Kicks (63214K) so they are not
    # negative on hit from up close; make Forward version +2."
    ("remy", "LK Cold Blue Kick", F.motion("63214", "LK"), 40, -4, 0),
    ("remy", "MK Cold Blue Kick", F.motion("63214", "MK"), 40, -4, 2),
    # TW2 "Increase hitstun on standing Strong, far Strong, standing Fierce, crouching Fierce,
    # standing Short, standing Forward, standing Roundhouse, crouching Forward, and Back+Forward."
    ("twelve", "s.MP", ["1 5+MP"], 40, -1, 0),
    ("twelve", "c.HP (three hits)", ["1 2+HP"], 40, -1, 0),
    ("twelve", "far s.MP", ["1 5+MP"], 140, -1, 0),
    ("twelve", "s.HP", ["1 5+HP"], 40, -1, 0),
    ("twelve", "far s.HP", ["1 5+HP"], 100, -1, 0),
    ("twelve", "s.LK", ["1 5+LK"], 40, -1, 0),
    ("twelve", "s.MK", ["1 5+MK"], 40, -1, 0),
    ("twelve", "s.HK", ["1 5+HK"], 40, -1, 0),
    ("twelve", "c.MK", ["1 2+MK"], 40, -1, 0),
    ("twelve", "4MK", ["1 4+MK"], 40, -1, 0),
    # SN1 "Increase hitstun of close and far Fierce, crouching Fierce, standing Forward, far
    # Roundhouse, crouching Forward, and Towards+Fierce (6HP) to not be negative on hit."
    ("sean", "close s.HP", ["1 5+HP"], 40, -2, 0),
    ("sean", "far s.HP", ["1 5+HP"], 100, -5, 0),
    ("sean", "c.HP", ["1 2+HP"], 40, -7, 0),
    ("sean", "s.MK", ["1 5+MK"], 40, -1, 0),
    ("sean", "far s.HK", ["1 5+HK"], 100, -7, 0),
    ("sean", "c.MK", ["1 2+MK"], 40, -3, 0),
    ("sean", "6HP", ["1 6+HP"], 40, -4, 0),
    # HU3 "Increase hitstun of crouching Jab (2LP) and crouching Short (2LK) by 1-2f."
    ("hugo", "c.LP", ["1 2+LP"], 40, -1, 1),
    ("hugo", "c.LK", ["1 2+LK"], 40, 2, 4),
]

# Moves next to the ones above that the spec does not change: they must be the same in both builds.
UNTOUCHED = [
    ("ryu", "s.LP", ["1 5+LP"], 40),
    ("ryu", "far s.HP", ["1 5+HP"], 100),
    ("remy", "close s.MK", ["1 5+MK"], 40),
    ("remy", "close s.HK", ["1 5+HK"], 40),
    ("sean", "close s.HK", ["1 5+HK"], 40),
    ("chunli", "c.LK", ["1 2+LK"], 40),
]


class Hitstun(unittest.TestCase):
    def test_items(self):
        for char, label, script, d, retail, target in ITEMS:
            with self.subTest(char=char, move=label):
                v, u = both(script + F.wait(120), p1=char, p2="ken", pos=(400, 400 + d))
                self.assertEqual(F.hit_advantage(v), retail, "vanilla")
                self.assertEqual(F.hit_advantage(u), target, "3s'")

    def test_untouched_moves(self):
        for char, label, script, d in UNTOUCHED:
            with self.subTest(char=char, move=label):
                v, u = both(script + F.wait(120), p1=char, p2="ken", pos=(400, 400 + d))
                self.assertIsNotNone(F.first_hit(v))
                self.assertEqual(F.hit_advantage(v), F.hit_advantage(u))


if __name__ == "__main__":
    unittest.main()
