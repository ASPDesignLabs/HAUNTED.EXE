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
; and on a HAZ line (2e): EMIT rate speed spread life, DIR degrees, RADIAL tiles; the censer's LEN tiles, HANG s, SWING s
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

### 2e. Run I's traps drawn, and what pours and drifts as particles (session 5)

Asked for on 26 November: art for the traps, each with a sign of where it is and what it does while idle, starting with run I's; the censer
on a chain between two ends the owner sets, three tiles of clearance at the least, its hang and swing set; the breath and the cold vault as
tiles from their crypt pictures; a mark that stays after a rockfall; votive wax from a candle in a wall sconce that tips and pours, its way
and rate set by painting; the fluids as particles, from emitters tuned in the World Editor, with a PREVIEW there and a trial by PLAY; the
reliquary's emitter radial, three tiles by three. The owner chose (asked): the sconce, its tip and rate by the cells painted; wax that pools,
cools and fades; a crack in the roof that stays, with rubble after that is walked through. Built (`game/side_fx.c`, its drawing in
`draw/side.c`'s `sd_draw_fx`):
- **Particles**: wax, vapour, rime, haze, smoke, grit; 900 at once. Wax and rime settle as a falling sand does, on a grid of two pixels: down,
  then aside while warm (wax cools over 2.6 s, burning above four tenths), so it runs, pools and stops. Gases drift with drag and slide along
  stone. What harms counts what touches the caster: hot wax one, rime and haze two, breath three; a bite each half second (TUNING:
  bite_every), by the trap's element; the reliquary takes a soul with each. The dream keeps out of tiles where two or more harm.
- **The censer**: hung at its anchor, its ends the first two cells painted; its swing eased, still for HANG at each end, SWING a pass; the
  chain ten links of verlet pinned to the hook and the bowl, two per cent slack so it bows on the swing; a thin smoke behind. Struck, a fifth
  of health and thrown the way it swings. Its chain three tiles at the least (the game makes it three; the editor refuses less) and the editor
  refuses an arc that meets stone. Set at head height, a crouch goes under it.
- **The rockfall**: a crack in the roof over each painted cell, always; grit from it in the warning; rubble on the cell after, till a life
  is lost or the passage is left.
- **The votive sconce**: an iron sconce on the back wall; its candle tips toward the side more cells are painted on, further the more there
  are (to four), and pours 18 a second for each, from its lip along the candle; lit while it stands.
- **The breath and the cold vault**: the crypt's `T_HZ_SLAB` (open in its warning and its turn) and `T_HZ_COLD`, each made one tile of its
  4 by 4 commonest colours (`side_art_traps`, as the run looks); their vapour and rime from the open side, kept to the painted cells if any.
- **The reliquary**: the crypt's sprite, open in its turn; its haze all round to its reach (1, three by three), marked on the floor.
- A turn: a warning (the thin breath first; the candle tipping), its turn (STAYS, at most 4 s; the sconce 2.2), a rest; while idle a wisp
  now and then that does no harm. `SIDE_TEXT[2]`, *Run I's Traps*, holds one of each; it is also `tools/mapedit/fixtures/side_traps1.txt`.
- **The editor**: on a trap's CHOSEN, THE EMITTER (rate, speed, spread, life, its way, the reliquary's reach; -1 or *its own* for the game's)
  or THE SWING (chain, hang, swing); ITS OWN puts them back; PREVIEW runs `SideCore.fxSim`, the game's rules over again, on the map, round
  and round, and follows each change. The map shows the censer's arc, the sconce's way and the reliquary's reach.
- **Run II's** (session 6, the owner having approved run I's sheet; my defaults, the sheet `media/sheets/side/traps_run2.png` to approve):
  **bile**, the crypt's `T_GRATE` as a tile, a green fluid from its open side (up and slow from a grate in the floor) that runs and pools,
  poison and a fifth slower, dripping while idle; **the sluice**, `T_HZ_SLUICE` shut and open, a surge of water toward the side more cells
  are painted on, carrying the caster along and chilling; **the galvanic pool**, standing water over its painted cells always shown,
  crackling in its warning, then a bolt from the roof and all in the water struck once; **silt**, mud over its cells, bubbling, a sallow
  gas that rises a little and hangs and poisons, set alight by a fire spell through it (`sd_fx_ignite` from `sd_shots`), burning out in a
  second; **the weeping saint**, `T_HZ_SAINT` and lit, tears falling from her alcove to a pool that drinks a soul with each bite; **the
  snare**, a web across its cells: walked into, it holds a second and a half (CHANCE), tears, sends the dead within eight tiles toward
  the caster, and is spun again after STAYS seconds. `SIDE_TEXT[3]`, *Run II's Traps*, one of each (`fixtures/side_traps2.txt`).
- **The dream and the traps** (session 6): it keeps out only of what burns (hot wax, flames, a galvanic pool in its warning, the censer's
  way) and walks through the rest as a person would, taking a few bites; it does not stop to fight while any harm is on it, and lets a
  drawn flask go when harm comes. It now holds the flask as long as the distance wants (`sdd_fight`).
- **A fault found and mended** (session 6): the flask's hold and the poison's bite shared one clock (`SD.held`), which the poison set back
  to four tenths every step, so every flask, a person's too, flew as if barely held. The poison has its own clock now (`SD.pois_t`), and
  the flask's throw follows how long it is held, as TUNING's flask_near, flask_far and flask_hold say. Throws held long now go further
  than they did: numbers tuned by hand since may want looking at again.

### 2f. Quicker hands, the flask aimed, powerups, timed switches, and things laid by a trigger (session 7)

The owner's asks of 26 November, before the squash; my defaults where they left the numbers open.
- **Casting twice as quick** in a passage: every spell's time between casts is the crypt's times TUNING's `cast_every` (0.5).
- **The flask**: a tap throws it at once to where it is aimed when pressed, the nearest of the dead ahead within its reach (about 227 px,
  `SD_FLASK_KMAX`), else halfway between near and far. Held past 0.15 s, left and right (the keys, the pad's d-pad or stick) carry the
  mark further or nearer, the way the caster faces being further, at TUNING's `flask_aim` (140 px a second); the caster stands still
  while it is held; let go, it flies. `flask_hold` no longer sets the throw. The dream only taps.
- **Powerups**: `POWER x y RAPID|MIGHT|WARD [secs]` (10 unless told; 120 at the most), taken in passing, once, and given back with a life
  lost as the gold is. *Quickened hands* halves the time between casts again; *Might* doubles all harm the caster's spells do and hangs a
  red glow about the caster; *Ward* lets nothing harm the caster (a blue glow). Each is said on taking, with a shimmer and a tone; its time
  is a bar under the lives; another of its kind starts it again. Their pictures are placeholders until the owner's art.
- **A timed switch**: `SWITCH ... TIMER s` (0 to 120). Thrown, it goes back by itself after its seconds, and that going back is a throw like
  any other to the triggers (FLIP undoes, HOLD lets go). The while, a metronome: a tick every two thirds of a second at first, an eighth at
  the end, louder and higher as it nears; on every other beat a chord, A minor and B flat by turns, both climbing a half step at a time
  to seven; at the end a knock and a low bell. A dial on the lever empties as the time runs.
- **Laid by a trigger**: `LEDGE x y w [OFF]`, a ledge of `=` w tiles wide a trigger's ON lays and OFF takes up (a dotted outline while it is
  up), and `FALL ... OFF`, a falling block not there till an ON (a faint outline). Each lays with a knock and goes with a creak.
- **Sound where it happens** (the owner: far creatures were heard): what the dead, the traps, the works and a flask's splash make is
  placed (`sd_sfx_at`): whole within 140 px of the caster, fading to nothing by 260 px (about 100 past the screen's edge; up and down
  count half again), panned to its side. The caster's own sounds and a timed switch's metronome stay whole.
- **The dream** goes to a lever and throws it when it has stood two seconds with no way on (only a lever a trigger listens to).
- **The editor**: *Switched ledge* among the ways, *Timed switch* and the three powerups among the things; the switch's BACK AFTER, the ledge's
  width, the powerup's kind and seconds, OFF on ledges and falling blocks; ledges and blocks may be a trigger's target; CHECKS warns of
  something off with nothing to lay it, and a timed switch without a name. `tools/mapedit/fixtures/side_works2.txt`, *The Timed Lever*,
  has one of each (for `HOST_SIDE`; not built in).

### 2g. The jump's sound and a life lost (session 7)

The owner asked to hear a few jumps and three death jingles, and chose. All are in `snd.c` at the end of the list, drawn in its own
synth: `SFX_SJUMP1` a cloak's swish, `SFX_SJUMP2` a hop, `SFX_SJUMP3` a boot and a breath, **`SFX_SJUMP4` the bone pipe (chosen)**;
**`SFX_SDEATH1` the toll (chosen)**, bells stepping down D, B flat, G, D and slowing, an organ's D minor and a deep bell under the last;
`SFX_SDEATH2` the music box running down, its last note sagging a whole tone; `SFX_SDEATH3` the choir's sigh, a harp falling and voices
sinking to a low D minor. The rest are kept, at the owner's word, for other things to do. TUNING's `jump_sound` (4) and `death_sound` (1)
choose among them, 0 being what was there before (the whoosh; the groan, and nothing for the drop); the toll plays for the drop too.
`host music none secs out.wav sfxN:gain@t` renders an effect alone.

### 2h. The dead in a side passage: what they see, how each kind fights, and what strikes (session 8)

The owner, 26 November: some of the dead were not moving or attacking properly in a passage; the skeletons' bones came too quick and
true to step out of; the bats' hits did not match the bats; what flies should strike by its pixels, not a box. All in `game/side_dead.c`.
- **Masks.** Each creature's picture is a mask made by the drawing's own rule (`sd_draw_spr`: half size, a pixel where two of its four
  are), cached per sprite (`sd_mask`); `sd_mob_spr` is the frame shown, the same function the drawing calls, so what is tested is what
  is drawn (checked: every drawn pixel of the bat, hound, whipper, spider and imp against its mask, none apart). The caster's figure is
  tested by its frame's pixels (`sd_fig_px`). A shot is moved a pixel at a time and each of its pixels tried (`sd_shot_shape`: the
  dead's bone a five-pixel stick turning, the rest round, the same pixels drawn); lightning and the drain strike the first picture their
  line crosses; the flask's splash reaches what has a pixel within its round. A blow lands where the creature's pixels, moved its reach
  ahead, meet the caster's (`sd_mob_touches`). The boxes were well off: the bat's covered 0 to 14 pixels over its feet while it is drawn
  14 to 30 over them (6 to 21 on the other beat), and the hound is 30 wide where its box was 16.
- **Sight and memory.** The dead see as before (`dead_sight`, a clear line from their picture's middle); once you are out of sight they
  go on to where they last saw you for `dead_memory` seconds; struck, they know from where.
- **Warnings.** Every blow, throw, leap and dive comes after a warning, the crypt's own wind-up (`ATTACKS`, `attacks.c`) times
  `dead_wind`, never less than a quarter second; its last moment the creature flashes white, a skeleton holds its bone up, an imp's
  ember and a wall skull's fire grow in its hand and mouth. Strength, element and wait are the crypt attack's own (`k`, `el`, `cool`).
  Having struck, a walker steps back and gathers itself half a second.
- **The bone.** Thrown after a half-second warning at where you stood then, never ahead of you; a high, slow lob, `bone_time` seconds
  in the air over seven tiles (never less than 0.8 of it, near or far), under `bone_grav` (380, where the caster falls at 950), so it
  hangs at the top some 40 pixels up; flatter under a low roof. It lands within `bone_aim` pixels of the mark (8, less for higher ranks):
  stood still you are hit, a step aside and you are not.
- **Each kind's way** (`sd_way_of`): walkers (the shambler, guard, drowned, hound, knight, spiderling, lightless, rat) come at you along
  the floor, hop a step a tile high, will not walk into spikes or poison (the drowned wade it) nor off an edge but after you and not far
  down; the quick ones (hound, ghoul, hunter, rat, spiderling, lightless) jump up to a ledge you stand on and over a gap of two or three
  tiles. The guard charges now and then. The skeleton keeps off (40 to 110 pixels) and throws. The hunting spider and the ghoul leap at
  where you stood, and land a moment helpless. The bat circles beside you, its middle a little over your hand so a straight bolt catches it as it
  bobs, flutters, dives along the line to where you were and climbs away; its bite (and a spiderling's, a rat's) only nudges you, where
  the rest throw you back as before (a bat over the Ossuary Stair's falling blocks threw the dream off them, a death the old bat, whose
  box lay under its picture, never dealt). The spectre drifts through the stone, shrieks when close and melts away, harmless. The shade comes at you; the wraith through
  the stone. The imp hangs off, high, and throws embers (`dead_shot`). The wall skull stays where it is set, glows a second and spits a
  slow fireball. The spider hangs on a thread from the roof over where it is set and drops on you passing under, then climbs back. The
  egg sac opens into three spiderlings when you come close. The brood mother lays a spiderling every five seconds, three at once at
  most. The deacon keeps back, calls the dead near the first time he sees you, and chants the fallen of bone up again as skeletons, four
  at most (a blow breaks the chant). The sexton keeps back and rings: the dead within fourteen tiles know where you are for eight
  seconds (frost stops the bell). The whipper-in winds his horn when he has hounds (they come on, quicker) and lashes with his whip,
  whose line strikes by its pixels. The gargoyle is stone till you come within three tiles, shakes itself loose and dives as a bat.
- **Tuned** (TUNING in the World Editor, at the end of the list): `bone_time` 1.1, `bone_grav` 380, `bone_aim` 8, `dead_wind` 1.0,
  `dead_memory` 3.0, `dead_shot` 110.
- **Not changed:** the creatures' pictures and their size against the caster (they stand about twice its height since the caster was
  halved; step 13 is their own art); the crypt and its dead.

## 3. Steps

| Step | What | State |
|---|---|---|
| 0 | This plan | Done |
| 1 | The draft art as a sheet: tiles, platform, block, water, spikes, doorway, switch, key, life; the caster's walk, run, jump, crouch, cast and fall | Done, a draft (`media/sheets/side/side_draft.png`), in the game as it stands |
| 2 | The mode: in by a doorway and out again, the caster's box and physics, the tiles, the camera, the status bar, lives and their end; a test passage built in; `CRAWL_TPSIDE` | Done |
| 3 | Moving platforms, falling blocks, toxic flows, spikes | Done |
| 4 | The spells, by element | Done; their numbers the owner's to tune |
| 5 | The creatures (the run's kinds, in their bestiary pictures until their own are drawn) | Done, harness only (2h): each kind its own way, warnings before every blow, pixel-true hits; their numbers the owner's to tune |
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

- `CRAWL_SIDECAM=x,y` (the view held at that tile, for frames) and, with `CRAWL_SIDETRACE`, `SIDE fx n: wax .. vapour .. rime .. haze ..
  smoke .. grit .., n settled` twice a second.
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

New files: `src/crawl/game/side.c`, `side_text.c`, `side_dead.c`, `side_dream.c`, `src/crawl/draw/side.c`, `src/art/side_art.c` and
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

**Session 5 (26 November 2026): run I's traps and the particles (2e).** Main took steps 7, 11 and 12 squashed first (`9720c44`) and the editor
was republished with that game before this began, so the other agent on the editor could fetch it. Built as 2e says. Tested:
- The frames: `media/sheets/side/traps_run1.png` (sent to the owner as a draft; not approved yet), from `HOST_SIDE` and `CRAWL_SIDECAM`.
- The dream through *Run I's Traps* on four seeds, four of four at the goal and no life lost (struck once by the censer on one); the other
  two built in as before. Under the sanitizers two runs of 400 s through it, clean. `make smoke` passed. The crypt's dream, seeds 1, 4 and
  7, 1,500 s, line for line the build before's but the time-taken line.
- `node tools/mapedit/test.js`: three built in read and written back; the emitters written and read; the censer refused on two tiles of
  chain and swinging into the floor; a sconce with nothing painted aside refused; the PREVIEW's run (wax settled, the bowl on its circle,
  the haze within its reach).
- Playwright's Chromium on the built editor: *Run I's Traps* opened; the censer's PREVIEW and its swing changed while running; a short
  chain refused in CHECKS; the sconce's rate raised and previewed, pouring and pooling; the reliquary's reach set to 2 and previewed; the
  breath previewed; the export carried each; PLAY of it with the browser build made here, the dream struck by the censer. No page errors.
Not played by hand; nothing under Wine. Open: the owner's word on the art; runs II to IV's traps the same way; the editor's PREVIEW is the
game's rules written again in JavaScript, near but not the same frame for frame.

**Session 6 (26 November 2026): the owner's Map 3, run II's traps (2e).** The owner approved run I's sheet. Their level *Storage* went
into `game/drawn.c` as the Compass, `DRAWN[2]` (docs/PLAN-mapedit.md's log). Run II's six built as 2e says; the dream's ways round the traps
and the flask's clock mended. Tested: every built-in passage (the Ossuary Stair, the Cistern Key, run I's, run II's) by the dream on eight
seeds, thirty-two of thirty-two at the goal with no life lost (before the dream's change run II's took all three lives each time, and the
first two failed for a while as it was mended); a fire bolt through silt gas lit 48 of its particles (a small passage, `CRAWL_SIDEFIRE`);
`make asan` clean on two passages for 300 s and the silt's; `make smoke` passed; the crypt's dream, seeds 1, 4 and 7, 1,500 s, line for
line the build before's (the Compass already in it). `node tools/mapedit/test.js`: run II's passage read back, each PREVIEW making its
own, a web, a pool and a mud with no cells refused. Playwright: run II's example opened and each emitter's PREVIEW run, no page errors.
Harness and Playwright only; nothing by hand, nothing under Wine. Open: the owner's word on run II's sheet; runs III and IV's traps.

**Session 7 (26 November 2026): the owner's asks before the squash (2f).** Built as 2f says. Tested in the harness: the four built-in
passages by the dream on eight seeds after the quicker casting and the tapped flask, thirty-two of thirty-two at the goal with no life
lost; a scripted person through *The Timed Lever*: the lever thrown, the ledge and block laid at 6.7 s and taken up at 14.7 s, eight
seconds on; the metronome's beats from 0.65 s to 0.14 s apart and its chords by turns, climbing from 0 to 6 (`CRAWL_SIDEMETRO`); the
flask tapped 219 px to a creature, held with right 227 px, held with left 124 px (`EVENT side_flask`); each powerup taken (`EVENT
side_power`). The dream, stopped with no way on for two seconds, now goes to a lever it can reach that a trigger listens to, throws it and looks again
(`sdd_plan`; a timed one again once it is back): *The Timed Lever* on eight seeds, eight of eight at the goal, the lever thrown each
time, no life lost; the four built in again after that change, thirty-two of thirty-two, no life lost. A timed lever left while it ran
is back when you come in again. `make asan` clean on the lever, run II's and a scripted person's flasks (400 s, 400 s, 120 s); `make
smoke` passed; the crypt's dream, seeds 1, 4 and 7, 3,000 s, line for line the build before's but the time-taken line. `node
tools/mapedit/test.js`: *The Timed Lever* read back letter for letter, four bad lines refused. Playwright on the built editor: a switched
ledge, a timed switch at 12 s, the three powerups (one changed to Might, 15 s) and a block off at the start placed and exported as the
game reads them, no page errors.
The World Editor was republished from this branch before the squash, at the owner's asking (version 5), its PLAY with this branch's game. Not played by hand; nothing under Wine. Open: the owner's art for the powerups; the numbers (the metronome, the flask's aim, the
powerups' seconds) theirs to tune.

Later the same day the owner, playing the republished editor, heard creatures far off the screen; every sound a passage makes away from
the caster is now placed as 2f says (`CRAWL_SIDESND=1` logs each, its distance and its gain). Checked by that log on run I's and run
II's passages with a person standing at the door; under the sanitizers on run I's. Harness only.

Then four jumps and three jingles to hear (2g), rendered with `host music` and sent; the owner chose the bone pipe and the toll. Each death
sounds once (`EVENT side_death_sound`), the drop included; the sanitizers clean on three passages' worth of each; `make smoke` passed.
Harness only, but for the owner's ear on the rendered files.

**Session 8 (26 November 2026): the dead in a side passage (2h).** The owner: some of the dead did not move or attack properly in a
passage, the skeletons' bones came too quick and true to avoid, the bats' hits did not match the bats, and what flies should strike by its
pixels. Built as 2h says, in a new part, `game/side_dead.c` (between `side_works.c` and `side_dream.c`); the old creature and shot loops
left `side.c`. A built-in example to meet them all, *The Dead's Gallery* (`SIDE_TEXT[4]`, a crypt doorway names none), and its frames as
`media/sheets/side/dead_gallery.png`. Tested, harness only:
- The masks against the drawing: every drawn pixel of the bat, hound, whipper, spider and imp, and of eight kinds together, the same as
  the mask's (none apart; the flipped facing, as all faced the caster).
- The bone: a scripted person at seven tiles, level 3, rank I. Standing still, 5 of 6 throws struck; stepping back and forth, 1 of 7.
  Each throw comes after half a second's warning and is in the air about a second, rising some 40 pixels (at first it rose 7 to 22 and
  landed within 18 pixels either way, so a person standing still was mostly missed: the arc raised and the aim tightened to 8).
- Every kind alone in a passage of its own, the dream through it (24 kinds, level 6): at the goal each time, no life lost; and mixed
  (the deacon and his bones, the whipper and his hounds, the sexton, spiders under a low roof with a gargoyle and a wall skull): each act
  seen in the `side_dead` events. The deacon raised the fallen once nobody struck him (struck, his chant broke, as in the crypt).
- The dream through the four built in, eight seeds each, 600 s, against the build before (`61b54b1`): the Cistern Key and run I's traps
  the same, run for run; run II's hurt 15 to 32 times (17 to 30 before); the Ossuary Stair hurt 4 to 14 (6 to 10) and slaying more,
  since its bats can be struck now; no life lost anywhere, before or after. On the way, the Ossuary Stair's bat over the falling blocks
  threw the dream off them on two seeds (six lives, two runs ended): mended by the bat's bite nudging rather than throwing, and the
  bats circling low enough to be struck. Two tries at teaching the dream to wait for fliers made it worse and were taken out.
- *The Dead's Gallery* on eight seeds at level 4: at the goal each time, no life lost; levels 2 and 6 the same; at 8 and 10 the end bays
  (the bell and the deacon's call bringing all at once) cost the dream nine lives before it got through, with a first-run flask.
- The crypt's dream, seeds 1, 4 and 7, run 7, 1,500 s, line for line the build before's but the time-taken line. `make asan`: the gallery
  at levels 4 and 9, the Ossuary Stair, run II's, and two mixed passages, 300 s each (`ASAN_OPTIONS=detect_leaks=0`): clean. `make
  smoke` passed. `node tools/mapedit/test.js` passed (five built in now). The browser build (`make web`, Emscripten 6.0.13) and its
  smoke test in Playwright's Chromium passed; the editor's page served locally opened the gallery and showed the six new sliders, no
  page errors.
- The World Editor republished (version 9, the same link; its maps kept, the title as the owner left it) with this build in PLAY.
Not played by hand; nothing under Wine; the Windows build to CI. Open: the owner's word on the feel and the six numbers; the creatures
still stand about twice the caster's height (step 13); the dream does not dodge, so a passage crowded with callers can wear it down.


**Released (26 November 2026)** as 1.0.0b21.12, "The Dead's Gallery", at the owner's word after trying the Ossuary Stair in an artifact
built to go straight into it. With it, the browser's page takes `?side=n` (and `&level=l`): a new run straight into passage n, CATACOMB
alone, its saves under `haunted.exe/side/` (`web/haunted.js`, ahead of `PREFIX`); the World Editor's PLAY sets its own and is left alone.
Tested in Playwright's Chromium: `?side=0` in the passage with its saves apart, the page without it booting HAUNT.EXE as before.

**Session 9 (27 November 2026): WHEREAMI, from the owner's level 2.** The owner's passage *WhereAmI* (drawn in the World Editor) is
`SIDE_TEXT[1]`, as its export asks; *The Cistern Key* moves to 5, kept (its dice change with its number: hurt 4 to 6 times on eight
seeds as before, the same goals). Their redrawn tutorial level 2 is laid as drawn (`drawn.c`, `DRAWN2_*`, 25 wide now): a back room
behind a barred door its plate opens from inside, a second secret east of the ice, eight lights hung, and two side doorways to
WhereAmI, link 0 in the secret's alcove (the way in) and link 1 in the back room (where its first goal lets you out; its second goal,
in the sealed room of chests, comes out where you went in). The dream in a passage could not get through it, and was taught:
- A goal doorway the open tiles join to (`sdd_open_to`): it had aimed at the last goal, sealed in the room of chests.
- Jumps no further than the passage's own run, jump and pulls allow (`sdd_across`), never more than before: the game's numbers give the
  first version's 5, 4, 4, 3 exactly; WhereAmI's 4, 4, 3 and none three up. It had leapt five over a pit for a lone pillar and fallen.
- On at a run in the air, on a leap of its own to no lower than it left, in a passage with its own moves (`sdd_own_moves`), when
  walking would come down short. Running on everywhere had sent it faster into the dead on *The Dead's Gallery* (hurt 42 to 71 times
  against 24 to 38, four lives lost against none; each change switched off in turn showed this one alone), so it is held to these.
- A hop onto a platform that waits at its end 0.6 s or more yet; a lift ridden (every row its way passes is reached from it, to step
  off where the way goes on); a walk off an edge a tile across without room over it (the shaft's ledge has its ceiling on it).
Tested, harness only:
- WhereAmI by the dream, eight seeds at level 2 and eight at 3, 600 s: through every time by the lift, the lever and its gate's five
  seconds to the first goal; no life lost, never hurt, never giving up.
- The other five built in, eight seeds each at level 3, 600 s, against the build before (`2bcbb45`): the Ossuary Stair, run I's and
  run II's traps and the gallery the same seed for seed; the Cistern Key as said.
- The crypt's dream, eight seeds for 1,500 s against the build before: level 1 the same; level 2 begins at the same moment and lasts
  as long, within a few seconds. It does not go into WhereAmI: link 0 is behind the new secret (the dream finds neither of level 2's
  secrets, before or after) and link 1 behind the barred door, so a person finds it. 42 levels laid against 41, 2 deaths against 9
  (on levels 3 and 4, after level 2 has turned the dice: chance more than this).
- `make asan`: WhereAmI at level 2, the gallery at 3, the crypt 700 s through level 2: clean. `make smoke` and `node
  tools/mapedit/test.js` (six built in now) passed. The browser build in Playwright's Chromium went straight into WhereAmI at level 2.
- The World Editor republished (version 10, the same link, its title and maps kept), and *HAUNTED.EXE First Passage* now goes straight
  into WhereAmI at level 2.
Not played by hand; nothing under Wine. Open: the owner's play of WhereAmI and of level 2's doorways; the dream still does not time
what moves (it waits for a lift at its end), and does not open level 2's secrets.
