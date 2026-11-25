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
SWITCH x y thing [TOGGLE]      ; the crypt thing (an index in its _THING) whose USE triggers it fires
```

### 2c. The editor's side (step 9)

A second kind of map in the World Editor: a side passage, drawn in its tiles, its things laid with the same palette and checks,
and exported as 2b's text; PLAY runs it. The crypt map gets a thing, a side doorway (`DT_SIDEDOOR`, its kind `16 * side + link`),
laid on a wall cell of a room, a hall or a secret room, and the side passage's switches choose their crypt thing from a list.
It is built in files of its own (`tools/mapedit/side.js`) and a few marked lines in `editor.html`, to keep out of the other
agent's way.

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
| 7 | The dream in a side passage | A first version: finishes the test passage; the crypt's dream does not yet walk into side doorways by itself |
| 8 | The text format and its reader; `HOST_SIDE=file` and `sidecheck` in the harness | The reader done (`sd_read`); the harness's door not yet |
| 9 | The World Editor: the side map, its export, its checks, PLAY; the crypt's side doorway | |
| 10 | Side doorways on drawn levels (rooms, halls, secret rooms); the crypt doorway's own texture | |
| 11 | Lives at the merchant, 500 gold; `LG_SIDE` in the ledger | |
| 12 | Touch's stones and the controller in a side passage | |
| 13 | Art for each run's creatures from the side, and the caster in the gear worn (as sheets first) | |
| 14 | `make asan` and long runs; the dream's log on levels without side passages the same, line for line, as the build before | |

## 4. Debug

- `CRAWL_TPSIDE=n` (debug build): a side doorway to built-in side passage n in the first wall ahead of the start, every link of
  it going to that one doorway; `CRAWL_SIDE=n` straight into it as the level opens.
- Events: `EVENT side_in`, `side_out`, `side_life`, `side_done`, `side_over`.

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
