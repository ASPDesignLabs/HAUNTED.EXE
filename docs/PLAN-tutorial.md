# The tutorial: the first two levels, drawn and taught

**A draft; nothing built** (11 November 2026, on `claude/sweet-carson-qxvyfi` from main at 1.0.0b21.3). The owner gave the brief
and run I level 1's drawing the same day; level 2's drawing is to come. The drawing has been read cell by cell (section 2) and
put beside the game's terms; the questions are in section 12, each with the answer I'd take marked *(recommended)*. Every number
here that isn't the owner's is a default, to be changed by playing. Nothing of it has been played.

*The brief, in a line: levels 1 and 2 of the first run laid out the same every time, from the owner's drawings; a paged screen
that pauses the crypt and teaches as you go; the view taken and turned at a few moments; triggers in the floor that teach, or
turn you back until you have done what the level asks.*

## 1. What was asked

The owner's words, cut to their substance:

- **Fixed levels.** "The first level needs to be modified to be the same exact layout, every time, regardless of seed. Same with
  the second level, as that will be where we introduce the Map as a loot item placed somewhere specific." The drawings are
  "the defining guide": enemies, gold, the key, the healing fountain, locked doors, triggers and the exit are marked on them.
- **A tutorial screen.** "A pop-up Tutorial UI that explains these steps to the player, pausing crypt activity while viewing and
  reading", with several pages and a way to move through them. "The player is largely autonomous outside of the moments when a
  window needs to appear or they pass a specific trigger."
- **Run 1, level 1: the basics.** Movement, interactions, healing, dealing damage, gold, resource management and leaving.
  1. *Set to a path.* The caster opens a door and is asked to walk through flames, which should hurt enough to send them to the
     healing fountain at the hall's end. A free wisp guides them there, and they are pointed that way once past the fire. The
     screen says how to use the fountain with their controls, and that fountains alert the dead. *Covers: navigation, sprint,
     health and status damage, opening doors, basic interactions, the dead's ways.*
  2. *After healing* the view is taken and turned to the hall on their left, and the screen tells them, by their class, to be
     ready to fight what is ahead with their named spell: the button, and how to aim. At the first kill: souls power spells and
     come from many places, the dead among them. *Covers: combat, resources.*
  3. *Exploring.* At trigger 1: explore everywhere; more gold and secrets lie off the way; a locked door's key lies somewhere.
     Taking the gold key: do you remember a locked door? (trigger 2 goes off). Crossing trigger 2 without it: check again, the
     key is near, and the view is turned towards the door that holds it; again each time. Unlocking either gold door turns
     trigger 3 off; crossing it before then: go and find that door, turned round; again each time. Then: leave. *Covers: keys,
     key doors, backtracking.*

## 2. The drawing, read

The drawing is kept as [`docs/tutorial/level1-master.png`](tutorial/level1-master.png), and my reading of it, beside it, as
[`docs/tutorial/level1-read.png`](tutorial/level1-read.png): **look at the second first**, since everything below follows from it.
`tools/tutorial_read.py` reads a drawing like it (`python3 -I tools/tutorial_read.py <png>`: the grid, the cells, the coloured
edges, the dots), for level 2's.

It is drawn to scale on a grid of 52.6 pixels a cell: **22 cells across, 32 down**. Below, x runs east from 0 and y south from
0, as on the drawing; in the game's 45 x 45 map it sits at **(11, 6)**, so add 11 to x and 6 to y (a cell of stone all round).

### What had to change

The game's walls are whole cells; the drawing's are lines. So:

- **Doors** drawn on an edge become the narrow cell beside it, which is always a neck a cell wide: (4,1), (17,15), (3,23),
  (11,26), (15,31), and the gold doors (2,26) and (4,31).
- **Two thin walls** become stone. Between the small room west of the east hall and the hall (x = 14, rows 16-18): column 13
  is stone at rows 16 and 18, its door at (13,17), and the room is 3 x 3. Between the vault and the narrow way (x = 4, row
  26): (4,26) is stone, so the narrow way is a cell wide from row 26 down, as it already was from 27.
- **Triggers** drawn as dashed lines become the cells they close off: trigger 1 is (4,23) and (5,23), entered from the hall
  above; trigger 2 is (14,30), (13,31) and (14,31), before the door to the exit's hall; trigger 3 is (19,30) and (19,31).
- **The fountain and the exit** are faces of stone cells: the fountain on (3,12), faced from (3,11); the exit on (20,29),
  faced from (20,30).
- **Dots** stand where drawn, in cells (most are on grid crossings, so on the corner of four cells), nudged to a third of a cell
  off stone where one was closer: two pieces of gold by the thin walls ((13.0, 18.0) to (12.7, 18.0); (5.0, 26.1) to
  (5.3, 25.9)) and three in the vault, a cell wide. The tables below give them as they will stand.

### The map

```
     x 0         1         2
       0123456789012345678901
 y  0  ##S..#################      S  the start, facing east
    1  ####D#################      .  floor
    2  ##FFF#################      F  the burning floor (section 5)
    3  ##FFF#################      D  a door
    4  ##FFF#################      G  a gold door (locked; the gold key)
    5  ##FFF#################      h  the fountain, in the wall, faced from above
    6  ##...#################      X  the exit, in the wall, faced from below
    7  ##...#################      #  stone
    8  ##...#################
    9  ##...............#####      triggers (cells): 1 (4,23) (5,23)
   10  ##...............#####                        2 (14,30) (13,31) (14,31)
   11  ##...............#####                        3 (19,30) (19,31)
   12  ###h##########...#....
   13  ##############...#....      K  the gold key at (11.2, 24.6)
   14  ##############...#....
   15  ##############...D....
   16  ##########...#...#####
   17  ##########...D...#####
   18  ##########...#...#####
   19  ##############...#####
   20  ####.............#####
   21  ####.............#####
   22  ####.............#####
   23  ...D..################
   24  ...#..####...#########
   25  ..##..####...#########
   26  ..G.#.#####D##########
   27  ..#.#.##.......#######
   28  ..#.#.##.......#######
   29  ..#.#.##.......#####X#
   30  ..#.#..........#......
   31  ..#.G..........D......
```

Its places, as I'll call them: **the start** (row 0), **the burning hall** (x 2-4, rows 2-8), **the long hall** (rows 9-11),
**the east hall** (x 14-16, rows 12-19), **the east room** (x 18-21, rows 12-15), **the side room** (x 10-12, rows 16-18), **the
lower hall** (rows 20-22), **the narrow way** (x 4-5 from row 23), **the west room** (x 0-2, rows 23-31), **the key room** (x
10-12, rows 24-25), **the big room** (x 8-14, rows 27-29, and the bottom hall under it), **the vault** (x 3, rows 26-31) and **the
exit's hall** (x 15-21, rows 30-31).

### Doors

| Door | Where | Kind | Leads |
|---|---|---|---|
| (4,1) | the start's end, on the right as you face east | plain | the burning hall |
| (17,15) | the east hall | plain | the east room |
| (13,17) | the east hall | plain | the side room |
| (3,23) | the narrow way, beside trigger 1 | plain | the west room |
| (11,26) | the big room's north wall | plain | the key room |
| (15,31) | the bottom hall's east end, past trigger 2 | plain | the exit's hall |
| (2,26) | the west room | **gold** | the vault |
| (4,31) | the bottom hall | **gold** | the vault, its other end |

Both gold doors open on the one vault (six cells in a column, its hoard strewn down it), which is why the brief says *either*.

### The dead

Eleven, all drawn the same but four drawn larger (question T5).

| # | Where | Place | Note |
|---|---|---|---|
| 1 | (15.1, 13.0) | the east hall's head | drawn larger. **The only one within the mending's sixteen steps** (13), so the one the fountain calls |
| 2 | (21.0, 13.1) | the east room | |
| 3 | (19.0, 15.0) | the east room | |
| 4 | (16.0, 16.0) | the east hall | drawn larger (a smaller ring) |
| 5 | (11.0, 18.0) | the side room | |
| 6 | (15.0, 20.0) | the east hall's foot | drawn larger (a smaller ring) |
| 7 | (5.1, 22.0) | the lower hall, by trigger 1 | drawn larger |
| 8 | (1.0, 28.1) | the west room | |
| 9 | (5.9, 31.1) | the bottom hall | |
| 10 | (9.1, 31.1) | the bottom hall | |
| 11 | (1.0, 31.1) | the west room | |

### Gold

Thirty-eight pieces: thirty-two in the open and six in the vault.

| Place | Pieces |
|---|---|
| the burning hall | (4.0, 7.0) |
| the long hall | (6.0, 10.0), (12.0, 10.0), (16.0, 10.0), (9.1, 11.1), (3.0, 11.0) by the fountain |
| the east hall | (15.0, 14.1), (16.0, 18.0), (16.0, 19.0) |
| the east room | (21.0, 14.0), (20.1, 15.0), (21.0, 15.0) |
| the side room | (11.0, 17.1), (12.7, 18.0) |
| the lower hall | (8.1, 21.0), (13.0, 22.1) |
| the narrow way | (5.0, 24.1), (5.3, 25.9) |
| the west room | (0.9, 24.1), (0.9, 27.1), (1.0, 29.1), (1.0, 30.1) |
| the key room | (11.0, 25.1), (12.0, 25.1), beside the key |
| the big room and bottom hall | (9.1, 28.1), (13.1, 28.1), (12.0, 30.1), (7.0, 31.1), (14.1, 31.1) |
| **the vault** | (3.4, 26.5), (3.4, 27.0), (3.6, 27.4), (3.4, 28.4), (3.7, 29.2), (3.3, 29.8) |
| the exit's hall | (20.0, 31.0), (18.9, 31.1), (21.1, 31.1) |

### What the layout already does

Measured on the cells (steps, doors open):

- The start to the fountain is 14 steps; the start to the exit 68; to the key 63.
- **Trigger 1 can't be missed**: the lower hall's only way down is the narrow way, through it. Trigger 2 can't either: the way to
  the exit runs through it. The key room is off the big room, reached after trigger 1 and before trigger 2.
- The key is 14 steps from the gold door at (4,31) and 21 from the one at (2,26); 17 from the exit.
- The gold door at (4,31) stands in the bottom hall on the way to trigger 2, so a person has walked past it before the key's
  page asks whether they remember a locked door.
- Only the first of the dead hears the fountain (13 steps; the next nearest is 18).
- The start faces east down a passage three cells long, with the door on the right at its end: the first page says so.

## 3. Where it sits

- **Which levels.** Levels 1 and 2, the first time down. Past level 40 the runs come round; levels 41 and 42 are generated as
  today. A replay of level 1 or 2 at the Accursed Portal is the drawn level, with no lessons.
- **Whose.** A person's run: BEGIN A NEW RUN on the title, her room (her question, the class, first), then level 1. A page
  opened at level 1, and a death (back to her room before level 1), meet the drawn level again; the lessons come again only as
  T2 settles. Whether the dream gets the drawn levels too is T1.
- **Her room before level 1** is as it is (T13 asks about a page after level 1).
- **The release** is T14.

## 4. A level from a drawing

### In the source

Each drawn level is a map as strings in `scene_crawl.c`, as the bosses' arenas are (`BossDef.map`, `stamp_arena`), with the
legend above (`#`, `.`, `F`, `D`, `G`, `h`, `X`, `S` and a lane mark), and a table beside it of what stands in it in cells
(the dead, the gold, the key, the triggers, the lights, room types), so that the drawing and the source can be laid side by side.
`drawn_level(level)` says whether a level is one: 1 and 2 of the first descent, and not under `CRAWL_DRAWN=0` in the debug build
(the generated levels back, for balance runs against an older build).

### The build

`start_level` calls `gen_drawn()` in place of `gen_level()` on a drawn level. What `start_level` does after `gen_level` is kept,
skipped or answered from the drawing:

| Pass | On a drawn level |
|---|---|
| the start (`G.cx`, `G.cy`, `G.h` at random) | the drawing's S, facing as drawn (east) |
| `place_items` | the drawing's gold, of fixed kinds and worth; the key; **no potion** |
| `place_exit` | the drawing's X |
| `place_altar`, `place_sites`, `pz_make`, `pz_hide`, `tri_place` | none (today levels 1 and 2 have none of these but one of the run's fixed hazards, a site) |
| `place_rests` | the drawing's fountain only (section 6.6, T15) |
| `place_marked`, `place_fights` | none |
| `ward_make` | none (no Wraith before level 9) |
| `spectres_set` | none |
| `place_creatures` (the budget) | the drawing's dead, rank I, each a resident at its post (`RL_POST`, facing its room's way in); no wanderers |
| the respawn ("something wakes ahead") | none while the level is drawn |
| secrets (`place_secrets`, three links on level 1 today) | none (T7) |
| wall skulls (from 3), the gilded (from 7) | not on these levels anyway |

`G.nlinks` is 0, so "every link found" must not pay a coin (`COIN_ALL`) nor count as a level's secrets all found, on a level that
had none; the tally's and the reckoning's secret lines say nothing.

### The look

Run I's look as any of its levels (`G.faced`): rooms are `Room` records with their run I types, the halls `TH_CORRIDOR`, and
`lay_faces` lays the walls; `dress_level` lays bones, chains, props and torches. **Its dice are a fixed seed** for the drawn level
(the run's `G.rs` kept and put back after), so the props and torches lie the same every time and nothing else changes. The room
types, for the owner to change:

| Place | Type |
|---|---|
| the east room | burial vault |
| the side room | ossuary |
| the key room | ossuary |
| the west room | undercroft |
| the big room | chapter house |
| the vault | knights' vault |
| the halls, the start, the exit's hall | halls |

Lights beside the generator's torches: the burning floor's own (section 5), the fountain's candles, a lamp in the key room so the
key is seen from its door, the exit's runes. Fog as level 1 has it (little).

### The vault

One vault (`G.nvaults` 1, gold) with **two doors**: both `locked` 1, colour gold; `G.vault_door[0]` is the one at (4,31), nearer
the key, which is the one the dream and a key wisp go by (`past_vaults`). But `open_door` **spends the key** on the door it opens
(`G.has_key` cleared), so today one gold key would open one of the two and leave the other shut for good, from both sides. I'd
make the two one lock: opening either unlocks both (T19).

### The dream's lanes

The dream walks only lanes (`apass`), and lanes are every cell with x or y = 2 (mod 4) on the generator's grid of blocks. The
drawing isn't on that grid, and can't be put on it: its doors need lanes on rows 15, 17, 23, 26 and 31, which no one offset gives.
So on a drawn level `lane()` reads a mask drawn with the level (`G.lane_mask`) and `lane_dist` a field made from it, and the
generator's placers that do sums on the grid (sites, puzzles, the arena's) aren't run there. The mask I'd draw (`+`), connected
from the start, through every door, to the fountain's front, the key and the exit's front, with every floor cell within a step of
it (so every piece of gold is in the dream's reach, `lane_dist` 1):

```
       0123456789012345678901
    0  ##S++#################
    1  ####D#################
    2  ##Fff#################      f  a lane through the burning floor
    3  ##FfF#################
    4  ##FfF#################
    5  ##FfF#################
    6  ##.+.#################
    7  ##.+.#################
    8  ##.+.#################
    9  ##.+.............#####
   10  ##.++++++++++++++#####
   11  ##.+...........+.#####
   12  ###h##########.+.#.+..
   13  ##############.+.#++++
   14  ##############.+.#.+..
   15  ##############.++D++++
   16  ##########.+.#.+.#####
   17  ##########+++D++.#####
   18  ##########.+.#.+.#####
   19  ##############.+.#####
   20  ####...........+.#####
   21  ####+++++++++++++#####
   22  ####+............#####
   23  .++D++################
   24  .+.#.+####.+.#########
   25  .+##.+####.+.#########
   26  .+G+#+#####D##########
   27  .+#+#+##...+...#######
   28  .+#+#+##+++++++#######
   29  .+#+#+##...+...#####X#
   30  .+#+#+.....+...#....+.
   31  .+#+G++++++++++D++++++
```

The debug map (`CRAWL_MAP=1`) prints the lanes as it does, and its `MAP lanes reachable` line is the check.

## 5. The burning floor

- **What it is.** In the Order's crypts: lamp oil spilled across the hall and alight, the whole width of it for four cells. Drawn
  with the floor fires the game has (`Burn`'s flames, the ember imps' and the cask's), thicker, over a scorched floor; lit by its
  own flickering light, so it shows through the door's crack before it opens; a crackle and a roar heard near it.
- **What it does.** A share of the caster's health each half second the body is in it, and On Fire on the first touch (C2's
  `af_ignite`: the harm again over four seconds after leaving). The targets (defaults): a walk through, about **three fifths of
  health left at the far side and two fifths once the burning is done**; a sprint through, about half again of what a walk leaves.
  **It never takes you under a quarter of your health**, the burning after included: the page says it will hurt, and it must not
  kill. The numbers are set in the harness.
- **After.** Once you are past it and its page is shut, it burns low and goes out over twenty seconds, leaving the scorch:
  nothing lies behind it, and it shouldn't punish a person who goes back to look.
- **The dead** keep out of it (none are near). **The dream** walks through it: it is marked soft (`HZ_SOFT`) on `hz_dmap`, as a
  stope's firedamp is, so `decide` takes it, there being no other way.

## 6. The lessons

### 6.1 State

`TU`, static, kept while the program runs: which lessons are shown this level, which triggers are on, a beat waiting, the page.
A level isn't saved part way through (the game saves at her room), so a person who quits half way through level 1 comes back to
her room before it, and the level's lessons start again. What lasts is a byte in the prefs record (6.7).

### 6.2 Beats

A beat is a moment that opens a lesson (a screen of one or more pages). Each comes once unless marked *again*.

| Beat | When | The game | Lesson |
|---|---|---|---|
| B1 arrival | level 1 opens, its card gone | | WALKING, DOORS (2 pages) |
| B2 the door | the door at (4,1) opens | | FIRE |
| B3 through | the body two cells past the flames (y 7.0) | the view turned south; a free wisp to the fountain | HEALTH, BURNING, MENDING (3) |
| B4 mended | mending ends having mended some (any way: full, moved, a blow); or, not mended, the body four cells east of the burning hall (x 9) | the view turned east | the class's (2) |
| B5 first blood | the first of the dead slain by the caster | | SOULS |
| B6 trigger 1 | into its cells | | EVERYWHERE, LOCKS (2) |
| B7 the key | the gold key taken | trigger 2 off | THE GOLD KEY |
| B8 trigger 2 *again* | into its cells, the key not held | the view turned to the key room's door (11,26), set back half a cell | NOT YET (shorter after the first) |
| B9 the gold door | a gold door opened | trigger 3 off | THE HOARD, AND THE WAY OUT |
| B10 trigger 3 *again* | into its cells, no gold door opened | the view turned west, set back half a cell | THE GOLD DOOR (shorter after the first) |

Lessons that fall together come one after the other. A trigger that turns you back fires again only once you have been out of its
cells. **The turnings wait while anything hunts you within six cells**: a person isn't turned away from a fight; the beat comes
when it is over. Level 1's lessons are counted done when the level is cleared.

### 6.3 Taking the view

`tu_steer(angle, back)`: for half a second the person's keys, mouse and sticks move nothing; the view eases round to the angle
(smooth at both ends), and with `back` the body steps back half a cell, out of the trigger; a sprint, a charge and a mend in
progress are let go. Then the page. It is the only time the game turns a person's head, and it is short.

### 6.4 The screen

`UI_TUTOR`, a menu like the rest, so the crypt waits under it as it does under the L list or the wardrobe. A slab in the run's
stone (docs/PLAN-ui.md's kit: `ui_slab`, the heading in the inscribed face, the words in the small one), in the middle of the
view, the crypt dimmed round it; up to eight lines a page, about 32 letters a line; keys drawn as caps by `hint8`, so a pad's
person reads buttons (`pk`, and `PK_JOB` for the layout they chose); a pip a page; a small picture where one helps (the class's
spell, the key, a soul, the wisp), from the icons the kit has. At its foot: *A/D PAGES  E ON* (a pad: the D-pad or bumpers, A),
*ESC* (B) shuts the lesson. It takes no key for its first 0.4 s, so a key already down can't skip it unread. Sheet first (step 4).

**Skipping.** The first lesson's foot also offers *Q NO LESSONS* (the pad's X), asked twice; it ends them for good (6.7).

### 6.5 The wisp to the fountain

A wisp kind of its own, `WK_MEND` (the place to mend's front cell; free; the candles' warm white), kindled at B3 in front of the
caster: *A WISP KINDLES. IT KNOWS WHERE TO MEND*. It hovers there until you are within a cell and a half, as the exit's does.
Nothing else casts it.

### 6.6 The fountain

The drawing's healing fountain is the run's **place to mend** (`T_REST`, docs/PLAN-ai.md section 28): in run I an arched niche
with a ledge of candles and a seep. It already does what the brief asks: E facing it kneels, standing still mends you to three
quarters over twenty seconds from nothing, and **every second of it is a noise sixteen steps off that tells the dead where you
are**. A walk through the fire leaves enough to mend. (T15 asks whether the owner wants a fountain of its own.)

### 6.7 What lasts

The prefs record (`SLOT_PREFS`: the pad's layout, the title's scene) gains a byte, `PL.tut`: level 1's lessons done, level 2's,
skipped, and *again* (the lessons asked for on the next new run). `PL_BYTES_TUT` 25; an older build reads its first 24 bytes and
writes 24, which drops it, as the title's byte was dropped going back. Windows keeps the record as `PadMap`; the browser's page
reads and writes the same record (`web/haunted.js`) and must keep the extra byte; the harness writes `prefs.bin`.

The title's **HELP** (still *to be written*, `title_help`) becomes the lessons to read again, page by page, and a row *THE
LESSONS ON A NEW RUN: YES / NO*.

### 6.8 Beside it

- The controls card (the hints shown six seconds as a person takes over) is held back on level 1 while the lessons run: the first
  page says the same.
- A creature's caption (`name_newcomers`), toasts and the level's card wait while a page is up.
- Death while the lessons run: T4.
- Debug: `CRAWL_TUT=0|1` (no lessons, or lessons whatever the prefs say), `CRAWL_TUTBEAT=k` (begin at beat k, set where it
  happens), `CRAWL_DRAWN=0`; `EVENT tut beat <k>`, `tut page <lesson> <n>`, `tut steer`, `tut skip`, `tut trigger <n> on|off`.

## 7. The lessons' words

Drafts, mine, in the game's voice, for the owner to change. In capitals, as the game prints them; 32 letters a line at most.
Keys are written as on the keyboard; a pad's person reads their buttons in their place (`pk`). *LESSON* is the heading.

**B1. WALKING**
```
WASD WALKS. THE MOUSE TURNS YOU.
SHIFT SPRINTS FORWARD, A WHILE:
A SLIM BAR OVER THE TABLET SAYS
HOW LONG, AND FILLS AGAIN AFTER.
```
**B1. DOORS** (with *Q NO LESSONS* at its foot)
```
E OPENS DOORS, TAKES WHAT LIES
ABOUT AND USES WHAT CAN BE USED.
THIS PASSAGE ENDS IN A DOOR ON
YOUR RIGHT. OPEN IT.
```
**B2. FIRE**
```
THE ORDER'S LAMP OIL HAS SPILLED
AND CAUGHT. THERE IS NO WAY
ROUND. GO THROUGH IT SPRINTING:
THE LESS TIME YOU GIVE IT, THE
LESS IT TAKES. IT WILL HURT.
IT WILL NOT KILL YOU.
```
**B3. HEALTH**
```
THE TABLET UNDER THE VIEW HOLDS
YOUR HEALTH, YOUR SOULS, YOUR
KEYS AND THE SPELL IN HAND. IT
SLIDES AWAY WHEN NOTHING
CHANGES; H BRINGS IT BACK.
```
**B3. BURNING**
```
THE FLAME ON THE TABLET'S EDGE
SAYS YOU ARE STILL ALIGHT. IT
BURNS ON A FEW SECONDS MORE.
COLD, POISON AND A BLOW TO THE
HEAD LEAVE MARKS OF THEIR OWN.
```
**B3. MENDING**
```
THE WISP KNOWS A PLACE TO MEND.
FOLLOW IT, FACE THE NICHE AND
PRESS E TO KNEEL. STAND STILL
AND IT MENDS YOU, TO THREE
QUARTERS. THE DEAD HEAR IT.
BE READY.
```
**B4. THE WITCHFINDER** (the class's name, whichever it is). Its first line is `SOMETHING HEARD YOU MEND.`, or, if they walked on
unmended, `SOMETHING WAITS AHEAD.`; then the class's spell:

| Class | Its lines |
|---|---|
| the Witchfinder | `YOUR FIRE BOLT FLIES WHERE YOU` / `LOOK AND BURSTS ON WHAT IT` / `MEETS. NOW AND THEN IT SETS` / `THEM ALIGHT.` |
| the Anatomist | `YOUR FROST SHARD FLIES WHERE` / `YOU LOOK. EACH ONE CHILLS WHAT` / `IT STRIKES AND SLOWS IT; FOUR` / `AND IT FREEZES.` |
| the Galvanist | `YOUR LIGHTNING STRIKES AT ONCE` / `WHAT STANDS NEAREST THE MIDDLE` / `OF YOUR VIEW. WITH NOTHING` / `THERE, IT FIZZLES. NOW AND` / `THEN IT STUNS.` |
| the Plague Doctor | `YOUR BLIGHT FLASK IS LOBBED` / `OVER WHAT IS IN THE WAY TO` / `WHAT YOU LOOK AT, AND BURSTS,` / `POISONING WHAT IT SPLASHES.` |
| the Sin-Eater | `YOUR SOUL DRAIN STRIKES AT ONCE` / `WHAT STANDS NEAREST THE MIDDLE` / `OF YOUR VIEW, AND MENDS YOU BY` / `SOME OF WHAT IT TAKES, FOR A` / `LITTLE OF YOUR SOULS.` |
| the Headsman | `YOUR BONE SPEAR FLIES FAST AND` / `STRAIGHT, AND TAKES A BONE A` / `THROW. WITH NONE LEFT, YOU` / `THROW SPLINTERS.` |

and last:
```
CLICK CASTS.
```
(The Headsman isn't offered at the start, Physical being shut till the Lampstand's first gift, but a page of his may come back to
level 1 by a death if T2 lets the lessons come again; his lines are there for that.)

**B4. THE DEAD**
```
THEY THROW WHAT THEY ARE MADE
OF. STEP ASIDE FROM WHAT COMES,
AND KEEP THEM IN FRONT OF YOU. A
BLOW WHILE YOU CAST CAN BREAK
THE SPELL.
```
**B5. SOULS**
```
WHAT YOU SEND BACK LEAVES ITS
SOUL WHERE IT FELL, A FEW
SECONDS: WALK THROUGH IT. SOULS
PAY FOR STRONGER SPELLS, FOR
WISPS (L, A SOUL EACH) AND FOR
MUCH IN HER HALL. URNS HOLD THEM
TOO. YOUR FIRE BOLT, AS IT IS
NOW, COSTS NOTHING.
```
(the spell's name by the class.)

**B6. EVERYWHERE**
```
THE WAY ON IS SELDOM THE ONLY
WAY. HALLS BRANCH, AND ROOMS
KEEP GOLD; GOLD BUYS HER WARES
BETWEEN LEVELS. DEEPER DOWN,
CRACKED STONE GIVES TO E AND
OPENS ON SECRETS.
```
**B6. LOCKS**
```
A DOOR BANDED IN GOLD, SILVER
OR CRIMSON IS LOCKED. ITS KEY
LIES SOMEWHERE ON THE SAME
LEVEL. FIND IT, AND REMEMBER
WHERE THE DOOR WAS.
```
**B7. THE GOLD KEY**
```
DO YOU REMEMBER A DOOR BANDED
IN GOLD? IT WILL OPEN FOR YOU
NOW. WHAT IS LOCKED AWAY IS
WORTH THE WALK BACK.
```
**B8. NOT YET** (then, each time after: `THE KEY IS STILL OUT THERE.`)
```
NOT YET. THERE IS A KEY ON THIS
LEVEL, AND IT IS NEAR: BEHIND A
DOOR YOU HAVE NOT OPENED.
```
**B9. THE HOARD, AND THE WAY OUT**
```
WHAT WAS LOCKED AWAY IS YOURS.
WHEN YOU HAVE IT, THE WAY OUT IS
EAST, PAST THE LAST DOOR: RUNES
IN THE WALL. FACE THEM, PRESS E.
WHAT YOU LEAVE BEHIND IS SHOWN
YOU BEFORE YOU GO.
```
**B10. THE GOLD DOOR** (then, each time after: `THE GOLD DOOR IS STILL SHUT.`)
```
YOU HOLD THE GOLD KEY AND HAVE
NOT USED IT. GO BACK: THE DOOR
IT OPENS IS WEST OF HERE.
```
**After level 1, in her room** (if T13 takes it). **HER WARES**
```
GOLD BUYS WHAT STANDS ON HER
PEDESTALS. FACE ONE AND PRESS E
TO SEE WHAT IT WOULD CHANGE.
THE WAY ON IS THE FAR DOOR.
```

## 8. Level 2 (its drawing to come)

What is known: it is drawn as level 1 is, read the same way (`tools/tutorial_read.py`), built by the same code, and **the map lies
in it as loot, somewhere drawn**.

- **The map as a thing on the floor.** A new item, `IT_MAP`, drawn as her map ware is (`SP_MAPWARE`, the floating scroll), lit so
  it is seen. Taking it sets `RUN.has_map` (kept through death and her claim, as a bought one is), so she no longer sells it.
  If the map is already held (bought, or a later run's death and return), its place holds gold instead (T18). Its lesson, **THE
  MAP**:
```
A MAP. M UNROLLS IT, AND YOU
WALK ON WITH IT OPEN. IT DRAWS
WHAT YOU SEE AND WHERE YOU HAVE
WALKED. THE ARROWS MOVE IT. IT
IS YOURS NOW, THROUGH DEATH.
```
- **What else level 2 teaches** is the owner's (T17). Candidates, from what a person meets early and isn't told: the L list of
  wisps (a soul each, now that they have souls); a cracked wall and a secret; the bestiary (B) once a second kind is met; the
  wardrobe (Tab) once a piece is worn; the reckoning at the exit. Whatever the drawing marks, I'd teach that and no more.

## 9. The dream

If the drawn levels are the dream's too (T1 a), it walks them on the lanes of section 4, crosses the fire (section 5), mends at
the fountain when under two fifths as it does anywhere (`rest_goal`), takes the key and opens the vault (`key_goal`, both doors
gold), and leaves. It has no lessons, triggers or turnings. Its balance runs from level 1 change for those two levels only; a new
baseline is made in the checks (step 8), and `CRAWL_DRAWN=0` gives the old levels back to compare with older builds.

## 10. What it touches

- `start_level`: the drawn branch, and the passes it skips (section 4); `gen_drawn` and the maps and tables, new.
- `lane`, `lane_dist`: the mask on a drawn level.
- The burning floor: a hazard of its own or `Burn`s that never go out on their own (built as whichever reads more plainly), its
  floor on health, `hz_dmap`'s soft mark.
- `wisp_target`, `summon_wisp`: `WK_MEND`.
- The lessons: `TU`, `UI_TUTOR` (`update_tutor`, `render_tutor`), `tu_steer` (in `update_human`, before the walk), the beats'
  hooks in `open_door` (B2, B9), `rest_end` (B4), `kill_mob` (B5), `collect` (B7) and a cell check each frame (B3, B4, triggers).
- `human_begin` (the controls card held back), `name_newcomers` and `toast` (waiting under a page).
- `pl_read`, `pl_write`, `web/haunted.js` (the prefs byte); `title_help` (HELP).
- `death_costs` (T4); the coin and the secrets' lines with no links (section 4).
- Level 2: `IT_MAP` (`collect`, the sprite, the item's light).
- No save format changes: nothing of it is in a run's save, only the prefs byte.

## 11. Steps, once it is settled

1. **The reading settled.** The owner looks at `docs/tutorial/level1-read.png` and answers section 12.
2. **The drawn level in play** (level 1): the map and table, `gen_drawn`, the skipped passes, room types, the fixed seed, the
   lanes, the vault with two doors; `CRAWL_DRAWN=0`. Frames from the game for the owner: the debug map, and the start, the
   burning hall, the fountain, the key room, the vault and the exit.
3. **The burning floor**, its numbers set in the harness by a scripted walk and sprint through it.
4. **The screen's sheet** (`build/uisheet tutor`, in `make sheets`): every page of section 7 in keys and in a pad's buttons, with
   the fit checks failing on any overflow; `docs/tutorial/pages.png`, for approval.
5. **The machinery**: `TU`, the beats, triggers, `tu_steer`, `WK_MEND`, the prefs byte, skipping, the title's HELP, the debug
   switches.
6. **The words in**, every lesson, the classes' lines, her room's page if wanted.
7. **Level 2**, once drawn: read, built, the map as loot, its lessons.
8. **The checks**: a scripted person (keys, and a scripted pad) through every branch: the key first; trigger 2 without it, twice;
   trigger 3 without a gold door, twice; through each gold door; a fight under a turning (it waits); skipping; the lessons again
   from HELP; a death under T4's answer; frames of every page. The dream on the drawn levels on sixteen seeds (no stall, every
   level cleared, the median time), and from level 1 against 1.0.0b21.3 for the first 3,000 s, five classes, as J21 was. ASan on
   a scripted person and on the dream. The smoke test, the browser's smoke test, and the Windows build by CI. The README's *What
   has been verified*, the page's *Beta status*, a preview strip of the lessons, and the release notes.

## 12. Questions for the owner

1. **T1. Whose levels.** (a) Everyone's: the dream plays the drawn levels too, on the lanes of section 4, so level 1 is level 1
   whoever plays it *(recommended: what "every time, regardless of seed" says, and sleepwalking needs the lanes anyway)*. (b) A
   person's runs only; the dream's levels 1 and 2 stay generated, and the screensaver keeps its variety there.
2. **T2. When the lessons come.** (a) On a person's first run on this computer, and again whenever *THE LESSONS ON A NEW RUN*
   is set under the title's HELP *(recommended)*. (b) On every new run, with the skip. (c) Every time levels 1 and 2 are played.
3. **T3. Skipping.** (a) The first lesson offers *Q NO LESSONS*, asked twice *(recommended)*. (b) No skip.
4. **T4. Dying while the lessons run** (the dead can still kill a careless first-timer). (a) The old mercy on a drawn level until
   its lessons are done: the level again from its start, nothing lost, the lessons already shown not shown again *(recommended)*.
   (b) Death's cost, as on any person's run.
5. **T5. The four larger marks** (the dead 1, 4, 6 and 7). (a) Ordinary, all alike; the first is the one the fountain calls
   *(recommended)*. (b) They mean something (stronger, a pair, waiting asleep): say what.
6. **T6. Which dead, and the Plague Doctor.** Skeletons are the run's own, and shrug off poison (x0.5), so the Plague Doctor's
   first fight is the hardest of the five. (a) Skeletons, rank I, and on the drawn levels every element bites them alike, so each
   class's first fight is fair; the bestiary still shows the kind's own *(recommended)*. (b) Skeletons as they are, and the
   Plague Doctor's page says they shrug off poison. (c) Some marks bats (weak to poison): say which.
7. **T7. Secrets on level 1.** (a) None, as drawn; B6's page mentions them for deeper down *(recommended)*. (b) One, where the
   owner marks it.
8. **T8. The rest of a level 1** (things that want a spell, the potion, spectres, the run's fixed hazards). (a) None of it: the
   drawing is the whole level, with the run's dressing laid on a fixed seed *(recommended)*. (b) Keep the potion.
9. **T9. The fire.** (a) As section 5: about two fifths of health left after a walk through, more for a sprint, never under a
   quarter, and out twenty seconds after you're past *(recommended)*. (b) It burns all level. (c) Other numbers: say.
10. **T10. "After healing."** (a) As B4: when mending ends having mended some, or four cells on if they walked past it unmended
    *(recommended)*. (b) Only once they have mended; until then nothing more comes.
11. **T11. Turning back.** (a) As B8 and B10: the view turned in half a second, the body set back half a cell, then the page;
    again each time, waiting while anything hunts within six cells *(recommended)*. (b) A door that won't open instead of a
    turning.
12. **T12. "Then advise them to leave."** (a) At once, as the gold door opens (B9) *(recommended)*. (b) As they come out of the
    vault.
13. **T13. Her room after level 1.** (a) One page as it opens, on gold and her wares (section 7's last) *(recommended)*. (b)
    Nothing: her prompts already say it. (c) A page before her question at the start as well.
14. **T14. The release.** (a) A point release, **1.0.0b21.4**, when level 2 is drawn and built; the art pass and the lower storey
    keep 1.0.0b22 and b23 *(recommended)*. (b) **1.0.0b22**, ahead of the art pass (which moves to b23, the lower storey to b24).
    (c) After the art pass.
15. **T15. The fountain.** (a) The run's place to mend, as section 6.6: it already mends and calls the dead *(recommended)*. (b) A
    fountain of its own (new art), mending at once to full.
16. **T16. The words.** Section 7's are drafts; change any, or (a) take them and change them by playing.
17. **T17. What level 2 teaches** beside the map: (a) whatever its drawing marks, no more *(recommended)*. (b) Some of section
    8's candidates: say which.
18. **T18. The map already held** when level 2 comes: (a) its place holds gold *(recommended)*. (b) Nothing there.
19. **T19. The vault's two gold doors and one key.** A key is spent on the door it opens. (a) The two doors are one lock: either
    opened, both are *(recommended)*. (b) The other stays shut: the vault is left by the door you came in by. (c) A second gold
    key somewhere: say where.

## 13. Hand-off log

- **11 November** (`claude/sweet-carson-qxvyfi`, from main at 1.0.0b21.3). The owner gave the brief and run I level 1's drawing.
  Read it cell by cell (`tools/tutorial_read.py`, written for it; the reading drawn as `docs/tutorial/level1-read.png` beside
  the drawing, kept as `level1-master.png`), fitted it to whole cells (section 2), measured it, drew the dream's lanes, and wrote
  this draft with its questions. Nothing built. Next: the owner's answers, and level 2's drawing.
