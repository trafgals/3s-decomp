# Third Strike Upper (3s') - Makoto

Makoto is the character 16 of the game (AGENTS.md section 3.1). The retail
sources are `pass16.c` and `plpat16.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl11.c`.

Retail base data (`PLCNT.c:super_arts_data[16]`):

| slot | command | gauge length | stocks | dtm | move |
|:--|:--|:--|:--|:--|:---|
| SA I | 22 | 120 | 1 | 65536 | "Seichusen Godanzuki" |
| SA II | 20 | 88 | 2 | 65536 | "Abare Tosanami" (multi hit) |
| SA III | 21 | 96 | 1 | 10922 | "Tanden Renki" (a sixth of the damage) |

The four strengths of a super are four consecutive entries of the group 0 of
the assignment table of Makoto (`bin2obj/asstbl.c:_assadr_lv_9900[16][0]`):
the SA I is 58 to 61, the SA II is 43 to 45 and the SA III is 54 to 57.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Stun damage of Abare Tosanami (SAII) reduced by 50%" | the stun of a move is `UNK_7.piyo`, an index into the per character table `_add_piyo_gauge[player][16]`, which is in the repository (`bin2obj/gauge.c`), so the absolute value of the stun of a move is readable | done - the three versions of the super are scaled to 500 permille (`upper3s/pl11.c`) |
| "Stun damage of L/M/H Fukiage reduced from 29/31/31 to 27/28/31" | the row of Makoto of `_add_piyo_gauge` is `{ 0, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 24, 25, 29, 31 }`, so the index 14 is a stun of 29 and the index 15 a stun of 31; the attack table of the disc holds exactly three entries with those two indexes, which is the 29/31/31 of the document | done - the three strengths of the Fukiage are the char_ix 36, 37 and 38 of the special move slot, and the override is the absolute value of the document (`upper3s/pl11.c`) |
| "Tanden Renki (SAIII) can be manually deactivated early by inputting QCT+K (236K) while the super art is active" | the deactivation is a new input and a new state of the super art, which is the shared code (`PLPAT.c`, `PLPATUNI.c`); the state of the super art of Makoto is not in a file of the character | needs-disc - a hook in the shared super art state machine would be needed |
| "Karakusa startup animation will no longer automatically throw-tech if Makoto is thrown out of it" | the throw tech window of a move is opened by the move itself (`PLW.tk_dageki`, `PLW.tk_nage`); the Karakusa is a shared special move (`PLPATUNI.c`) | needs-disc - a hook in the shared special move code would be needed |
| "Increase vacuum effect and hitstun time on the first hit of Seichusen Godanzuki (SAI)" | the vacuum and the hitstun are the frame chain and the attack table of the disc | needs-disc |
| "Fix bug where EX Oroshi (214PP) will not knock down when it connects with an opponent's limb hurtbox" | the knockdown flag is a bit of the hitbox entry of the attack table of the disc | needs-disc |
| "Fix bug where one frame of Hayate's (236[P]) startup/hold animation has a get-thrown-box that is too small compared to its' pushbox" | the boxes are the entries of the attack table of the disc | needs-disc |
| "Remove invincibility on Makoto's recovery animation after a successful Karakusa (63214K); no change to invincible recovery animation on neutral throw (headbutt)" | the invincibility of the recovery of a throw is the throw invulnerability of the shared code (`HITCHECK.c`, `upper3s/upper3s_throw.c`); the Karakusa is a shared special move | needs-disc - a hook in the shared throw invincibility would be needed, and it conflicts with the item B1 of the system mechanics |
| "No change on EX Oroshi xx Abare Tosanami comboing on Hugo" | nothing to change | drop |

### How the Fukiage was found

The index of the Fukiage was the last piece that was missing for the stun
item, because the move is not named anywhere in the sources: it is one of the
special moves of the character, and the specials are reached through the
command tables of `cmd_data.c` and the assignment table of the disc.

The chain is short and every link of it is in the repository:

1. The commands of the character are the table `cmd_data.c:p11_cmd[56]`, one
   entry per command.  The command 28 is the 623 motion with the punch
   buttons, which is the Fukiage (`tools/upper3s/decode_cmd.py` prints the
   motion of every command of every character).

   The command tables are `cmd_data.c:pl_cmd[20]`, and the table of a
   character is the entry of that array at the index of the character -
   `PLW.player_number`, which is the character index of the game (16 for
   Makoto).  The *name* of the entry is the name the original source gave
   to the slot: the characters 0 to 9 are `p0_cmd` to `p9_cmd`, the
   characters 10 to 14 are `pA_cmd` to `pE_cmd` and the characters 15 to 19
   are `p10_cmd` to `p14_cmd`.  The number in the name is therefore not the
   number of the character: the table of Makoto is `p11_cmd`, because that is
   the name of the seventeenth slot.

   The number in the name of the file of the patch is not the number of the
   character either: the data of Makoto is in `upper3s/pl11.c`, and the
   table that says which file belongs to which character is
   `upper3s/upper3s.c:upper3s_moves[]`.  The retail sources of Makoto are
   `pass16.c` and `plpat16.c`, which are numbered the character index.

   A command slot the character does not use holds the placeholder
   `dm_cmd_xx` and is not a move.

2. The column of a command is `cmdixconv(i)`, which is the entry
   `cmdixconv_table[i - 20]` of `PLS03.c`.  For the command 28 it is the
   column 1.

3. The column is a pointer into the assignment table of the character,
   `bin2obj/asstbl.c:_assadr_lv_9900[character][column]`, and the pointer
   names the array of the moves of that column - `D_00471A7C` for Makoto and
   `D_0047168C` for Ken.  The char_ix of the moves are the entries of that
   array.

4. The four strengths of a special move are the four consecutive entries of
   that array, at the offset `(i - 28) * 4`: the command 28 is the first
   special move of its column, so its offset is zero.

   The array of Makoto holds the char_ix
   `[36, 37, 38, 39, 32, 33, 34, 35, 28, 29, 30, 31, 50, 51, 52, 53]`, so
   the command 28 is the char_ix 36 (light), 37 (medium), 38 (heavy) and 39
   (EX) of the special move slot of the character.

The same chain gives the char_ix of the jab Dragon Punch of Ken, because the
array of the column 1 of Ken is `D_0047168C` and holds the char_ix
`[28, 29, 30, 31, 24, 25, 26, 27, 32, 33, 34, 35]`: the command 28 is the
char_ix 28, 29, 30 and 31, and the jab Dragon Punch is the char_ix 28.  That
is the value the patch already used, so the derivation reproduces a number
that was already known.

The stun of the move is the index of the attack data into
`bin2obj/gauge.c:_add_piyo_gauge[16]`.  The attack table of Makoto holds
exactly three entries with the index 14 or the index 15 - the entries 38,
61 and 62, with the damage of 24, 28 and 32, which is the progression of the
light, the medium and the heavy.  They are the 29/31/31 of the document.  The
fourth strength of the move, the EX, is the entry 63 with the index 10, a
stun of 21; the document does not quote it, so it keeps the value of the
disc.  The values of the override are absolute, so 27 and 28 can be used even
though the row of the character does not hold them.

### The conflicts of the invincibility item

The item B1 of the system mechanics ("all successful throws are invincible for
the entire animation") is implemented in the shared code
(`upper3s/upper3s_throw.c:upper3s_throw_invuln_start()`). The item of the
document that removes the invincibility of the recovery of the Karakusa is
the opposite of B1 for one move, so the two cannot both hold as they are
written: the invincibility has to be removed for the Karakusa only, which
means a per move exception in the shared code. The document itself notes that
the neutral throw is left alone, so the exception has to be per move and not
per character.
