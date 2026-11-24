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

## 3. Steps

| | Step | Proof |
|---|---|---|
| 0 | This plan | |
| 1 | **The editor:** new maps start at level 3; the compass, relics, sockets, torches and the barred door; the events tab (triggers, actions, the owner's notes, the guidance); the dream walk follows triggers; the export's new arrays. The owner's example drawn as a sample level | `test.js`: the tutorial's round trip still exact; the sample level finishes in the walk and fails when its socket is unbound |
| 2 | **The unlit torch's sheet:** plain walls and each run's own, lit beside unlit, and the flicker's frames. **Stop for the owner** | the sheet in `media/sheets/events/` |
| 3 | **The game's data:** `DrawnTorch`, `DrawnTrig` (with its type, how long it holds, whether it goes back, its actions for going back), `DrawnAct`, `DrawnPick`; the compass, relics, sockets, levers and plates as things; `B`; **24 rooms** on a drawn level (`Room rooms[12]` and the arrays sized to it: `mroom`, `st_room`, `room_shrine`, `order` in `generate.c`) and **each secret its own room** (`gen_drawn` takes every `q` as one box today); a thing in one of its rooms by the run's dice, its cell chosen as `roomSpot` chooses | the sample level, and the owner's level 3, compiled from their exports |
| 4 | **The runner:** triggers heard where the game already acts (picking up, setting down, a spell landing, a cell entered), actions done; torch groups' light and look | the sample played by a scripted person: each trigger fires once, in order (`EVENT` lines) |
| 5 | **The dream:** fetch a relic to its socket, pick up what a trigger waits on, wait on a door an event opens | the sample finished by the dream on eight seeds |
| 6 | **The proof:** the dream's logs on the tutorial and generated levels against the build before, line for line; `make asan` on the sample | logs, sanitizers |

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
