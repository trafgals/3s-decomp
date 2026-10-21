# The tools of the Third Strike Upper patch

The frame data of the moves of *Street Fighter III: 3rd Strike* is not in the
repository.  It is read from the disc image at runtime and interpreted by
`CHARSET.c` / `HITCHECK.c`, so the numbers the design document talks about -
startup, active and recovery frames, the hit boxes, the push boxes, the
damage, the stun and the hitstun - are not visible from the sources.

These tools read that data out of the disc image and turn it into something
that can be reviewed, so that the values of the patch are derived and not
guessed.

## The workflow

1. **Dump the data of the disc.**  The dump tool lives in the game itself
   (`src/anniversary/sf33rd/Source/Game/upper3s/upper3s_dump.c`), because the
   data is only in memory while the game runs.  Build it and run it until
   every character is dumped:

   ```sh
   make PLATFORM=linux UPPER_3S=1 UPPER3S_DUMP=1 BUILD_DIR=build/dump -j6
   bash tools/upper3s/run_dump.sh
   ```

   The dump lands in `upper3s_dump.txt` in the root of the repository (it is
   not tracked: it is 4 MB of data that the disc already holds).  The memory
   of the game cannot hold the data of all twenty characters at once, so the
   tool is run several times and picks up where it stopped; the `END` marker
   of a character is what tells it that the character is complete.

2. **Decode the frame chain.**  `decode_chain.py` walks the chain of a move
   the way the retail game does.  The chain is not a flat list of blocks, it
   is a little program with jumps, so the walk follows them:

   ```sh
   python3 tools/upper3s/decode_chain.py upper3s_dump.txt --all \
       --att-table upper3s_dump.txt --csv build/move-table.csv
   ```

3. **Read the moves.**  `move_report.py` prints the moves of a character with
   their frames, their startup, their active frames, their damage, their stun
   and their hitstun:

   ```sh
   python3 tools/upper3s/move_report.py --char ken --slot 4 \
       --att-table upper3s_dump.txt \
       --power-data src/anniversary/sf33rd/Source/Game/POW_DATA.c
   ```

4. **Compute the advantage of a hit.**  `hit_advantage.py` computes the
   advantage of the hit of every move out of that data.  The advantage is not
   written down anywhere: it is the difference between how long the defender
   is held and how long the attacker is locked.  The model it uses is written
   down at the top of the file, and it reproduces the number of the design
   document for the standing Roundhouse of Q (-10 on hit):

   ```sh
   python3 tools/upper3s/hit_advantage.py upper3s_dump.txt --char q \
       --att-table upper3s_dump.txt
   ```

5. **Check that the decoding is sane.**  `validate_chain.py` checks the dump
   and the decoding against the retail tables of the repository (the order of
   the characters, the super arts of every character, the damage of the
   attacks of Ken, the startup of the normals of the whole cast and so on):

   ```sh
   python3 tools/upper3s/validate_chain.py
   ```

6. **Check the walk of the game against the decoder.**  The patch does not walk
   the chain of a move with the decoder of this folder but with the walk of
   the game itself (`upper3s/upper3s_chain.c`), so the two have to agree.
   `compare_startup.py` compares the startup and the recovery that the game
   reads with the ones the decoder reads, over every move of every character:

   ```sh
   make BUILD_DIR=build/verify build/verify/tests/test_upper3s_chain_dump
   ./build/verify/tests/test_upper3s_chain_dump upper3s_dump.txt > build/chain_startup.txt
   python3 tools/upper3s/compare_startup.py upper3s_dump.txt build/chain_startup.txt
   ```

   The report lists the moves whose startup or recovery differ.  The two
   walks do not read a branching chain the same way - the decoder follows
   the commands of the chain and the runtime machinery does not - so those
   are the moves whose chain branches on the state of the game.  The patch
   never touches them, because a change is only made when the chain still
   holds the number the retail data has.

   What the check fails on is a change of the patch that the disc does not
   take: the tables of the patch are read, and every change of the startup
   or of the recovery is looked up in the data of the disc.  A move that
   does not hold the value the patch expects is a change of the patch that
   is not there, and that fails the check.  A run that compared nothing
   fails as well.

   The reader of the dump is a tool and not a test: the dump is not part of
   the repository, so `make test` runs it only when the dump is there and
   writes its output to `$(BUILD_DIR)/chain_startup.txt`.  The comparison
   runs there too, and its failure is the failure of the test.

7. **Name the special moves.**  The special moves of a character are not named
   anywhere in the sources: they are reached through the command tables of
   `cmd_data.c` and through the assignment table of the disc.
   `decode_cmd.py` reads both and prints, for every command of every
   character, the motion of the input and the `char_ix` of the four
   strengths of the move:

   ```sh
   python3 tools/upper3s/decode_cmd.py --char makoto --min 28 --brief --assignment
   ```

   The motion is written the way a player reads it, in the numpad notation
   (`236` is down, down-forward, forward).  The `char_ix` is the index the
   override tables of `upper3s/plXX.c` address a move by, so this is what
   turns "the stun of the Fukiage" into a number that can be written down.

   The decoder checks itself against the commands the game is known for:

   ```sh
   python3 tools/upper3s/decode_cmd.py --self-test
   ```

   The test covers the three specials of Ryu (the Dragon Punch is 623, the
   Hadouken is 236 and the Tatsumaki Senpukyaku is 214) and the two moves
   whose index the patch already used (the jab Dragon Punch of Ken is the
   char_ix 28, the Fukiage of Makoto is the char_ix 36 to 39).  It also
   checks that a character can be named (`--char makoto`) and that the
   placeholder of a command slot the character does not use is not read as
   a move.

   The lever of the game is a nibble with the bits 1 for up, 2 for down, 4 for
   forward and 8 for back; the horizontal bits are relative to the character,
   because `CMD_MAIN.c:pl_lvr_set()` swaps them when the character faces to
   the left.  The bit 0x8000 of the lever of a step says that the lever has
   to be exactly that direction instead of only containing it.

## What the tools are for

The values of the patch live in `src/anniversary/sf33rd/Source/Game/upper3s/`
and in the per character tables `upper3s/plXX.c`.  The tools above are how
the numbers in those tables were derived, and they are how a change to them
can be checked.

One thing to keep in mind: a move is identified by the **index of the move in
the slot of the character** and by the **kind of character data** it lives in
(`U3S_KOC_NORMAL` for the normal attacks, `U3S_KOC_ATTACK` for the special
moves and the super arts).  For the normal attacks that index is the
`char_ix` of the assignment table of the disc (`bin2obj/asstbl.c`), and not
the index of the move in the move list.
