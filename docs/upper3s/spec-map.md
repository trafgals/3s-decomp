# Third Strike Upper (3s') - Spec to code map

This maps every item of the [design specification](design-spec.md) to the code
that implements it, the value the retail game uses, and the value 3s' should
use.

Status values:

* `todo` - not started
* `wip` - in progress
* `done` - implemented and verified
* `drop` - deliberately not implemented (with the reason)
* `needs-disc` - the value is only in the disc image; the tool that reads it
  out of the disc is in place (see "The values that are only in the disc"
  below), the change itself is not made yet.

## The values that are only in the disc

The frame data of the moves - the startup, the active and the recovery frames,
the hitboxes, the pushboxes, the cancel windows - is read from the disc image
at runtime. The retail values therefore have to be read out of the disc
before they can be changed:

```
make UPPER_3S=1 UPPER3S_DUMP=1 BUILD_DIR=/tmp/sf33rd-dump PLATFORM=linux
```

The game then writes `upper3s_dump.txt`, one line per move of every
character. The dump follows the loader of the game, so it needs the disc
image in `rom/THIRD` and a few runs of the game: the memory of the game
cannot hold the data of all twenty characters at once, and the dump picks up
where the last run stopped.

The layout of the file and the walk of the frame chain are described in
[`implementation-guide.md`](implementation-guide.md).

## How to read the numbers

The design document was written from arcade play, and some of its literal
numbers do not agree with the decompiled retail data. Where they disagree the
**intent** wins and the concrete value is re-derived from the retail tables;
the reasoning is recorded in the "Note" column.

The most important example is the super art stock count. The retail data
stores the number of gauge stocks a super costs in
`PLCNT.c:super_arts_data[20][4]` (`SA_DATA.store_max`), and the gauge length
in the same struct (`SA_DATA.gauge_len`, 0x40-0x80). The designer talks about
"the same length as Shippu" and "3 stocks instead of 2", which does not map
one-to-one onto those fields, so every stock/length change has to be derived
from the table rather than transcribed.

## How the tables are keyed

Two keys have to be right, and both of them are easy to get wrong.

**The character index** is the index of the game (Gill 0, Alex 1, Ryu 2, ...
Remy 19) - not the order in which the characters were introduced and not the
charset id. `PLCNT.c:plid_data` permutes the index and the charset id, and
every per character table of the game is indexed by the index of the game.
The order and its evidence are in `AGENTS.md` section 3.1.

**The move index** is the `char_ix` of the disc's assignment table
(`bin2obj/asstbl.c`), not the index of the move in the move list. For the
normal attacks it is

```
char_ix = base + kos * 3 + koa
```

with `kos` the button (0 LP, 1 MP, 2 HP, 3 LK, 4 MK, 5 HK) and `koa` the
variant - for the standing normals (base 0) 0 without a dash (5X), 1 with a
dash forward (6X) and 2 with a dash back (4X); for the crouching normals
(base 18) 0 without a dash (2X), 1 with a dash forward (3X) and 2 with a
dash back (1X). The super arts use the `char_ix` of their entry in
`bin2obj/asstbl.c:_assadr_lv_9900`, the four strengths of a super being
four consecutive entries.

The **slot** of a move is the one the engine uses: the normal attacks are in
slot 4, the special moves and the super arts in slot 5 (they share it and
are told apart by their index), the reaction moves in slot 0.

## System mechanics

| # | Spec item | Code | Retail | 3s' | Status |
|:--|:---|:---|:---|:---|:---|
| SM1 | Whiffed normals scale metergain by half each time (100/50/25/12.5%), whiffed specials unchanged; resets on hit/block/parry contact, on being hit/blocking/parrying, and on projectile hits (parried projectiles reset only for the thrower) | `src/anniversary/sf33rd/Source/Game/PLS02.c` - `add_sp_arts_gauge_guard()` (whiffed normal, table `_add_arts_gauge[player][add_arts_point][1]`), `add_sp_arts_gauge_tokushu()` (whiffed special), `add_sp_arts_gauge_hit_dm()`, `add_sp_arts_gauge_paring()`; call sites in `HITCHECK.c` | every whiffed normal grants the full whiff value | successive whiffed normals scale by 1/2 per whiff, uncapped at 12.5% | done |
| SM2 | +2f parry cooldown after attempting a parry; being hit during that cooldown by an attack parried the same way deals 25% extra damage | parry handling in `src/anniversary/sf33rd/Source/Game/upper3s/upper3s_throw.c` (`upper3s_parry_cooling()`, `upper3s_parry_damage_permille()`), hooked in `HITCHECK.c` (`defense_ground`, `defense_sky`, `plef_at_vs_player_damage_union`) | the parry window is 2 frames (3 for a special move) and there is no cooldown; a missed parry has no penalty | a parry attempt starts a 2f cooldown for that kind of parry, and a hit that lands during the cooldown and could have been parried the same way does 25% more damage | done |
| SM3 | "Guess parries" (parry during wakeup, recovery roll, air land-on-feet hitstun) are active for half the normal window, keep the same cooldown, and cannot tech throws | guess parries in `src/anniversary/sf33rd/Source/Game/upper3s/upper3s_throw.c` (`upper3s_parry_is_guess_state()`, `upper3s_parry_threshold()`, `upper3s_parry_guess_locked()`), hooked in `HITCHECK.c` (`defense_ground`, `defense_sky`, `set_caught_status`) | guess parries have the full window and a throw can still be teched | the window is halved (2 frames become 1), the cooldown is the same, and a throw that lands in the window or in the cooldown cannot be teched | done |
| SM4 | Charge partition timer resets to maximum as soon as a charge reaches 1/3 | `src/anniversary/sf33rd/Source/Game/upper3s/upper3s_sm4.c` (`upper3s_sm4_charge_partition()`), hooked in `src/anniversary/sf33rd/Source/Game/PLCNT.c` (`Player_control()`) | the partition timer (`waza_work[].w_int`) counts down while the charged direction is not held, and is only reloaded when the command times out, so the window depends on where that timer happened to be | the timer is reloaded with its maximum from the command table as soon as a charged direction has reached one third of its charge (14 of the 42 frames) | done |
| SM5 | Left-right unblockables are not removed, but hitting a character during block animation while facing the other way scales damage to 50% (hit and follow-ups) | damage scaling in `src/anniversary/sf33rd/Source/Game/HITCHECK.c` and `PLPDM.c` | left-right unblockables deal full damage | 50% damage scaling for cross-facing hits during block | done |

## Additional bugfixes

| # | Spec item | Code | Status |
|:--|:---|:---|:---|
| B1 | All successful throws are invincible for the entire animation (Remy/Urien glitch, Q throw bug) | throw handling in `src/anniversary/sf33rd/Source/Game/upper3s/upper3s_throw.c` (`upper3s_throw_invuln_start()`, `upper3s_throw_frame()`), hooked in `HITCHECK.c` (`set_caught_status`, `hit_check_main_process`) | the victim of a throw can be hit out of the throw animation | the victim of a successful throw is invincible for the whole animation | done |
| B2 | Characters cannot move during the opponent's superflash (currently possible when a projectile is touched on the same frame) | superflash handling in `src/anniversary/sf33rd/Source/Game/upper3s/upper3s_throw.c` (`upper3s_superflash_hit()`), hooked in `HITCHECK.c` (`plef_at_vs_player_damage_union`, `set_guard_status`) | the damage stop of a hit replaces the hit stop of the superflash, so the frozen character can walk away | the damage stop of a hit is dropped while the defender is in a superflash | done |
| B3 | Universal overheads (MP+MK) and personal actions (HP+HK) work while holding back or forward | input handling in `src/anniversary/sf33rd/Source/Game/PLS03.c` (`check_leap_attack`, `check_chouhatsu`), `src/anniversary/sf33rd/Source/Game/upper3s/upper3s_input.c` | both commands are thrown away when the stick is not neutral, and the command turns into a normal attack | the away and the towards direction do not block them any more; the up and the down direction still do | done |
| B4 | Characters cannot attack immediately after teching an air throw (Makoto and others) | throw tech handling in `src/anniversary/sf33rd/Source/Game/PLS00.c` (`nm_49000`), `src/anniversary/sf33rd/Source/Game/upper3s/upper3s_throwtech.c` | only the ground throw tech is locked out of its input until the character has landed; the air throw tech may attack in the air | the air throw tech is locked out of its input until the character has landed as well | done |
| B5 | Fix rare move-interaction soft locks (X.F.L.A.T. vs EX Yagyou Dama / Aegis Reflector / Tengu Stone, Critical Combo Attack vs Magnetic Storm) | move-specific code | todo |
| B6 | Replace half-circle motions with quarter-circle motions | command parsing in `cmd_data.c` | todo (wishlist in the spec, not a balance change) |
| B7 | Characters passing through each other | - | drop (the designer explicitly ruled this out of scope: it would need continuous collision) |

## Characters

All character work splits into four kinds of change:

* **Super art data** - stock count, gauge length and per-hit damage live in
  `PLCNT.c:super_arts_data[20][4]` (and the `super_arts_DATA[20][4]` variant
  used when the "all supers" option is on). The 3s' values are applied through
  `upper3s_super_art_<name>[4]` in `upper3s/plXX.c`.
* **Stun bar** - per-character base size in `PLCNT.c:pl_piyo_tbl[20]`.
* **Frame data, hitboxes, damage, hitstun** - **not in the repository**. The
  per-character move data is read from the disc image at runtime and
  interpreted by `CHARSET.c` / `HITCHECK.c`; the 3s' values are applied
  through `upper3s_moves_<name>[]` in `upper3s/plXX.c`. Startup / recovery
  frame counts and box geometry need the on-disc frame chain and are tracked
  as `needs-disc`.

  The damage and the hitstun of a move are read out of the disc image by the
  dump tool (`upper3s/upper3s_dump.c`) and decoded by
  `tools/upper3s/decode_chain.py`; `tools/upper3s/hit_advantage.py` computes
  the advantage of the hit of every move out of that data, which is how the
  values of the hitstun items are derived.  The hitstun of a move is the hold
  of the defender, the field `hs_you` of the attack data (`UNK_7.hs_you`),
  and not the field `impact`, which is the knockback of the hit.
* **Behaviour** (cancels, new moves, deactivations) - `passXX.c` /
  `PLPATxx.c`.

Per-item detail is filled in as each character is worked on; the full item
list is in `design-spec.md`.

### What is in the patch, and what is not

The patch has two layers:

* **the data layer** (`upper3s/plXX.c`) - everything that can be read from
  the retail tables of the repository: the super art stock counts and gauge
  lengths (`PLCNT.c:super_arts_data`), the per hit damage of the supers
  (`POW_DATA.c:Power_Data`), the stun bar size (`PLCNT.c:pl_piyo_tbl`) and
  the health (`Com_Data.c:Com_Vital_Unit_Data`);
* **the source layer** (`#if defined(UPPER_3S)` in the shared sources) - the
  system mechanics: the whiffed metergain, the cross up damage scaling, the
  stun and the health tables.

The **frame data** of the moves - startup, active and recovery frames, the
hitboxes and the pushboxes, the cancel windows and the hit advantage - is
**not in the repository**: it is read from the disc image at runtime. Those
items are `needs-disc`: the place they have to be changed is documented in
[`implementation-guide.md`](implementation-guide.md), and they become
implementable as soon as the disc image is available.

The **startup and the recovery** of a move are the exception: they are now
implemented. The frame chain of a move lives in `wk->char_table[koc]`, and
`upper3s/upper3s_chain.c` walks it, reads the hold of every frame and changes
the number of the frames of the startup and of the recovery: the frames are
shortened from the longest one, and a startup that is longer than the one of
the retail data is made longer instead:

* the **startup** of a move is the number of the game frames before the first
  frame of its chain that carries the attack data;
* the **recovery** of a move is the number of the game frames that follow the
  last frame of its chain that carries the attack data.

A change is only made when the chain still holds the number the retail data
has (`startup_from` and `recovery_from` in `upper3s/plXX.c`), so a chain that
the walk cannot follow is never touched. The walk is cross-checked against
the offline decoder of the disc (`tools/upper3s/decode_chain.py`) over every
move of every character: 2592 moves have the same startup, and every move the
two disagree on is one the patch does not touch.

The items that the document lists for a character but that are expressed in
terms of the disc data are marked `needs-disc` in the table below.

| Character | Items | Status |
|:---|:--:|:---|
| Chun-Li | 8 | 4 done (the stocks and the bar of the SA II, the stocks and the bar of the SA III, the hitstun of the Cold Blue Kick, the startup of the crouching Jab), 4 needs-disc (the cancel window of the crouching Forward, the middle hits of the SA II, the wake up behaviour, the hurtbox of the crouching head); the SA III also needs the disc for its hitbox and its cross up advantage |
| Ken | 4 | 3 done (SA III stocks and damage, the damage of the jab DP), 1 needs-disc (the throw crash) |
| Yun | 4 | 2 done (SA II stocks, SA III bar), 1 needs-disc (the hitbox of the up kick), 1 needs-disc (the super priority flag of "Genei Jin", which lives in the disc data) |
| Makoto | 9 | 2 done (the stun of "Abare Tosanami", the stun of the three "Fukiage"), 7 needs-disc (the new input, the tech behaviour, the three bugfixes and the invincibility removal) |
| Dudley | 7 | 2 done (the stun of the two supers), 5 needs-disc |
| Urien | 2 | 1 done (SA I stocks, bar and damage), 1 needs-disc (the connectivity of the supers) |
| Akuma | 4 | 2 done (the stun bar, the bar of "Messatsu Gou Rasen"), 1 half done (the KKZ: the damage is reduced to 95, the motion is still the retail one), 1 drop (the design document keeps his hitbox) |
| Yang | 4 | 0 done, 4 needs-disc - the design document explicitly keeps the bar of "Sei'ei Enbu" the shortest of the game, so that item is a deliberate no-op |
| Ryu | 6 | 1 done (the hitstun batch), 5 needs-disc (the knockback, the movement, the hitbox and the juggle) |
| Oro | 5 | 1 done (the bar of "Kishi Riki"), 4 needs-disc (the pushbox of "Jinchu Nobori", the hitstop and the juggle of "Oniyama", the new forward throw and the deactivation of "Kishi Riki") |
| Elena | 6 | 2 done (the stocks of "Spinning Beat" and the speed of the health recovery of the healing super, `PLPAT08.c`), 4 needs-disc (the chain of the crouching Short, the startup of "Mallet Smash", the cancel of the standing Strong, the juggle of "Brave Dance" and the speed of the cancel animation of the healing super) |
| Ibuki | 5 | 3 done (the health, the stocks of "Yami Shigure", the throw tech of the personal action), 2 needs-disc (the cancel of "Tsuiji Goe" and the knockdown fix) - the second half of the personal action item (parrying or attacking in the air) needs a hook in the shared reaction code and is reported, not implemented |
| Necro | 4 | 0 done, 4 needs-disc (the startup, the pushbox and the vacuum, the new EX move and the commentary) |
| Alex | 10 | 2 done (the cancel of the crouching Strong and the cancel of the crouching Forward - the rules are in `upper3s/upper3s_ch_alex.c`, the shared hook is in `PLS00.c:check_cg_cancel_data()` through the dispatcher `upper3s/upper3s_cancel.c`), 8 needs-disc |
| Hugo | 6 | 3 done (the hitstun of the crouching Jab and the crouching Short, the startup of the EX Giant Palm Bomber, the startup and the recovery of the "Meat Squasher"), 3 needs-disc |
| Q | 12 | 7 done (the health bonus of the personal actions, the hitstun batch, the cancel of the crouching Jab, the cancel of the standing Short, the cancel of the crouching Short and the cancel of the crouching Forward - the rules are in `upper3s/upper3s_ch_q.c`, the shared hook is in `PLS00.c:check_cg_cancel_data()` through the dispatcher `upper3s/upper3s_cancel.c` - and the startup of the dash standing Jab), 5 needs-disc |
| Remy | 11 | 5 done (the stun bar, the damage of the crouching Strong, the bar of "Blue Nocturne", the hitstun batch, and the hit advantage of the Cold Blue Kick - which is a move of Chun-Li, so the change is in `upper3s/pl14.c`), 6 needs-disc (the superflash of "Blue Nocturne" needs a hook in the shared super art code, the rest is the disc data) |
| Twelve | 12 | 7 done (the health, the stocks of "X.F.L.A.T.", the hitstun batch, the cancel of the standing Strong, the cancel of the far Strong, the cancel of the crouching Strong, the cancel of the crouching Forward and the cancel of the air attacks - the rules are in `upper3s/upper3s_ch_twelve.c`, the shared hook is in `PLS00.c:check_cg_cancel_data()` through the dispatcher `upper3s/upper3s_cancel.c`), 5 needs-disc |
| Sean | 12 | 3 done (the hitstun batch, the startup of the Ryuubi Kyaku - the three strengths of the move share one chain in the data of the disc, so the chain is brought to 19 frames and the three of them start earlier - and the startup of the Dragon Smash), 9 needs-disc (the super cancel of the Sean Roll is the cancel byte of its chain, the rest is the disc data) |
| Gill | 0 | nothing to do - the design document does not change him |
