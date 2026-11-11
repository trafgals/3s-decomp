"""The system mechanics measured in the real engine: the whiff meter scaling (SM1), the parry
cooldown and its damage penalty (SM2) and the universal overhead and taunt while holding a
direction (B3)."""

import unittest

from common import F, both, skip_without_game


def setUpModule():
    skip_without_game()


def frames(n, p1, p2=None):
    """A script of `n` single frames. `p1` and `p2` map a frame (1-based) to that player's input;
    every other frame is neutral."""
    p2 = p2 or {}
    return [f"1 {p1.get(i, '5')} {p2.get(i, '5')}" for i in range(1, n + 1)]


def lost(rows, d=1):
    return rows[0]["p"][d]["vit"] - rows[-1]["p"][d]["vit"]


# HITCHECK.c sets routine_no[2] to 31-34 on a parry (defense_ground(), defense_sky()), in the normal
# state group (routine_no[1] 0: set_paring_status()). Other groups use the same numbers for other things.
PARRY_STATES = (31, 32, 33, 34)


def parried(rows, d=1):
    return any(r["p"][d]["rno"][1] == 0 and r["p"][d]["rno"][2] in PARRY_STATES for r in rows)


def meter_gains(rows, p=0):
    """The increases of a player's super meter, in whole units."""
    return [b["p"][p]["gauge"] - a["p"][p]["gauge"] for a, b in zip(rows, rows[1:]) if b["p"][p]["gauge"] > a["p"][p]["gauge"]]


class WhiffMeter(unittest.TestCase):
    def test_whiffed_normals_halve_the_gain(self):
        # SM1 "each successive whiffed normal gains half as much meter as the previous one". Ryu
        # whiffs a standing Fierce six times from across the screen. The halved gain is rounded to
        # the nearest unit (1.5 -> 2, 0.75 -> 1, 0.375 -> 0).
        script = ["@stocks 1 0"]
        for _ in range(6):
            script += ["1 5+HP"] + F.wait(45)
        v, u = both(script, p1="ryu", p2="ken", pos=(200, 600))
        self.assertEqual(meter_gains(v), [3] * 6)
        self.assertEqual(meter_gains(u), [3, 2, 1])


class ParryCooldown(unittest.TestCase):
    # Ryu's close standing Fierce at frame 50 hits Ken (40 apart) on frame 57. Ken taps forward
    # (6) for one frame. The interpreter sees an input three frames late.
    ATTACK = {50: "5+HP"}
    HIT = 57

    def fight(self, p2, build, p1=None):
        return F.run(frames(110, p1 or self.ATTACK, p2), p1="ryu", p2="ken", pos=(400, 440), build=build)

    def test_the_hit(self):
        v, u = both(frames(110, self.ATTACK), p1="ryu", p2="ken", pos=(400, 440))
        self.assertEqual(F.first_hit(v), self.HIT)
        self.assertEqual(lost(v), lost(u))

    def test_parries_in_the_window_are_unchanged(self):
        # "Successful parries are unchanged": a tap 3 to 12 frames before the hit parries it.
        for before in range(1, 14):
            with self.subTest(before=before):
                v, u = (self.fight({self.HIT - before: "6"}, b) for b in ("vanilla", "upper3s"))
                self.assertEqual(parried(v), 3 <= before <= 12)
                self.assertEqual(parried(u), parried(v))

    def test_a_hit_in_the_cooldown_does_25_percent_more(self):
        # "if you attempt a Forward(6) parry and miss, and are then hit by an attack that can be
        # parried with Forward(6) during the cooldown period, you take 25% extra damage". The
        # retail cooldown runs to 24 frames after the tap; 3s' adds two frames.
        retail = lost(self.fight({}, "vanilla"))
        for before in range(13, 30):
            with self.subTest(before=before):
                v, u = (self.fight({self.HIT - before: "6"}, b) for b in ("vanilla", "upper3s"))
                self.assertFalse(parried(v) or parried(u))
                self.assertEqual(lost(v), retail)
                self.assertEqual(lost(u), round(retail * 1.25) if before <= 26 else retail)

    def test_the_cooldown_is_two_frames_longer(self):
        # "+2f on the cooldown period after attempting a parry, before another parry is allowed."
        # A first tap `k` frames before a second one that would parry: the second tap only counts
        # once the cooldown of the first is over.
        def second_tap_parries(k, build):
            return parried(self.fight({self.HIT - 5 - k: "6", self.HIT - 5: "6"}, build))

        self.assertEqual([k for k in range(18, 30) if second_tap_parries(k, "vanilla")], list(range(24, 30)))
        self.assertEqual([k for k in range(18, 30) if second_tap_parries(k, "upper3s")], list(range(26, 30)))

    def test_only_the_parry_that_was_missed(self):
        # "if you attempt a Forward(6) parry and miss, and are hit by an attack that must be parried
        # Down(2), you will not take any extra damage." Ryu's crouching Forward is a low; it hits on
        # frame 60 when pressed on frame 50.
        low = {50: "2+MK"}
        retail = lost(self.fight({}, "vanilla", low))
        self.assertEqual(lost(self.fight({60 - 15: "6"}, "upper3s", low)), retail)
        self.assertEqual(lost(self.fight({60 - 15: "2"}, "upper3s", low)), round(retail * 1.25))
        self.assertEqual(lost(self.fight({self.HIT - 15: "2"}, "upper3s")), lost(self.fight({}, "vanilla")))

    def test_no_bonus_on_a_throw(self):
        # "Being thrown out of parry cooldown does not grant extra damage." Ryu's forward throw
        # catches Ken on frame 55.
        throw = {50: "6+LP+LK"}
        for before in (5, 15, 20):
            with self.subTest(before=before):
                v, u = (self.fight({55 - before: "6"}, b, throw) for b in ("vanilla", "upper3s"))
                self.assertGreater(lost(v), 0)
                self.assertEqual(lost(u), lost(v))

    def test_red_parries_are_unchanged(self):
        # Ken blocks the first hit of Ryu's Collarbone Breaker (6MP, hits on frames 48 and 59) and
        # taps forward in the blockstun to parry the second.
        def red(t, build):
            p2 = {i: "4" for i in range(40, t - 1)}
            p2[t - 1] = "5"
            p2[t] = "6"
            return parried(F.run(frames(120, {30: "6+MP"}, p2), p1="ryu", p2="ken", pos=(400, 440), build=build))

        window = [t for t in range(48, 62) if red(t, "vanilla")]
        self.assertEqual(window, [53, 54, 55])
        self.assertEqual([t for t in range(48, 62) if red(t, "upper3s")], window)


def parry_taps(p1, hit, befores, p2=None, pos=(400, 440), frames_=190, start=0):
    """For each `before`, (vanilla, 3s') of: Ken taps forward `before` frames before the hit on
    frame `hit` and parries it. `p2` holds Ken's other inputs."""
    out = {}
    for before in befores:
        inputs = dict(p2 or {})
        inputs[hit - before] = "6"
        out[before] = tuple(parried(F.run(frames(frames_, p1, inputs), p1="ryu", p2="ken", pos=pos, build=b)[start:])
                            for b in ("vanilla", "upper3s"))
    return out


class GuessParry(unittest.TestCase):
    # SM3 "Parries input during any wakeup animation, recovery roll, or air land-on-feet hitstun
    # are only active for half as long as a regular parry". The retail ground parry is active
    # for 10 frames: a tap 4 to 13 frames before the hit parries it here (the hit is checked one
    # frame later than in ParryCooldown). Half of it is 5 frames: 4 to 8.
    BEFORE = range(2, 16)
    RETAIL = [b for b in BEFORE if 4 <= b <= 13]
    GUESS = [b for b in BEFORE if 4 <= b <= 8]

    def check(self, results):
        self.assertEqual([b for b, (v, u) in results.items() if v], self.RETAIL)
        self.assertEqual([b for b, (v, u) in results.items() if u], self.GUESS)

    def test_wakeup(self):
        # Ryu sweeps Ken (who gets up on frame 90), walks up and hits him with a standing Forward
        # on frame 94.
        p1 = {10: "2+HK", 86: "5+MK"}
        p1.update({i: "6" for i in range(55, 86)})
        self.check(parry_taps(p1, 94, self.BEFORE, start=85))

    def test_recovery_roll(self):
        # Ryu's Fierce Shoryuken knocks Ken down; Ken techs the landing (down on frame 64) and is
        # up on frame 114 instead of 141. Ryu's Hadoken hits him on frame 118.
        p1 = {1: "6", 2: "2", 3: "3+HP", 86: "2", 87: "3", 88: "6+HP"}
        self.check(parry_taps(p1, 118, self.BEFORE, p2={64: "2"}, start=100))

    def test_air_land_on_feet(self):
        # Ryu's jab hits Ken out of a jump (frame 22); Ken falls (damage reaction 17) and lands on
        # his feet on frame 59. Ryu walks up and his standing Forward hits on frame 66.
        p1 = {16: "5+LP", 58: "5+MK"}
        p1.update({i: "6" for i in range(30, 58)})
        p2 = {i: "8" for i in range(10, 14)}
        results = parry_taps(p1, 66, self.BEFORE, p2=p2, pos=(400, 450), frames_=150, start=40)
        # Only the taps made in the fall are guess parries: the ones up to 10 frames before the
        # hit are made after the landing and keep the retail window.
        self.assertEqual([b for b, (v, u) in results.items() if v], self.RETAIL)
        self.assertEqual([b for b, (v, u) in results.items() if u], [b for b in self.BEFORE if 4 <= b <= 10])

    def test_a_normal_parry_keeps_its_window(self):
        # A tap when Ken is standing is not a guess parry.
        results = parry_taps({50: "5+HP"}, 58, self.BEFORE, frames_=110)
        self.assertEqual([b for b, (v, u) in results.items() if u], [b for b, (v, u) in results.items() if v])

    def test_no_throw_tech_after_a_guess_parry(self):
        # "If you are thrown during the parry window or cooldown from one of these parries, you
        # cannot tech the throw." Ryu sweeps Ken and throws him as he gets up (caught on frame
        # 97); Ken techs on frame 98.
        p1 = {10: "2+HK", 92: "6+LP+LK"}
        p1.update({i: "6" for i in range(55, 92)})

        def thrown(build, tap=None):
            p2 = {98: "5+LP+LK"}
            if tap:
                p2[tap] = "6"
            rows = F.run(frames(200, p1, p2), p1="ryu", p2="ken", pos=(400, 440), build=build)
            self.assertTrue(any(r["p"][1]["rno"][1] == 3 for r in rows), "Ken is caught")
            return rows[80]["p"][1]["vit"] - rows[-1]["p"][1]["vit"] > 0

        self.assertFalse(thrown("vanilla"))
        self.assertFalse(thrown("upper3s"))
        for tap in (80, 84):  # a forward tap while Ken is down
            with self.subTest(tap=tap):
                self.assertFalse(thrown("vanilla", tap))
                self.assertTrue(thrown("upper3s", tap))


class OverheadAndTaunt(unittest.TestCase):
    def test_while_holding_a_direction(self):
        # B3 "Universal overheads (MP+MK) and taunts (HP+HK) work while holding away or towards".
        # Ryu's overhead is cix 56 and his taunt cix 64; held forward or back, the retail game
        # walks and does a normal instead.
        for d in ("6", "4"):
            for buttons, cix in (("MP+MK", 56), ("HP+HK", 64)):
                with self.subTest(lever=d, buttons=buttons):
                    v, u = both([f"5 {d}", f"1 {d}+{buttons}", f"40 {d}"] + F.wait(30), p1="ryu", p2="ken", pos=(300, 600))
                    self.assertNotIn(cix, {r["p"][0]["cix"] for r in v})
                    self.assertIn(cix, {r["p"][0]["cix"] for r in u})


if __name__ == "__main__":
    unittest.main()
