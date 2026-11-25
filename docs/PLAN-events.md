# PLAN-events: scripted events on drawn levels

The owner asked for levels from level 3 on drawn in the map editor (docs/PLAN-mapedit.md), the compass as a thing to be found
in one of them, and scripted events to bring it in: "I don't want to manually code them ... This should be done with triggers,
objects the player can interact with, shoot, or pick up." Their example, which is the plan's test level:

> Flicker all of the lights when the compass is picked up, illuminating the torches in one of three different paths from the area.
> The player explores that branch then returns, they place an object they picked up, the previous lane of torches flickers out
> causing another lane to flicker to life.

## 1. What the owner settled

1. **Events are bound in the editor, not coded.** A trigger and its actions are chosen from lists; the export carries them as data
   and one runner in the game plays every level's.
2. **The paths are fixed:** an action names the group it lights. No dice.
3. **Events may stand between the player and the exit,** so the dream learns to play them, and the editor's dream walk models them:
   a chain that cannot be finished shows as *cannot finish*.
4. **Hints are for the owner, not the player:** a note on each trigger in the owner's words (what the interaction should feel
   like), and guidance in the editor that explains each trigger and action as it is bound. A line on screen is an action the owner
   may add, not a hint.
5. **Torches are placed by hand, in named groups,** lit or unlit at the start. The unlit torch is new art: **a sheet to the owner
   before it goes in the game.**

## 2. The parts (defaults, the owner's to change)

**Things to bind** (new in the editor; each a pick-up or a fixture; the lever and the plate use the puzzles' own, `TR_LEVER`, `TR_PLATE`):

| Thing | What it is |
|---|---|
| Compass | Picked up, it is yours as if bought (`RUN.has_compass`), and a death takes it as it does now. Held already: the pick-up still fires its trigger, and it is worth her price in gold's place (a default) |
| Relic | Something to carry, named by the owner ("A BONE KEY"). One carried at a time, as the puzzles' things are (`things.c`) |
| Socket | A place to set a relic, named; it takes the relic the owner binds, or any |
| Torch | On a wall's face, in a group ("path A"), lit or not |
| Barred door (`B`) | A door that stays shut till an action opens it |

**Triggers** (when): a thing picked up; a relic set in its socket; a thing struck with a spell (the things that want one: coffer,
cage, the class's own, the switch, the seal); a lever pulled (the use key); a pressure plate stepped on; a box of cells walked into;
every one of a list of triggers fired. Each may wait on others ("only after"). **Its type** (the owner's second ask): *once*;
*every time*; a *switch*, on at one touch and back at the next, or by itself after a duration; *timed*, on for its duration and then
back. Going back, its torches and doors return as they were (unless the owner says not), and it may have actions of its own for that.

**A thing in one of a set of rooms** (the owner's first ask): any thing but the dead may be tagged with rooms; each play the run's
dice choose one. In a room that holds its own spot it stands there; in another, on the floor cell nearest the room's middle within a
step of a lane, not a door and not taken (`roomSpot`), so the owner sees exactly where each choice puts it.

**Actions** (what), each after a delay of its own if wanted: a torch group **lights**, **goes out**, **flickers to life**, **flickers
out**; **every light flickers** for a moment; a door **opens**, **unlocks** or **shuts**; a group of spawn points **wakes**; a
secret's wall **slides**; a **line** on screen; a **sound** from the game's own (a chime, the bell, a rumble, a knock).

## 2b. The format the game reads (settled 24 November 2026)

The editor's export (`tools/mapedit/core.js`, `exportDrawn`) writes what follows; `tools/mapedit/fixtures/events_full.c` holds every part
of it and `events_sample.c` the owner's example, both written by `node tools/mapedit/test.js`. **The game is built to this, not the
other way round;** a change to it is made in both and the fixtures written again. Coordinates are the drawing's cells (add `ox`, `oy`).
Every new field of `Drawn` goes at the end, so the tutorial's two rows, which leave them off, read as zeros: none of anything.

```c
enum { DT_GOLD, DT_KEY, DT_DEAD, DT_SGOLD, DT_MARK, DT_SWITCH, DT_GSEAL, DT_MAP,          /* as now */
       DT_COMPASS, DT_RELIC, DT_SOCKET, DT_LEVER, DT_PLATE, DT_SPOT };                  /* RELIC kind: its number in _RELIC;
                                                                                           SOCKET kind: the relic it takes, -1 any;
                                                                                           SPOT kind: its tag's number */
typedef struct { float x, y; int kind, chance, echance, elem, rank, wake, spot; } DrawnSpawn;   /* wake, spot: 1 + the group's, the
                                                                                           tag's number; 0 none (laid as the level is,
                                                                                           here) */
typedef struct { int x, y, d, group, lit; } DrawnTorch;                                 /* the wall's cell; d its face, 0 east 1 south
                                                                                           2 west 3 north; group a number */
enum { TG_PICKUP, TG_PLACE, TG_STRIKE, TG_ENTER, TG_USE, TG_STEP, TG_ALL };
enum { TT_ONCE, TT_EVERY, TT_TOGGLE, TT_TIMED };
typedef struct { int kind, type; float hold; int undo, thing, x0, y0, x1, y1; unsigned needs; int act, nact, back, nback; } DrawnTrig;
enum { TA_FLICKER_ON, TA_FLICKER_OFF, TA_LIGHT_ON, TA_LIGHT_OFF, TA_FLICKER_ALL, TA_FIELD_ON, TA_FIELD_OFF, TA_DOOR_OPEN,
       TA_DOOR_UNLOCK, TA_DOOR_SHUT, TA_WAKE, TA_SECRET, TA_LINE, TA_SOUND };
typedef struct { int kind, arg, x, y; float after; const char *text; } DrawnAct;       /* arg: the torch group, the field, the wake
                                                                                           group or the SFX_*; x, y a door's cell */
typedef struct { int thing; unsigned rooms; int tag; } DrawnPick;                       /* rooms: a bit a room in _ROOM; tag: the
                                                                                           spots', -1 none */
enum { DF_FIRE, DF_ICE };
typedef struct { int kind, x, y, on, dmg, chance, kills; } DrawnField;                  /* a cell of it; dmg: % of health each 0.5 s;
                                                                                           chance: % of its status each tick */
/* Drawn's row, after the spawn points: torch, ntorch, trig, ntrig, act, nact, relic (names), nrelic, pick, npick, field, nfield */
```

What each part does, as the editor's dream walk has it (`dream` in `core.js`; the game should agree with it):

- **A trigger** fires when its thing is reached and every trigger in `needs` (a bit each, by index in `_TRIG`) has fired: picked up
  (`TG_PICKUP`), a relic set in its socket (`TG_PLACE`), a spell landing on it (`TG_STRIKE`: a mark, the switch, the seal), the box
  walked into (`TG_ENTER`, the first time; `TT_EVERY` each time it is entered again), the lever used with the use key (`TG_USE`), the
  plate stood on (`TG_STEP`), or, for `TG_ALL`, once every one of `needs` has. `TT_ONCE` fires once; `TT_EVERY` each time; `TT_TOGGLE`
  on at one touch and back at the next, or by itself after `hold` seconds if `hold` > 0; `TT_TIMED` on, and back after `hold`. Going
  back, if `undo`, its torch and field and door actions are reversed in the reverse order (on/off swapped, a door opened shuts), then
  its `back` actions run. An action's `after` delays it from the moment its trigger fires (or goes back). A trigger never fires while
  what it waits on has not.
- **Torches:** a torch hangs on its wall's face as the run's own light (`T_FEAT + F1_TORCH`, three frames), lit or not; unlit, guttering
  and snuffed are the approved sheet's (`media/sheets/events/torch_unlit.png`, `tools/torchmock.c`'s `make_unlit` and `make_unlit_run`,
  moved into the art). Its light is a static light whose share is scaled each frame (lit 1, guttering about a third, snuffed a glow of
  embers, cold none). Flicker to life: cold, gutter, cold, gutter, lit over about 0.6 s, the group's torches one after another; flicker out:
  lit, gutter, lit, gutter, snuffed for 1 s, cold. `TA_FLICKER_ALL`: every light on the level stutters for about a second.
- **Fields:** every cell of its kind (`F` or `I`) joined to its cell. Live: fire harms `dmg`% of health each 0.5 s while stood in (the
  burning floor's own harm, `drawn_fire_tick`), with a `chance`% of setting the caster On Fire at the first touch; ice gives a Chilled stack
  each 0.4 s at `chance`% and holds what stands on it as the ice does now (`ice_tick`). Unless `kills`, neither takes the caster below a
  quarter of health, as now. Not live: the floor shows burnt out or thawed and does nothing. The tutorial's two levels keep their own fire
  and ice (they export no fields). **Mend this first:** `ice_tick` still tests `g_drawn != 2`, so ice does nothing on any other drawn level.
- **Picks:** each play the run's dice (not the drawing's seed) choose one of a thing's places: each spot of its `tag`, and for each room in
  `rooms`, each spot (of any tag) standing in it, or, with none there, the floor cell nearest the room's middle within a step of a lane, not
  a door, not taken (`roomSpot`: ties go up and left). A spawn point with a spot tag is laid at one of those spots the same way.
- **Things:** the compass, picked up, sets `RUN.has_compass` as if bought (held already: worth her price, `COMPASS_PRICE`, in gold); a relic
  is carried as the puzzles' things are (one at a time) and set in a socket that takes it with the use key; a lever (`TR_LEVER`'s look) is
  used with the use key; a plate (`TR_PLATE`'s) is stood on; a spot draws nothing.
- **Doors:** `B` is a door barred till an action opens or unlocks it; `TA_DOOR_SHUT` shuts and bars any door.
- **Rooms:** up to 24 on a drawn level; **secrets:** each `c` with its own room of `q` cells, up to `MAXSECRETS`.

## 2c. Hazards on drawn levels (asked and settled 25 November 2026)

The owner: "At the moment I can't place elemental traps, like rock falls or fire falls. Hazards with their impacted areas and settings
for their logic." Asked, they settled: **both** placed hazards and zones; set off **on a clock, by walking in, and by triggers** (any mix);
settings **harm and its status, warning time, live at the start, may kill**; the editor offers **the level's own run's** hazards, under
the run's name (a subcategory in the palette). The kinds are the game's own (`HZ_*`: each run's fixed ones `HZ_FIXED`, arising ones
`HZ_ARISE` and its sapper `HZ_SAPPER`, read by `extract.py`), with their art, sounds, warnings and elements; no new art.

**The shape of every hazard:** an **anchor** cell (on the floor, or in a wall for those set in one: the breath's niche, the cold crack,
bile's grate, the sluice, the weeping saint, the lodestone seam, the fissure) and the **cells it covers**, painted, in the order painted
(a line's order: votive wax, Hellrent's cracks, the cart's rails, the censer's arc, the timber strip, the fissure's breath). **Placed**
(`zone` 0): each time it goes off it covers all its cells, as the game lays that kind. **Zone** (`zone` 1): each time, one place within
its cells, chosen by the run's dice, as the game would choose near you (a rock on one cell; a crack line within the cells). A fixed kind
(a site) is always placed: its area is its own. **What sets it off** (`how`, bits): `HH_CLOCK` every `every` s with `give` s either way,
the first after `first` s; `HH_ENTER` each time the caster walks into the box `x0, y0, x1, y1` (inclusive, the drawing's cells), at most
once each `every` s if set; neither: only a trigger (`TA_HAZ_ON` makes it live, `TA_HAZ_OFF` not; going back swaps them). A fixed site
that runs by itself (the censer swinging, the cart's runs, the crystal's hum) runs while live, at the game's own pace or `every` if set.
**Live at the start** (`on`); a hazard not live does nothing and shows nothing of its warning. **Settings**, each -1 for the game's own:
`harm` (% of health a strike takes: `Hazard.share`), `chance` (% of its status: burning, poison, chill, the hold), `warn` (seconds of
warning, never under 0.9: `hz_warn`), and `kills` (0: it never takes the caster below a quarter of health, as the omens; 1: it may kill).
The dream steps round them as it does every hazard (`hz_dmap`).

```c
enum { HH_CLOCK = 1, HH_ENTER = 2 };
typedef struct { int kind, ax, ay, cell, ncell, zone, how; float every, give, first; int x0, y0, x1, y1, on, harm, chance, kills;
                 float warn; } DrawnHaz;                       /* kind: HZ_*; cell, ncell: a run in _HZCELL */
typedef struct { int x, y; } DrawnCell;
/* TA_* gains, at its end: TA_HAZ_ON, TA_HAZ_OFF (arg: the hazard, its index in _HAZ) */
/* Drawn's row, after the fields: haz, nhaz, hzcell, nhzcell */
```

At most 12 hazards a drawn level (`MAXSITE`) and 256 cells in all. The editor's dream walk ignores them (the game's dream steps round
them; a hazard cannot shut a way). The tutorial's two levels export none.

## 3. Steps

| | Step | Proof |
|---|---|---|
| 0 | This plan | |
| 1 | **The editor:** new maps start at level 3; the compass, relics, sockets, torches and the barred door; the events tab (triggers, actions, the owner's notes, the guidance); the dream walk follows triggers; the export's new arrays. The owner's example drawn as a sample level | `test.js`: the tutorial's round trip still exact; the sample level finishes in the walk and fails when its socket is unbound |
| 2 | **The unlit torch's sheet:** plain walls and each run's own, lit beside unlit, and the flicker's frames. **Stop for the owner** | the sheet in `media/sheets/events/` |
| 3 | **The game's data:** `DrawnTorch`, `DrawnTrig` (with its type, how long it holds, whether it goes back, its actions for going back), `DrawnAct`, `DrawnPick`; the compass, relics, sockets, levers and plates as things; `B`; **24 rooms** on a drawn level (`Room rooms[12]` and the arrays sized to it: `mroom`, `st_room`, `room_shrine`, `order` in `generate.c`) and **each secret its own room** (`gen_drawn` takes every `q` as one box today); a thing in one of its rooms by the run's dice, its cell chosen as `roomSpot` chooses | the sample level, and the owner's level 3, compiled from their exports. **Done** (the game's side, 24 November; harness only) |
| 4 | **The runner:** triggers heard where the game already acts (picking up, setting down, a spell landing, a cell entered), actions done; torch groups' light and look | the sample played by a scripted person: each trigger fires once, in order (`EVENT` lines). **Done** (harness only) |
| 5 | **The dream:** fetch a relic to its socket, pick up what a trigger waits on, wait on a door an event opens | the sample finished by the dream on eight seeds. **Done** (harness only) |
| 6 | **The proof:** the dream's logs on the tutorial and generated levels against the build before, line for line; `make asan` on the sample | logs, sanitizers. **Done** |

Nothing here is saved but the compass, which is saved already; an event's state is the level's and goes with it.

## 4. Hand-off log

**Session 1 (23 November 2026).** The plan, after the owner's four answers (fixed paths; events may gate the exit and the dream
learns them; hints are the owner's notes and the editor's guidance; torches placed by hand in groups).

*Step 1 done, in the artifact only.* New maps start at level 3. The barred door (`B`), the compass, relics (named), sockets (named,
bound to a relic or any), torches (on a wall's face, a group, lit or not) and a spawn point's wake group. The EVENTS tab: a trigger's
kind and its guidance, its thing chosen on the map or its box drawn, *only after*, the owner's note, its actions in order, each with
its guidance, its group, door, line or sound, and a delay. The dream walk follows the events (a relic carried to the socket that
takes it, one at a time; doors opened and shut by actions; a thing a trigger waits on left till it counts), and shows each torch lit or
not at the step shown. Checks for torches, relics, sockets, triggers (what they watch, a loop of waiting, more than 32) and a barred
door nothing opens. The export's new arrays (`DrawnTorch`, `DrawnTrig`, `DrawnAct`, the relics' names, `DT_COMPASS`, `DT_RELIC`,
`DT_SOCKET`, a spawn point's wake group): a level with none of them exports as before, so the tutorial's round trip is unchanged. The
owner's example is `sampleEvents()` in `core.js` and an example under OPEN: it finishes in 125 steps with every trigger fired in order
(the compass, the south room, the bone), and fails without the door's action or with the socket bound to a missing relic (`test.js`).
Tested by `test.js` and in Playwright's Chromium (a trigger bound on the map by hand; no page errors; no sideways scroll at 400 px).

*Step 2: the sheet, for the owner.* `build/torchmock out.png` (`tools/torchmock.c`) draws `media/sheets/events/torch_unlit.png`.
Each run has its own light on the wall (`T_FEAT + F1_TORCH`, three frames): the Crypts a sconce torch, the Catacombs a hanging
lantern, the Hollows an oil lamp, the Deep a seam of fire in the rock. Each is shown lit, guttering (two frames), just snuffed (embers,
a thread of smoke) and cold (a charred torch, dark glass, a bare spout, a cooled crack with a dull red vein), lit in its own light and
the rest in the dark. **Stopped for the owner's look.** The plain torch (`T_TORCH0`, off the drawn looks) is not on the sheet; it would
take the Crypts' treatment.

**Session 2 (23 November 2026): the owner's two asks, in the editor.** *A thing in one of a set of rooms:* any thing but the dead
may be tagged with rooms (the CHOSEN tab); the map shows a ghost where each choice would lay it; the export's `DrawnPick` (the thing,
a bit a room). The dream walk tries every way the dice can lay the level (all of them up to 64, else each thing's rooms in turn) and
shows the first that fails. *Triggers:* **Bound to**, a list of what each kind may watch beside choosing on the map; **Type**, once,
every time, a switch (with or without a duration) or timed (with one); whether its torches and doors go back with it; its actions
for going back. Two new things to bind: a **lever** (*Pulled or pressed*) and a **pressure plate** (*Stepped on*). The dream walk
keeps time at the dream's pace (`CRUISE`, 2.1 cells a second; it is slower round corners, so this flatters it a little) and, when a
timed door opens, says how far it is and how long it holds; a door that shuts before the dream has been through it is marked, and
a stuck dream waits for what is due to go back and tries the switch again. Checks: a timed trigger with no duration; a trigger that
goes back on a pick-up or a socket (the thing stays taken, so a door it held shuts for good); a room a thing may appear in that is
gone or has no floor to stand on. The room limit is 24 in the editor (12 noted as today's); several secrets are a note, not an
error. Tested by `test.js` (a lever holding the sample's barred door for 1 s cannot finish and says why, for 30 s finishes in 63 s;
a switch with a duration goes back by itself; the bone in one of two rooms, both finish) and in Playwright's Chromium (no page
errors). The owner's level 3 ("Storage", in the artifact's store) read through the checks: see the session's report. Step 2's sheet
still waits on the owner.

**The game's side, steps 3 to 6 (24 November 2026; its own worktree branch, for the work branch).** Built to section 2b as written;
nothing in the format changed. Harness only: nobody has played it, nothing ran under Wine, the Windows and browser builds were not
compiled here (no MinGW or Emscripten in the session; `clang -fsyntax-only` on `scene_crawl.c` shows no new warning).

*Step 3, the data.* `drawn.c` has section 2b's types and enums and the new fields at the end of `Drawn` (a `#pragma` lets the
tutorial's rows and short spawn points leave them off, so the editor's round trip and `extract.py` read it as before). `B` is a door
barred (`GATED`) and drawn as a portcullis (`T_GATE`); like the tutorial's gate it stays up once raised (`g_gate` in `generate.c`), and
a door an event shuts becomes one. `MAXROOMS` 24 (`Room rooms[]`, `mroom`, `st_room`, `room_shrine`, `mz_vault`'s `order`); the
generator's levels test `room_shrine` at 12 as before. Each `c` takes the `q` cells joined to the cell behind it as its own room (the
owner's level 3: (4,39)-(6,41) and (20,37)-(27,41)). The picks (`drawn_picks`) are chosen before the level is laid, on the run's dice,
from `roomSpot`'s rule; the spot-tagged spawn points after them, in their order. The compass is an item (`IT_COMPASS`, drawn as her
ware, floating low): yours as if bought, or $2,500 if held (`COMPASS_PRICE` moved to `state.c`). Relics, sockets, levers and plates are
the puzzles' things (`SP_PZ_FOSSIL`, `SP_PZ_PLINTH`, `SP_PZ_LEVER0`, the flagstone), `Thing.ev` and `Item.ev` saying which drawn thing
each is. A drawing that hangs torches has those alone (the generator's along the halls are not laid: `hall_torches`, moved out of
`gen_dress` unchanged). Fields (`ev_fields`) on a drawing that exports them, with their harm, chance and whether they kill; the
tutorial's DF and IC untouched; `ice_tick` reads any drawn level. The unlit torch's four frames are the art's (`src/art/parts/unlit.c`,
`T_UNLIT`), and `build/torchmock` draws the approved sheet from them byte for byte.

*Step 4, the runner* (`game/events.c`, after `things.c`). Heard in `collect` (pickups), `pz_touched` (a relic taken or set, a lever,
a plate), `mk_finish` (a thing struck: when it is done, its pips filled), and each frame (boxes; triggers due back). Needs, the four
types, `hold`, undo in reverse then the `back` actions, each action's `after` from a queue; a trigger going back drops what it had still
to do. Torches: lit, cold, flickering in (cold, gutter, cold, gutter, lit, 0.12 s a beat, the group's 0.15 s apart) and out (lit,
gutter, lit, gutter, snuffed 1 s, cold); their light's share 1, 0.33, 0.08, 0. Every light stutters 1.1 s. Fields catch over 1 s and
burn low over 1.5 s; ice turns cell by cell as the tutorial's. `EVENT ev_fire`, `ev_back`, `ev_act`, `ev_wake`, `ev_dream`,
`ev_fire_floor`, `compass_taken` in the debug build. A scripted person through the sample (`HOST_INPUT`, taking over at 21 s, seed 1):
the compass at 29.4 s, the south room at 33.5 s, the bone at 51.5 s, each once, in the editor's order; it was not walked to the exit.

*Step 5, the dream* (`ev_goal`, after the puzzles in `pick_goal`). The nearest of: the socket that takes what it carries; a relic some
free socket takes; a lever, a plate, a thing a trigger watches struck (from the lane cell nearest it in reach, where `ev_at_strike`
casts as `dream_looks` does); an item a trigger waits on; a box; and, first, the far side of a door held open for a time. When nothing
new is left and the way out is shut, what fires again is tried again (up to four times), as the editor's walk does. It leaves an item
whose trigger waits on another (`worth_taking`, `update_items`, `key_goal`). A socket refuses a relic while its trigger waits, and keeps
what is set in it.

*Step 6, the proof.* (a) The dream's logs (`apl crawl 3000`, `CRAWL_RUNID=7`) against `c9f836e` built in a worktree, seeds 1 to 8, 11,
13 and 15 (eight distinct runs: seeds are OR-ed with 1): line for line the same but the line that prints a time taken (`PORTRAITS ...
kept in`), the tutorial's two levels and 8 to 10 levels a run. (b) The fixtures pasted as slot 3 in scratch copies (not committed), the
dream from level 3 (`CRAWL_LEVEL=3`, 500 s), seeds 1, 3 ... 15: `events_sample` finished on all eight, its triggers in the editor's
order each time (the compass, the south room, the bone); `events_full` compiled and finished on all eight (the lever, the timed plate
going back and fired again in passing, the field put out, the compass at a spot); `storage_level3` finished on seeds 1, 3, 5, 7 (its
compass stays behind the barred door, which nothing opens yet). Extra checks the same way: the sample with the bone's trigger timed for
20 s (the dream ran for the door and through it, seeds 1 to 3); the sample with 24 rooms and the bone picked into rooms 22 or 23
(both laid, finished); the owner's level 3 with a strike trigger on its switch opening B, the compass after it and an "all of these"
sliding a secret (finished, seeds 1 and 2). (c) `make asan`, `ASAN_OPTIONS=detect_leaks=0`, the three fixtures, seeds 1 and 4, 600 s:
clean. (d) Saves untouched: `Item`, `Thing` and the events' state are the level's and never saved; the compass found sets what buying
does, saved as before.

*Read where 2b leaves room:* `TA_SECRET` slides the first secret not yet found, in the drawing's reading order (a moment later if
another is moving). `TG_ENTER` fires again on entering only when `TT_EVERY`, and `TG_ALL` once, as the editor's walk. A timed plate holds
while stood on and `hold` after (the editor's guidance). A strike trigger not ready, or one that fires again, has its thing whole again
2 s after. A spot draws nothing. Limits: 64 torches, 32 triggers, 8 fields of 192 cells in all, 96 actions waiting, 256 things.

*Left:* the owner's look at the torches, the barred door's portcullis and the borrowed looks of relic, socket, lever and plate, in the
game (new art only on their word); the owner's level 3 with events, once drawn; the browser's and Windows builds compiled by CI; the
owner's hand on any of it.

**Session 4 (25 November 2026).** The owner's answers: new art for the barred door, relic, socket, lever and plate (a sheet first);
level 3's path is the relic set in a socket to open the barred door; a place to mend may face a secret room; wake-group points stay
empty till their event. Built on the work branch: a place to mend falls back to a secret room's floor when nothing else is open beside it
(`drawn_front`'s `secret_ok`; the exit keeps to open floor), in the game and the editor's checks alike; `EV_TORCHES` 120 (the owner's
level hangs 87; `at_torch` is a byte); a torch may hang on the map's outer ring, facing in (four of the owner's do); the editor warns
past 120 torches and says that a gate an event works should be a barred door. Proof: the dream's logs, seeds 1 and 4, 1,500 s, against
`68876dd`, the same but the time-taken line, for each change. The play test: the owner's Storage (saved version 18) on the branch
`claude/optimistic-thompson-boccz1-playtest`, never to be merged: its two gates drawn as barred doors (the owner's map still has them
as gates), a new run begins on level 3 and the build keeps its own registry key (`HalloweenVGASaver-Playtest`), so the owner's saves are
apart. In the harness, seeds 1, 3 and 5, 420 s: the dream goes round the top loop (the plate in, the plate shutting it behind, the lever
out), sets the relic, takes the compass and reaches level 4 without dying; the fire lever and its wake on seeds 1 and 5. 87 torches
hung; 1.54 ms a frame on the level. `make asan`, seed 1: clean. The Windows build from CI's run on that branch, for the owner to play.

**Session 4, later (25 November 2026).** The owner could not take the relic on Storage. The generator's dressing, laid on a drawn level
by its seed, had set a pillar in the relic's cell (19,1): `drawn_clear` turned props back to floor but not pillars (pillars, and the
totems, lamps and flowstone laid as them). It clears those too now; a drawing has no pillar of its own. Proof: a scripted person at the
relic, E: "YOU TAKE A RELIC"; the dream's logs, seeds 1 and 4, 1,500 s, against `68876dd`: the same but the time-taken line; Storage read
from text, seed 1: the same triggers in the same order, the compass, level 4 (its times a few hundredths apart, the pillar gone).

**Hazards, the game's side (25 November 2026; its own worktree branch, for the work branch).** Built to section 2c as written; nothing in
the format changed. Harness only: nobody has played it, nothing ran under Wine, the Windows and browser builds were not compiled here (no
MinGW or Emscripten in the session; `clang -fsyntax-only` on `scene_crawl.c`, with and without the debug switches, shows no new warning).

*The data and the reader.* `drawn.c` has 2c's `HH_*`, `DrawnHaz`, `DrawnCell`, `TA_HAZ_ON` and `TA_HAZ_OFF`, and `haz, nhaz, hzcell, nhzcell`
at the end of `Drawn` (the tutorial's rows leave them off). `drawn_text.c` reads `HAZ` and `HZCELL`, the row's four new values (checked
against what was read), every `HZ_*` the editor offers (all but the knight's arc and the Wraith's hands), and `a | b` between two names or
numbers. It refuses, by name: a kind it does not lay, an anchor off the drawing and its ring, cells outside the drawing or past `_HZCELL`,
a `TA_HAZ_*` on no hazard, settings out of range. `drawncheck` compares hazards and cells. It also read `events_full.c` wrongly as out of
range (a spawn point laid at a spot is written at (-4.5,-4.5)); it reads it now.

*The runner* (`game/drawn_haz.c`, after `events.c`; its state `DH` in `drawn.c`, the level's, never saved). Laid where the generated levels'
sites are (`dh_lay` in `start_level`): drawn levels still lay none of the run's own, and nothing arises there but the drawing's. The fixed
kinds stand as sites (`G.site`, run by `update_sites`), the rest arise as `update_arising` lays them (`G.haz`), so their art, sounds,
warnings, harm to the dead, naming and the dream's map (`hz_dmap`) are the game's own. A hazard not live is still and shows nothing.
`EVENT dhaz <i> <kind> <clock|enter|trigger|own> at (x,y)`, `dhaz_on`, `dhaz_off`, `dhaz_drink` (a sapper begins to drink); `DRAWN level n:
k hazards, c cells` where it is laid, and a line for one that has nowhere to stand.

*Read where 2c leaves room.*
- **Anchors** may lie on the drawing's ring (the sample's niche is at (17,-1), a wall in the edge). A wall kind stands on the floor beside its
  anchor that is painted, else the side nearest the first painted cell. The dressing laid on the drawing's dice (props, pillars) is cleared
  off a hazard's cells and a floor anchor, as off the drawing's things; a cell that is not open floor (a door, a secret's room) is dropped.
- **The clock:** the first `first` s into the level, or after a trigger wakes it; then `every` s, give or take up to `give` on the run's dice,
  at least half a second. **The box:** the caster's cell coming into it, at most once each `every` s (the export always writes one).
  **Neither:** each `TA_HAZ_ON` wakes it and sets it off once, at once (the editor's guidance); a fixed one with neither then runs at the
  game's own pace while live (its first turn at the trigger). `TA_HAZ_OFF` stops it at once, its warning and all (the turn put out, the
  grate a wall again, fallen timbers cleared); going back swaps them, as the other field actions.
- **Fixed kinds with turns** (breath, cold, sluice, fissure, timbers, firedamp, saint, cart, lodestone, crystal): one turn each time set
  off; a turn still running holds the clock till it ends, and a box entered meanwhile is not counted. **Those that run by themselves**
  (censer, web, dark cold, tithe, mirror, reliquary) run while live: the clock gives the censer its swing (a swing every `every` s) and knits
  the web again every `every` s once torn; their box does nothing.
- **Their shape from the cells:** a line's cells in the order painted (24 at most a site or a turn, `HZ_CELLS`; the rest are dropped); a
  patch's steps from the anchor. The censer swings the way its arc runs most, as far as its farthest painted cell (half a cell to four).
  The lodestone and the crystal reach their farthest painted cell (one to four). The mirror, the tithe and the reliquary keep the game's
  own reach; the web is its anchor's cell. The cart runs from the first painted cell to the last, and on the Hollows' levels its cells take
  the rails. The crystal and the reliquary make their anchor a prop, as the game's do.
- **What arises:** placed, a rock, void or glass on each cell (void and glass half a cell wide, so that they do not stack; a zone's is the
  game's size); ash one fall over all its cells; votive wax the rack at the anchor and then the cells in order; Hellrent the cells in order;
  bile, galvanic water and silt all their cells, stepping through them from the grate or the first. A zone: one cell, near the caster and in
  sight (within six) if any is, else any, on the run's dice: a rock, void, glass or ash there; Hellrent or wax a straight run within the
  cells from it (four to six, three or four long), the way most of them run; bile, water and silt the game's patch within the cells (two,
  four, one or two steps). Bile's grate is the anchor's wall while it runs, as the omen's.
- **The settings:** `harm` is the share of a strike, and of a second for what burns or seeps (Hellrent, wax, fissure, cold, ash, the dark
  cold); a sapper drinks souls at the game's rate and, with `harm`, that share of health a second too. The game's cap of a quarter a stroke
  (`hz_strike_el`) stands, so above 25 only the rock and what seeps feel it. `chance` takes the place of the stroke's and the cold's roll
  (`af_dchance` while that hazard is at work), is rolled once a turn at the first touch for poison (breath, firedamp, silt, bile), sets
  you On Fire at the first touch, as a field does, for Hellrent, wax and the fissure (whose own have no such roll), and is the web's chance
  of holding you. `warn` goes through `hz_warn`. `kills` 0: `hurt_ex` never takes the caster under a quarter of health from a hazard of a
  kind every drawn hazard of which may not kill. It goes by the kind, the only thing blame carries, so a fire field and a Hellrent that
  may not kill share `HZ_FIRE`'s blame (the field's own floor still holds).
- The first of each kind is named, as anywhere (`RUN.omens`, saved already).

*The proof.* (a) The dream's logs without drawn hazards (`apl crawl 3000`, `CRAWL_RUNID=7`) against `dc7880c` built in a worktree, seeds 1
to 8, 11, 13 and 15 (eight distinct runs): line for line the same but the time-taken line (`PORTRAITS ... kept in`), the APL lines too;
the tutorial's two levels and 8 to 11 levels a run. (b) `tools/mapedit/fixtures/hazards.c` with `HOST_DRAWN` (a new run on its level 3),
the dream, 500 s, seeds 1, 3, 5 and 7: the roof went off on its clock 26 to 30 times, the first 2 s into the level (t=22.53 each time),
always at one of its three cells; the niche once each run, when the dream walked into its box (t=83.31, 79.36, 78.17, 79.33); the candles
once each, on the compass's `TA_HAZ_ON` 1 s after it was taken (`dhaz_on 2` at 30.24 or 30.66) and never before; the level finished each
time, no death on it. A scripted person (`CRAWL_TPAT=22.5,8.5,270`, `CRAWL_STAGE=0`, `CRAWL_STAGEDIST=99` for no creatures,
`CRAWL_TITLE=0`, `HOST_INPUT`) stood in the top room: the frames show the grit in the dark, then the rock's blast and its rubble on (16,0)
and health 98% to 78% (the drawing's 20); walking north into the box set the niche off (the slab cracked open, its seam lit, the view gone
green with poison). Four scratch variants, not committed, the same map with each run's other kinds (hazard 2 of each woken by the
compass): the Crypts' censer, cold vault, a placed rock woken, the reliquary and a wax zone sure to burn (level 3; the censer woken by the
south room on a timed trigger and stopped 8 s later as it went back); the Catacombs' sluice, web, bile at half chance, a galvanic zone,
silt at its box and the saint at its own pace (13); the Hollows' timbers, firedamp at its box, a lodestone woken, the cart, Hellrent
placed and as a zone and the tithe (23); the Deep's fissure, crystal, a void zone woken, the dark cold, ash, glass by clock and box, and
the mirror (33). Each kind went off, or drank, ran or held, with no error; the first three levels finished on every seed; on 33 a new
run's caster died to the dead six to ten times in 500 s (none to a hazard: every death was a creature's), as it does there with no hazards
(3 and 4 deaths, seeds 1 and 3). A frame of the Deep's: the mirror drinking, its violet thread to you. (c) `make asan`,
`ASAN_OPTIONS=detect_leaks=0`, 600 s, seeds 1 and 4: the fixture, `events_full.c` and the four variants: clean, no report.
`build/host_asan drawncheck` on the fixture cut short every 7 bytes (869 cuts) and at every byte of its last 1,376 (the hazards and the
row), and on the Deep's variant at every byte of its last 1,900 (with `HH_CLOCK | HH_ENTER`): each refused, or read when the row was
whole; no crash. Eight broken texts refused by name. The fixture pasted as `DRAWN[2]` in a scratch copy: "the same, field for field".
`make smoke` passes. (d) Saves untouched: `Hazard`, `HzSite` and `DH` are the level's and never saved, and `save.c` and `ledger.c` did not
change.

*Left:* the owner's hand on any of it; the editor's palette, settings and painting for hazards (the other track's); whether `harm` should
lift the quarter-a-stroke cap for a drawn hazard; the mirror, tithe and reliquary from their painted cells, if the owner wants them so;
the browser's and Windows builds compiled by CI.

**The things' sheet (25 November 2026), for the owner.** `media/sheets/events/event_things.png`, drawn by `build/lightmock things`
(`tools/lightmock.c`; docs/PLAN-lights.md section 4 has the tool). Today's borrowed looks on its top row, then each run's: the
**barred door**, the run's own door with a beam across it in two staples and a chain crossed over it to an iron plate with a hand
cut in it (no keyhole, so nothing says a key opens it), and unbarred, the beam and plate gone and a chain hung loose, for an
unlock before the door slides; the **relic**, a bone bound in gold, a key's teeth at its end, a red seal on a cord; the
**socket**, a plinth with two stone hands held up on it and the relic's shape cut in its face, and holding, the relic across the
hands and the cut alight; the **lever**, a stone post, a toothed iron quadrant and a twisted arm, up and down; the **plate**, a
floor texture, a flagstone in a deep joint with a rose cut in a roundel, and pressed, sunk, darker, dust at the joint. A row in
the dark, and crops of the game's view at its own size in the Crypts and the Deep. Nothing in the game. *For the owner:* the
relic is one look for every relic whatever it is named; the hands are small at a distance; in the Deep the barred door is dark.

**The owner on the events' things' sheet (25 November 2026).** One relic look is fine for now; **from here on, think of twelve unique
relic types** (a sheet of twelve when the time comes, each its own shape, chosen per relic in the editor). The socket's small hands and the
Deep's dark barred door: left as they are for now. **An unlock shows the bar gone** (the sheet's "unbarred" look) before the door is
opened. One detail pass more over the sheet, then back to the owner.
