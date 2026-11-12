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



class TwelveSuperjump(unittest.TestCase):
    # TW6 "Allow crouching Strong to be superjump-cancelled on hit/block, like standing Forward". A
    # superjump (down, then up) entered on the frame after the normal makes contact; it is normal
    # state 17 and goes 146 high.
    @staticmethod
    def superjumps(normal, build, defend="hit"):
        alone = play("twelve", [f"1 {normal}"] + F.wait(60), {}, 0, 40, build)
        h0 = F.first_hit(alone)
        p2 = {i: "1" if normal.startswith("2+") else "4" for i in range(max(1, h0 - 6), h0 + 60)} if defend == "block" else {}
        rows = play("twelve", [f"1 {normal}"] + F.wait(60), p2, 0, 40, build)
        h = F.first_hit(rows)
        end = F.free(rows, 0, h)
        press = F.press_frame(rows, 0)
        rows = play("twelve", [f"1 {normal}", f"{h - press} 5", "1 2", "8 8"] + F.wait(60), p2, 0, 40, build)
        jump = F.first(rows, lambda r: r["p"][0]["rno"][1] == 0 and r["p"][0]["rno"][2] == 17, h)
        return jump is not None and jump < end

    def test_crouching_strong(self):
        for defend in ("hit", "block"):
            with self.subTest(defend=defend):
                for build in ("vanilla", "upper3s"):
                    self.assertTrue(self.superjumps("5+MK", build, defend), "s.MK, the retail control")
                self.assertFalse(self.superjumps("2+MP", "vanilla", defend))
                self.assertTrue(self.superjumps("2+MP", "upper3s", defend))



def damage_frames(rows):
    return [b["f"] for a, b in zip(rows, rows[1:]) if b["p"][1]["vit"] < a["p"][1]["vit"]]


class CancelWindows(unittest.TestCase):
    def special_after(self, char, normal, follow, d, after, build):
        """The frame a special move (slot 5) starts when its motion begins on frame `after` of a
        fight that starts with `normal`, or None."""
        press = F.press_frame(F.run([f"1 {normal}"] + F.wait(70), p1=char, p2="ken", pos=(400, 400 + d), build=build), 0)
        rows = F.run([f"1 {normal}", f"{after - press} 5"] + follow + F.wait(70), p1=char, p2="ken", pos=(400, 400 + d),
                     build=build)
        return F.first(rows, lambda r: r["p"][0]["koc"] == 5, after)

    def test_elena_crouching_short_chains_into_itself(self):
        # EL1 "Allow her to chain crouching Short (2LK) into itself." A second c.LK pressed in the
        # first one's cancel window hits before the first would have ended (frame 27).
        for build, chains in (("vanilla", False), ("upper3s", True)):
            for k in (1, 5, 10):
                with self.subTest(build=build, k=k):
                    rows = F.run(["1 2+LK", f"{4 + k} 2", "1 2+LK"] + F.wait(50), p1="elena", p2="ken", pos=(400, 440),
                                 build=build)
                    hits = damage_frames(rows)
                    self.assertEqual(len(hits) >= 2 and hits[1] < 27, chains)

    def test_elena_standing_strong_second_hit(self):
        # EL3 "Allow cancels from the 2nd hit of standing Strong, not just the first hit." It hits
        # on frames 10 and 15 (the damage shows a frame later); Rhino Horn (214LK) entered after the
        # first hit comes out in both builds, after the second only in 3s'.
        rhino = F.motion("214", "LK")
        for build in ("vanilla", "upper3s"):
            with self.subTest(build=build):
                self.assertEqual(damage_frames(F.run(["1 5+MP"] + F.wait(40), p1="elena", p2="ken", pos=(400, 440), build=build)),
                                 [11, 17])
                self.assertIsNotNone(self.special_after("elena", "5+MP", rhino, 40, 12, build))
                self.assertIsNone(self.special_after("elena", "5+MP", rhino, 40, 22, build))
        self.assertIsNone(self.special_after("elena", "5+MP", rhino, 40, 17, "vanilla"))
        self.assertIsNotNone(self.special_after("elena", "5+MP", rhino, 40, 17, "upper3s"))

    def test_q_back_roundhouse_from_further_away(self):
        # Q5 "Extend the cancel window on B+RH (4HK) past the first active frame, so it can be
        # cancelled from further away." From 60 apart it hits on its first active frames, from 110
        # on a later one. The follow-up is Q's 214LP, entered on the frame of the hit.
        follow = F.motion("214", "LP")
        for build in ("vanilla", "upper3s"):
            with self.subTest(build=build):
                self.assertIsNotNone(self.special_after("q", "4+HK", follow, 60, 16, build))
        self.assertIsNone(self.special_after("q", "4+HK", follow, 110, 19, "vanilla"))
        self.assertIsNotNone(self.special_after("q", "4+HK", follow, 110, 19, "upper3s"))

    def test_alex_target_combo_window(self):
        # AL4 "Increase the cancel window for his Target Combo (cr.Short->cr.Fwd) to be longer than
        # 1f, so that it can be done as easily as any other Target Combo in the game." c.LK hits on
        # frame 9 and ends on 24; MK is pressed (down held) on a frame after the hit, and the
        # target combo's c.MK (animation 30) starts before the c.LK would have ended.
        def window(build):
            ok = []
            for frame in range(8, 25):
                rows = F.run(["1 2+LK", f"{frame - 5} 2", "1 2+MK", "50 2"], p1="alex", p2="ken", pos=(400, 440), build=build)
                tc = F.first(rows, lambda r: r["p"][0]["rno"][1] == 4 and r["p"][0]["cix"] == 30, 9)
                self.assertFalse(any(r["p"][0]["rno"][1] == 4 and r["p"][0]["cix"] == 0 for r in rows), "a wrong animation")
                if tc is not None and tc < 24:
                    ok.append(frame)
            return ok

        self.assertEqual(window("vanilla"), [18, 19])
        self.assertEqual(window("upper3s"), list(range(10, 20)))  # Ken's s.MP->s.HP: 11 frames



class RemyChains(unittest.TestCase):
    # RM6 "Allow chaining from crouching Jab->crouching Short and crouching Short->crouching Jab."
    # The second normal is pressed on a frame after the first hits, down held; the chain worked when
    # it starts (its animation: c.LP 18, c.LK 27, c.MK 30) before the first would have ended.
    @staticmethod
    def chains(first, second, cix, build):
        alone = F.run([f"1 2+{first}", "40 2"], p1="remy", p2="ken", pos=(400, 440), build=build)
        h = F.first_hit(alone)
        end = F.free(alone, 0, h)
        for k in range(0, 8):
            rows = F.run([f"1 2+{first}", f"{h - 4 + k} 2", f"1 2+{second}", "40 2"], p1="remy", p2="ken", pos=(400, 440),
                         build=build)
            started = F.first(rows, lambda r: r["p"][0]["rno"][1] == 4 and r["p"][0]["cix"] == cix, h)
            if started is not None and started < end:
                return True
        return False

    def test_crouching_lights(self):
        for first, second, cix in (("LP", "LK", 27), ("LK", "LP", 18)):
            with self.subTest(first=first, second=second):
                self.assertFalse(self.chains(first, second, cix, "vanilla"))
                self.assertTrue(self.chains(first, second, cix, "upper3s"))

    def test_nothing_else(self):
        self.assertFalse(self.chains("LP", "MK", 30, "upper3s"))


class SeanTackle(unittest.TestCase):
    # SN6 "Allow Fierce Tackle to be faked/cancelled by tapping K during startup." The Tackle is
    # back, down, forward + P (cmd_data.c: pC_cmd_28). Fierce is the dash animation 26 (koc 5) for
    # 24 frames; when the punch is still held at its end it grabs (measured), otherwise it ends in
    # animation 24. Its startup is the dash. The kick is pressed k frames into the dash.
    @staticmethod
    def tackle(button, kick_at, build, hold=False):
        press = f"5+{button}" if hold else "5"
        rows = F.run(["1 4", "1 2", f"1 6+{button}", f"{kick_at} {press}", f"1 {press}+LK", f"40 {press}"] + F.wait(40),
                     p1="sean", p2="ken", pos=(300, 360), build=build)
        start = F.first(rows, lambda r: r["p"][0]["koc"] == 5)
        free = F.free(rows, 0, start) or len(rows)  # a grab lasts past the end of the log
        kicked = any(r["p"][0]["koc"] == 4 for r in rows[start:free + 5])
        grabbed = any(r["p"][1]["rno"][1] == 3 for r in rows)
        return free - start, kicked, grabbed

    def test_fierce_tackle_feint(self):
        full = self.tackle("HP", 40, "vanilla")[0]
        self.assertGreater(full, 25)
        for k in (3, 8, 15, 20):
            for hold in (False, True):
                with self.subTest(k=k, hold=hold):
                    length, kicked, grabbed = self.tackle("HP", k, "vanilla", hold)
                    self.assertGreater(length, 20, "the retail Tackle cannot be faked")
                    length, kicked, grabbed = self.tackle("HP", k, "upper3s", hold)
                    self.assertLessEqual(length, k + 2, "the Tackle ends when the kick is pressed")
                    self.assertFalse(kicked, "the kick that fakes it does not come out")
                    self.assertFalse(grabbed)

    def test_not_after_it_grabs(self):
        # The punch held through the dash: it grabs Ken, and a kick then changes nothing.
        for build in ("vanilla", "upper3s"):
            with self.subTest(build=build):
                self.assertTrue(self.tackle("HP", 30, build, hold=True)[2])

    def test_only_the_fierce_tackle(self):
        self.assertEqual(self.tackle("MP", 5, "upper3s")[:2], self.tackle("MP", 5, "vanilla")[:2])


class RyuJoudan(unittest.TestCase):
    # RY4 "Expand hitbox on LK Joudan Sokutou Geri (41236LK) downward so that it will not miss on
    # some characters after close Fierce (HP) or crouching Fierce (2HP)." Measured: after close s.HP
    # on a crouching opponent it misses 15 of the 20 characters; after c.HP it does not miss.
    @staticmethod
    def connects(normal, victim, build, kick="LK"):
        v = "2"  # the victim crouches
        alone = F.run([f"1 {normal} {v}", f"60 5 {v}"], p1="ryu", p2=victim, pos=(400, 440), build=build)
        h = F.first_hit(alone)
        press = F.press_frame(alone, 0)
        rows = F.run([f"1 {normal} {v}", f"{h - press} 5 {v}"] + [" ".join(l.split()[:2] + [v]) for l in F.motion("41236", kick)]
                     + [f"60 5 {v}"], p1="ryu", p2=victim, pos=(400, 440), build=build)
        return len([b for a, b in zip(rows, rows[1:]) if b["p"][1]["vit"] < a["p"][1]["vit"]]) >= 2

    def test_after_close_fierce(self):
        missed = [c for c in F.CHARS if not self.connects("5+HP", c, "vanilla")]
        self.assertGreaterEqual(len(missed), 10)
        self.assertEqual([c for c in F.CHARS if not self.connects("5+HP", c, "upper3s")], [])

    def test_only_the_lk_boxes_change(self):
        # MK and HK share the LK one's boxes in the data; only the LK animation is changed. "aby" is
        # the lowest bottom edge of the attack boxes the hit check is given on a frame.
        def bottoms(kick, build):
            rows = F.run(F.motion("41236", kick) + F.wait(40), p1="ryu", p2="ken", pos=(300, 600), build=build)
            return [r["p"][0]["aby"] for r in rows if r["p"][0]["aby"] >= 0]

        self.assertEqual(bottoms("LK", "vanilla"), [50, 72, 72])
        self.assertEqual(bottoms("LK", "upper3s"), [30, 52, 52])
        for kick in ("MK", "HK"):
            with self.subTest(kick=kick):
                self.assertEqual(bottoms(kick, "upper3s"), bottoms(kick, "vanilla"))


class QExSlaps(unittest.TestCase):
    # Q8 "Extend hitbox of first hit of EX High Speed Barrage downward so that it more consistently
    # connects against crouching characters." High Speed Barrage is 214+P; the EX one is special
    # animation 39, and its first slap uses attack box set 73 (upper3s/upper3s_boxes.c).
    @staticmethod
    def slaps(victim, build, hold, lead=()):
        """Q's EX slaps (after the script lines `lead`) on `victim` holding `hold`; the rows."""
        lines = list(lead) + [" ".join(l.split()[:2] + [hold]) for l in F.motion("214", "LP+MP")] + [f"90 5 {hold}"]
        return F.run(lines, p1="q", p2=victim, pos=(400, 440), build=build)

    @staticmethod
    def first_slap_connects(rows):
        """A hit (the health drops) or a block (the damage state starts) on a frame after one with the
        first slap's box set."""
        first = {r["f"] for r in rows if r["p"][0]["koc"] == 5 and r["p"][0]["bx"][1] == 73}
        return any(b["f"] - 1 in first and (b["p"][1]["vit"] < a["p"][1]["vit"] or
                                            (a["p"][1]["rno"][1] != F.RNO_DAMAGE and b["p"][1]["rno"][1] == F.RNO_DAMAGE))
                   for a, b in zip(rows, rows[1:]))

    def test_first_slap_on_crouchers(self):
        for hold in ("2", "1"):  # crouching, crouch-blocking
            with self.subTest(hold=hold):
                missed = [c for c in F.CHARS if not self.first_slap_connects(self.slaps(c, "vanilla", hold))]
                self.assertEqual(set(missed), {"elena", "oro", "chunli"})
                self.assertEqual([c for c in F.CHARS if not self.first_slap_connects(self.slaps(c, "upper3s", hold))], [])

    def test_first_slap_after_crouching_short(self):
        # c.LK xx EX slaps (Q1 makes c.LK special-cancellable): Makoto, in crouching hitstun, is lower.
        def after_short(victim):
            alone = F.run(["1 2+LK 2", "60 5 2"], p1="q", p2=victim, pos=(400, 440))
            lead = ["1 2+LK 2", f"{F.first_hit(alone) - F.press_frame(alone, 0)} 5 2"]
            return self.first_slap_connects(self.slaps(victim, "upper3s", "2", lead))

        self.assertEqual([c for c in F.CHARS if not after_short(c)], [])

    def test_only_the_ex_first_slap_changes(self):
        def bottoms(button, build):
            rows = F.run(F.motion("214", button) + F.wait(90), p1="q", p2="ken", pos=(300, 600), build=build)
            return [(r["p"][0]["bx"][1], r["p"][0]["aby"]) for r in rows if r["p"][0]["koc"] == 5 and r["p"][0]["aby"] >= 0]

        for button in ("LP", "MP", "HP"):
            with self.subTest(button=button):
                self.assertEqual(bottoms(button, "upper3s"), bottoms(button, "vanilla"))
        ex_vanilla, ex = bottoms("LP+MP", "vanilla"), bottoms("LP+MP", "upper3s")
        self.assertEqual({a for s, a in ex_vanilla if s == 73}, {59})
        self.assertEqual({a for s, a in ex if s == 73}, {55})
        self.assertEqual([x for x in ex if x[0] != 73], [x for x in ex_vanilla if x[0] != 73])


class TwelveBackForward(unittest.TestCase):
    # TW4 "Activate the earlier hitboxes in Back+Forward (4MK) that exist but cannot connect." Frame 2
    # of the chain carries low attack boxes (set 16) but no attack data, so no attack is armed on it
    # ("aho" is att_hit_ok); upper3s/pl17.c makes it start the attack. The opponent crouches or stands
    # from 20 frames before, so that a down press is not a parry tap.
    @staticmethod
    def back_forward(build, hold="5", d=40):
        return F.run([f"20 5 {hold}", f"1 4+MK {hold}", f"60 5 {hold}"], p1="twelve", p2="ken", pos=(400, 400 + d), build=build)

    def test_the_low_boxes_are_armed(self):
        for build, armed in (("vanilla", {0}), ("upper3s", {1})):
            with self.subTest(build=build):
                rows = self.back_forward(build, d=250)  # a whiff
                self.assertEqual({r["p"][0]["aho"] for r in rows if r["p"][0]["koc"] == 4 and r["p"][0]["bx"][1] == 16}, armed)

    def test_it_hits_once_two_frames_earlier(self):
        for hold in ("5", "2"):
            for d in (40, 90):
                with self.subTest(hold=hold, d=d):
                    hits = {}
                    for build in ("vanilla", "upper3s"):
                        rows = self.back_forward(build, hold, d)
                        hits[build] = [b["f"] for a, b in zip(rows, rows[1:]) if b["p"][1]["vit"] < a["p"][1]["vit"]]
                    self.assertEqual(len(hits["vanilla"]), 1)
                    self.assertEqual(len(hits["upper3s"]), 1)
                    self.assertEqual(hits["upper3s"][0], hits["vanilla"][0] - 2)

    def test_the_rest_of_the_move_is_unchanged(self):
        # On hit it goes on as a hit on frame 3 does (its on-hit jump to block 4), so the advantage is
        # the one set for the hitstun item (0, test_hitstun.py); the whiff is unchanged.
        hit = self.back_forward("upper3s")
        self.assertEqual(F.hit_advantage(hit, start=20), 0)
        whiffs = [self.back_forward(build, d=250) for build in ("vanilla", "upper3s")]
        self.assertEqual([F.free(rows, 0, 25) for rows in whiffs][0], [F.free(rows, 0, 25) for rows in whiffs][1])
        self.assertEqual([[r["p"][0]["hix"] for r in rows if r["p"][0]["koc"] == 4] for rows in whiffs][0],
                         [[r["p"][0]["hix"] for r in rows if r["p"][0]["koc"] == 4] for rows in whiffs][1])


class RemyTargetCombo(unittest.TestCase):
    # RM5 "Decrease grounded opponent knockback on standing Forward (MK), enlarge hitboxes on 2nd hit of
    # Target Combo (standing Forward->RH) so that it connects on everyone standing and most people
    # crouching." The opponent stands or crouches from 20 frames before (no parry tap).
    @staticmethod
    def target_combo(victim, build, hold="5", d=30):
        """The hits of s.MK -> s.HK on `victim`, the s.HK pressed on the frame the s.MK hits."""
        lead = [f"20 5 {hold}"]
        alone = F.run(lead + [f"1 5+MK {hold}", f"40 5 {hold}"], p1="remy", p2=victim, pos=(400, 400 + d), build=build)
        h = F.first_hit(alone, start=20)
        press = F.press_frame(alone, 0, start=20)
        rows = F.run(lead + [f"1 5+MK {hold}", f"{h - press} 5 {hold}", f"1 5+HK {hold}", f"60 5 {hold}"], p1="remy", p2=victim,
                     pos=(400, 400 + d), build=build)
        return len([b for a, b in zip(rows, rows[1:]) if b["p"][1]["vit"] < a["p"][1]["vit"]])

    def misses(self, build, hold, d):
        return {c for c in F.CHARS if self.target_combo(c, build, hold, d) < 2}

    def test_second_hit(self):
        for d in (20, 50):
            with self.subTest(d=d):
                self.assertEqual(self.misses("vanilla", "5", d), {"yun", "oro", "yang"})
                self.assertEqual(self.misses("upper3s", "5", d), set())
                self.assertGreaterEqual(len(self.misses("vanilla", "2", d)), 19)
                self.assertEqual(self.misses("upper3s", "2", d), {"elena", "oro", "chunli"})

    def test_less_knockback_on_the_close_standing_forward(self):
        def after(button, build, d):
            rows = F.run(["20 5 5", f"1 5+{button}", "60 5"], p1="remy", p2="ken", pos=(400, 400 + d), build=build)
            h = F.first_hit(rows, start=20)
            return rows[h + 30]["p"][1]["x"] - rows[h + 30]["p"][0]["x"], F.hit_advantage(rows, start=20)

        (gap_retail, adv_retail), (gap, adv) = after("MK", "vanilla", 30), after("MK", "upper3s", 30)
        self.assertLess(gap, gap_retail)
        self.assertEqual(adv, adv_retail)
        # The far s.MK keeps its knockback (its advantage differs: RM4 raised its hitstun).
        self.assertEqual(after("MK", "upper3s", 120)[0], after("MK", "vanilla", 120)[0])

    def test_the_standing_roundhouse_alone_is_unchanged(self):
        def bottoms(build):
            rows = F.run(["20 5 5", "1 5+HK", "60 5"], p1="remy", p2="ken", pos=(300, 600), build=build)
            return [(r["p"][0]["bx"][1], r["p"][0]["aby"]) for r in rows if r["p"][0]["aby"] >= 0]

        self.assertEqual(bottoms("upper3s"), bottoms("vanilla"))


class ChunLiTenseiRanka(unittest.TestCase):
    # CL4 (the startup hitbox) "Tensei Ranka (SAIII) startup hitbox reaches down to the floor so it can't
    # be crouched". Tensei Ranka is 236236+K, special animation 52; its startup hit is block 4 of the
    # animation ("blk"), which shares box set 94 with the later hits.
    @staticmethod
    def tensei_ranka(victim, build, hold, d=50):
        script = ["@stocks 1 3", f"20 5 {hold}"] + [" ".join(l.split()[:2] + [hold]) for l in F.motion("236236", "LK")] + [f"120 5 {hold}"]
        return F.run(script, p1="chunli", p2=victim, sa1=2, pos=(400, 400 + d), build=build)

    @staticmethod
    def startup_hit(rows):
        """Whether a hit lands on a frame after one of block 4 (the startup hit)."""
        startup = {r["f"] for r in rows if r["p"][0]["koc"] == 5 and r["p"][0]["blk"] == 4 and r["p"][0]["bx"][1] == 94}
        return any(b["f"] - 1 in startup and b["p"][1]["vit"] < a["p"][1]["vit"] for a, b in zip(rows, rows[1:]))

    def test_the_startup_hit_cannot_be_crouched(self):
        crouched = [c for c in F.CHARS if not self.startup_hit(self.tensei_ranka(c, "vanilla", "2"))]
        self.assertGreaterEqual(len(crouched), 19)
        for hold in ("2", "5"):
            with self.subTest(hold=hold):
                self.assertEqual([c for c in F.CHARS if not self.startup_hit(self.tensei_ranka(c, "upper3s", hold))], [])

    def test_only_the_startup_box_changes(self):
        def boxes(build):
            rows = self.tensei_ranka("ken", build, "5", d=600)  # a whiff
            return [(r["p"][0]["blk"], r["p"][0]["aby"]) for r in rows if r["p"][0]["koc"] == 5 and r["p"][0]["bx"][1] != 0]

        retail, upper = boxes("vanilla"), boxes("upper3s")
        self.assertEqual({a for b, a in retail if b == 4}, {26})
        self.assertEqual({a for b, a in upper if b == 4}, {-76})
        self.assertEqual([x for x in upper if x[0] != 4], [x for x in retail if x[0] != 4])


class ChunLiCrouchingForward(unittest.TestCase):
    # CL2 "Cancel window on cr.Fwd (c.MK) fixed to not allow super-late cancels." The c.MK's frames
    # carry two cancel windows ("can" bits 0x60): one on the hit and a second one later; the second goes.
    @staticmethod
    def windows(build, defend="hit"):
        """The runs of frames (from the press) whose cancel bits allow a special or super."""
        hold = "1" if defend == "block" else "5"
        rows = F.run(["20 5 5", f"1 2+MK {hold}", f"60 5 {hold}"], p1="chunli", p2="ken", pos=(400, 440), build=build)
        press = F.press_frame(rows, 0, start=20)
        frames = [r["f"] - press for r in rows[press:press + 60] if r["p"][0]["koc"] == 4 and r["p"][0]["can"] & 0x60]
        runs = []
        for f in frames:
            if runs and f == runs[-1][1] + 1:
                runs[-1][1] = f
            else:
                runs.append([f, f])
        return [tuple(r) for r in runs]

    @staticmethod
    def super_at(build, k):
        """Houyoku-sen (SA II, 236236+K) with its kick k frames after the c.MK's press: does it start?"""
        motion = F.motion("236236", "LK")
        lead = k - len(motion)  # the press is frame 0; each line of the motion is one frame, the kick on the last
        rows = F.run(["@stocks 1 3", "20 5 5", "1 2+MK", f"{lead} 5"] + motion + F.wait(60), p1="chunli", p2="ken", sa1=1,
                     pos=(400, 440), build=build)
        return F.first(rows, lambda r: r["p"][0]["koc"] == 5, 20) is not None

    def test_one_window(self):
        for defend in ("hit", "block"):
            with self.subTest(defend=defend):
                retail, upper = self.windows("vanilla", defend), self.windows("upper3s", defend)
                self.assertEqual(len(retail), 2)
                self.assertEqual(upper, retail[:1])

    def test_the_late_super_is_gone(self):
        late = self.windows("vanilla")[1][0]
        self.assertTrue(self.super_at("vanilla", late))
        self.assertFalse(self.super_at("upper3s", late))
        early = self.windows("vanilla")[0][0] + 2
        self.assertTrue(self.super_at("upper3s", early))


class IbukiTsuijiGoe(unittest.TestCase):
    # IB3 "Allow Tsuiji Goe (623P, command jump) to be special- and super-cancelled much earlier in the
    # animation, almost directly after takeoff; make Kunai cancelled from it face the correct direction
    # if it hasn't passed over the opponent yet." Tsuiji Goe is special animations 107-110; the air kunai
    # is 48. Ibuki turns round as she leaves the ground, so the kunai's motion is 214 on screen until she
    # passes over the opponent (236 after).
    @staticmethod
    def kunai(build, k, d=300, dirs="214"):
        """Tsuiji Goe (LP), then a kunai with its punch k frames after the press: (frame, facing the opponent)."""
        m = F.motion(dirs, "LP")
        rows = F.run(["20 5 5"] + F.motion("623", "LP") + [f"{k - len(m)} 5"] + m + F.wait(70), p1="ibuki", p2="ken",
                     pos=(300, 300 + d), build=build)
        press = F.press_frame(rows, 0, start=20)
        f = F.first(rows, lambda r: r["p"][0]["koc"] == 5 and r["p"][0]["cix"] == 48, press)
        if f is None:
            return None
        a, b = rows[f + 2]["p"]
        return f - press, (a["rl"] == 1) == (b["x"] > a["x"])

    def test_the_window_opens_after_takeoff(self):
        for button, retail in (("LP", 28), ("MP", 29), ("HP", 33)):
            with self.subTest(button=button):
                out = {}
                for build in ("vanilla", "upper3s"):
                    rows = F.run(["20 5 5"] + F.motion("623", button) + F.wait(70), p1="ibuki", p2="ken", pos=(300, 700), build=build)
                    press = F.press_frame(rows, 0, start=20)
                    air = F.first(rows, lambda r: r["p"][0]["y"] > 0, press)
                    can = F.first(rows, lambda r: r["p"][0]["koc"] == 5 and r["p"][0]["can"] & 0x60, press)
                    out[build] = (air - press, can - press)
                self.assertEqual(out["vanilla"][1], retail)
                self.assertEqual(out["upper3s"][1], out["upper3s"][0] + 2)  # the second frame in the air

    def test_an_early_kunai(self):
        self.assertIsNone(self.kunai("vanilla", 20))
        self.assertEqual(self.kunai("upper3s", 20), (20, True))
        self.assertEqual(self.kunai("upper3s", 8), (9, True))

    def test_the_kunai_faces_the_opponent(self):
        self.assertEqual(self.kunai("vanilla", 30), (30, False))  # retail: thrown away before passing over
        self.assertEqual(self.kunai("upper3s", 30), (30, True))
        for build in ("vanilla", "upper3s"):  # after passing over: unchanged
            with self.subTest(build=build):
                self.assertEqual(self.kunai(build, 30, d=80, dirs="236"), (30, True))


class SeanRoll(unittest.TestCase):
    # SN2 "Allow Sean Roll (214P) to be super-cancelled during the middle." Sean Roll is special animations
    # 48-50; the middle of the roll gets the super cancel bit (upper3s/pl15.c) and arms it itself
    # (upper3s_ch_sean.c). The super is SA I, Hadou Burst (236236+P).
    @staticmethod
    def roll_super(build, button, k):
        """Sean Roll, then Hadou Burst with its punch k frames after the roll's press: the frame it starts."""
        m = F.motion("236236", "LP")
        rows = F.run(["@stocks 1 3", "20 5 5"] + F.motion("214", button) + [f"{k - len(m)} 5"] + m + F.wait(70), p1="sean",
                     p2="ken", sa1=0, pos=(300, 700), build=build)
        press = F.press_frame(rows, 0, start=20)
        roll = rows[press + 1]["p"][0]["cix"]
        f = F.first(rows, lambda r: r["p"][0]["koc"] == 5 and r["p"][0]["cix"] != roll, press + 1)
        return None if f is None else f - press

    def test_in_the_middle(self):
        for button, k in (("LP", 10), ("MP", 15), ("HP", 20)):
            with self.subTest(button=button):
                self.assertIsNone(self.roll_super("vanilla", button, k))
                self.assertEqual(self.roll_super("upper3s", button, k), k)

    def test_not_after_the_roll(self):
        for button, k in (("LP", 18), ("MP", 28)):
            with self.subTest(button=button):
                self.assertIsNone(self.roll_super("upper3s", button, k))


class YunUpkicks(unittest.TestCase):
    # YU4 "Upkicks (623K) hitbox adjusted to reach further downward, so it misses standing/crouching
    # opponents less often." The upkicks are special animations 32-35 (box sets 107 and 108); the amount
    # (40 lower) is the user's choice from the measurements in issue #20. The opponent stands or crouches
    # from 20 frames before.
    @staticmethod
    def upkicks(victim, build, hold, normal=None, d=40):
        """Hits of the LK upkicks on `victim`, alone or cancelled from `normal` on the frame it hits."""
        lead = [f"20 5 {hold}"]
        kick = [" ".join(l.split()[:2] + [hold]) for l in F.motion("623", "LK")]
        if normal is None:
            rows = F.run(lead + kick + [f"60 5 {hold}"], p1="yun", p2=victim, pos=(400, 400 + d), build=build)
            start = 20
        else:
            alone = F.run(lead + [f"1 {normal} {hold}", f"60 5 {hold}"], p1="yun", p2=victim, pos=(400, 400 + d), build=build)
            h = F.first_hit(alone, start=20)
            press = F.press_frame(alone, 0, start=20)
            rows = F.run(lead + [f"1 {normal} {hold}", f"{h - press} 5 {hold}"] + kick + [f"60 5 {hold}"], p1="yun",
                         p2=victim, pos=(400, 400 + d), build=build)
            start = h + 1
        s = F.first(rows, lambda r: r["p"][0]["koc"] == 5, start)
        return len([b for a, b in zip(rows, rows[1:]) if b["f"] >= s and b["p"][1]["vit"] < a["p"][1]["vit"]])

    def misses(self, build, hold, normal=None):
        return {c for c in F.CHARS if self.upkicks(c, build, hold, normal) == 0}

    def test_crouching_point_blank(self):
        self.assertGreaterEqual(len(self.misses("vanilla", "2")), 15)
        self.assertEqual(self.misses("upper3s", "2"), set())

    def test_standing_after_a_cancel(self):
        self.assertGreaterEqual(len(self.misses("vanilla", "5", "5+MP")), 10)
        for normal in ("5+LP", "2+LP", "2+MP", "5+MP", "5+HP"):
            with self.subTest(normal=normal):
                self.assertEqual(self.misses("upper3s", "5", normal), set())

    def test_crouching_after_a_cancel(self):
        for normal, left in (("5+MP", set()), ("2+MK", set()), ("5+HP", {"chunli"})):
            with self.subTest(normal=normal):
                self.assertGreaterEqual(len(self.misses("vanilla", "2", normal)), 18)
                self.assertEqual(self.misses("upper3s", "2", normal), left)

    def test_only_the_upkicks_change(self):
        def bottoms(lines, build):
            rows = F.run(["20 5 5"] + lines + F.wait(60), p1="yun", p2="ken", pos=(300, 700), build=build)
            return [(r["p"][0]["bx"][1], r["p"][0]["aby"]) for r in rows if r["p"][0]["bx"][1] != 0]

        retail, upper = bottoms(F.motion("623", "LK"), "vanilla"), bottoms(F.motion("623", "LK"), "upper3s")
        self.assertEqual({a for s, a in retail}, {66, 69})
        self.assertEqual({a for s, a in upper}, {26, 29})
        for lines in (["1 5+HK"], ["1 2+HK"], F.motion("236", "LP")):
            with self.subTest(lines=lines):
                self.assertEqual(bottoms(lines, "upper3s"), bottoms(lines, "vanilla"))


class IbukiTargetComboFierce(unittest.TestCase):
    # IB4 "Fix the unique-animation standing Fierce in her Target Combos (B+Strong->Fierce and
    # Jab->Strong(1)->Fierce) to never knock down ... "always reset" is preferable". Both target combos end
    # in normal animation 165; its Fierce on an airborne opponent knocked down or reset depending on the
    # direction they flew (upper3s_ch_ibuki.h). The opponent jumps straight up; Ibuki's 4MP catches them
    # in the air w frames after the jump, and the Fierce follows on the frame it hits.
    @staticmethod
    def outcome(victim, build, w, d=50):
        lead = ["20 5 5", "1 5 8"]
        alone = F.run(lead + [f"{w} 5 5", "1 4+MP 5", "80 5 5"], p1="ibuki", p2=victim, pos=(400, 400 + d), build=build)
        h = F.first_hit(alone, start=21)
        if h is None or alone[h]["p"][1]["y"] <= 0:
            return None
        press = F.press_frame(alone, 0, start=21)
        rows = F.run(lead + [f"{w} 5 5", "1 4+MP 5", f"{h - press} 5 5", "1 5+HP 5", "90 5 5"], p1="ibuki", p2=victim,
                     pos=(400, 400 + d), build=build)
        hits = [b["f"] for a, b in zip(rows, rows[1:]) if b["p"][1]["vit"] < a["p"][1]["vit"]]
        if len(hits) < 2:
            return None
        return "down" if any(r["p"][1]["pat"] == 38 for r in rows[hits[1]:hits[1] + 90]) else "reset"

    def outcomes(self, build):
        out = {}
        for c in F.CHARS:
            got = {o for o in (self.outcome(c, build, w) for w in (0, 1, 26, 28)) if o}
            if got:
                out[c] = got
        return out

    def test_always_reset(self):
        retail, upper = self.outcomes("vanilla"), self.outcomes("upper3s")
        self.assertEqual(retail["ken"], {"down"})
        self.assertEqual(retail["yun"], {"reset"})
        self.assertGreaterEqual(len(upper), 19)
        self.assertEqual({c for c, o in upper.items() if o != {"reset"}}, set())

    def test_on_the_ground_it_is_unchanged(self):
        def tc(build):
            alone = F.run(["20 5 5", "1 4+MP", "60 5"], p1="ibuki", p2="ken", pos=(400, 440), build=build)
            h = F.first_hit(alone, start=20)
            press = F.press_frame(alone, 0, start=20)
            rows = F.run(["20 5 5", "1 4+MP", f"{h - press} 5", "1 5+HP", "60 5"], p1="ibuki", p2="ken", pos=(400, 440), build=build)
            hits = [b["f"] for a, b in zip(rows, rows[1:]) if b["p"][1]["vit"] < a["p"][1]["vit"]]
            return hits, F.hit_advantage(rows, start=hits[1] - 1), [r["p"][1]["rno"][2] for r in rows[hits[1]:hits[1] + 20]]

        self.assertEqual(tc("upper3s"), tc("vanilla"))


class QRushes(unittest.TestCase):
    # Q3 "Allow all overhead/low rushpunches ([4]6P/K/[P]) to be super-cancelled on hit/block/parry.
    # Make the riskier versions at least AS useful as the safe version." Q charges back for 50
    # frames, rushes with 6 + the button (held for the overhead version, `hold`), and enters a super
    # art the frame after the rush makes contact. The mid rush ([4]6+P) is the safe version: it is
    # super-cancellable in the retail game.
    @staticmethod
    def super_cancels(button, build, hold=False, defend="hit"):
        lead = ["@stocks 1 3", "50 4"]
        tail = [f"1 6+{button}"] + ([f"30 5+{button}"] if hold else []) + F.wait(90)
        alone = play("q", lead + tail, {}, 0, 50, build)
        h0 = contact(alone, "hit", 52)
        assert h0 is not None, f"Q's {button} rush does not hit"
        p2 = {i: "1" if button.endswith("K") else "4" for i in range(h0 - 6, h0 + 60)} if defend == "block" else {}
        rows = play("q", lead + tail, p2, 0, 50, build)
        h = contact(rows, defend, 52)
        assert h is not None, f"Q's {button} rush: no {defend}"
        end = F.free(rows, 0, h)
        press = 52 + 3
        held = f"{h - press} 5+{button}" if hold else f"{h - press} 5"
        rows = play("q", lead + [f"1 6+{button}", held] + F.motion("23623", "LP") + F.wait(90), p2, 0, 50, build)
        started = F.first(rows, lambda r: r["p"][0]["koc"] == 5 and r["p"][0]["cix"] >= 40, h)
        return started is not None and started < end

    def test_the_mid_rush_already_cancels(self):
        for build in ("vanilla", "upper3s"):
            with self.subTest(build=build):
                self.assertTrue(self.super_cancels("MP", build))

    def test_low_and_overhead_rushes(self):
        for name, button, hold in (("low MK", "MK", False), ("low HK", "HK", False), ("overhead MP", "MP", True),
                                   ("overhead HP", "HP", True)):
            for defend in ("hit", "block"):
                with self.subTest(name, defend=defend):
                    self.assertFalse(self.super_cancels(button, "vanilla", hold, defend))
                    self.assertTrue(self.super_cancels(button, "upper3s", hold, defend))


if __name__ == "__main__":
    unittest.main()
