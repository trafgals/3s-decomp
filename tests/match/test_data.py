"""The data items: super art stocks and bar lengths, stun bars and health, read from the players
of a fight at the start of the round."""

import unittest

from common import F, skip_without_game


def setUpModule():
    skip_without_game()


def start(char, sa=0, build="upper3s"):
    return F.run(F.wait(2), p1=char, p2="ken", sa1=sa, build=build)[0]["p"][0]


# (item, character, super art slot, (bar length, stocks) retail, (bar length, stocks) 3s')
SUPER_ARTS = [
    ("CL1 Houyoku-sen: 1 stock, the longest bar", "chunli", 1, (104, 2), (128, 1)),
    ("CL4 Tensei Ranka: 2 stocks, Shippu's bar", "chunli", 2, (72, 3), (80, 2)),
    ("KN1 Shippu Jinrai Kyaku: 2 stocks", "ken", 2, (80, 3), (80, 2)),
    ("YU2 Genei Jin: Sourai Rengeki's bar", "yun", 2, (72, 1), (88, 1)),
    ("YU3 Sourai Rengeki: 2 stocks", "yun", 1, (88, 3), (88, 2)),
    ("UR1 Tyrant Slaughter: 1 stock, a slightly longer bar", "urien", 0, (104, 2), (112, 1)),
    ("AK2 Messatsu Gou Rasen: shorter than SA I and SA III", "akuma", 1, (112, 2), (96, 2)),
    ("OR5 Kishin Riki: Tengu Stone's bar", "oro", 0, (104, 1), (112, 1)),
    ("IB2 Yami Shigure: 2 stocks", "ibuki", 2, (80, 1), (80, 2)),
    ("TW11 X.F.L.A.T.: 2 stocks", "twelve", 1, (112, 1), (112, 2)),
    ("RM11 Blue Nocturne: a bit longer bar", "remy", 2, (80, 1), (96, 1)),
    ("EL6 Spinning Beat keeps 3 stocks", "elena", 0, (80, 3), (80, 3)),
]


class Data(unittest.TestCase):
    def test_super_arts(self):
        for item, char, sa, retail, target in SUPER_ARTS:
            with self.subTest(item):
                v, u = start(char, sa, "vanilla"), start(char, sa, "upper3s")
                self.assertEqual((v["glen"], v["stockmax"]), retail)
                self.assertEqual((u["glen"], u["stockmax"]), target)

    def test_akuma_and_remy_stun_bars_are_regular(self):
        # AK1, RM1: "Stun bar increased to 'regular' size instead of 'short' size."
        regular = start("ryu", build="vanilla")["stunmax"]
        for char in ("akuma", "remy"):
            with self.subTest(char):
                self.assertLess(start(char, build="vanilla")["stunmax"], regular)
                self.assertEqual(start(char, build="upper3s")["stunmax"], regular)

    def test_health(self):
        # IB1 "Health increased ... same as Twelve's health."
        # TW1 "Health increased ... the same as Sean/Remy/Necro/etc."
        twelve, necro, remy = (start(c, build="vanilla")["ovit"] for c in ("twelve", "necro", "remy"))
        self.assertEqual(necro, remy)
        self.assertLess(start("ibuki", build="vanilla")["ovit"], twelve)
        self.assertEqual(start("ibuki", build="upper3s")["ovit"], twelve)
        self.assertEqual(start("twelve", build="upper3s")["ovit"], necro)
        for char in ("ryu", "akuma", "necro"):
            with self.subTest(untouched=char):
                self.assertEqual(start(char, build="vanilla")["ovit"], start(char, build="upper3s")["ovit"])


if __name__ == "__main__":
    unittest.main()
