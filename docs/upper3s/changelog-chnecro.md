# Third Strike Upper (3s') - Necro

Necro is the character 5 of the game (AGENTS.md section 3.1). The retail
sources are `PLPAT05.c` and `PASS05.c`; the data layer of the patch is
`src/anniversary/sf33rd/Source/Game/upper3s/pl04.c`.

Retail base data (`PLCNT.c:super_arts_data[5]`):

| slot | command | gauge length | stocks | move |
|:--|:--|:--|:--|:---|
| SA I | 20 | 88 | 1 | the first super (236+P) |
| SA II | 22 | 104 | 1 | "Slam Dance" (236+P+P) |
| SA III | 21 | 72 | 2 | the third super (623+P) |

The design document does not ask for a stock or gauge length change on Necro,
so nothing is overridden in `upper3s/pl04.c`.

## The items of the design document

| item | what the retail code holds | status |
|:---|:---|:---|
| "Change Slam Dance (SAII) startup from 0+2 to 2+0, so the total startup time is unchanged but it can't be jumped on reaction after the superflash" | the startup of a super is the super flash block of the frame chain of the disc; the "0+2" of the document is the flash time plus the first active frames | needs-disc |
| "Fix pushbox on Electric Shock (623P), and add a slight vacuum effect to the middle hits of all versions" | the pushbox and the vacuum are the movement data of the frame chain and the entries of the attack table of the disc | needs-disc |
| "Add EX Electric Shock, possibly allows juggles afterward, certainly safer on block than the Fierce version" | a new move needs a new entry in the command table of `cmd_data.c` and a new move in the frame chain of the disc; there is no `char_ix` to point at yet | needs-disc |
| the commentary of the document ("he has crouching Short xx Jab Tornado Hook to be safe on block and hitconfirm into super") | nothing to change | drop |

### The hook that the new EX move would need

The command table of Necro is `cmd_data.c:p5_cmd`, the array of the 56 command
sets of the character. An EX version of a special is a new strength of an
existing command: the header of the command set holds the four routine
numbers (the four strengths) and the four EX button combinations, and the
strengths of a command are four consecutive entries of the group 1 of the
assignment table of the character
(`bin2obj/asstbl.c:_assadr_lv_9900[5][1]`). Adding the EX Electric Shock is
therefore

1. a new entry in `cmd_data.c:p5_cmd_XX` (the EX button combination of the
   header, and the routine number of the EX version), and
2. a new entry in the group 1 of the assignment table of Necro, which is the
   data of the disc and cannot be written in the repository.

Step 1 is a change of the shared file `cmd_data.c`, step 2 needs the disc.
