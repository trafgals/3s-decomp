"""The fight harness itself: the fight it sets up, the inputs it plays, and the
measurements, checked against the retail numbers the 3s' design document quotes."""

import unittest

from common import F, both, skip_without_game


def setUpModule():
    skip_without_game()


class Setup(unittest.TestCase):
    def test_characters_and_super_arts(self):
        for build in ("vanilla", "upper3s"):
            rows = F.run(F.wait(5), p1="makoto", p2="remy", sa1=1, sa2=2, build=build)
            p1, p2 = rows[0]["p"]
            self.assertEqual((p1["char"], p2["char"]), (16, 19))
            self.assertEqual((p1["sa"], p2["sa"]), (1, 2))

    def test_walking_follows_the_facing(self):
        rows = F.run(["10 5 5", "30 6 5", "30 4 5"], p1="ryu", p2="ken")
        start, fwd, back = (F.player(rows, f, 0)["x"] for f in (10, 40, 70))
        self.assertEqual(F.player(rows, 1, 0)["rl"], 1)  # player 1 starts on the left, facing right
        self.assertGreater(fwd, start)
        self.assertLess(back, fwd)


class RetailNumbers(unittest.TestCase):
    """The vanilla build has the retail frame data the design document quotes, so the
    measurements mean what the document means."""

    def startup(self, char, script, pos=(420, 470)):
        return F.startup(F.run(script + F.wait(90), p1=char, p2="ken", pos=pos, build="vanilla"))

    def test_startup(self):
        self.assertEqual(self.startup("chunli", ["1 2+LP"], (420, 460)), 2)  # "a 2f normal"
        self.assertEqual(self.startup("q", ["1 5+LP"], (420, 500)), 6)  # "far standing Jab from 6f"
        self.assertEqual(self.startup("hugo", F.motion("63214", "LP+MP"), (420, 480)), 22)  # EX GPB "22f"
        for k in ("LK", "MK", "HK"):  # Ryuubi Kyaku "all being 25f"
            self.assertEqual(self.startup("sean", F.motion("236", k)), 25)
        self.assertEqual(self.startup("sean", F.motion("623", "LP"), (420, 460)), 5)  # Dragon Smash "5/5/9/8"
        self.assertEqual(self.startup("sean", F.motion("623", "HP"), (420, 460)), 9)
        self.assertEqual(self.startup("sean", F.motion("623", "LP+MP"), (420, 460)), 8)
        # The document gives 5 for the medium Dragon Smash; the disc data has 7.
        self.assertEqual(self.startup("sean", F.motion("623", "MP"), (420, 460)), 7)

    def test_q_standing_roundhouse_is_minus_10_on_hit(self):
        # "Standing Roundhouse is -10 on hit, for goodness' sake."
        rows = F.run(["1 5+HK"] + F.wait(90), p1="q", p2="ken", pos=(420, 480), build="vanilla")
        self.assertEqual(F.hit_advantage(rows), -10)


class Regressions(unittest.TestCase):
    def test_an_untouched_move_is_the_same_in_both_builds(self):
        # The 3s' hitstun overrides once read the attack data index from the first frame
        # of the move, which patched Ryu's standing Jab instead of the moves of the table.
        v, u = both(["1 5+LP"] + F.wait(60), p1="ryu", p2="ken", pos=(420, 470))
        hv, hu = F.first_hit(v), F.first_hit(u)
        self.assertEqual(F.player(v, hv, 0)["hsyou"], F.player(u, hu, 0)["hsyou"])
        self.assertEqual(F.hit_advantage(v), F.hit_advantage(u))

    def test_the_3s_build_survives_the_start_of_a_fight(self):
        # PLS00.c:check_cg_cancel_data() once dereferenced wk->as before the first move.
        rows = F.run(F.wait(120), p1="alex", p2="q", build="upper3s")
        self.assertGreaterEqual(len(rows), 120)


if __name__ == "__main__":
    unittest.main()
