# The tutorial: the first two levels, drawn and taught

**Settled; being built** (11 November 2026, on `claude/sweet-carson-qxvyfi`, from main at 1.0.0b21.3 with 1.0.0b21.4 merged in
since). The owner gave the brief and run I level 1's drawing, answered the first round (T1-T19, section 13), gave level 2's drawing
and brief, and answered the second round (T20-T24) the same day. It is **1.0.0b21.5** (T14). Both drawings are read cell by cell
(sections 2 and 3). Every number here that isn't the owner's is a default, to be changed by playing; the defaults taken on level 2
are listed in section 13. Section 12's steps are being built in order; the hand-off log (section 14) says how far.

*The brief, in a line: levels 1 and 2 of the first run laid out the same every time, from the owner's drawings; a paged screen
that pauses the crypt and teaches as you go; the view taken and turned at a few moments; triggers in the floor that teach, or
turn you back until you have done what the level asks; on level 2, an ice field, a switch, a one-way gate, two keys, a secret and
the map, which you must have to leave.*

## 1. What was asked

The owner's words, cut to their substance.

**Both levels.** "The first level needs to be modified to be the same exact layout, every time, regardless of seed. Same with the
second level, as that will be where we introduce the Map as a loot item placed somewhere specific." The drawings are "the defining
guide". "A pop-up Tutorial UI that explains these steps to the player, pausing crypt activity while viewing and reading", with
several pages and a way to move through them. "The player is largely autonomous outside of the moments when a window needs to
appear or they pass a specific trigger."

**Run 1, level 1: the basics** (movement, interactions, healing, dealing damage, gold, resource management, leaving).

1. *Set to a path.* The caster opens a door and is asked to walk through flames, which should hurt enough to send them to the
   healing fountain at the hall's end. A free wisp guides them there, and they are pointed that way once past the fire. The screen
   says how to use the fountain with their controls, and that fountains alert the dead. *Covers: navigation, sprint, health and
   status damage, opening doors, basic interactions, the dead's ways.*
2. *After healing* the view is taken and turned to the hall on their left, and the screen tells them, by their class, to be ready to
   fight what is ahead with their named spell: the button, and how to aim. At the first kill: souls power spells and come from many
   places, the dead among them. *Covers: combat, resources.*
3. *Exploring.* At trigger 1: explore everywhere; more gold and secrets lie off the way; a locked door's key lies somewhere. Taking
   the gold key: do you remember a locked door? (trigger 2 goes off). Crossing trigger 2 without it: check again, the key is near,
   and the view is turned towards the door that holds it; again each time. Unlocking either gold door turns trigger 3 off; crossing
   it before then: go and find that door, turned round; again each time. Then: leave. *Covers: keys, key doors, backtracking.*

**Run 1, level 2** (elemental switches and objects, more of the dead, two keys, the map, minor scripted events, secret areas).

- "This level starts like any other, without an intense walkthrough at the start, just a UI that suggests the named class explore
  the area."
- "Two keys act as the suggested path through the level, encouraging more backtracking."
- "The player can only leave once the Map has been acquired; a trigger in front of the exit door should tell the player to go back
  and look for it if they have not found it yet."
- "The Map room is designed to only be accessed via the southern entrance, with a one-way gate at the north. An elemental switch
  allows the player to deactivate the ice field after they strike it three times. Once the player acquires the Map the ice field is
  re-enabled, turning the player to face it as it does, taking movement from them for two seconds while it settles, then turning
  them back toward the gate at the north. We then offer a Tutorial UI that explains how to use the map and its functions. Trigger 2
  acts as a stop, routing the player back to the edge of the ice damage area from where they entered, before they freeze."
- "When the player picks up the Gold Key we turn them toward the secret entrance's cracks, alerting them to their presence and asking
  them to inspect them closer."

## 2. Level 1's drawing, read

The drawing is kept as [`media/sheets/tutorial/level1-master.png`](../media/sheets/tutorial/level1-master.png), and my reading of it, beside it, as
[`media/sheets/tutorial/level1-read.png`](../media/sheets/tutorial/level1-read.png): **look at the second first**, since everything below follows from it.
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
    2  ##FFF#################      F  the burning floor (section 6)
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

Eleven, all drawn the same but four drawn larger (ordinary, T5 a).

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

## 3. Level 2's drawing, read

Kept as [`media/sheets/tutorial/level2-master.png`](../media/sheets/tutorial/level2-master.png), my reading beside it as
[`media/sheets/tutorial/level2-read.png`](../media/sheets/tutorial/level2-read.png) (look at it first). A grid of 48.3 pixels a cell: **15 cells across, 35
down**, north up as the drawing's arrow says. In the game's map it sits at **(15, 5)**. It has no thin walls: every wall is a whole
cell already, so only the doors move onto their necks.

### The map

```
     x 0         1
       012345678901234
 y  0  ##S...#########      S  the start, facing east
    1  ####..#########      .  floor
    2  ###...#########      I  the ice field (section 7)
    3  ##.....########      G  the gold door;  C  the crimson door;  D  a plain door
    4  #.......#######      =  the elemental gate, one way: opens from the south only
    5  #...#.........#      h  the fountain, in the wall, faced from above
    6  #..###........#      X  the exit, in the wall, faced from below
    7  #...#.........#      c  the secret's cracked wall, faced from above
    8  #...#...####..#      #  stone
    9  ##..#..#####..#
   10  ###.#.##X####=#      triggers: 1 (12,9) (13,9), entered from the north
   11  ..G.#.#...#....                2 the line between rows 17 and 18 across the ice
   12  ..###.#...#....                x (7,12) (8,12) (9,12): mine, the exit's (T20)
   13  ..#...#...#....
   14  ..#.#.#...#....      K  the gold key (7.6, 30.8); the crimson key (1.0, 19.6)
   15  ..#.#.#...#....      M  the map (12.8, 15.4)
   16  ..#...#...#IIII      sw the elemental switch (14.3, 11.6)
   17  ..#..##...#IIII
   18  ..#.h##...#IIII
   19  ..#.#.C...#IIII
   20  ..#.#.######II#
   21  ##..#..#####II#
   22  #...#...####II#
   23  #...#.........#
   24  #..###........#
   25  #..###........#
   26  #...#.........#
   27  #.......#####D#
   28  ##.....##.....#
   29  ###...##......#
   30  ###...#.......#
   31  ###...#.......#
   32  #######c#######
   33  #######...#####
   34  #######...#####
```

Its places: **the start** (rows 0-4), **the west passage** (x 1-3, rows 5-10, down to the gold door), **the cross** (the stone plus
at x 3-5, rows 5-7, and its twin at rows 23-26), **the east hall** (rows 5-7), **the gate passage** (x 12-13, rows 8-10), **the map
room** (x 11-14, rows 11-15), **the ice** (rows 16-22), **the west room** (x 0-1, rows 11-20, behind the gold door: the crimson key),
**the middle way** (x 3-5 down from row 8, past the fountain), **the exit room** (x 7-9, rows 11-19, behind the crimson door), **the
great hall** (rows 23-26), **the south-west lobe** (rows 27-31, x 1-7), **the south-east room** (x 7-13, rows 28-31: the gold key)
and **the secret** (x 7-9, rows 32-34).

### Doors and the rest

| What | Where | Leads |
|---|---|---|
| gold door | (2,11), the west passage's foot | the west room, and the crimson key |
| crimson door | (6,19), reached from (5,20) below it | the exit room |
| plain door | (13,27), the great hall's east end | the south-east room, and the gold key |
| the elemental gate | (13,10), the gate passage's foot | the map room; it opens only from the room's side |
| the secret's wall | (7,32), the south-east room's south wall, beside the gold key | the secret: four pieces of gold |
| the fountain | the wall (4,18), faced from (4,17), on the middle way | |
| the exit | the wall (8,10), faced from (8,11), the exit room's north end | |
| the switch | (14.3, 11.6), the map room's north-east corner | seen from the ice's south edge, up the ice passage |

### The dead

Fifteen (eleven on level 1): half skeletons, a quarter shamblers, a quarter bats (T23), all rank I, so that no more than one
thrower stands in any one place. Shamblers wait under turned earth before a grave laid for each, as they do today, and rise when
you come near.

| # | Where | Place | Kind | | # | Where | Place | Kind |
|---|---|---|---|---|---|---|---|---|
| 1 | (2.3, 5.0) | the start, west | skeleton | | 9 | (1.4, 18.2) | the west room | shambler |
| 2 | (13.0, 5.7) | the east hall's end | bat | | 10 | (7.1, 22.6) | the middle way's foot | skeleton |
| 3 | (3.4, 9.4) | the west passage | skeleton | | 11 | (3.1, 22.9) | the middle way's foot | shambler |
| 4 | (11.7, 12.6) | the map room | skeleton | | 12 | (11.6, 26.3) | the great hall | skeleton |
| 5 | (7.9, 13.1) | the exit room | shambler | | 13 | (12.9, 28.8) | the south-east room | bat |
| 6 | (3.5, 15.0) | the middle way, by the fountain | skeleton | | 14 | (9.3, 30.8) | the south-east room | skeleton |
| 7 | (9.5, 16.4) | the exit room | skeleton | | 15 | (3.6, 31.3) | the south-west lobe | shambler |
| 8 | (14.3, 17.6) | **in the ice** | bat | | | | | |

The fountain calls the skeleton 6 (3 steps); the shambler 11 (6 steps) lies buried, and the buried hear nothing.

### Gold

Thirty pieces: twenty-six in the open, four in the secret.

| Place | Pieces |
|---|---|
| the start | (5.3, 2.5), (2.5, 3.5) |
| the west passage | (1.4, 7.0), (1.3, 7.9) |
| the east hall | (11.1, 6.4) |
| the middle way | (7.4, 8.7) in the nook, (5.6, 12.5), (3.3, 13.6), (5.7, 15.5) |
| the map room and the ice | (11.3, 13.9); (11.3, 17.4) on the ice |
| the exit room | (7.6, 11.3), (9.3, 11.4), (9.6, 18.6) |
| the west room | (1.6, 14.9); (0.4, 20.6), (1.6, 20.6) by the crimson key |
| the great hall | (9.6, 24.1), (12.2, 25.4) |
| the south-west lobe | (1.3, 27.5), (7.7, 27.7), (2.8, 28.8), (4.0, 30.5), (4.9, 31.7) |
| the south-east room | (9.4, 28.5), (8.6, 29.3) |
| **the secret** | (9.4, 33.5), (8.5, 33.6), (8.5, 34.5), (9.5, 34.5), all off the sliding stone's path |

### The elemental interactions

Ten grey squares, each a thing that wants a spell (1.0.0b16's `Marked`; which kinds is T22): **a** (5.5, 4.4) the start; **b**
(11.8, 7.6) the east hall; **c** (2.4, 9.5) the west passage; **d** (7.3, 14.8) the exit room; **e** (0.3, 16.2) the west room; **f**
(3.2, 16.5) by the fountain; **g** (11.7, 23.3) the great hall, at the ice passage's mouth; **h** (3.7, 26.3) the great hall's west;
**i** (5.5, 31.4) the south-west lobe; **j** (11.1, 31.6) the south-east room. With the switch and the gate that is twelve, which is
`MAXMK` exactly.

### What the layout already does

Steps, doors open: the start to the fountain 23; to trigger 1 19; to the gold key 47. The gold key to the gold door 52, the crimson
key nine beyond it; the crimson key back to the crimson door 61, to the exit 71. The backtracking is long, as asked. The way from the
great hall to the map room is only through the ice; the gate is the map room's only other way, and opens only from inside. The
switch can be seen and struck from the ice's south edge. Trigger 2's line spans the ice from wall to wall, so nobody crosses without
meeting it. The gold key lies two cells from the secret's wall, so the cracks show from where it is taken (they show within about
two cells).

## 4. Where it sits (settled)

- **Which levels.** Levels 1 and 2, the first time down. Past level 40 the runs come round; levels 41 and 42 are generated as
  today. A replay of level 1 or 2 at the Accursed Portal is the drawn level, with no lessons.
- **Whose** (T1 a). Everyone's: the dream plays the drawn levels too, on hand-drawn lanes (5.5), with no lessons, triggers or
  turnings. A person's run reaches them from BEGIN A NEW RUN: her room (her question, the class, first), then level 1. A page opened
  at level 1 or 2, and a death (back to her room before level 1), meet the drawn levels again.
- **When the lessons come** (T2 a, T3 b). On a person's first run on this computer, all of them, with no skip; and again on a new
  run whenever *THE LESSONS ON A NEW RUN* is set under the title's HELP. Without them the drawn levels are the same, their
  mechanics too (the fire, the ice, the switch, the gate, the secret), but nothing is turned, no trigger fires and the exit doesn't
  ask for the map.
- **Her room after level 1** (T13 a) has one page as it opens, on gold and her wares. While the lessons run she keeps the map off
  her wares until level 2 is behind (it is waiting down there).
- **The release** (T14): **1.0.0b21.5**, after 1.0.0b21.4's touch controls; the art pass and the lower storey keep 1.0.0b22 and b23.

## 5. A level from a drawing

### 5.1 In the source

Each drawn level is a map as strings in `scene_crawl.c`, as the bosses' arenas are (`BossDef.map`, `stamp_arena`), with the
legend above (sections 2 and 3's letters and a lane mark), and a table beside it of what stands in it in cells (the dead, the gold,
the keys, the things that want a spell, the triggers, the lights, the room types), so that the drawing and the source can be laid
side by side. `drawn_level(level)` says whether a level is one: 1 and 2 of the first descent, and not under `CRAWL_DRAWN=0` in the
debug build (the generated levels back, for balance runs against an older build).

### 5.2 The build

`start_level` calls `gen_drawn()` in place of `gen_level()` on a drawn level. What `start_level` does after `gen_level` is kept,
skipped or answered from the drawing:

| Pass | On a drawn level |
|---|---|
| the start (`G.cx`, `G.cy`, `G.h` at random) | the drawing's S, facing as drawn (east, on both) |
| `place_items` | the drawing's gold, of fixed kinds and worth; the keys; the map (level 2); **no potion** |
| `place_exit` | the drawing's X |
| `place_altar`, `place_sites`, `pz_make`, `pz_hide`, `tri_place` | none (today levels 1 and 2 have none of these but one of the run's fixed hazards, a site) |
| `place_rests` | the drawing's fountain only (8.6) |
| `place_marked`, `place_fights` | level 1 none; level 2 the drawing's ten, the switch and the gate (section 7) |
| `ward_make` | none (no Wraith before level 9) |
| `spectres_set` | none |
| `place_creatures` (the budget) | the drawing's dead, rank I (level 1 skeletons, T6 a; level 2 as T23), each a resident at its post (`RL_POST`, facing its room's way in), level 2's shamblers buried; no wanderers |
| the respawn ("something wakes ahead"), the lull's watch | none while the level is drawn |
| secrets (`place_secrets`) | level 1 none (T7 a); level 2 the drawing's one, laid by hand (7.5) |
| wall skulls (from 3), the gilded (from 7) | not on these levels anyway |

The dead on the drawn levels take every element alike (T6 a): `weak_el` gives 1 for them, so each class's first fights are fair;
the bestiary still shows the kind's own. On level 1 `G.nlinks` is 0, so "every link found" must not pay a coin (`COIN_ALL`) nor
count as a level's secrets all found; the tally's and the reckoning's secret lines say nothing there.

### 5.3 The look

Run I's look as any of its levels (`G.faced`): rooms are `Room` records with their run I types, the halls `TH_CORRIDOR`, and
`lay_faces` lays the walls; `dress_level` lays bones, chains, props and torches. **Its dice are a fixed seed** for each drawn level
(the run's `G.rs` kept and put back after), so the props and torches lie the same every time and nothing else changes. The room
types, for the owner to change:

| Level 1 | Type | | Level 2 | Type |
|---|---|---|---|---|
| the east room | burial vault | | the west room | ossuary |
| the side room | ossuary | | the map room | knights' vault |
| the key room | ossuary | | the exit room | chapter house |
| the west room | undercroft | | the south-east room | burial vault |
| the big room | chapter house | | the great hall, the south-west lobe | undercroft |
| the vault | knights' vault | | the secret | a closet (`CLOSET`), as secrets are |
| the halls, the start, the exit's hall | halls | | the start, the passages, the middle way | halls |

Lights beside the generator's torches: level 1's burning floor (section 6), the fountains' candles, a lamp where each key lies so it
is seen from the room's door, the map's own glow, the switch's and the gate's seals, the exits' runes. Fog as these levels have it
(little).

### 5.4 Level 1's vault

One vault (`G.nvaults` 1, gold) with **two doors**: both `locked` 1, colour gold; `G.vault_door[0]` is the one at (4,31), nearer
the key, which is the one the dream and a key wisp go by (`past_vaults`). `open_door` **spends the key** on the door it opens
(`G.has_key` cleared), so the two are made one lock (T19 a): opening either unlocks both.

### 5.5 The dream's lanes

The dream walks only lanes (`apass`), and lanes are every cell with x or y = 2 (mod 4) on the generator's grid of blocks. The
drawings aren't on that grid and can't be put on it (level 1's doors want lanes on rows 15, 17, 23, 26 and 31, which no one offset
gives). So on a drawn level `lane()` reads a mask drawn with the level (`G.lane_mask`) and `lane_dist` a field made from it, and the
generator's placers that do sums on the grid (sites, puzzles, the arena's) aren't run there. The masks I'd draw (`+`; `f` through
the fire, `i` through the ice), each connected from the start through every door to the fountain's front, the keys, the map and the
exit's front, with every floor cell within a step of a lane (so every piece of gold is in the dream's reach, `lane_dist` 1):

Level 1:

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

Level 2:

```
       012345678901234
    0  ##S++.#########
    1  ####+.#########
    2  ###.+.#########
    3  ##..+..########
    4  #+++++++#######
    5  #.+.#.+.......#
    6  #.+###++++++++#
    7  #.+.#.+......+#
    8  #.++#.+.####.+#
    9  ##.+#++#####.+#
   10  ###+#+##X####=#
   11  ++G+#+#.+.#.++.
   12  .+###+#.+.#.++.
   13  .+#+++#.+.#.++.
   14  .+#+#+#.+.#.++.
   15  .+#+#+#.+.#.++.
   16  .+#+++#.+.#IiiI
   17  .+#++##.+.#IiiI
   18  .+#+h##.+.#IiiI
   19  .+#+#+C+++#IiiI
   20  .+#+#+######ii#
   21  ##.+#+.#####ii#
   22  #..+#+..####ii#
   23  #+++#+++++++++#
   24  #.+###...+....#
   25  #.+###...+....#
   26  #+++#+++++++++#
   27  #+++++++#####D#
   28  ##..+..##....+#
   29  ###.+.##++++++#
   30  ###.+.#+++++++#
   31  ###.+.#+.....+#
   32  #######c#######
   33  #######+++#####
   34  #######...#####
```

The debug map (`CRAWL_MAP=1`) prints the lanes as it does, and its `MAP lanes reachable` line is the check.


## 6. Level 1's burning floor (T9 a)

- **What it is.** In the Order's crypts: lamp oil spilled across the hall and alight, the whole width of it for four cells. Drawn
  with the floor fires the game has (`Burn`'s flames, the ember imps' and the cask's), thicker, over a scorched floor; lit by its
  own flickering light, so it shows through the door's crack before it opens; a crackle and a roar heard near it.
- **What it does.** A share of the caster's health each half second the body is in it, and On Fire on the first touch (C2's
  `af_ignite`: the harm again over four seconds after leaving). The targets: a walk through, about **three fifths of health left at
  the far side and two fifths once the burning is done**; a sprint through, about half again of what a walk leaves. **It never
  takes you under a quarter of your health**, the burning after included: the page says it will not kill. Set in the harness.
- **After.** Once you are past it and its page is shut, it burns low and goes out over twenty seconds, leaving the scorch.
- **The dead** keep out of it (none are near). **The dream** walks through it: it is marked soft (`HZ_SOFT`) on `hz_dmap`, as a
  stope's firedamp is, so `decide` takes it, there being no other way.

## 7. Level 2's ice, switch, gate, map and secret

### 7.1 The ice field

- **Its cells**: rows 16-19 across the map room's width (x 11-14) and rows 20-22 of the passage (x 12-13); twenty-two, within a
  hazard's twenty-four (`HZ_CELLS`). Live as the level opens.
- **Its look**: the floor rimed blue-white with a low mist over it, as run I's cold patch (`HZ_COLD`, `T_HZ_COLD`) and Winter's
  Lance's rime (`SP_RIME`) are drawn; a cold breath heard near it (`SFX_FROST`). Going down, it thaws in two seconds (steam, drips);
  coming back, it creeps in from the walls over two.
- **What it does** (a default; the owner's words are "before they freeze"): a Chilled stack (C2's `af_chill`) every 0.4 s on it,
  each slowing 8%, **held at three on these levels**, so it never freezes a person; it hurts nothing else. That is enough to reach
  three by the middle from either side.
- **Trigger 2** is the line between rows 17 and 18, from wall to wall. Crossing it while the ice is live (either way) **leads the
  caster back** (a default for the owner's *routing*): the view taken (8.3), turned to the side they came in by, and walked back at a walk to the first
  cell off the ice there ((12,23) or (13,23) from the south; row 15 from the north), no stack added while led; then the page. Again
  each time.
- **The one in the ice** (the dead 8, a bat; a default): held in it, drawn frozen (`DR_ICE`), out of every fight till the ice goes
  down; then it comes free. So a person striking the switch from the edge has nothing coming at them from the ice.
- **The dream** keeps off the live ice (it marks harm on `hz_dmap`), strikes the switch as it answers any thing that wants a spell
  near its lane, and crosses when it is down.

### 7.2 The switch

A thing that wants a spell (`Marked`), a kind of its own: *A RIMED SEAL* in the map room's north-east corner, a frost-white seal of
three pips. Three strikes of **any spell** (a default) and the ice goes down; a strike that lands counts, from anywhere it can be
seen (from the ice's south edge it can, up the passage). It is new art (a sprite and its done state), on the level 2 sheet. Once the
map is taken and the ice is back, **it is spent** (a default: its seal cracks and goes dark), so the gate is the way on.

### 7.3 The gate

(13,10), one way (T21 a): a portcullis in the gate passage. Its seal is on the south face only. From the map room, a person sees the
seal and strikes it with any spell, and the gate rises and stays up. From the north it is bare iron, which says *IT OPENS FROM THE
OTHER SIDE* to E. It uses the door locks the gated things already have (`GATED`, `mk_gate_up`, the hearth's way: up for good).

### 7.4 The map

`IT_MAP`, a new item, drawn as her map ware is (`SP_MAPWARE`, the floating scroll) with its own glow, at (12.8, 15.4) by the ice.
Taking it sets `RUN.has_map` (kept through death and her claim, as a bought one is), so she no longer sells it. Held already when
level 2 comes, its place holds gold instead (T18 a, taken as the default since it wasn't answered).

**The scripted moment** (B16), while the lessons run: as it is taken the ice comes back. The view is taken and turned to face the
ice (south), held there **two seconds** while it creeps in and settles, with no movement; then turned to face the gate (north); then
the map's pages. Without the lessons the ice comes back all the same, and nothing is turned.

### 7.5 The secret

One `Secret` laid by hand: its wall (7,32), stood before from (7,31), pressed south; its room (7..9, 33..34), carved as a secret's
(`Cell.secret` 1 the room, 2 the wall); the stone slides two cells, to (7,34), and the four pieces of gold lie off its path. It is
the level's one link (`G.nlinks` 1), so finding it counts a secret (its caption, the tally and the record), and, every link found, brings the
run's first Cursed Coin (`COIN_ALL`, *FOR THE SHRINE*). Its cracks show within about two cells, as every secret's do; the gold key lies two
cells from it.

### 7.6 The things that want a spell (T22 a)

Ten (a-j in section 3), placed as drawn, of kinds chosen as the level is laid (`place_marked` isn't run). On level 2 a person holds
one line, their class's, so every one must answer it: the four on the main way (a, b, g, j) **take any line** (the bound coffer, the
hanging cage), and the other six are **the class's own kind** (the pricket or the votive rack for Fire; the keeper's urn for Frost;
the galvanic coil for Lightning; the knot of roots for Poison; the bound soul for Spirit; the bone barricade for Physical), each the
same kind every time for the same class. Their rewards are their kinds' own.

### 7.7 Two keys

The gold key (7.6, 30.8) in the south-east room, by the secret; the crimson (1.0, 19.6) at the west room's foot, behind the gold
door. Each is spent on its door, as any key is.

## 8. The lessons

### 8.1 State

`TU`, static, kept while the program runs: which lessons are shown this level, which triggers are on, a beat waiting, the page.
A level isn't saved part way through (the game saves at her room), so a person who quits half way through a level comes back to her
room before it, and the level's lessons start again. What lasts is a byte in the prefs record (8.7).

### 8.2 Beats

A beat is a moment that opens a lesson (a screen of one or more pages). Each comes once unless marked *again*.

**Level 1.**

| Beat | When | The game | Lesson |
|---|---|---|---|
| B1 arrival | level 1 opens, its card gone | | WALKING, DOORS (2 pages) |
| B2 the door | the door at (4,1) opens | | FIRE |
| B3 through | the body two cells past the flames (y 7.0) | the view turned south; a free wisp to the fountain | HEALTH, BURNING, MENDING (3) |
| B4 mended | mending ends having mended some (any way: full, moved, a blow); or, not mended, the body four cells east of the burning hall (x 9) (T10 a) | the view turned east | the class's (2) |
| B5 first blood | the first of the dead slain by the caster | | SOULS |
| B6 trigger 1 | into its cells | | EVERYWHERE, LOCKS (2) |
| B7 the key | the gold key taken | trigger 2 off | THE GOLD KEY |
| B8 trigger 2 *again* | into its cells, the key not held | the view turned to the key room's door (11,26), set back half a cell | NOT YET (shorter after the first) |
| B9 out of the vault | a gold door opened (trigger 3 off at once), then the body out of the vault having been in it (T12 b) | | THE HOARD, AND THE WAY OUT |
| B10 trigger 3 *again* | into its cells, no gold door opened | the view turned west, set back half a cell | THE GOLD DOOR (shorter after the first) |

**Her room after level 1:** B10a, as it opens: HER WARES.

**Level 2.**

| Beat | When | The game | Lesson |
|---|---|---|---|
| B11 arrival | level 2 opens, its card gone | | the class's name (1 page) |
| B12 a thing that wants a spell (T24 a) | the first of them within four cells and in sight | | THE CRYPT ANSWERS |
| B13 trigger 1 (T20 a) | into its cells, the map not held | | BEYOND THE GATE |
| B14 trigger 2 *again* | across its line while the ice is live, the map not held | led back to the side they came in by (7.1) | THE ICE (shorter after the first) |
| B15 the switch | its third strike | the ice goes down | a caption only: *THE ICE DRAINS AWAY* |
| B16 the map | taken | the ice comes back: the view to it two seconds, then to the gate (7.4) | THE MAP (3 pages) |
| B17 trigger 2 after *again* | across its line while the ice is live, the map held | led back north | THE WAY ON |
| B18 the gold key | taken | the view turned to the secret's cracks (7,32) | CRACKS |
| B19 the crimson key (T24 a) | taken | | A SECOND KEY |
| B20 trigger x *again* (T20 a) | into its cells, the map not held | the view turned south, set back half a cell | NOT WITHOUT THE MAP (shorter after the first) |

Lessons that fall together come one after the other. A trigger that turns you back fires again only once you have been out of its
cells. **The turnings wait while anything hunts you within six cells**: a person isn't turned away from a fight; the beat comes when
it is over. A level's lessons are counted done when it is cleared.

### 8.3 Taking the view

`tu_steer(angle, back)`: for half a second the person's keys, mouse, sticks and fingers move nothing; the view eases round to the
angle (smooth at both ends), and with `back` the body steps back half a cell, out of the trigger; a sprint, a charge and a mend in
progress are let go. `tu_hold(seconds)` keeps it so (the map's two seconds); `tu_lead(x, y)` walks the body to a cell at a walk
(the ice's way back). Then the page. These are the only times the game moves a person, and each is short.

### 8.4 The screen

`UI_TUTOR`, a menu like the rest, so the crypt waits under it as it does under the L list or the wardrobe. A slab in the run's stone
(docs/PLAN-ui.md's kit: `ui_slab`, the heading in the inscribed face, the words in the small one), in the middle of the view, the
crypt dimmed round it; up to eight lines a page, about 32 letters a line; a pip a page; a small picture where one helps (the class's
spell, a key, a soul, the wisp, the map), from the icons the kit has. At its foot: *A/D PAGES  E ON*, *ESC* shuts the lesson. It
takes no key for its first 0.4 s, so a key already down can't skip it unread. There is no skip (T3 b). Sheet first (step 4).

**Three voices.** Keys are written as on the keyboard. A pad's person reads buttons in their place (`pk`, and `PK_JOB` for the
layout they chose). By touch (1.0.0b21.4) `pk` speaks the stones' words only in play (`G.ui == UI_NONE`), and the lessons are a
menu, so they take the touch words through a `pk` of their own that keeps them under `UI_TUTOR`; a slide turns the pages and a tap
goes on, as touch drives any menu (docs/PLAN-mobile.md).

### 8.5 The wisp to the fountain

A wisp kind of its own, `WK_MEND` (the place to mend's front cell; free; the candles' warm white), kindled at B3 in front of the
caster: *A WISP KINDLES. IT KNOWS WHERE TO MEND*. It hovers there until you are within a cell and a half, as the exit's does.
Nothing else casts it.

### 8.6 The fountains

The drawings' healing fountains are the run's **places to mend** (T15 a; `T_REST`, docs/PLAN-ai.md section 28): in run I an arched
niche with a ledge of candles and a seep. E facing one kneels; standing still mends you to three quarters over twenty seconds from
nothing; and **every second of it is a noise sixteen steps off that tells the dead where you are**.

### 8.7 What lasts

The prefs record (`SLOT_PREFS`: the pad's layout, the title's scene, touch's turning and smoothing) gains a byte, `PL.tut`: level
1's lessons done, level 2's, and *again* (the lessons asked for on the next new run). `PL_BYTES_TUT` 27, after touch's 26; an older
build reads what it knows and writes 26, which drops it, as each byte before it was dropped going back. Windows keeps the record as
`PadMap`; the browser's page reads and writes the same record (`web/haunted.js`) and must keep the extra byte; the harness writes
`prefs.bin`.

The title's **HELP** (still *to be written*, `title_help`) becomes the lessons to read again, page by page, and a row *THE LESSONS
ON A NEW RUN: YES / NO*.

### 8.8 Beside it

- The controls card (the hints shown six seconds as a person takes over) is held back on level 1 while the lessons run: the first
  page says the same.
- A creature's caption (`name_newcomers`), toasts and the level's card wait while a page is up.
- **Death while the lessons run** (T4 a): on a drawn level, until its lessons are done, the old mercy: the level again from its
  start, nothing lost; the lessons already shown aren't shown again.
- Debug: `CRAWL_TUT=0|1` (no lessons, or lessons whatever the prefs say), `CRAWL_TUTBEAT=k` (begin at beat k, set where it
  happens), `CRAWL_DRAWN=0`, `CRAWL_ICE=0` (level 2's ice down from the start); `EVENT tut beat <k>`, `tut page <lesson> <n>`,
  `tut steer`, `tut lead`, `tut trigger <n> on|off`, `ice down|up`, `map_taken`.

## 9. The lessons' words

Drafts, mine, in the game's voice, taken to be changed by playing (T16 a). In capitals, as the game prints them; 32 letters a line
at most. Keys are written as on the keyboard; a pad's or a finger's person reads their own (8.4). *LESSON* is the heading.

### Level 1

**B1. WALKING**
```
WASD WALKS. THE MOUSE TURNS YOU.
SHIFT SPRINTS FORWARD, A WHILE:
A SLIM BAR OVER THE TABLET SAYS
HOW LONG, AND FILLS AGAIN AFTER.
```
**B1. DOORS**
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
level 1 by a death with the lessons asked for again under HELP; his lines are there for that.)

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
**B9. THE HOARD, AND THE WAY OUT** (as they come out of the vault, T12 b)
```
WHAT WAS LOCKED AWAY IS YOURS.
THE WAY OUT IS EAST, PAST THE
LAST DOOR: RUNES IN THE WALL.
FACE THEM AND PRESS E. WHAT YOU
LEAVE BEHIND IS SHOWN YOU
BEFORE YOU GO.
```
**B10. THE GOLD DOOR** (then, each time after: `THE GOLD DOOR IS STILL SHUT.`)
```
YOU HOLD THE GOLD KEY AND HAVE
NOT USED IT. GO BACK: THE DOOR
IT OPENS IS WEST OF HERE.
```
**B10a. HER WARES** (her room, after level 1)
```
GOLD BUYS WHAT STANDS ON HER
PEDESTALS. FACE ONE AND PRESS E
TO SEE WHAT IT WOULD CHANGE.
THE WAY ON IS THE FAR DOOR.
```

### Level 2

**B11. THE WITCHFINDER** (the class's name)
```
THE DEAD ARE MORE HERE, AND THE
WAY IS LESS PLAIN. LOOK IN EVERY
ROOM: SOMEWHERE ON THIS LEVEL
LIES A MAP, AND YOU WILL NOT
LEAVE WITHOUT IT.
```
**B12. THE CRYPT ANSWERS**
```
SOME THINGS HERE WANT A SPELL.
A SEAL SHOWS ON THEM, AND ITS
PIPS SAY HOW MANY STRIKES.
STRIKE ONE WITH YOUR FIRE BOLT
AND SEE WHAT IT GIVES.
```
(the spell's name by the class.)

**B13. BEYOND THE GATE** (T20 a)
```
THE MAP LIES BEYOND THIS GATE,
AND IT OPENS ONLY FROM THE FAR
SIDE. THE WAY IN IS FROM THE
SOUTH, ACROSS THE ICE.
```
**B14. THE ICE** (then, each time after: `STRIKE THE SWITCH FIRST.`)
```
THE ICE FREEZES WHAT STANDS ON
IT. IN THE FAR CORNER OF THE
ROOM BEYOND IT IS A SWITCH:
STRIKE IT THREE TIMES FROM HERE
AND THE ICE GOES DOWN.
```
**B16. THE MAP** (three pages)
```
THE MAP IS YOURS, AND STAYS
YOURS, THROUGH DEATH AND ALL.
M UNROLLS IT. YOU WALK, TURN
AND CAST WITH IT OPEN: THE
CRYPT DOES NOT WAIT FOR YOU.
```
```
IT DRAWS WHAT YOU SEE, AND
DARKENS WHERE YOU HAVE WALKED.
IT MOVES WITH THE ARROWS, AND
COMES BACK TO YOU AFTER. AT ITS
FOOT: THE DEAD, THE GOLD AND THE
SECRETS LEFT ON THE LEVEL.
```
```
THE ICE HAS CLOSED BEHIND YOU.
THE GATE TO THE NORTH OPENS
FROM THIS SIDE: STRIKE ITS SEAL.
```
(The arrows are the D-pad on a pad, and by touch the map's stone; the pad's and touch's voices of these pages are written with
them.)

**B17. THE WAY ON**
```
THE ICE HAS CLOSED. THE WAY ON
IS THE GATE TO THE NORTH.
```
**B18. CRACKS**
```
LOOK AT THE WALL BEFORE YOU.
CRACKS IN THE STONE MEAN A
SECRET: WALK UP TO IT AND PRESS
E, AND IT GIVES. THEY SHOW ONLY
WHEN YOU ARE CLOSE.
```
**B19. A SECOND KEY**
```
A CRIMSON KEY. A KEY IS SPENT
ON THE DOOR IT OPENS, SO THE
GOLD ONE IS GONE. THIS ONE IS
FOR A DOOR BANDED IN CRIMSON.
```
**B20. NOT WITHOUT THE MAP** (then, each time after: `THE MAP IS STILL OUT THERE.`)
```
NOT YET. YOU WILL NOT LEAVE
WITHOUT THE MAP. IT LIES IN THE
ROOM PAST THE ICE.
```

## 10. The dream

It plays the drawn levels (T1 a) on the lanes of 5.5. Level 1: it crosses the fire, mends at the fountain when under two fifths
as it does anywhere (`rest_goal`), takes the key and opens the vault (`key_goal`, the two doors one lock), and leaves. Level 2: it
answers the things that want a spell near its lanes, strikes the switch from the ice's edge and crosses when the ice is down, takes
the map if it comes to it (a thing on the floor, as gold is), strikes the gate's seal from inside, takes the keys in turn and
leaves; the exit doesn't ask it for the map. It has no lessons, triggers or turnings. Its balance runs from level 1 change for these
two levels only; a new baseline is made in the checks (step 9), and `CRAWL_DRAWN=0` gives the old levels back to compare with older
builds.

## 11. What it touches

- `start_level`: the drawn branch, and the passes it skips (5.2); `gen_drawn` and the maps and tables, new.
- `lane`, `lane_dist`: the masks on a drawn level.
- `weak_el`: every element alike for the drawn levels' dead.
- Level 1's burning floor: a hazard of its own, or `Burn`s that never go out on their own (built as whichever reads more plainly),
  its floor on health, `hz_dmap`'s soft mark.
- Level 2's ice field (a hazard of its own that holds, with the cold's harm held at three stacks), the switch and the gate's seal (two
  `MKK` kinds, their sprites), the gate (`GATED`), `IT_MAP` (`collect`, its sprite and glow), the secret laid by hand (`Secret`,
  `Cell.secret`), the dead one held in the ice.
- `open_door`: the vault's two doors one lock (T19 a).
- `wisp_target`, `summon_wisp`: `WK_MEND`.
- The lessons: `TU`, `UI_TUTOR` (`update_tutor`, `render_tutor`), `tu_steer`, `tu_hold`, `tu_lead` (in `update_human`, before the
  walk), the beats' hooks in `open_door` (B2, B9), `rest_end` (B4), `kill_mob` (B5), `collect` (B7, B16, B18, B19), the switch's
  strikes (B15) and a cell check each frame (B3, B4, B9, the triggers); `begin_shop` (B10a, and the map kept off her wares).
- `human_begin` (the controls card held back), `name_newcomers` and `toast` (waiting under a page), `pk` (the touch voice in a menu).
- `pl_read`, `pl_write`, `web/haunted.js` (the prefs byte); `title_help` (HELP).
- `death_costs` (T4 a); the coin and the secrets' lines with no links (5.2).
- No save format changes: nothing of it is in a run's save, only the prefs byte.

## 12. Steps

1. **The readings settled.** Done (11 November).
2. **Level 1 in play**: the map and table, `gen_drawn`, the skipped passes, room types, the fixed seed, the lanes, the vault with
   two doors as one lock, every element alike; `CRAWL_DRAWN=0`. Frames from the game for the owner: the debug map, and the start,
   the burning hall, the fountain, the key room, the vault and the exit.
3. **The burning floor**, its numbers set in the harness by a scripted walk and sprint through it.
4. **The sheets** for approval: the lessons' screen (`build/uisheet tutor`, in `make sheets`: every page of section 9 in keys, in a
   pad's buttons and by touch, the fit checks failing on any overflow; `media/sheets/tutorial/pages.png`), and level 2's new art (the ice
   field on the floor, the switch and its done state, the gate's seal from both sides, the map lying on the floor;
   `media/sheets/tutorial/level2-art.png`).
5. **The machinery**: `TU`, the beats, triggers, `tu_steer`, `tu_hold`, `tu_lead`, `WK_MEND`, the prefs byte, the title's HELP, the
   old mercy while the lessons run, the debug switches.
6. **Level 1's lessons in**, every page, the classes' lines, her room's page.
7. **Level 2 in play**: its map and table, the ice, the switch, the gate, the map, the secret, the ten things, the dead one in the
   ice; frames from the game.
8. **Level 2's lessons in**, the scripted moment at the map.
9. **The checks**: a scripted person (keys, a scripted pad, and touch in the browser's build) through every branch of both levels:
   level 1's key first; trigger 2 without it, twice; trigger 3 without a gold door, twice; through each gold door; a fight under a
   turning (it waits); level 2's ice from the south before the switch (led back), the switch, the map and its moment, the ice from
   the north after (led back), the gate, trigger 1, the exit without the map (turned back, twice), the gold key's turning, the secret
   and its coin, the crimson key; the lessons again from HELP; a death on each level under T4 a; frames of every page. The dream on
   the drawn levels on sixteen seeds (no stall, every level cleared, the median time), and from level 1 against 1.0.0b21.4 for the
   first 3,000 s, five classes, as J21 was. ASan on a scripted person and on the dream. The smoke test, the browser's smoke test,
   and the Windows build by CI. The README's *What has been verified*, the page's *Beta status*, a preview strip of the lessons, and
   the release notes, for **1.0.0b21.5**.

## 13. Questions for the owner

### The first round (answered 11 November)

T1 a (everyone's levels), T2 a (the lessons on a person's first run here, and again from HELP), **T3 b (no skip)**, T4 a (the old
mercy while the lessons run), T5 a (the four larger marks are ordinary), T6 a (skeletons at rank I, every element alike), T7 a (no
secret on level 1), T8 a (the drawing is the whole level), T9 a (the fire as section 6), T10 a (B4 as written), T11 a (turnings, set
back half a cell, waiting while hunted), **T12 b (the way out told as they come out of the vault)**, T13 a (a page in her room after
level 1), **T14: 1.0.0b21.5** (1.0.0b21.4 went out with touch since), T15 a (the places to mend), T16 a (the words as drafted, to be
changed by playing), T17 a (level 2 teaches what its drawing marks; the owner gave its scripted moment), T19 a (the vault's two doors
one lock). **T18 wasn't answered**; I've taken (a): the map already held, its place holds gold.

### The second round: level 2 (answered 11 November)

T20 a (trigger 1 where drawn, pointing to the way in from the south; and a trigger of mine, **x**, across the exit room, turning back
a person without the map), T21 a (the gate's seal on its south face only), T22 a (four things that take any line on the main way,
six of the class's own kind), **T23: half skeletons, a quarter shamblers, a quarter bats**, so the player doesn't juggle many
missiles at once (section 3's table; level 1 stays as T6 a), T24 a (the pages at the first thing that wants a spell and at the crimson
key).

### Defaults taken on level 2 (change any)

- The ice: a Chilled stack every 0.4 s on it, held at three on these levels, so it never freezes a person; trigger 2 leads them
  back at a walk to the side they came in by, then the page (7.1).
- The switch: any spell, three strikes; spent once the map is taken and the ice is back (7.2).
- The one in the ice (a bat): frozen in it till the ice goes down (7.1).
- Without the lessons the level is the same (ice, switch, gate, secret; the ice back when the map is taken), but nothing is turned
  and the exit doesn't ask for the map (section 4).
- While the lessons run, she keeps the map off her wares until level 2 is behind.
- The room types (5.3).

## 14. Hand-off log

- **11 November** (`claude/sweet-carson-qxvyfi`, from main at 1.0.0b21.3). The owner gave the brief and run I level 1's drawing.
  Read it cell by cell (`tools/tutorial_read.py`, written for it; the reading drawn as `media/sheets/tutorial/level1-read.png` beside
  the drawing, kept as `level1-master.png`), fitted it to whole cells, measured it, drew the dream's lanes, and wrote the draft
  with its questions. Nothing built.
- **11 November, later.** The owner answered T1-T19 (T18 left; (a) taken) and gave level 2's drawing and brief. Main at 1.0.0b21.4
  ("By Touch") merged in (one conflict, CLAUDE.md's paragraphs, both kept), so the prefs byte is the 27th, after touch's two, and
  the lessons speak touch's words too. The reader taught level 2's colours (peach floor, ice, the grey elemental marks, crimson,
  magenta) and to find a grid whose walls skip lines; level 2 read (`level2-master.png`, `level2-read.png`), its lanes drawn, the
  plan rewritten with both levels, the second round T20-T24 and the defaults taken. Nothing built.
- **11 November, later still.** The owner answered T20-T24 (T23: half skeletons, a quarter shamblers, a quarter bats on level 2;
  assigned by mark so that no two throwers stand together). Settled. The owner asked that the drawn levels keep their dressing
  (decorations, seeded or placed): they keep the generator's whole dressing, laid on the drawing's own seed (5.3).
- **Step 2 built** (harness only). `gen_level`'s tail is `gen_dress` now, shared (the dream from level 1 for 400 s line for line
  1.0.0b21.4's on the same seed and run id). `g_drawn`, `g_lane`, `g_ldist` beside `lane()`; `DRAWN` (`Drawn`, `DrawnRoom`,
  `DrawnThing`; level 1's map with its lanes, `DRAWN1_MAP`, its rooms and its 50 things) before `start_level`, built by `gen_drawn`,
  `drawn_start`, `drawn_items`, `drawn_walls` (the exit and the place to mend), `drawn_dead`; `start_level` lays it on the drawing's
  seed and gives the run its own dice back after; `begin_shop` clears `g_drawn`. Skipped on a drawn level: the potion, sites, things
  that want a spell, spectres, the respawn, the lull's watch; every element alike (`weak_el`); the vault's doors one lock
  (`open_door`). `CRAWL_DRAWN=0` gives the generated level 1 back; the debug build prints `DRAWN level n: ...`. Checked: the debug map
  (8 doors, 39 items, 11 rank I skeletons, 22 lights, 39 decorations, every lane reachable), the dream clearing level 1 on four seeds
  (167-187 s, the key taken, 85-92% of the gold, no stall), ASan clean through level 1, the smoke test. The burning floor is plain
  floor till step 3.
- **Step 3 built** (harness only). The burning floor is `DF` beside the lane mask (`DFIRE_*`): its cells from the map's `F`/`f`,
  `drawn_fire_tick` from `update_hazards` (whoever plays: the first touch sets you alight, `af_ignite`, a fifth of your health to
  come; then 17% of it each half second in it, by `hurt_ex` as a Fire stroke, `HURT_ELEM`; never under a quarter, what is still to
  burn counted), `drawn_fire_lights` (a light a row), two flames a cell (`SP_BURN_A/B`, in `render_sprites`; the list grew by
  `DFIRE_CELLS * 2`), a crackle near it, and `drawn_fire_dmarks` (soft on `hz_dmap`, so the dream crosses). Two cells past it (the
  drawing's y 8.0) it starts to go out over twenty seconds (`DFIRE_OUT`), harmless once under a third. Tried by a person taken over
  on level 1 (`CRAWL_TITLE=0`, `CRAWL_TPAT=15.5,6.5,90`, E at the door, W through): a walk leaves 44% once the burning is done (the
  far side 53%), a sprint 56% (69%), standing in it holds at 26%. Debug: `EVENT drawn_fire hurt|past|out`. The scorched floor it
  leaves is not drawn yet. The lesson waits for the page (step 5/6); for now it goes out once you are past, page or none.
- **Step 4, the lessons' sheet, drawn** (for the owner's approval). The words are `src/crawl_tutor.c` now (`TUT`, by lesson `TL_*`:
  each page's heading and lines, keys in brackets, `{CLASS}` and `{SPELL}` filled by `tut_lines`/`tut_head`; the past's names, spells
  and B4's lines; the shorter words for a second time), shared by the game and the sheet, and so is the page, `ui_tutor` (`UiTutor`: a
  tablet in the run's stone, the heading inscribed, each line's keys on caps as `ui_keys` sets them, a lozenge a page, the keys to go
  on at its foot). `tut_wrap` sets a page's lines again only when another voice's longer words (a pad's buttons, touch's stones)
  outgrow one. `build/uisheet tutor <dir>` (in `make sheets`) draws every page, a few in a pad's and touch's words, and fails on any
  overflow (none): `media/sheets/tutorial/pages1.png` (level 1 and her room) and `pages2.png` (level 2). B16's second page was reworded so it
  reads in every voice (*IT MOVES WITH THE ARROWS, AND COMES BACK TO YOU AFTER*; the map's Space has no pad button). The level 2 art
  sheet (the ice, the switch, the gate's seal, the map on the floor) comes with step 7, before it is in play.
- **Steps 5 and 6 built** (harness only; the sheet approved, *Looks incredible! Go for it*). The machinery is the lessons' block in
  `scene_crawl.c` after the drawn levels' (`TU`, `TB_*`, `tu_level_begin` from `start_level`, `tu_beat`, `tu_queue`, `tu_show`,
  `tu_tick` and `tu_steering` at the head of `update_human`, `tu_trig` the drawing's trigger cells, `tu_hunted`, `tu_steer`, `tu_close`,
  `tu_after`, `tu_cleared` from the level's clearing, `tu_shop_begin` from `begin_shop`); the page is `UI_TUTOR` (`update_tutor`,
  `render_tutor` with `ui_tutor`), the crypt waiting under it as under any menu, taking no key its first 0.4 s; `tu_voice` puts a
  pad's buttons (by the layout, `pk`'s `PK_JOB`) or touch's stones in the brackets' place, and `tut_wrap` sets the lines again when
  they outgrow one; a tap on the screen goes on. Level 1's beats B1-B10 are in as section 8.2 has them, with the view turned (0.5 s,
  eased) at B3, B4 and the two triggers that turn back (set back to the last place outside their cells), the turnings waiting while
  anything hunts within six cells; B3's free wisp is `WK_MEND` (gold, to the place to mend's front). Her room's page (B10a) comes
  once, after level 1. The prefs byte is `PL.tut` (the 27th, `PL_BYTES_TUT`: `TUT_L1`, `TUT_L2`, `TUT_HER`, `TUT_AGAIN`), written as
  a level's lessons are cleared; the title's HELP is a page of its own now (`title_help`), its E toggling *THE LESSONS AGAIN ON A NEW
  RUN* once they have been given, and a new run with it set clears the byte (`run_new`). While the lessons run a death costs nothing
  (`death_costs` asks `tu_mercy`, T4 a). The controls card is put away while they run. The tally's SECRETS row says *NONE HERE* on a
  drawn level with none. Debug: `CRAWL_TUT=0|1` (none, or on whatever the prefs say), `CRAWL_TUTBEAT=k` (the beats before k counted
  shown), `CRAWL_TUTKEY=1` (the gold key held; 2, and the dead gone), `CRAWL_TUTCALM=1` (as if nothing hunted), `CRAWL_TUTAUTO=s`
  (each page turned after s seconds); `EVENT tut beat`, `tut page`, `tut steer`, `tut closed`, `tut done level n prefs p`. Tried by a
  person taken over in the harness, each beat from its place (`CRAWL_TPAT`): B1 to B4 in order with the wisp and a mend, trigger 1,
  trigger 2 twice (the shorter words the second time), trigger 3, the key, out of the vault, the exit (`tut done level 1 prefs 1`,
  the 27 bytes written) and her page; a scripted person dying four times under the mercy (nothing lost, no lesson shown twice;
  her claim's mark is kept off while the lessons run, since a death then costs nothing); the dream clearing level 1 (158 s, no
  lessons for it). ASan clean on a person through the lessons and those deaths, a person to her room, and the dream for 600 s; the
  smoke test.
  Not yet: `tu_hold` and `tu_lead` (level 2's), the HELP's paging through the lessons (it toggles only), the scorched floor.
- **Level 2's art sheet, drawn** (for the owner's approval; sent, and level 2 built with it meanwhile, to be redrawn at their word):
  `media/sheets/tutorial/level2-art.png` (`make sheets`' `tutorial2`, `draw_tutorial2` in `tools/sheet.c`). The switch is a squat stele with a
  six-armed seal sunk in its face, rimed and hung with icicles (`spr_mk_rseal`, `SP_MK_RSEAL0`), thawed and running wet (`1`), split
  and dull once spent (`2`); the gate's south face is the portcullis with a riveted plate and a sunk boss for the seal
  (`gen_gate_seal`, `T_GATE_SEAL`, laid on the run's stone as `T_GATE` is), its north face the bare `T_GATE`; the map lies where it
  fell, half unrolled towards you (`spr_mapfloor`, `SP_MAPFLOOR`); the ice is a tile of old ice, cracked, bubbles in it, rime over it
  (`gen_ice`, `T_ICE`), laid over the floor texel by texel in drifts as it creeps (`crawl_ice_at`, shared by the game and the sheet).
- **Step 7 built** (harness only). Level 2 is `DRAWN2_MAP` and `DRAWN2_THING` (the drawing's letters grown by C a crimson door, = the
  gate, I and i the ice, c the secret's wall, q and Q its room), `DRAWN2_ROOM`, and `Drawn.vault` a room for each key's colour (gold
  and crimson, no silver: the vault loops step over a gap). `gen_drawn` lays the gate (`GATED`, `IC.gate_door`), the ice (`IC`), the
  secret by hand (a `Secret` of one link, its stone sliding two cells), the walls' fronts north first (`drawn_front`), the exit's
  vault (`G.exit_in_vault`); after the dressing, `drawn_clear` takes what it laid off the ice, off what the drawing sets down and off
  every lane (an undercroft's prop had stood on level 2's great hall lane and cut its east end off; level 1 lost one prop on a lane
  in its east room too, its only change). `drawn_items` lays the secret's gold out of the count and the map (`IT_MAP`, gold in its
  place when the map is held, T18 a) with a glow; `drawn_dead` lays T23's kinds, the shamblers buried under turned earth
  (`riser_make`); `drawn_marks` the ten things (T22 a: a coffer or a cage on the main way, the class's own kind elsewhere, a rack for
  every other of Fire's) and the two new kinds, `MKK_RSEAL` (the switch, three strikes) and `MKK_GSEAL` (the gate's seal, one, drawn
  only as its seal over the plate), which any spell answers (`mk_strike`), never placed by the generator (tier 9). Things drawn on a
  lane were set off it: a a row north, f, g and h against their walls. The ice: `ice_k` (each cell turning over `ICE_TURN` after a
  wait, the walls' side first), `ice_down` (the switch's `mk_finish`), `ice_up` (the map taken: back for good, the switch spent),
  `ice_tick` (a Chilled stack every 0.4 s on it, held at three, never Frozen; whatever else stands on it is held frozen, so the bat
  drawn in it waits there), its floor mark `FM_ICE` (`ice_marks` in `hz_marks`), soft on the dream's map. The gate: its face by
  which side you stand (`render_walls`), up for good once risen (`update_doors`), E from the north says *IT OPENS FROM THE OTHER
  SIDE*. The dream answers the switch from 13 cells (the ice's south edge); the rest of things from 6.5 as before. The lane mask
  grew a lane down the great hall's middle (x 9, rows 24-25; 5.5): without it the hall's north row and the crimson door's way were
  an island while the gate was shut, and the dream, holding the crimson key, paced the start's hall; the debug map's lane check now
  counts a barred gate shut and names the lanes it can't reach (`MAP lane cut off`). Debug: `CRAWL_ICE=0` (the ice down from the
  start), `CRAWL_WHERE=1` (a person's place and angle twice a second), `DRAWN level 2: ... things`, `EVENT ice down|up`, `map_taken`.
  Tried in the harness by a person (the Witchfinder): three bolts at the switch from the ice's edge, the ice thawing, the walk across,
  the map taken and the ice back behind, the gate's seal struck from inside and the gate up, the north face bare and a bolt from there
  doing nothing, the secret pressed (*A SECRET! 1 OF 1*, the run's first Cursed Coin); frames in `media/sheets/tutorial/level2-frames.png`
  (the ice from its south edge, the switch by the gate, the map at the ice's edge, the gate's sealed face). The dream from level 2's
  room on eight seeds (1-15 odd, four pasts): every one cleared it, 250-334 s, no death, no stall; none went into the map room, so
  none struck the switch or took the map (section 10 lets it), and none found the secret on these seeds. ASan clean on a person
  through the switch, the map, the ice and the gate, on the secret, and on the dream twice (once with `CRAWL_ICE=0`); the smoke test.
  Level 1, its one prop gone, still clears (174 s on two seeds). Not yet: trigger x and the exit's refusal without the map, her room
  keeping the map off her wares, and the scripted moment at the map (all lessons, step 8).
- **Step 8 built** (harness only): level 2's lessons. The turnings are a short queue now (`tu_move`, `MV_TURN`, `MV_HOLD`, `MV_LEAD`;
  `tu_steer`, `tu_hold`, `tu_lead`, `tu_move_begin`, run by `tu_steering`), and while the lessons hold the body nothing hurts it
  (`hurt_ex` asks `tu_taken`) and the ice adds no stack. Level 2's beats in `tu_tick` (`TB2_*`): B11 its page as it opens, B12 the
  first thing that wants a spell within four cells and in sight, B13 trigger 1 north of the gate without the map (`tu_trig` knows
  level 2's 1 and x), B14 the line across the ice (the drawing's y 18, x 11-14) crossed while it is live and the map not held:
  turned to the side come in by and walked off the ice there at a walk (`TU_LEAD` 2.4 cells a second; from the south first into
  the passage's width, then to row 23), then THE ICE (its shorter words after the first); B16 the map taken (`tu_map_taken`, from
  `collect`): turned to the ice, held two seconds while it creeps in, turned to the gate's seal, then THE MAP's three pages; B17
  the line crossed southward with the map: walked back north to row 15, THE WAY ON; B18 the gold key: turned to the secret's
  cracks, CRACKS; B19 the crimson key, A SECOND KEY; B20 trigger x without the map (set back half a cell, turned south), and the
  exit itself refused the same way (`tu_exit_barred`, from `human_leave`). B15's words are the switch's own caption. Her room keeps
  the map off her wares while level 2's lessons are to come for a person (`g_map_kept`, set in `tu_shop_begin`). Debug: `EVENT tut
  steer|hold|lead`, `tut map_kept`. Tried by a person in the harness, each beat from its place: the line crossed onto live ice twice
  (led back both times, the shorter words the second), the switch, the map's moment (no harm taken in it; a skeleton that followed
  onto the ice frozen there as it came back), the line crossed southward with the map (led north), trigger 1, the gold key (turned to
  the cracks), the crimson key, trigger x and the exit refused, her room with the map kept back; level 1's turnings and its wisp as
  before. ASan clean on two runs through them. Not yet: the title's HELP paging through the lessons (it toggles only), the scorched
  floor, and step 9's checks.
- **12 November: released as 1.0.0b21.5, "First Steps"**, at the owner's word, before step 9 ("first, go ahead and offer it as a b21.5
  release, then we will gauge the testing need"). For it: the README's section and *What has been verified*, the page's paragraph,
  its tested row, the upgrade note and the beta status, the notes, a preview strip (`media/previews/preview_catacomb_tutorial.png`: the burning
  hall, a lesson's page, the wisp, level 2's ice, the ice draining, the ice back with a skeleton frozen in it). Two things found on the
  way: the controls card came up over level 1 when taken over mid-level (the takeover set it after the lessons had put it away; it
  stays away now while they run), and a player upgrading would have met the lessons, with no skip, the next time a death sent them to
  level 1: when the settings carry no lessons byte yet but someone has played here (settings of their own, or a page or progress in
  the ledger), `pl_read` counts them as given, so the title's HELP gives them back. Step 9's checks are still to do.
