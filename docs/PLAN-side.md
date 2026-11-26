# PLAN-side: side passages, a platformer inside the crypt

The owner asked for "a side scrolling component that plays within HAUNTED": a doorway in the crypt that the caster crosses, as
into Severance or the Backrooms, into a 2D platformer in the same 320x200, the same palette and the same pixels. What is carried
goes in and comes out. Several doorways can lead into one, and out of it, to different places in the crypt. Keys, switches and
toggles in one can change the crypt. Its levels are drawn in the World Editor (docs/PLAN-mapedit.md).

Working name in the code: a **side passage** (`SD`, `sd_*`, `game/side*.c`). What it is called in the game is the owner's to say.

Branch: `claude/eloquent-johnson-sjuioa`, begun from the editor's `claude/optimistic-thompson-boccz1` (at `85b823f`). Another agent
works that branch; section 6 says what this one touches of the code both share, so the two merge cleanly.

## 1. What the owner settled (25 November 2026)

1. **Through a doorway, like a pocket.** The crypt level waits as it is (its clock, creatures and Wraith stop) while you are in a
   side passage. Health, gold, keys, spirit, souls and gear are the caster's own all the way through, in and back out.
2. **Combat, with the caster's spells, each element its own way in 2D** (spaces are tiles, 16 pixels):
   - **Fire** goes a shorter way than the rest (6 tiles).
   - **Ice** flies a little slower than fire and goes 20% further (7.2 tiles).
   - **Lightning** is instant and reaches 5 tiles.
   - **Poison** is lobbed in an arc; while the cast is held, a mark shows where it will land.
   - **Physical** (the bone spear) flies on until it hits something.
   - **Spirit** is the shortest: 3 tiles, for now, while the owner tests.
3. **Hazards**: falling blocks that come back; toxic water that flows and drains on a clock, shutting a way while it runs; spikes.
4. **Moving platforms**, each told its speed, its direction and how far it goes.
5. **Up at a doorway goes through it.**
6. **The creatures are the run's own**, the kinds that can be met on that run, in new art made for the side view (until it is
   drawn, their bestiary pictures from the side stand in).
7. **Lives, apart from the crypt's health.** Three to begin with; finishing a side passage gives one back, never more than three;
   the merchant sells them, 500 gold each. A life lost resets the side passage to how it was when you came in. The last life lost
   ends the run as a death in the crypt does.
8. **One side passage can have several ways in and out**, each to its own doorway in the crypt; a level may now and then have a
   second or a third. Finishing one (its goal doorway) puts you back in the crypt.
9. **The dream plays them too.**
10. **Placed by hand**, in rooms, halls and secret rooms of drawn levels, in the World Editor.
11. **Draft art, tile-based**, as an old Apogee or Nintendo scroller: a set of tiles and a rough caster with walking, running,
    jumping, crouching and falling over dead. The figure is to show the gear worn, later.

Defaults taken where the owner left them (theirs to change):
- 16x16 tiles in the crypt's palette, drawn for mode 13h's tall pixels as the crypt's art is; 20 tiles across, 9 and a half down
  over the status bar, which stays.
- One draft tileset, in run I's stone; the run's piece plays, with the pockets' echo.
- Keys: A/D or the arrows to move, Shift to run, Space to jump, S or Down to crouch, W or Up at a doorway to go through, the
  left button or Ctrl to cast (the browser too: Space is the jump here); 1-9 and the wheel choose the spell as in the crypt.
  A controller: the stick or the pad to move, A to jump, the right trigger to cast, up on the stick at a doorway. Touch: stones
  for left, right, jump and cast (step 12).
- A life lost: the side passage as it was when you came in, you at the doorway you came in by, whole. What was picked up since
  you came in goes back (keys, gold), and any switch thrown since goes back.
- Going out by a doorway that is not the goal keeps what you did; coming back in, it is as you left it (for the rest of the
  crypt level's visit).
- The crypt level's state is not saved mid-level, so neither is a side passage's. The lives are saved (a section of the ledger).

## 2. How it is built

### 2a. The game's side

- **The mode.** `SD.on` while the caster is in a side passage. `crawl_update` hands the frame to `sd_update` after the menus and
  the console, so the crypt's own update never runs and `G` waits untouched; `crawl_render` draws `sd_render` in place of the
  raycaster and keeps the status bar, the messages and the menus. Nothing of `G` is copied: the caster's health (`G.hp`), keys
  (`G.has_key`) and the run's gold and gear (`RUN`) are read and written where they are.
- **The world** is a grid of tiles (`SD.w` x `SD.h`, 255 x 64 at most) and a list of things. Positions are in pixels; the
  physics steps at a fixed 1/240 s so the dream's runs come out the same each time.
- **The caster**: a box 10 wide and 26 tall (16 crouched). Walking 72 px/s, running 128; a jump of a little over three tiles,
  higher for holding Space; a short grace off a ledge's edge and a jump pressed just before landing still counts. Crouching
  lowers the box and stops the walk to a creep.
- **Tiles** (letters, as the drawn levels have theirs): `.` air (the dark), `,` the back wall (drawn, not solid), `#` stone,
  `%` brick, `=` a ledge you can stand on and jump up through, `^` spikes, `~` standing toxic water, `F` a falling block.
- **Things**: doorways (`DOOR`: its link, and whether it is the goal), moving platforms (`PLAT`: width, direction, distance, speed,
  back and forth or round, a wait at each end), toxic flows (`FLOW`: a box that fills for some seconds and drains for some),
  creatures (`MOB`: a kind or the run's choice, a rank), gold, keys, lives, switches (`SWITCH`: the crypt thing whose triggers it
  fires).
- **Doorways, both sides.** In the crypt a side doorway is a wall cell, walked into or E'd as a pocket's is; it is
  `(side, link)`. In the side passage a doorway has a link; you come in at the doorway of the link you came in by, and Up at a
  doorway of link n takes you out of the crypt doorway `(side, n)`, standing before it. The goal doorway does the same and finishes
  the passage: a life back, what it holds for finishing.
- **Switches** fire the crypt's own triggers (docs/PLAN-events.md): a side switch names a thing of the crypt level, and throwing
  it is `ev_touch(TG_USE, thing)` there, so every action the editor binds (doors, lights, fields, hazards, spawn points, lines)
  works from a side passage with nothing new.
- **Spells** cost and strike as in the crypt (`spell_tier`, `spell_cost`, `spend_spirit`, `spell_damage_at`, the augments'
  share, `cast_cycle`); only their flight is the side's (1.2).
- **Creatures** take the crypt's numbers (`mob_base_hp` with the level and rank, `mob_damage`'s blow, `KINDS`'s speed and
  elements) and simpler ways: walkers patrol a ledge and come at you, fliers drift and swoop, throwers throw. The run's kinds
  come from `random_type` on the side passage's own dice, so the crypt's dice are left as they were.
- **Lives**: `RUN.sd_lives` (3 at a run's start), in a ledger section `LG_SIDE`; an older page reads as three.
- **The dream** (step 7): a map of where it can stand and how it gets from one to the next (a walk, a jump, a drop, a ride),
  found by trying the real physics from each place, then a way through it to the goal; it casts at what is ahead.

### 2b. The format (text, the same for the game's own and the editor's)

A side passage is text, read by `sd_read` (`game/side_text.c`) whether it is compiled into the game or handed to it by PLAY, as
`crawl_drawn_text` reads a drawn level. A line a statement; `;` begins a note to the line's end.

```
SIDE 1                         ; the format's version
NAME "THE OSSUARY STAIR"
SIZE 96 20                     ; tiles across, down
ROW "################..."      ; SIZE's down of them, each SIZE's across long
DOOR x y link [GOAL]           ; a doorway standing on the tile below (x, y)
PLAT x y w dx dy dist speed [LOOP] [WAIT s] [AT f]   ; w tiles wide, dist tiles along (dx, dy), speed tiles a second; AT: where it begins, 0 to 1
FALL x y [DELAY s] [BACK s]    ; or the letter F: shakes DELAY after it is stood on, falls, comes back BACK after
FLOW x y w h on off [AT s]     ; toxic water filling the box for on seconds, drained for off; AT: where in its clock it begins
MOB x y kind [rank]            ; kind a MOB_ name or ANY (the run's choice); rank RK_I..RK_V or -1
GOLD x y IT_COINS              ; IT_COINS, IT_CHALICE, IT_GEMS, IT_CROWN, IT_CHEST
KEY x y colour                 ; 0 gold, 1 silver, 2 crimson
LIFE x y
SWITCH x y thing [TOGGLE]      ; the crypt thing (an index in its _THING) whose USE triggers it fires; -1 for none (a lever for TRIG alone)
TORCH x y
GATE x y w h [OPEN]            ; bars across the box, shut unless OPEN; ON opens, OFF shuts
PLATE x y                      ; pressed while the caster, or a loose thing, stands on it
ZONE x y w h                   ; pressed while the caster is in the box; never seen
CURRENT x y w h dx dy speed [POISON]   ; water running through the box (dx, dy each -1, 0 or 1; tiles a second); carries what is loose
HAZ HZ_kind x y [ZONE] [SAFE] [CLOCK every give first] [ENTER x0 y0 x1 y1] [HARM n] [CHANCE n] [WARN s] [STAYS s]
HZC x y                        ; a cell of the trap last named, in the order painted (where a creep or a line spreads from first)
TRIG source MODE [secs] tag:ON|OFF|FLIP ... CRYPT:n   ; MODE: ONCE, TOGGLE, HOLD (on while pressed), TIMED secs (on, then off secs after let go)
TUNE name value                ; this passage's own number for one of the tuned (2d)
```
Any thing takes `TAG name` (what a trigger calls it) and the flags `OFF` (a platform, flow, current or trap still until a trigger),
`LOOSE` (gold, a key or a life that falls and that water carries), `POISON`, `OPEN`. A source is a switch, a plate or a zone; a target
any thing with a tag. `CRYPT:n` uses the crypt's thing n, as a switch's own does.

### 2c. The editor's side (step 9)

Built (session 2). A second kind of map in the World Editor. NEW asks which: a crypt level or a side passage. OPEN lists both,
each passage marked with its number, and offers the built-in test passage as an example.
- `tools/mapedit/side.js` (`window.SideCore`): a passage as the editor keeps it (`kind: 'side'`, `num` its place in `SIDE_TEXT`,
  `level` the level PLAY begins on, `w`, `h`, `grid`, `things`); `exportText` writes 2b's text (an F in a row for a falling block
  with the game's own timings over the dark, else a FALL line; seconds with a decimal, tiles without), `exportC` the same as a block
  for `SIDE_TEXT[]`, `parse` reads text back by `sd_read`'s rules with the same refusals, `check` the errors and warnings.
- `tools/mapedit/side_ui.js` (`window.SideUI`): its own three columns, shown in place of the crypt's while a passage is open: select,
  erase, the seven tiles (shift and drag for a box), doorway, goal doorway, platform, lift, falling block, poison flow, torch,
  creature, the five golds, the three keys, a life, a switch; the canvas (a platform's way and its far end ghosted, a flow's box
  dashed with its clock, a doorway's link); CHOSEN (the passage's number, level and size, or the thing's fields) and CHECKS; its own
  undo. It saves to the same store as the crypt maps (`maps/<id>`, `kind: 'side'`), with save points; a draft in the browser.
- The crypt map's **side doorway** (`t: 'sidedoor'`, `num`, `link`): laid only on stone with floor beside it, at the cell's middle;
  exported as `{ DT_SIDEDOOR, 16 * num + link, x, y }`; the export's box takes in its wall when it lies just outside the floor's box.
- **PLAY.** A passage alone: a new run on its level, straight in (`sideNow`); its doorways lead back to where it began. A crypt
  level: its export, and for each passage number its side doorways name, the newest passage saved with that number (a number none is
  saved for is the game's own). The play page's shim (`build.py`) takes `sides`, `sideNow` and `level` from the message;
  `haunted.js` hands each to `web_side_load` and `web_side_now`; the game's `crawl_side_text` and `crawl_side_now` (`side.c`).
- The hooks in `editor.html`, each marked `side (PLAN-side)`: the script's placeholder, the tool and its picture, its fields, its
  placing, the header's buttons, the keys, OPEN, NEW, load, PLAY. `core.js`: the export, the label, the checks.

### 2d. The owner's parts for puzzles, the crypt's traps and the tuning (session 3)

Asked for on 26 November: the caster half as tall, moving and jumping as before; sliders to tune; the controller; coins staying where
they are laid; the crypt's traps painted into passages; triggers, seen and unseen; the parts for a first puzzle (a lever opens a gate,
a plate set water running, the water carries a key down through the gate). Built:
- **The caster** 13 pixels tall (`SD_HT`), 8 crouched; its figure drawn at half height (`side_art.c`, `SF_H` 18). Its box is
  smaller, so a one-tile way is standing room now. The creatures are still drawn at their old height.
- **Tuning** (`TUNE[]` in `side_text.c`: 37 numbers by name, each with its default, range, step and a line on what it is). A passage's
  `TUNE` lines set its own; `SDT[]` in `side.c` is what is in force. The editor's TUNING tab sets a passage's numbers (a reset each);
  during PLAY a drawer of the same sliders moves the game as it runs (`hvs-tune` to `web_side_tune`). `extract.py` reads the table.
- **The controller**, read as it is: the left stick or the pad to walk, pushed hard or B held to run, A to jump, down to crouch (with A,
  down through a ledge), up or Y at a doorway or a lever, the right trigger or X to cast, the bumpers through the spells held.
- **Coins and the rest** drawn where they are laid (they were drawn a tile low). With `LOOSE` they fall until they rest, and water carries
  them; a plate holds while one rests on it.
- **Works** (`game/side_works.c`): gates, plates, zones, currents, loose things, triggers (`sd_trigs`, `sd_act`), a thing's on or off
  (`SD.st`), a flow's clock and fill (`SD.fc`, `SD.fk`).
- **The crypt's traps** (`sd_traps`): every kind but the arc and the grasp, by family as the crypt has them. Falls (rock, timber, glass,
  crystal) drop on their cells after a warning; lines (fire, votive, fissure, lode, galvanic, cart, censer, snare) run along their cells;
  creeps (bile, silt, damp, sluice, cold, deep cold, saint, tithe) spread cell by cell from the first and stay a while (`STAYS`); clouds
  (void, ash, breath, reliquary, mirror) fill theirs. Each harms by its element; the void silences spells, the snare holds. What sets one
  off: a clock, a box walked into (`ENTER`), or a trigger; `ZONE` the cells themselves, `SAFE` none at the start. The editor paints them.
- **The puzzle's parts** shown working in `SIDE_TEXT[1]`, *The Cistern Key*, and `tools/mapedit/fixtures/side_puzzle.txt`: `SWITCH ...
  TOGGLE TAG lever`, `GATE ... TAG gate`, `PLATE ... TAG plate`, `CURRENT ... OFF TAG water`, `KEY ... LOOSE`; `TRIG lever HOLD
  gate:ON` and `TRIG plate HOLD water:ON`. The owner draws the real one.

## 3. Steps

| Step | What | State |
|---|---|---|
| 0 | This plan | Done |
| 1 | The draft art as a sheet: tiles, platform, block, water, spikes, doorway, switch, key, life; the caster's walk, run, jump, crouch, cast and fall | Done, a draft (`media/sheets/side/side_draft.png`), in the game as it stands |
| 2 | The mode: in by a doorway and out again, the caster's box and physics, the tiles, the camera, the status bar, lives and their end; a test passage built in; `CRAWL_TPSIDE` | Done |
| 3 | Moving platforms, falling blocks, toxic flows, spikes | Done |
| 4 | The spells, by element | Done; their numbers the owner's to tune |
| 5 | The creatures (the run's kinds, in their bestiary pictures until their own are drawn) | A first version: walkers, fliers, throwers |
| 6 | Keys, gold, lives to find, switches firing the crypt's triggers | Done; a switch on a drawn level not yet tried |
| 7 | The dream in a side passage | Done, harness only: it finishes both built-in passages; the crypt's dream walks into a side doorway (`sd_dream_goal`, `USE_SIDE`) once a visit while early, whole and with two lives, and not again on a level where it lost its last |
| 8 | The text format and its reader; `HOST_SIDE=file` and `HOST_SIDE_NOW` in the harness | Done |
| 9 | The World Editor: the side map, its export, its checks, PLAY; the crypt's side doorway | Done, harness and Playwright only; not used by the owner |
| 10 | Side doorways on drawn levels (rooms, halls, secret rooms); the crypt doorway's own texture | |
| 11 | Lives at the merchant, 500 gold; `LG_SIDE` in the ledger | Done, harness only: a votive lamp in her room's south-east corner while a life is lost (`LIFE_*`, `BUY_LIFE`, `SP_LIFEWARE`, a draft); `LG_SIDE`; a death that costs gives the three back |
| 12 | Touch's stones and the controller in a side passage | Done, harness and Playwright only: the controller (2d); touch: the left stick walks, crouches and, pushed up, goes through a doorway, a tap at the left uses, a finger on the right of the picture jumps, the right strip casts |
| 13 | Art for each run's creatures from the side, and the caster in the gear worn (as sheets first) | |
| 14 | `make asan` and long runs; the dream's log on levels without side passages the same, line for line, as the build before | |

## 4. Debug

- `CRAWL_TPLIFE=1` (before the life for sale in her room) and `CRAWL_SDLOST=n` (the passages' lives lost at a fresh start); `EVENT
  life_bought lost n left gold`.
- `CRAWL_TPSIDE=n` (debug build): a side doorway to built-in side passage n in the first wall ahead of the start, every link of
  it going to that one doorway; `CRAWL_SIDE=n` straight into it as the level opens.
- Events: `EVENT side_in`, `side_out`, `side_life`, `side_done`, `side_over`; `side_works` (a gate, current, flow, platform or trap
  turned on or off by a trigger) and `side_trap` (a trap going off, its cells).

## 5. Tests

Harness only, until the owner has played it. Frames of a scripted person in a side passage (`HOST_INPUT`); the dream's log on the
levels it plays, against the build before on eight seeds, the same on every level without a side passage.

## 6. What this branch touches that others share

New files: `src/crawl/game/side.c`, `side_text.c`, `side_dream.c`, `src/crawl/draw/side.c`, `src/art/side_art.c` and
`side_art.h`, `tools/sidemock.c`, and later `tools/mapedit/side.js`. Lines in shared files, each marked `/* side (PLAN-side) */`:
`scene_crawl.c` (the parts' list), `Makefile` (`SRC` and the mock-up), `game/update.c` (the hand-off in `crawl_update`),
`draw/frame.c` (the hand-off in `crawl_render`), `game/human.c` (the doorway walked into and used), `game/drawn.c` and
`drawn_text.c` (`DT_SIDEDOOR` at the end of the list), `game/state.c` (`RUN.sd_lives` at the end of `RunState`), `game/save.c`
(`LG_SIDE`), `game/merchant.c` (the life for sale), `editor.html` (step 9).

## 7. Hand-off log

**Session 1 (25 November 2026).** The owner's answers taken; the plan written; the branch begun from the editor's (`85b823f`).
Steps 1 to 6 built and a first dream (7). What is in the game:
- `src/art/side_art.c/.h`: the tiles and the figure (a rig of joints; `side_fig_part` keeps each pixel's part for the gear). The sheet,
  `media/sheets/side/side_draft.png` from `build/sidemock`: every tile, every frame, and a mock-up screen. Not yet shown to the owner.
- `game/side_levels.c` (the test passage, *The Ossuary Stair*, 80 by 20: a spike pit and a ledge, a step, a platform over standing poison,
  a lift to a gallery with a silver key, gems and a second way out, falling blocks over a drop, a low way to creep through that the poison
  floods on a clock, the goal's room with a life on a ledge, five torches, four creatures, a lever that names no crypt thing), `side_text.c`
  (2b's reader), `side.c` (the rest), `side_dream.c`, `draw/side.c`.
- The hooks, each marked `side (PLAN-side)`: `scene_crawl.c`, `update.c` (`sd_frame` after the menus), `frame.c` (`sd_render` with the
  status bar, messages, menus, toasts), `human.c` (`sd_walk_in`, bumped and E), `start_level.c` (`sd_level_laid` after `ward_make`),
  `drawn.c` and `drawn_text.c` (`DT_SIDEDOOR` at the end), `state.c` (`RUN.sd_lost`, at the end of `RUN`, not saved), the Makefile
  (`side_art.c` in `SRC`, `build/sidemock`).
- The crypt's side doorway looks like a pocket's for now (`T_PASSAGE`).

How it was tested (harness only; nothing under Wine, the Windows and browser builds not compiled here):
- The dream through *The Ossuary Stair* (`CRAWL_SIDE=0`, run 7, seeds 1 to 8, 900 s each; the seeds fall in pairs, so five different runs):
  27 times in, 26 out by the goal, 3 lives lost (all three in the pool under the platform, one run, which then ended as a death does:
  `side_over`, the level laid again). It rides the platform, waits out the flood and creeps the low way, slays what is in its way. The
  rounds before the last fixes lost most lives in the pool (stepping for a platform that had gone, overshooting a landing) and in the flood;
  the flood's dry spell was raised from 4 to 5 seconds, since the low way could not be crept in time.
- A scripted person (`HOST_INPUT`): H held to take over inside the passage, walking, jumping, each element cast (lightning's flash, the
  flask's arc and mark held and let go, the spear), the frames looked at.
- Nothing else moved: the dream's log, seeds 1, 2, 3, 4 and 6, run 7, 1,500 s, against `85b823f`'s, line for line but the time-taken line
  (no side passage on any level it played). `make asan`: a run through the passage found a signed overflow in the back wall's hash, fixed;
  then 400 s of the dream through the passage again and again under `make asan` (`ASAN_OPTIONS=detect_leaks=0`, seed 1): clean.

What is next, in order: the editor's side (step 9), with the other agent; the crypt's dream walking into side doorways (7); the harness's
`HOST_SIDE` (8); lives at the merchant and in the ledger (11); touch and the controller (12); the creatures' own art and the caster in the
gear (13). Questions for the owner: what a side passage is called in the game; whether the numbers (jump, speeds, each element's reach,
the poison's bite, spikes a third of health) feel right by hand.

**Session 1, later (25 November 2026): main merged in.** Main took the world editor's branch squashed (`a76f4d2`, its tip but for three
lines of PLAN-lights). This branch's two commits replayed on main without a conflict in the code; the one conflict was the plans table in
`CLAUDE.md` (main's rows kept, PLAN-side's after them). Recorded as a merge commit whose tree is exactly that, so the branch is main plus
the side passages and nothing else (22 files). Tested on the merged tree: every binary built without a warning (the Windows and browser
builds not here: no compilers for them in this container, so CI alone), `node tools/mapedit/test.js` passed, `make smoke` passed, the
dream's log line for line the same as main's (seeds 1, 2, 4, 6, run 7, 1,500 s, but the time-taken line), the side passage on eight seeds
as before (27 in, 26 at the goal, one run ended), 400 s of it under `make asan`: clean.

**Session 2 (25 to 26 November 2026): the World Editor's side (step 9).** The owner chose the editor before a play-test build. Read
first: the published artifact against this repository's build of the editor, the same but for the host's wrapper, so nothing had
been changed outside the repository. Built as section 2c says. Tested:
- `node tools/mapedit/test.js`: every built-in passage read and written back letter for letter (the test passage, 30 things); a new
  passage with every kind of thing exported and read back the same; six bad texts refused with `sd_read`'s reasons; a side doorway's
  export (`DT_SIDEDOOR, 37`) inside the map though its wall lies outside the floor's box (it lay at y -0.5 before the fix); its checks.
  The fixtures it writes (`side_0.txt`, `side_new.txt`, `side_crypt.c`) read by the game: `HOST_SIDE` both, the new one played straight
  in on level 4 by the dream to its goal; `HOST_DRAWN` the crypt export, its side doorway laid where the editor put it.
- Playwright's Chromium on the built page served locally: the example opened; a new passage drawn (tiles painted and a box of ledge,
  each kind of thing laid), a platform chosen and turned upward, the checks, SAVE, EXPORT TEXT, undo and redo; back to a crypt map,
  a side doorway laid on a wall and refused on floor, its passage and link set, EXPORT C; OPEN listing the passage. No page errors.
  The page's `.main` is a grid, which `[hidden]` does not hide: both sets of columns showed at once until `side_ui.js` added its rule.
- PLAY, with the browser build made here (Emscripten 6.0.13, as CI): the example passage played alone, the game booted on level 3 and
  straight into it, the dream playing; a crypt level (the tutorial's first, a side doorway laid) sent its export with
  `DT_SIDEDOOR, 16` and the passage saved as number 1.
- The game: `make host`, `build/host_debug`, `make asan`, `make web` with no new warnings; under the sanitizers a passage handed over
  by `HOST_SIDE` played 300 s and a crypt with a side doorway 60 s, clean; the dream's log, seeds 1 and 4, 1,500 s, line for line
  main's (`af939ef`) but the time-taken line.
Not used by the owner; nothing under Wine; the Windows build left to CI. Still to do: the crypt's dream walking into side doorways
(step 7), lives at the merchant and in the save (11), touch and the controller (12), the creatures' own art and the caster in the
gear (13); in the editor, a dream's walk for passages as the crypt's has.
Published as a new artifact, https://claude.ai/artifact/9f163JHMAJesUR8bPgMyWP (with `play/`; `db` at `admin` for `maps`, and
`downloads`, as the first), since the first editor's artifact could not be updated from this session (the tool took it as from outside
the account). Its store starts empty: maps from the first come across by BACKUP FILE there and LOAD FILE here.

**Session 3 (26 November 2026): the owner's parts (2d).** Built as 2d says. Tested:
- `node tools/mapedit/test.js`: both built-in passages read and written back letter for letter; a new one with every kind of thing,
  a painted trap and a trigger exported and read back. The fixtures it writes read by the game (`HOST_SIDE`).
- Playwright's Chromium on the built page: *The Cistern Key* opened; a new passage drawn with a plate, a gate, water and a rockfall
  painted on three cells; a trigger bound plate to gate in the TRIGGERS tab; a slider in TUNING; the export carried each
  (`PLATE`, `GATE`, `CURRENT`, `HAZ` with its `HZC`, `TRIG plate1 HOLD gate1:ON`, `TUNE walk 90`); no checks raised. PLAY of the
  example with the browser build made here: the drawer opened, jump set to 330 while the dream played, and the passage finished. No page
  errors.
- The game: the puzzle solved by a scripted person in the harness (the lever, the gate open, the plate, the water on, the key carried
  down and taken; frames in `media/sheets/side/side_works.png`, the top row); the fixture's four traps (bile, rock, fire, void) going off
  (the bottom row). The dream finished both built-in passages on eight seeds each, sixteen of sixteen with no life lost (it stood at a
  poison flow's edge to fight, its middle out of the poison and its side in it, and died there three times over; it now counts its whole
  width). The dream does not work puzzles: it plays past them. `make asan`: both passages and the fixture, 300 to 600 s, clean. `make
  smoke` passed. The crypt's dream, seeds 1 and 4, 1,500 s, line for line the build before's.
Harness and Playwright only; nothing under Wine; the Windows build to CI. Open: the creatures at the caster's new height; a crypt toast
(`+4 EXP A ROOM FOUND`) can show over a passage's status bar; the rest of section 3's steps.

**Session 4 (26 November 2026): steps 7, 11 and 12.** Main took the branch squashed first (`2787bbd`; merged back into the branch as
`7993542`). Built:
- Step 11. A life for the passages sold in her room at $500 while one is lost: a votive lamp floating in the south-east corner, by the way
  on (`LIFE_X`, `LIFE_Y`, `LIFE_FOR_SALE()` in `wisps.c`), its price over it, its prompt, its page among her wares (A/D go round to it),
  `life_buy`. The lamp is `SP_LIFEWARE` (`parts/hall.c`), the passages' own life drawn larger: a draft, as the passages' art is; its frames
  are `media/sheets/side/life_ware.png`. `LG_SIDE` in the ledger (`RUN.sd_lost` and three spare bytes); a page without it has its three.
  A death that costs gives the three back, as it takes the compass (`death_fall`).
- Step 7. The crypt's dream goes into a side doorway as into a chain's (`sd_dream_goal` after `chain_goal`, `USE_SIDE`, `sd_dream_in` from
  `PS_USE`). Doing it showed the fliers still swooped at the old figure's chest, over the new one's head, where straight bolts passed under
  them: they come at its middle now.
- Step 12. Touch in a passage (`sd_person`).
- The experience banner sat on a passage's bar: it sits over it now.
Tested: with `CRAWL_TPSIDE` the crypt's dream walked into the doorway and finished the passage on eight seeds of eight, no life lost (before
the fliers' fix, passage 0 took all three lives on seeds 2 and 4); each passage alone (`HOST_SIDE_NOW`) on eight seeds, sixteen of sixteen,
and the puzzle fixture. A life bought by a scripted person (`EVENT life_bought lost 1 left 300`), refused when short; `CRAWL_SAVETEST=1`
15 cases, 0 bad (four of them the lives). Touch in Playwright's Chromium as a Pixel 7 held sideways, on PLAY's path (taken over, the
title, BEGIN A NEW RUN, straight into *The Cistern Key*): walked right by the stick, jumped by a finger on the right
(`media/sheets/side/touch.png`). The crypt's dream, seeds 1, 4 and 7, 1,500 s, line for line the build before's but the time-taken
line; `make asan` clean on two crypts with a doorway (500 s each), the life bought and the save test; `make smoke` passed. Not played by
hand; nothing under Wine. Left: the creatures' own art and the caster in the gear worn (13, as sheets first); the lamp's look, the
owner's to approve.

