# Third Strike Upper - implementation guide

This document describes how the 3s' balance patch is implemented in this
repository. Read it before changing anything under
`src/anniversary/sf33rd/Source/Game/upper3s/` or before touching a shared
engine file.

The authoritative design document is `docs/upper3s/design-spec.md`; the item
by item status lives in `docs/upper3s/spec-map.md`.

---

## 1. The two layers

### 1.1 Source layer

Everything that is expressed in the C sources is patched in place, behind
`#ifdef UPPER_3S`. The vanilla build is left byte identical: the
`#if defined(UPPER_3S)` blocks are compiled out, so the vanilla object code
does not change.

The flag is set by the build system:

```
make PLATFORM=linux UPPER_3S=1
make PLATFORM=web   UPPER_3S=1
```

`Makefile` adds `-DUPPER_3S` to every platform's flag set. The files in
`src/anniversary/sf33rd/Source/Game/upper3s/` are **only** compiled when
`UPPER_3S=1`; the Makefile removes them from the source list otherwise, so
the vanilla link order - and therefore the byte match against retail
`THIRD_U.BIN` - is untouched.

### 1.2 Data layer

**The per character move data is not part of this repository.** Startup
frames, active/recovery frames, hitboxes, pushboxes, damage, stun, hitstun
and hit advantage are read from the disc image (`SF33RD.AFS`) at runtime and
interpreted by `CHARSET.c` and `HITCHECK.c`.

The loader is `texgroup.c`; it fills `char_init_data[charset_id]`, and
`CHARID.c:set_char_base_data()` copies it into the player work:

```c
wk->char_table[0] = cdat->nmca;   /* normals        */
wk->char_table[1] = cdat->dmca;   /* damage states  */
wk->char_table[2] = cdat->caca;
wk->char_table[3] = cdat->cuca;
wk->char_table[4] = cdat->atca;   /* special moves  */
wk->char_table[5] = cdat->saca;   /* super arts     */
wk->char_table[6] = cdat->btca;
wk->char_table[7] = cdat->exca;
wk->char_table[8] = cdat->cbca;
wk->char_table[9] = cdat->yuca;
wk->att_ix_table = cdat->atit;    /* the attack table */
```

Because of that, the numbers of a move are changed by **patching the loaded
tables at runtime**, right after they have been read. The hook is
`upper3s_apply_character()`, called at the end of `set_char_base_data()`.

---

## 2. How a move is addressed

### 2.0 The character index

There are **two** different numbers for a character and they are not the
same:

* the **character index** - 0 to 19, the index of the game: Gill 0, Alex 1,
  Ryu 2, Yun 3, Dudley 4, Necro 5, Hugo 6, Ibuki 7, Elena 8, Oro 9, Yang
  10, Ken 11, Sean 12, Urien 13, Akuma 14, Chun-Li 15, Makoto 16, Q 17,
  Twelve 18, Remy 19. It is `My_char[ix]`, and the player work keeps it in
  `PLW.player_number`. It is **not** the order in which the characters were
  introduced; the evidence is in `AGENTS.md` section 3.1.
* the **charset id** - the index into the disc data (`char_init_data`).
  `PLCNT.c:plid_data` maps the character index to it, and the mapping is
  *not* the identity:

  ```c
  const s8 plid_data[20] = { 6, 3, 5, 1, 2, 9, 7, 4, 10, 8,
                              12, 13, 14, 15, 16, 18, 19, 20, 21, 22 };
  ```

  So Gill (0) is charset 6 and Ken (11) is charset 13.

The override tables are keyed by the **character index**. `upper3s_apply_character()`
is therefore called from `PLCNT.c:set_base_data()`, right after
`wk->player_number = My_char[ix]`, and not from `CHARID.c`, where only the
charset id is known.

### 2.1 The move index

A move is addressed by a pair:

* `koc` - the "kind of character data" slot, i.e. the index into
  `wk->char_table[]` (see the table above and `U3S_KOC_*` in
  `include/sf33rd/Source/Game/upper3s.h`).
* `index` - the index of the move inside that slot.

The character state machines use the same pair. `PASSxx.c` / `PLPATxx.c`
call `Command_Attack(wk, reaction, tech_number, power_level, ex_shot)` and
`set_char_move_init(wk, koc, index)`; `set_char_move_init()` stores the
pair in `wk->now_koc` / `wk->char_index`.

The **slot** is the one the engine uses: the reaction moves are in slot 0,
the normal attacks in slot 4 and the special moves *and* the super arts in
slot 5 - the two share the slot and are told apart by their index inside
it.

**To find the index of a named move, look at its `char_ix`** - the index
the engine passes to `set_char_move_init()`. It is *not* the tech number of
the command, and it is *not* the index of the move in the move list:

* for a normal attack it comes from the assignment tables of the disc
  (`bin2obj/asstbl.c`), which are indexed by `[player][button][variant]`:

  ```
  char_ix = base + kos * 3 + koa
  ```

  with `kos` the button (0 LP, 1 MP, 2 HP, 3 LK, 4 MK, 5 HK) and `koa` the
  variant.  `_asstbl_lv_0010` holds the standing normals (base 0, koa 0
  without a dash = 5X, koa 1 with a dash forward = 6X, koa 2 with a dash
  back = 4X) and `_asstbl_lv_1010` the crouching ones (base 18, koa 0 = 2X,
  koa 1 = 3X, koa 2 = 1X).  The jumping normals are in `_asstbl_lv_2010`,
  `_asstbl_lv_3010` and `_asstbl_lv_4010`.  `PLS03.c:check_nm_attack()`
  picks the table from the stance and `PLS03.c:waza_select()` the variant.
* for a super art it comes from `bin2obj/asstbl.c:_assadr_lv_9900`; the
  four strengths of a super are four consecutive entries.

Note that `PASSxx.c` (uppercase) and `passxx.c` (lowercase) are different
files: the uppercase ones hold the *state machines*, the lowercase ones the
command dispatch. Both are named after the character index of the game.

### 2.2 The move header and the attack table

`char_table[koc][index]` is an offset (in `u32` units) into the table of
the slot. The move header is therefore at

```c
u32* set_char_ad = &wk->char_table[koc][wk->char_table[koc][index] / 4];
```

and holds, per frame of the move, `cgd_type` consecutive `u32`s. The
header is copied into the work structure by `CHARSET.c:setupCharTableData()`
and lands on the `cg_*` fields of `WORK`:

| offset in the block | field                       | meaning                       |
|---------------------|-----------------------------|-------------------------------|
| -2                  | `cgd_type`, `pat_status`    | size of the block, pattern id  |
| -1                  | `hit_range`, `sp_tech_id`   |                                |
| 0                   | `cg_type`, `cg_ctr`, `cg_se`| animation / effect             |
| 1                   | `cg_olc_ix`, `cg_number`    |                                |
| 2                   | `cg_hit_ix`, `cg_att_ix`    | **the attack data index**      |
| 3                   | `cg_cancel`, `cg_effect`    |                                |
| 4                   | `cg_zoom`, `cg_rival`       |                                |
| 5                   | `cg_add_xy`, `cg_next_ix`   |                                |

`cg_att_ix` is the index of the move's entry in `wk->att_ix_table`
(`UNK_7`):

```c
typedef struct {
    s8 level;    /* attack level, bit flags                        */
    s8 mkh_ix;
    s8 but_ix;
    s8 dipsw;
    s8 guard;    /* guard type                                     */
    s8 dir;      /* attack direction                               */
    s8 free;
    s8 pow;      /* damage - an INDEX into Power_Data              */
    s8 impact;   /* the knockback of the hit                        */
    s8 piyo;     /* stun - an INDEX into the per player stun table */
    s8 ng_type;
    s8 hs_me;    /* the hitstun of the attacker                     */
    s8 hs_you;   /* the hitstun of the defender                     */
    s8 hit_mark;
    s8 dmg_mark;
} UNK_7;
```

`CHARSET.c:set_new_attnum()` copies that entry into `wk->att`, and
`HITCHECK.c` reads `wk->att` when a hit is resolved.

**Important:** `att_ix_table` entries are *shared* between the moves that
use the same attack data. Changing one entry changes every move that
points at it. That is how the retail game works as well, but it has to be
kept in mind when writing an override.

### 2.3 Damage and stun are indexes

* `att.pow` is an index into `POW_DATA.c:Power_Data`, which is
  `index * 5`. The base damage of a move is therefore `Power_Data[att.pow]`.
* `att.piyo` is an index into `_add_piyo_gauge[player][16]`.

Neither can be replaced by a plain field write, so the framework handles
them at the point where they are used:

* `Pow_Pow.c:cal_damage_vitality()` and `cal_damage_vitality_eff()` call
  `upper3s_power()`, which applies `U3S_MV_DAMAGE` (absolute base damage)
  and `U3S_MV_DAMAGE_PERMILLE` (a scale, in permille).
* `HITCHECK.c` calls `upper3s_stun_value()` after the stun table lookup
  and uses the returned value instead.

Everything else - the knockback (`impact`), the hitstun of the attacker
(`hs_me`), the hitstun of the defender (`hs_you`) - is a plain value and is
patched directly in `att_ix_table` by `upper3s_apply_character()`.

### 2.4 The special moves and their commands

The special moves of a character are not named anywhere in the sources.  They
are reached through the command tables of `cmd_data.c` and through the
assignment table of the disc, and `tools/upper3s/decode_cmd.py` walks both.

**The command tables.**  `cmd_data.c` holds, per character, a list of 56
commands (`pl_cmd[character]`).  Every command is a header of twelve words
followed by a list of steps of four words each:

| word | meaning |
|:--|:--|
| 0 | the type of the step - the index of `chk_move_jp[]` in `CMD_MAIN.c`, so the type 1 is `check_0` |
| 1 | the number of frames the step may wait |
| 2 | the number of frames the state has to be held |
| 3 | the lever the step waits for, in its low nibble; the bit 0x8000 says that the lever has to be *exactly* that direction |

The header holds the reset value of the command, the "dead" values, the
routine of the move (`waza_r`, the four strengths), the button index
(`btix`) and the button of every strength (`exdt`).

**The lever.**  The nibble of the lever of the game is:

| bit | direction |
|:--|:--|
| 1 | up |
| 2 | down |
| 4 | forward |
| 8 | back |

The horizontal bits are relative to the character: `CMD_MAIN.c:pl_lvr_set()`
swaps them when the character faces to the left, so 4 is always "towards the
opponent" and 8 is always "away from it".  Written the way a player reads
them, in the numpad notation, the nine states are 5 (neutral), 8 (up), 2
(down), 6 (forward), 4 (back), 9 (up-forward), 3 (down-forward), 7
(up-back) and 1 (down-back).

**The assignment table.**  `bin2obj/asstbl.c:_assadr_lv_9900[character][column]`
holds the entries of the moves of a character, and the entry of a move holds
its `char_ix`.  The entry is a pointer to the array of the moves of that
column - `D_00471A7C` for the column 1 of Makoto - and the `char_ix` of the
moves are the entries of that array.  A special move is the command `i` of
the character, and its four strengths are the four consecutive entries that
start at the offset `(i - 28) * 4` of the array of the column
`cmdixconv(i)` (`PLS03.c:cmdixconv_table`).

The command tables are `cmd_data.c:pl_cmd[20]`, and the table of a character
is the entry of that array at the index of the character - `PLW.player_number`,
which is the character index of the game.  The *name* of the entry is the name
the original source gave to the slot: the characters 0 to 9 are `p0_cmd` to
`p9_cmd`, the characters 10 to 14 are `pA_cmd` to `pE_cmd` and the characters
15 to 19 are `p10_cmd` to `p14_cmd`.  The number in the name is therefore not
the number of the character: the table of Makoto, the character 16, is
`p11_cmd`.

The same is true of the files of the patch: the data of Makoto is in
`upper3s/pl11.c` and not in `upper3s/pl16.c`.  The table that says which file
belongs to which character is `upper3s/upper3s.c:upper3s_moves[]`:

| file | character | index | file | character | index |
|:--|:--|--:|:--|:--|--:|
| `pl00.c` | Alex | 1 | `pl10.c` | Ken | 11 |
| `pl01.c` | Ryu | 2 | `pl11.c` | Makoto | 16 |
| `pl02.c` | Yun | 3 | `pl12.c` | Urien | 13 |
| `pl03.c` | Dudley | 4 | `pl13.c` | Akuma | 14 |
| `pl04.c` | Necro | 5 | `pl14.c` | Chun-Li | 15 |
| `pl05.c` | Hugo | 6 | `pl15.c` | Sean | 12 |
| `pl06.c` | Ibuki | 7 | `pl16.c` | Q | 17 |
| `pl07.c` | Elena | 8 | `pl17.c` | Twelve | 18 |
| `pl08.c` | Oro | 9 | `pl18.c` | Remy | 19 |
| `pl09.c` | Yang | 10 | `pl19.c` | Gill | 0 |

The retail per character sources are numbered the character index
(`pass16.c` and `plpat16.c` are the sources of Makoto), so the number of the
retail file is the number of the character and the numbers of the other two
numberings are not.

A command slot that the character does not use holds the placeholder
`dm_cmd_xx`, which is not a move.

So the whole chain - "the Fukiage of Makoto" - is:

```
cmd_data.c:p11_cmd[28]  ->  the 623 motion with the punch buttons
cmdixconv(28) = 1        ->  the column of the assignment table
_assadr_lv_9900[16][1]  ->  D_00471A7C
D_00471A7C              ->  the char_ix 36, 37, 38 and 39
```

The same chain gives the char_ix of the jab Dragon Punch of Ken, because the
column 1 of Ken is `D_0047168C`, whose first four entries are the char_ix 28,
29, 30 and 31:

```
cmd_data.c:p11_cmd[28]  ->  the 623 motion with the punch buttons
cmdixconv(28) = 1        ->  the column of the assignment table
_assadr_lv_9900[11][1]  ->  D_0047168C
D_0047168C              ->  the char_ix 28, 29, 30 and 31
```

The jab Dragon Punch is the char_ix 28, which is the value the patch already
used, so the chain can be trusted for the moves that are not named in the
sources.

---

## 3. The override tables

### 3.1 Super art data

`PLCNT.c:super_arts_data[20][4]` holds, per character and super art:

```c
typedef struct {
    u8 nmsa_g_ix; u8 exsa_g_ix; u8 exs2_g_ix;
    u8 nmsa_a_ix; u8 exsa_a_ix; u8 exs2_a_ix;
    u8 ex4th_full;
    s8 gauge_type;
    s16 gauge_len;  /* length of the gauge bar, 0x40 - 0x80 */
    s16 store_max;  /* number of gauge stocks the super costs */
    s32 dtm;        /* damage multiplier */
} SA_DATA;
```

`PLCNT.c:set_super_arts_status()` copies the entry into `super_arts[ix]`
and then applies the in game option modifiers (`sag_stock_omake`,
`sag_length_omake`). The 3s' values replace the **retail base**, the
option modifiers are still applied on top of them, so the hook is:

```c
saptr = upper3s_super_art_data(My_char[ix], Super_Arts[ix], saptr);
```

The per character table is `upper3s_super_art_<name>[4]` in
`upper3s/plXX.c`, one entry per super art slot (`U3S_SA1`..`U3S_SA4`).

### 3.2 Move data

`upper3s_moves_<name>[]` in `upper3s/plXX.c`:

```c
{ koc, index, flags, damage, damage_permille, stun, impact, hs_me, hs_you }
```

The array is terminated by an entry whose `flags` are zero.

Flags: `U3S_MV_DAMAGE`, `U3S_MV_DAMAGE_PERMILLE`, `U3S_MV_STUN`,
`U3S_MV_IMPACT`, `U3S_MV_HS_ME`, `U3S_MV_HS_YOU`.

### 3.3 The mechanic modules

The system mechanics that are not a number in a table live in a module of
their own, next to the framework:

| module | mechanic |
|:---|:---|
| `upper3s_sys.c` | the state that is shared by the whole cast: the metergain of a whiffed normal (SM1), the damage of a cross up (SM5), the stun bar and the health |
| `upper3s_throw.c` | the parry (SM2, SM3), the throw (B1) and the superflash (B2) |
| `upper3s_sm4.c` | the partition timer of a charge (SM4) |
| `upper3s_input.c` | the universal overhead and the personal action (B3) |
| `upper3s_throwtech.c` | the lock after an air throw tech (B4) |
| `upper3s_ch_<name>.c` | the behaviour changes of one character |

A module holds the state that the mechanic needs, because `PLW` and `WORK`
must not grow. The rule is the same everywhere: **read the state of the
character out of the game instead of keeping a copy of it**, and only keep
in the module what the game does not tell.

The modules are hooked into the shared engine files, and every hook is
behind `#if defined(UPPER_3S)`. A hook that is left outside of its guard
changes the retail build; `tools/verify/ps2_touched_sources.sh` compares the
preprocessed retail sources with the base commit and reports the files that
would change.

### 3.4 The tests

`make test` builds every file of `tests/upper3s/` that has a `main()` of its
own as a program of its own and runs them all, so that a test of one
mechanic does not have to link against the tests of the others. A test that
exercises a mechanic builds a synthetic character - a fake `char_table`, a
fake `att_ix_table` and a fake move header - and checks the numbers of the
mechanic on it.

Every test has to fail when the change it covers is removed: neutralise the
change, run the test, see it fail, put the change back. A test that passes
with and without the change does not cover anything.

---

## 4. What still needs the disc image

The following kinds of change cannot be expressed by the framework above,
because they need the *frame chain* of a move or the box data:

* hitbox and pushbox geometry,
* frame based cancel windows.

(The startup and the recovery frame counts are implemented now - see "The
startup and the recovery" below.)

The frame chain of a move is **not a list**: it is a little program that the
engine runs for every frame of the move, and it lives in `char_table[koc]`.
`CHARSET.c:check_cm_extended_code()` walks it:

```
cg_ix = (pat - 1) * cgd_type        # the chain of a move starts at block (pat-1)
loop:
    block = set_char_ad[cg_ix]      # cg_ix counts u32 words
    if code >= 0x100: end
    if decode_chcmd[code](wk, block) == 0: stop   # the handler moved elsewhere
    cg_ix += cgd_type
```

A block is `cgd_type` u32 words and starts with `UNK11 { u16 code; s16 koc;
s16 ix; s16 pat; }`; the third word is `cg_hit_ix` (u16) and `cg_att_ix`
(s16). The commands that move the chain are the ones that matter for the
frame data: `2 comm_end` loops back to `(pat - 2) * cgd_type`, `3 comm_jmp`
and `4 comm_jpss` switch to another move, `5 comm_jsr` calls one (the return
address is `cmsw.pat = cg_ix / cgd_type + 2`) and `6 comm_ret` comes back,
`16 comm_rja` / `17 comm_uja` (and the pairs 2..7) jump inside the chain, and
`10 comm_if_l` / `11 comm_djmp` branch on the state of the game.

Shortening the startup of a move means moving the block that carries the hit
data (`cg_hit_ix` / `cg_att_ix`) earlier in that chain, which changes the
animation that plays with it. Doing that safely needs the actual disc data
to walk and verify the chain.

Until then these items are listed as `needs-disc` in
`docs/upper3s/spec-map.md` together with the exact mechanism that will be
used.

### The startup and the recovery

The startup and the recovery of a move are now implemented, by
`upper3s/upper3s_chain.c`:

* the chain of a move is opened with `upper3s_chain_open()`, which reads the
  offset of the move out of `wk->char_table[koc]` and the size of the slot out
  of the tables that follow it in memory;
* `upper3s_chain_frames()` walks the chain the way the engine does: a block
  whose first word is below `0x100` is a command block and not a frame, the
  relative jumps `comm_ixfw` (49) and `comm_ixbw` (50) are followed, and the
  walk stops at the two sentinels of the end of the animation - the frame
  whose type is `0xFF` and the frame that is held for 250 game frames. Neither
  of them is a frame of the move;
* the **startup** of the move is the sum of the holds of the frames before the
  first frame that carries the attack data, and the **recovery** is the sum of
  the holds of the frames after the last one;
* `upper3s_chain_set_startup()` and `upper3s_chain_set_recovery()` shorten the
  holds of those frames, the longest one first, and never below a hold of one
  game frame: a frame that is held for no game frame at all is a command block
  for the engine, not a frame of the animation.

A change is only made when the chain still holds the number the retail data
has (`startup_from` and `recovery_from` in `upper3s/plXX.c`), so a chain that
the walk cannot follow is never touched.

A chain that already holds the target counts as done and is left alone.  That
is the case when the patch is applied again - it is applied every time the
character is loaded - and when several moves of the patch share one chain.

### The chains that several moves share

The disc does not give every move a chain of its own.  The slot of the
character holds a byte offset per move, and the same offset can be in the
slot twice: the light, the medium and the heavy version of the Ryuubi Kyaku
of Sean are the entries 32, 33 and 34 of the slot of the special moves and
they all point at the offset 3312, and the 6LP and the 4LP of Q are the
entries 1 and 2 of the slot of the normal attacks and they share their
offset as well.

A change of a chain is a change of every move that shares it, so:

* the table of the patch holds **one entry per chain**, not one entry per
  move - the Ryuubi Kyaku has one entry, for the chain that the three
  strengths share;
* a move that the document gives a different target than another move of the
  same chain cannot be expressed.  The Ryuubi Kyaku is the case: the
  document asks for 17/19/21 frames and the chain can only hold one number,
  so the chain is brought to 19 frames, the target of the medium version and
  the value that is the closest to the three of them.  The difference between
  the strengths of the document is not expressible in the data of the disc;
* the setter accepts a chain that already holds the target, so a second
  entry for the same chain is not a dropped change.

The walk is cross-checked against the offline decoder of the disc
(`tools/upper3s/compare_startup.py`) over every move of every character. The
moves the two disagree on are the ones whose chain calls another move or
branches on the state of the game; the patch does not touch them.

### The cancel window

The cancel permission of a move is the `cg_cancel` byte of the frame chain,
and the engine copies the whole block of the chain into the work structure
every frame (`CHARSET.c:setupCharTableData()`), so the byte is the twelfth
byte of the block - the low byte of the fourth word - and the dump of the
disc holds it. The cancel bits are `0x20` (into a special move) and `0x40`
(into a super art), and `PLS00.c:check_cg_cancel_data()` reads them.

The cancel rules that the patch already has (`upper3s/upper3s_cancel.c` and
the per character modules) give the cancel for the whole move, which is what
the design document asks for when it grants a cancel to a move. The items
that ask for a *window* - a cancel that has to end with the active frames,
or one that has to reach past them - need the byte of the chain itself, the
same way the startup and the recovery do.

### The dump of the disc

The tool that reads the values out of the disc is built with
`make UPPER3S_DUMP=1`. It writes `upper3s_dump.txt`, one line per move:

```
CHAR <index> <name> charset=<id>
MOVE koc=<slot> idx=<index> att=<attack index> hit=<hitbox index> frames=<n> ...
```

* `koc` is the slot of the move (4 for the normals, 5 for the specials and
  the supers, 0 for the reactions),
* `idx` is the index of the move in that slot, the index the disc uses,
* `att` is the index of the attack data of the move, the index the damage,
  the stun, the knockback and the hitstuns are read from,
* `frames` is the number of frames of the frame chain of the move,
* `codes` is the command code of every frame of the chain.

The dump follows the loader of the game: the data of a character is read
when the game needs it, and the dump asks the loader for the next
character until all of them have been read. The hook is in
`texgroup.c:q_ldreq_texture_group()`, which is the only place where the
character data of the disc is available.

---

## 5. The browser build and the deployment of the game

### The build

`make PLATFORM=web UPPER_3S=1 BUILD_DIR=build/anniversary/web` builds the game
for the browser with the emscripten toolchain. The module holds the code and
the data of the game; the disc image is **not** in it, because it is 612
megabytes. The browser build reads the disc image over HTTP range requests
instead: `src/anniversary/port/web/web_cd.c` asks for the blocks of four
megabytes it needs, keeps the last sixteen of them in memory and hands them
to the loader of the game as if they came from the disc.

The location of the disc image is the relative path `rom/THIRD/SF33RD.AFS`, and
the query parameter `?afs=<url>` overrides it. The parameter is read by the
module itself, so a build that is served from anywhere can be pointed at a
copy of the disc image somewhere else.

### The deployment on GitHub Pages

The workflow `.github/workflows/pages.yml` builds the site and deploys it to
the Pages site of the repository: <https://trafgals.github.io/3s-decomp/>.

The deployment holds the module (eight megabytes) and the disc image (612
megabytes). Pages accepts a deployment of that size, but a **file of a
repository** may not be over 100 megabytes, so the disc image is not
committed: it is published as the asset `SF33RD.AFS` of the release
`web-data-1` of the repository, and the workflow fetches it from there.

The image has to be served from the same origin as the page: the browser
reads it with a cross origin request, and the asset of a release is served
without the header that allows it. That is the reason the disc image is part
of the deployment and not only a link in the page.

### The check of the deployment

`tools/verify/web_boot.py` runs the game in a headless browser and waits for
the game loop to report a frame, so the deployment can be checked the same
way the local build is:

```
python tools/verify/web_boot.py --real-time --timeout 150
```

### The check of the published site

The check above needs the build of the sources and a dev server. The site that
is published is checked by `tools/verify/web_public.py`, which opens the
public address in a headless browser and watches the traffic of the browser:

```
python tools/verify/web_public.py --timeout 240
```

The oracle is the netlog of the browser: it holds every request of the page
with its address and the number of bytes it read, which is the record of what
the site served. The markers of the boot travel in the address of the request
to `/__trace`, so the same record tells whether the game reached its frame
loop and whether it read the disc image from the site. The answers of the
server are asked for directly, one request per file, because the netlog of
this browser does not hold them.

The published site has no dev server, so those requests answer 404. That is
not a failure: the marker is in the address of the request, not in its
answer.

The wait ends as soon as the game reports a frame, and not after a fixed
time. The disc image is 612 megabytes, and the first read of it after a
deployment is served by a network that has to fetch it from the origin
first, so the boot takes longer than the runs that come after it.

---

## 6. Rules

* Never change the behaviour of the vanilla build. Everything goes behind
  `#if defined(UPPER_3S)`.
* Never add a field to `PLW`, `WORK` or `SA_WORK`: their sizes are part of
  the PS2 memory map. Use the `upper3s` module for extra state.
* Keep the Japanese Capcom comments and the existing naming.
* Record every change in `docs/upper3s/spec-map.md` (status column) and in
  `docs/upper3s/changelog.md` (what actually changed).
* One concern per change.
