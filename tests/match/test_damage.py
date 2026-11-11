"""The damage and stun items, measured in the real engine: the health and the stun the defender
loses or gains in the vanilla and the 3s' build."""

import unittest

from common import F, both, skip_without_game


def setUpModule():
    skip_without_game()


def bar_lost(rows, d=1):
    """Health the defender lost, in the game's health-bar units (160 is a full bar)."""
    return rows[0]["p"][d]["vit"] - rows[-1]["p"][d]["vit"]


def stun_dealt(rows, d=1):
    """Stun dealt to the defender: the sum of the increases of the stun gauge (its recovery
    between hits does not count)."""
    return sum(max(0, b["p"][d]["stun"] - a["p"][d]["stun"]) for a, b in zip(rows, rows[1:]))


def stun_per_hit(rows, d=1):
    return [b["p"][d]["stun"] - a["p"][d]["stun"] for a, b in zip(rows, rows[1:]) if b["p"][d]["stun"] > a["p"][d]["stun"]]


class Damage(unittest.TestCase):
    def test_ken_shippu_does_15_percent_less(self):
        # KN1 "Shippu Jinrai Kyaku (SAIII) ... damage reduced by 15%." Every strength ends in
        # the same finishing animation, which has to be reduced too.
        for k in ("LK", "HK"):
            with self.subTest(k):
                v, u = both(["@stocks 1 3"] + F.motion("236236", k) + F.wait(220), p1="ken", p2="ken", sa1=2, pos=(400, 440))
                self.assertAlmostEqual(bar_lost(u) / bar_lost(v), 0.85, delta=0.02)

    def test_ken_jab_dp(self):
        # KN2 "Jab DP (623LP) damage reduced from 15 to 11.5."
        v, u = both(F.motion("623", "LP") + F.wait(80), p1="ken", p2="ken", pos=(400, 440))
        self.assertAlmostEqual(bar_lost(u) / bar_lost(v), 11.5 / 15, delta=0.03)

    def test_remy_crouching_strong_almost_matches_ryus(self):
        # RM2 "Increase damage of crouching Strong by 6pt to almost match Ryu's."
        v, u = both(["1 2+MP"] + F.wait(60), p1="remy", p2="ken", pos=(400, 440))
        ryu = bar_lost(F.run(["1 2+MP"] + F.wait(60), p1="ryu", p2="ken", pos=(400, 440), build="upper3s"))
        self.assertLess(bar_lost(v), ryu - 4)
        self.assertLess(bar_lost(u), ryu)
        self.assertGreaterEqual(bar_lost(u), ryu - 1)

    def test_urien_tyrant_slaughter_does_25_percent_more(self):
        # UR1 "... increase damage by 25% to match Shin Shoryuken."
        v, u = both(["@stocks 1 3"] + F.motion("236236", "LP") + F.wait(240), p1="urien", p2="ken", pos=(400, 440))
        self.assertAlmostEqual(bar_lost(u) / bar_lost(v), 1.25, delta=0.03)

    def test_akuma_kkz(self):
        # AK4 "... damage reduced from 107 to 95." Many small hits round, so the total is
        # checked loosely and the big first hit tightly.
        script = ["@stocks 1 3", "1 2", "1 5", "1 2", "1 5", "1 2+LP+MP+HP"] + F.wait(200)
        v, u = both(script, p1="akuma", p2="ken", pos=(400, 440))
        self.assertLess(bar_lost(u), bar_lost(v))
        first = [next(a["p"][1]["vit"] - b["p"][1]["vit"] for a, b in zip(rows, rows[1:]) if b["p"][1]["vit"] < a["p"][1]["vit"])
                 for rows in (v, u)]
        self.assertAlmostEqual(first[1] / first[0], 95 / 107, delta=0.03)


class Stun(unittest.TestCase):
    def test_makoto_abare_tosanami_does_half_the_stun(self):
        # MK1 "Stun damage of Abare Tosanami (SAII) reduced by 50%." It only connects from near
        # a wall, so Makoto walks into the corner first.
        v, u = both(["150 4 6", "@stocks 1 3"] + F.motion("236236", "LK") + F.wait(160), p1="makoto", p2="ken", sa1=1)
        # The stun gauge keeps fractions between hits, so the increments of each hit are not
        # exactly halved (9, 2, 4, 7 -> 4, 1, 1, 3); the total is.
        self.assertEqual(len(stun_per_hit(v[150:])), len(stun_per_hit(u[150:])))
        self.assertAlmostEqual(stun_dealt(u[150:]) / stun_dealt(v[150:]), 0.5, delta=0.1)

    def test_makoto_fukiage(self):
        # MK2 "Stun damage of L/M/H Fukiage reduced from 29/31/31 to 27/28/31." Fukiage is an
        # anti-air: the opponent jumps in first.
        for k, delay, x2, retail, target in (("LP", 0, 440, 29, 27), ("MP", 6, 480, 31, 28), ("HP", 2, 480, 31, 31)):
            with self.subTest(k):
                v, u = both(["4 5 9", f"{delay} 5 5"] + F.motion("623", k) + F.wait(80), p1="makoto", p2="ken", pos=(400, x2))
                self.assertEqual(stun_per_hit(v), [retail])
                self.assertEqual(stun_per_hit(u), [target])

    def test_dudley_super_stun(self):
        # DD1 "Reduce stun damage on Rocket Uppercut (SAI)." DD2 "Increase stun damage on
        # Rolling Thunder (SAII)."
        for sa, change in ((0, -1), (1, 1)):
            with self.subTest(sa=sa + 1):
                v, u = both(["@stocks 1 3"] + F.motion("236236", "LP") + F.wait(240), p1="dudley", p2="ken", sa1=sa, pos=(400, 440))
                self.assertGreater(stun_dealt(v), 0)
                self.assertEqual((stun_dealt(u) > stun_dealt(v)) - (stun_dealt(u) < stun_dealt(v)), change)


if __name__ == "__main__":
    unittest.main()
