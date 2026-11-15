# Stamps: drawn rooms for the secrets, joiners and intended places

**Settled; not begun** (15 November 2026, on `claude/ecstatic-heisenberg-0ljx7r`, from 1.0.0b21.6). The owner drew ten secret rooms
and ten joiners (`media/sheets/stamps/owner-secret-areas.png`, `owner-joiners.png`), asked for input, and settled four questions
(section 2). Two agents build it on one branch, a track each (section 7). Nothing here is tested by hand yet.

*The brief, in a line: the generator's rooms are random empty rectangles and its secret rooms are plain boxes; replace them with
drawn shapes (stamps) whose features are placed on purpose, so a level is made of intended places and the maze only joins them.*

## 1. What was asked

- Secret rooms: use the owner's ten drawn shapes (bends, spirals, a long L) instead of the 1-block, 2-block and 2x2 boxes.
- Joiners: large rooms (the owner's ten, and more of ours) where paths branch, or are cut short, for visual variety.
- Other structural room types, so a level has more intended variety than the themed rectangles (`RT_*`) give.
- Dressing by intent: altars, loot, fires and ambushes go where the drawing says, not scattered over an empty room.

## 2. What the owner settled

1. **The slide in a bent room is option B:** the entrance's stone retracts 1 cell sideways into the wall beside it and the room is
   left empty to walk. (It was: A, slide along the first leg and rest in the bend; B, this; C, a plain door.)
2. **The drawings' sizes are literal cells.** Most go on the block lattice; our smaller versions come beside them.
3. **A level gets 1 big joiner and 2 to 3 small ones, with a heavy focus on intentionally placed elements over randomly generated
   empty rooms.**
4. **Every stamp is used in all 8 rotations and mirrors**, stored once.

Not asked, assumed (say so in the log if it turns out wrong): the two or three magenta squares at a secret room's corner are
alternatives, one chosen per level; a chain of one gets no doorway even where its drawing has one.

## 3. What the generator does today (read before touching it)

- **The plan** is `MG` = 23 squares of blocks (`MM[][]`), a block 3x3 cells with a 1-cell wall between: `mstart(m)` maps a plan index
  to its first map cell; the map is `MAPN` = 45. A box of n blocks is 4n - 1 cells: **3, 7, 11 or 15**. Nothing else lies on the lattice.
- **`gen_level`** (`game/gen_level.c`, from line 101) tries up to 9 chambers of 1, 3, 5 or 7 blocks a side (`SPAN`), carving each with
  `mcarve` and a room number; fills what is left with maze (`mz_grow`); joins regions by the walls between blocks (the `con[]` list, a
  tree plus a few loops); then vaults (`mz_vault`), widths (`mz_widths`), `mz_draw` (the plan to cells), doors, `place_secrets`,
  `gen_dress`.
- **Lane:** every hall keeps its middle lane, cells with x or y = 2 (mod 4) (`lane`, `g_lane` in `game/generate.c`); the dead's and the
  autopilot's pathing lean on it. A stamp must give it back (section 4).
- **Secrets** (`place_secrets`, `secret_door`, `game/generate.c` 446-547; `open_secret`, `update_slide`, `game/goals.c` 12-49;
  `Secret` in `game/state.c`): a room carved from solid blocks beside a hall, `Cell.secret` 1 room, 2 the cracked wall, 3 the doorway on
  into a pocket. The wall block slides `len` cells through the room along `sc->d`; loot is kept off its path. Option B replaces that.
- **Faces** (`game/faces.c`, `lay_faces`): each straight stretch of wall is textured by its room type. A stepped edge makes many
  one-cell stretches; check it looks right, do not assume.
- **Docs:** `docs/catacomb/levels.md` and `secrets.md` hold the rest. Read `docs/HARNESS.md` for the debug switches before adding any.

## 4. The stamp, in text

A stamp is a block of rows of one glyph per cell, stored in C as adjacent string literals, one stamp per entry:

```c
typedef struct {
    const char *name;                    /* "secret_c", "joiner_plus_L" */
    uint8_t kind;                        /* ST_SECRET, ST_JOINER, ST_PLACE */
    uint8_t w, h;                        /* cells; (w+1) and (h+1) divisible by 4 for ST_JOINER / ST_PLACE */
    uint8_t minstep, maxstep;            /* the run's levels it may appear on, 0..9 */
    uint8_t weight;                      /* 1..9 */
    uint8_t runs;                        /* bit per run I..IV; 0 means all */
    const char *rows;                    /* h rows, each w glyphs, joined with "\n" */
} StampDef;
```

| Glyph | Meaning |
|---|---|
| `#` | Stone |
| `.` | Floor |
| `P` | Port that must connect: a floor cell on the stamp's edge, at a **block centre** (index = 1 mod 4 from its origin) |
| `p` | Port that may be walled off (a hall may or may not attach) |
| `E` | Secret entrance: a stone cell outside the room, beside a floor cell. All `E` in a stamp are alternatives; one is chosen per level |
| `D` | Secret doorway on to the next link: a stone cell with solid rock behind it. Dropped (left stone) for a chain of one |
| `a` `b` `c` `s` `t` | Floor cell that asks for an altar, brazier, loot, a spawn group, a torch on the nearest wall face. Provisional: add letters as the content needs them, in the log |
| `0`-`9` | Floor cell with a number the stamp's own rule can use (a puzzle's plate, a trial's door); unused at first |

Rules every stamp must keep (the sheet tool, section 6, checks them):

- Every `.`, mark and `P`/`p` cell is one connected region. Every `P`/`p` is on the box's edge at a block centre. A 2-wide arm must
  include its block's middle cell. Every `E` and `D` is stone, touching floor, outside the room, not on a port.
- A joiner and a place are on the lattice (3, 7, 11 or 15 cells a side); a secret room need not be (the generator takes the box
  that holds it from whole blocks and fills the rest with stone).
- **Lane:** cells inside the stamp with x or y = 2 (mod 4) from its origin, if floor, are lane; the loader derives it (section 5, A1).
- **Transforms:** rotate and mirror all glyphs and the port/entrance/doorway positions with the stamp. Eight in all; duplicates
  (a symmetric stamp) are dropped at load.

### The owner's drawings, as read

Cell counts are from the gridlines of the pictures and are estimates: **check each against the picture when transcribing**.

Secret rooms (`owner-secret-areas.png`, 10 in all; magenta = `E`, green = `D`, the drawn grey "Stone" is the legend only):

| # | Shape | About |
|---|---|---|
| S1 | Box, entrances top and left, doorway bottom-right | 3x3 |
| S2 | Box, wider, entrances top and left, doorway on the right | 4x3 |
| S3 | Long strip, entrances top and left, doorway on the right | 7x2 |
| S4 | L, entrance pair at the top-left corner, doorway at the foot's end | 4x5 |
| S5 | L turned, the same | 4x5 |
| S6 | Arrowhead (stepped), entrance pair top-left, doorway at the point | 4x6 |
| S7 | C, entrance pair top-left, doorway on the inner arm facing the notch | 5x6 |
| S8 | Spiral with an inner stub, doorway at the far foot | 5x6 |
| S9 | J, entrance at the top of the long leg, doorway at the foot's west end | 4x7 |
| S10 | One-wide approach, then a wide foot, doorway on the foot's top | 6x12 |

Joiners (`owner-joiners.png`, 10 in all; ports are not drawn: **add them as the legend above says, and say so in the log**):

| # | Shape | About | Reading |
|---|---|---|---|
| J1 | Diamond with a vertical bar and crossbar of stone | 16x16 | A cruciform monument; the halves meet by the 1-cell gaps at the tips |
| J2 | Diamond with a cross-shaped core | 16x16 | A ring road round a monument |
| J3 | Diamond with four L-shaped pillars | 16x16 | A four-way fight with corner pockets |
| J4 | Chevron | 12x7 | A bend joining two halls; ports at the feet |
| J5 | Plus with a small cross core | 16x16 | A transept hub, four ports |
| J6 | Four-pointed star with a small core | 16x16 | A thin-armed crossing, four ports |
| J7 | Wings with a stub above and a V body | 24x7 | A T-junction: ports at the wingtips and the stub |
| J8 | Inverted V | 14x7 | A fork: ports at the feet and the apex |
| J9 | Great nave with six slab pillars | 35x10 | By far the largest: at most one a level, and only where the plan has the room |
| J10 | Spine | 3x16 | A processional gallery, ports at both ends |

**To fit the lattice** (see the log's open question, which has a default): a box of n blocks is 4n - 1 cells, so the axis of a 2- or 4-block
box (7, 15 cells) is a wall column and no port can sit on it, while a 3- or 5-block box (11, 19 cells) has a block middle on its axis.
The default is to draw the symmetric shapes (J1-J3, J5, J6, J8's apex) on **11 or 19 cells with 3-wide arms**, keeping each design's
look, and to keep 7-cell versions only where the shape needs no port on its axis. A port run is 1 to 3 cells, within one block, with a
cell at index 1 mod 4 along the edge. J9 (35 wide) has no axis port, so trims to 31 (7 blocks); J4, J7, J10 and the secret rooms need no
change but a check. Say what was done in the log and put every change on the sheet for the owner.

### Our additions (track B proposes; the owner approves them on the sheet before they go in the game)

Joiners: **ladder** (two halls with optional rungs), **horseshoe arc**, **hourglass** (two chambers and a 1-wide neck: for a boss, a
vault, a trial), **dogleg** (a 3-wide diagonal joining offset halls), **broken ring** (J2 with four optional spokes), **pinwheel**
(four arms entering a core tangentially), **trident** (three prongs from one wall), **small octagon** (7 and 11 cells).

Places (`ST_PLACE`: intended rooms, not joiners): **cloister** (a loop round a solid yard), **antechamber** (a vestibule before every
coloured vault, so the key's moment has a room), **gauntlet hall** (built for the hazard system), **alcove hall** (1-cell recesses
down both sides: niches, ambushes, torches, mice's holes), **shrine spur** (a 1-wide dead end ending in an altar or a lamp), **sunken
ring** (concentric stepped rings, an arena for the Deep).

## 5. How it is built, and by whom

Two agents, one branch, a track each (section 7). The shared, fixed part is the pure parser: it lives in **`src/crawl/crawl_stamp.c`
and `.h`** (no game state; like `crawl_gear.c`), so the sheet tool links it without the game. The interface is the header and the
`StampDef` of section 4; **A1 puts the header on the branch first**, so B can write data and the sheet against it at once.

### Track A: the engine (owns `crawl_stamp.c/.h`, `game/stamps.c`, and every edit to the existing generator)

- **A1.** `crawl_stamp.c/.h`: parse a `StampDef` to a `Stamp` (`cells[][]`, ports, entrances, doorways, marks, lane); `stamp_xform(const
  Stamp *, int t)` for t 0..7; `stamp_valid(const Stamp *, char *why, size_t)` (the rules above); `stamp_set(kind, &n)` to list a
  kind's stamps with every transform and duplicates dropped. No `rnd`/`G`: the caller passes its random number.
- **A2. Secrets.** In `place_secrets`: take a secret stamp by chain length, step and weight; find a box of whole blocks that holds it
  beside a hall; carve its cells (not the blocks wholesale); the chosen `E` is `Secret.sx/sy`, the chain's `D` replaces `secret_door`'s
  pick. **Option B:** `Secret` gets a slide direction and length 1 (`sd`, `len`), the block slides into the stone beside the entrance;
  `open_secret`, `update_slide`, `slide_covers`, `walls.c`'s draw, the dream's `goals.c` and `human.c`'s grab all read it. Bump nothing in
  the save unless `Secret` is saved (read `docs/SAVES.md` first; say in the log what it says).
- **A3. Joiners and places.** In `gen_level`, before the chambers: choose 1 big and 2 to 3 small joiners and the places the level's
  rules want (an antechamber for each vault; a shrine spur on a long chain), claim their boxes on the plan with `mcarve`-like marks
  (cells, not whole blocks), then let `mz_grow` and the `con[]` pass join them through their ports (an optional port is walled off one
  time in `n`, as the log says). **Fall back** to today's chambers if a stamp cannot fit: no seed may fail to generate. Keep the
  trunk (the start to the farthest room) connected, and keep a vault, a puzzle's wing, a trial's room and the arena able to find a place.
- **A4. Marks.** `dress_level` (and `gen_dress`) reads a stamp's marks: where it says altar, brazier, loot, spawn, it puts one; where
  it says nothing it still dresses by room type as today. Log what each mark became.
- **A5. Switches** (debug build only; add to `docs/HARNESS.md`): `CRAWL_NOSTAMPS=1` (today's generator, byte for byte, for the
  proofs), `CRAWL_STAMP=name` (force one), and a `STAMPS level n: ...` line per level (each stamp, its box, its orientation).

### Track B: the content and the proof (owns `crawl_stamp_data.c`, `tools/stampsheet.c`, the sheets, the runs, the docs)

- **B1.** `src/crawl/crawl_stamp_data.c`: transcribe the owner's twenty drawings as `StampDef`s (the tables above), the trims noted, ports
  added. Add the file to `SRC` in the Makefile after `crawl_stamp.c`. Add nothing to the game's rules.
- **B2.** `tools/stampsheet.c` and `make sheets` entry: `build/stampsheet <dir> [secrets|joiners|places|all|check]` draws each stamp, all 8
  orientations, as a PNG sheet (as `tools/titlemock.c` does), with ports, entrances and marks coloured as the owner's own sheets are,
  and `check` runs `stamp_valid` on every one and prints each failure. **The owner approves the sheet before any stamp is used by the
  game** (a standing rule).
- **B3.** Our additions (section 4), after the owner's are on the sheet; then the places' marks, with A.
- **B4.** The proofs and the numbers: see section 6.
- **B5.** The docs: `docs/catacomb/levels.md` and `secrets.md` (what changed), `docs/HARNESS.md` (the switches), this plan's log.

### Order

A1 (the header, first push) -> B1 and B2 in step with A1's parser -> **the sheet goes to the owner** -> A2 and B3 -> A3, A4 -> B4.
A2 and A3 may not turn on in a build that the owner plays until the sheet is approved.

## 6. Proving it

- **Phase 1 moves nothing** (the parser, the data, the sheet): `sh tools/tidy_check.sh all` against 1.0.0b21.6 must show every object and
  the dream's fixed runs the same, and `CRAWL_NOSTAMPS=1` must match from then on.
- **The sheet check** (`build/stampsheet check`) is clean before A2 starts.
- **Generation never fails:** a loop of `CRAWL_SEED` 1..200 across levels 1, 5, 10, 11, 20, 21, 30, 31, 40 with no fallback loop, no
  unreachable stairs, key, vault or boss door (the harness's existing reachability events say which), no secret room or doorway
  unreachable, no stamp overlapping a vault, wing, trial or the arena.
- **The balance** (a standing rule): eight seeds at the least, sixteen when it matters, against 1.0.0b21.6 in a `git worktree`, the same
  seeds. The autopilot's deaths and its time to clear are the numbers: wider and stepped places have made it worse before (see the README
  on the wider crypt). Put them, good or bad, in `docs/KNOWN-ISSUES.md` and the README's "What has been verified".
- **The sanitizers:** `make asan`, then long runs of the dream and of a scripted player with `ASAN_OPTIONS=detect_leaks=0`.
- **Say how each thing was tested**: harness only, nothing under Wine, CI alone for the Windows and browser builds, nothing by hand.

## 7. Working together

- **One branch, a track each.** `claude/ecstatic-heisenberg-0ljx7r`. Pull with rebase and build before every push; never force-push; change only
  what your track owns (the lists in section 5); if the other track's file must change, say so in the log and let its owner do it.
- **Add to the log only above your own track's end mark**, so the two never collide.
- **The owner's preferences** (they play it by hand and rely on it behaving predictably): back up before an edit that replaces something
  they use; say what a change touched and what it did not; ask before changing an existing file in a way the plan did not name; new
  files in your own track need no asking. Backups of their saves are theirs to make: remind them before a build for them to play.
- **Not for the repo:** the console's codes (`CON_CODES`), model identifiers, anything the owner has not approved as art. British
  spelling and the plain dry voice in anything a player sees; code comments short.
- **Release is the owner's word, one squashed commit to main.** Nothing here is for the page or the README until it is released.
- **If a decision is the owner's** (a trim, a new stamp's look, how many of a thing a level gets), put it as a question with a default in
  the log and carry on with the default; they like options first for big things and defaults for small ones.

## 8. Hand-off log

Each track writes above its own end mark, newest first, dated, one paragraph: what was done, how it was tested, what is next, what was
left open. Do not edit the other's.

### Track A: the engine

- **15 November 2026, A1 done.** `src/crawl/crawl_stamp.h` and `crawl_stamp.c` are on the branch: `stamp_parse`, `stamp_xform` (eight
  orientations; `xf` bit 0 a mirror in x first, bits 1-2 quarter turns clockwise), `stamp_valid`, `stamp_same`, `stamp_set`, `stamp_weight`.
  Tested alone against a made-up set of seven stamps under ASan and UBSan; nothing in the game, the harness or the Makefile reads it.
  **Track B: add `src/crawl/crawl_stamp.c` and your `crawl_stamp_data.c` to `SRC` together** (the parser alone will not link, for want of
  `STAMP_DEFS`). Things B needs to know: ports are one entry a cell, a corner port faces the way its run does not run; a port run is at
  most 3 cells, within one block, and takes in index 1 mod 4 along its edge; an `E` must have its open side outside the box, a `D` must
  have stone or the outside behind it; a joiner or place is 3, 7, 11 or 15 cells a side. `stamp_valid` says which rule a stamp breaks.
  **Open (a finding, with a default; see the owner's section):** the symmetric 16-cell designs cannot put a port on their axis.
  Next for A: nothing begun on A2; it waits for the owner's approval of the sheet.

<!-- TRACK A END -->

### Track B: the content and the proof

- **15 November 2026, B3 begun: nine extra joiners on a sheet for the owner.** Added to `crawl_stamp_data.c`: ladder (15x11), horseshoe
  arc (15x11), hourglass (11x11), dogleg (15x15), broken ring (19x19), pinwheel (11x11), trident (11x11), small octagon at 7 and at 11.
  `build/stampsheet <dir> extras` draws them (`all` does too); the approved secrets and joiners sheets are unchanged, as the tool keeps
  ours apart by their notes. Sheet: `media/sheets/stamps/proposed-extra-joiners.png`. `check` is clean: 34 definitions, 150 oriented.
  Tested: harness only (the parser's rules and a look at the sheet); nothing in the game reads them. **Not drawn yet: the six places**
  (cloister, antechamber, gauntlet hall, alcove hall, shrine spur, sunken ring), which wait for the owner's word on these, and need
  A's marks (A4). Defaults for the owner to change: the ladder's bottom ports and the trident's side ports may be walled off, the rest
  must connect; the dogleg's feet are a block-width apart; the broken ring is pinched at its north-east and south-west corners.
  **A:** none of these need an engine change; the octagon at 7 has its two ports off the axis, which the validator accepts.

- **15 November 2026, the sheet is approved.** The owner saw `proposed-secrets.png` and `proposed-joiners.png` and said "Those look great!"
  I take that as approval of the sheet as it stands, the regridded joiners included. The three questions in the entry below it were not
  answered, so their defaults stand (two required ports on the hubs; the steps and weights as set; no marks in the monument rooms).
  **A2 may now turn on.** Next for B: B3, our extra joiners and places, as new stamps on a second sheet for the owner to approve.
  Not run: nothing new was built or tested.

- **15 November 2026, B1 and B2 done; the sheet is ready for the owner.** `src/crawl/crawl_stamp_data.c` holds 25 stamps (the ten
  secret rooms as drawn, 15 joiners); `tools/stampsheet.c` builds `build/stampsheet <dir> [secrets|joiners|places|all|check]` (`make
  sheets` runs it into `build/sheets/`); the sheet as proposed is `media/sheets/stamps/proposed-secrets.png` and `proposed-joiners.png`.
  `build/stampsheet check` is clean: all 25 pass A's `stamp_valid` in all 8 orientations (118 distinct oriented stamps: secrets 80,
  joiners 38). **Makefile (the one shared place): I added `src/crawl/crawl_stamp.c src/crawl/crawl_stamp_data.c` to the end of `SRC`
  (A said both together), a `build/stampsheet` rule, and the sheet to the `sheets` target.** Tested: the secrets' cells, entrances and
  doorways checked against the owner's picture by script, cell by cell (all 10 match); `make host` builds and links; harness only, nothing
  run in the game (nothing reads the stamps yet). **Facts about the drawings, for the plan's table:** J7 is 22x7, J8 is 14x10 (not 24x7,
  14x7); secrets' sizes in the plan are mixed (floor box for most, with the entrance row for S9 and S10); every secret's box here takes in
  its E and D cells, so S1 is 4x5. **What was done to the joiners** (all on the sheet): J1, J2, J3, J5, J6 are drawn again at 19 and 11
  (A's default (a)), keeping each look: stem 1 wide, crossbar 5 or 3; core crosses; pillars with their wall pockets kept; J5's arms end 3
  wide so a port lies in one block; J6's arms 3 wide at 19 and 1 wide at 11. J4 is 11x7 (centre column cut, feet at 1 and 9). J7 is 19x7
  (stub 3 wide). J8 is 11x11. J9 stays 35 wide (9 blocks) with one row added (11 tall), so the end ports sit on the axis: A's plan said 31,
  but 35 needs no axis port on its width, so I kept the owner's size; A may drop it if it never fits. J10 is 3x15. Ports: N and S `P`,
  W and E `p` on the symmetric ones; J4's feet, J7's wingtips, J8's feet, J9's ends and J10's ends `P`; J7's stub, J8's apex and J9's
  north and south middles `p`. **Questions for the owner, defaults taken:** (1) all four ports of the hubs must connect, or two as now?
  (default: two); (2) steps and weights (the plainer secrets from level 1, S7 to S9 from step 2, S10 from step 4; 19-cell joiners from
  step 2, the nave from step 5); (3) altars, braziers or loot in the monument rooms (J1, J2, J3)? None drawn, so none placed. **Next:**
  the owner's word on the sheet; then B3 (our extra joiners and the places). **Open for A:** `tools/tidy_check.sh` lists its units by
  name and will not know `crawl_stamp`, `crawl_stamp_data` or `stampsheet`; the objects of the game are unchanged by this work.

<!-- TRACK B END -->

### Owner's answers and decisions

- **Open, default taken (15 November 2026): where the symmetric joiners' ports go.** A 4-block (15-cell) box has its axis on a wall column
  and the drawings' arms are 4 wide, so a port cannot sit in one block on the axis. Options: (a) draw J1-J3, J5, J6 on 11 or 19 cells with
  3-wide arms, (b) keep the sizes and offset the ports half a block, (c) taper each arm to 3 cells at the box's edge. **Default: (a).**
  Track B transcribes on that basis and shows the owner the result on the sheet; say so here if the owner picks otherwise.

- 15 November 2026: the four answers in section 2, and "Format this approach so another agent can assist with the build first."
