# 1.0.0b12: the lower storey, the runs' hazards, puzzles and trials

**Status: questions for the owner (30 October 2026); nothing built.** The owner asked for five things: a lower
storey under each level from run II, growing run by run; art that shows an edge can't be crossed; more hazards of
each run's own, using the elements; logic puzzles played with things in the world, with no screen of their own;
and timed trials behind marked doors. This plan maps that onto the game as it stands on
`claude/eloquent-carson-1x049c` (1.0.0b11, steps 1-13 built: elements, lines, classes, souls, Soul Sap), says where
it rubs, drafts the samples the owner asked for, and asks what only the owner can answer.

Each question has a recommended answer (*); a question the owner leaves alone takes it. Answers go into **Decided**
below, as 1.0.0b9's and 1.0.0b11's did. **Whoever builds this: build nothing until the owner's answers are written
there, and not before 1.0.0b11 is finished and released.** Read it with `CLAUDE.md`, [`PLAN-b7.md`](PLAN-b7.md)
(hazards and omens, the boss arena's seal), [`PLAN-b10.md`](PLAN-b10.md) (each run's look, room types, how walls are
laid) and [`PLAN-b11.md`](PLAN-b11.md) (the elements, the map at a walk, souls, replays). The questions are gathered
at the end. When this plan lands beside 1.0.0b11's work, `CLAUDE.md` should point to it as it does to the others.

## Decided

*Nothing yet.*

## What the owner asked for (30 October 2026)

1. **A lower storey**, from run II. Spaces about a quarter the size of the level, navigable as a second storey
   beneath it, adding a quarter to the space walked in run II; half (50% more) in run III; three quarters (75%) in
   run IV. No transition: both storeys can be seen from each other. Stairs lead down, and a staircase can turn a
   corner. Parts of the lower storey run under the level out of sight from above, as branching ways. Each lower
   storey is more dangerous than its level, and may hold key doors, secrets and the exit.
2. **Edges that say no.** Where there is no way across, as at an edge above a wall on the storey below, the art
   should show it can't be passed. Nobody falls: the only way between storeys is a surface made for it.
3. **More hazards of each run's own**, themed, none of them killing at once, more of them as the run goes on. Earlier
   runs' hazards come back in later runs, rarely and more rarely the further back they are. They use the elements,
   and one of them saps souls while you stay inside it. The owner wants samples to work from, and will add their own.
4. **Logic puzzles with no screen of their own.** The player takes and works things in the world to open larger
   parts of the map. From after level 5, every other level, one kind of puzzle for each run (perhaps more, if the
   kinds go well together). Nothing a spell or an ability does is ever the key: only thought. The owner wants a good
   number of examples to pick from or change.
5. **Trials against the clock.** Doors marked with a skull and crossbones, themed for each run. One cell past the
   door it shuts behind you and the room turns into something else, a challenge of the run's own, sixty seconds to
   finish it. Falling there is a death. The reward: full health, full spirit and bones, and a boon for the rest of the
   level, a fifth more experience from combat. The dream should almost always win, but close to the time.

## What it touches in the game as it is

**The renderer** is Wolfenstein's (`render_walls`, `render_floor`, `render_sprites`, `zbuf`): one ray a column,
stopped by the first wall it meets, every wall one storey high, one floor and one ceiling plane, the eye at half
height (`G.camz` 0.5). The map is one grid of 45 by 45 cells (`Cell`, `G.map`) and nothing in it has a height.
Collision (`passable`), the creatures' hunt field (`build_hunt`), the autopilot's lanes (`apass`), sight
(`line_clear`), missiles, the lightmap (`LMR`), the fog, wisps, hazards' floor marks, secrets, the map at a walk and
pockets all take one storey for granted. A second storey that can be seen from the first is the largest change to
the engine since it began; everything that walks, flies, shoots or is drawn has to learn it.

**Hazards** (1.0.0b7; `Hazard`, `hz_new`, `update_hazards`, `hz_dmap`): `HZ_ROCK`, `HZ_FIRE` (Hellrent), `HZ_BILE`,
`HZ_VOID`, and the bosses' `HZ_ARC` and `HZ_GRASP`. Each warns (never under 0.9 s), harms, fades and hurts by a share
of health; Premonition warns a second sooner, and the autopilot keeps out of what warns. The runs' only hazards of
their own are the omens, one a run on its last four levels (every 90, 60, 40, 30 s): run I rock, II bile, III
Hellrent, IV the void. The elements (1.0.0b11) give each a kind of harm and a resist.

**Using things**: E opens doors, presses secrets, opens the altar's, the shrine's, the bell's and the font's menus,
and kindles the Wraith's braziers (held a second). Nothing is carried, turned, pulled, pushed or stood on to work it.
Puzzles need new ways to touch the world.

**Rooms that shut**: the boss's arch seals (`Door.locked` `SEALED`) and its fight runs a clock; a failed trial in a
pocket seals a doorway (`T_SEALED`). A trial room can use the same seal.

**Levels** are built afresh each visit (a replay is the same level by number, with new walls), so puzzles and trials
are made with their level and kept in nothing but it.

---

## Decide first

**G1. Which release.**
- (a*) 1.0.0b12, built after 1.0.0b11 is released.
- (b) Folded into 1.0.0b11 before it goes out.

**G2. One release or two.** The lower storey means a new renderer and every system that moves learning stairs; the
hazards, puzzles and trials fit the engine as it is.
- (a*) Two. 1.0.0b12: the hazards, the puzzles and the trials. 1.0.0b13: the lower storey and its edges, once a first
  test of the new renderer has been seen and timed. Puzzles and trials can use the lower storey when it comes.
- (b) One release, the lower storey first.
- (c) One release, the lower storey last.

---

# Part A: the lower storey

## 1. How it can be drawn

**L1. The engine.**
- (a*) **Two storeys on one grid.** Each cell keeps two storeys: the level's, and one beneath. A cell of the level is
  floor with rock beneath it, floor with a lower passage beneath it, or open (a well or a gallery: no floor, looking
  down into the lower storey). The renderer becomes a column renderer with clip windows, as Doom draws its sectors,
  but on the grid: each column walks its cells front to back and draws what each shows (the lower storey's walls seen
  down a well, the well's lip and the face of the drop, the level's walls), floors and ceilings at two heights, until
  the column is full. Sprites are clipped to the windows their storey shows through. A passage under floored cells
  can't be seen from above, and the level's floor can't be seen from below, which gives the owner's hidden branches.
- (b) **Sunken floors only.** One storey with floors at two heights: lower places are pits and galleries open above,
  seen from their edges, reached by stairs. Cheaper (a height a cell), but nothing can run under the level out of
  sight, which the owner asked for.

Under (a), the first step is a test (*default*): one hand-made level with a gallery, a well, a stair down, a passage
under the floor and a corner stair, drawn by the new renderer, with frames for the owner and its time a frame against
today's 2.2-2.5 ms. Nothing else is built until the owner has seen it.

**The storey's height.** The lower storey's floor is one storey down. Looking down a well from the level you see its
floor a storey and a half below the eye, its walls, and what moves there; looking up from below you see the level's
walls standing two storeys tall round the opening and its ceiling far above. Where the lower storey runs under the
level's floor its ceiling is low: the underside of the floor, beams and slabs.

## 2. How much, where, and how dangerous

**How much** (as asked): run II a quarter of the level's floor again, run III half, run IV three quarters, counted in
floor cells. Not in run I, not in pockets, her hall or (L2) on a boss's level.

**L2. A boss's level.** (a*) No lower storey: the arena and its band keep the south of the map, and the fight stays
as it is. (b) A lower storey outside the arena, as on any level.

**Its shapes** (*defaults*, laid on the same plan of blocks as the level):
- **Galleries**: a room's middle open to the storey below, the level's floor a walk round it, the room below seen
  from every side.
- **Wells**: a hall widening over a drop, or a shaft between two halls, the lower passage seen down it.
- **Passages under the floor**: halls and rooms beneath the level's own, branching off from below a gallery or a
  well, out of sight from above. These are the owner's hidden branches.
- **Stairs**: a straight flight two cells long, each a half-storey; or a flight turning a corner of a room or hall, an
  L of two cells. Every lower part has at least one stair, and most have two, so a branch can come back up elsewhere.

**L3. What makes it more dangerous.**
- (a*) Its creatures come a rank higher than the level's mix (as a pocket's first link does), a third more of them, and
  the run's hazards twice as often there.
- (b) Only more of them.
- (c) Another measure (say which).

**What it may hold** (as asked): vaults and their doors, secrets, and the exit. *Defaults*: the exit is below one level
in three from run II, more often in run IV; vaults and secrets lie below in proportion to its size; Black Altars stay
on the level's own storey; a key can lie on the other storey from its door.

**L4. Between storeys, for things that aren't you.**
- (a*) Walkers use the stairs (the hunt field runs through them); bats and gargoyles fly up and down wells and
  galleries; spectres and the Wraith pass through floors as they pass through walls.
- (b) Every creature keeps to its storey.

**L5. Striking across storeys.**
- (a*) Spells, missiles and creatures' throws cross between storeys through wells and galleries, at what can be seen;
  a spell aimed down at a creature below flies down to it. Missiles gain a height.
- (b) What is on the other storey can be seen but not struck. Simpler, but odd to look at.

**The autopilot** (*default*): lanes on both storeys, stairs in its walk, the exit and keys found below as above; it
treats the lower storey's danger as it treats a pocket's (it goes below with two thirds of its health or more).

**The map at a walk** (*default*): the storey you stand on drawn in ink, the other faint beneath it in a paler ink,
stairs as hatched steps, wells and galleries as dashed edges; the level's numbers count both storeys. Seen and walked
cells are kept per storey.

**L6. Ladders.** The owner asked for a surface made for travel between storeys. (a*) Stairs only. (b) Ladders too, in
the Hollows (a mine has them): E climbs, slowly, and nothing can be cast on one.

## 3. Edges that say no

Nobody falls. Where the level's floor ends at an open well or gallery, the edge is a wall to anything that walks, you
included, but not to sight, missiles or fliers. Stairs are the only way down.

**At every edge** (*defaults*):
- The floor's last texels are worked as a kerb, a lip of paler stone a few pixels wide.
- Below the lip, the face of the drop: the cut edge of the floor, with corbels or beams under it, darker toward the
  bottom, and the fog thicker in a well, so its depth reads.
- Walking into an edge stops you flat (no sliding off along it), a little grit falls ticking down the drop, and the
  first time a caption says *NO WAY DOWN BUT THE STAIRS*. The pad gives a small knock.
- **An edge above a wall** (the owner's case: the drop below is a wall of the lower storey, nothing to stand on):
  the parapet there is raised to a full balustrade, so it reads at once as no way through.

**Each run's edges** (samples, to pick or change; E1 below):

| | Low edge (a kerb or parapet) | A balustrade (above a wall below) | Something of its own |
|---|---|---|---|
| II. The Catacombs | a slate parapet with drain gaps, the flood line along it | iron railings gone green on slate posts | a chain slung between bollards along a canal's edge |
| III. The Hollows | a timber guardrail pegged to posts | a rope barrier on stakes with rags tied on, and a miners' danger mark (crossed picks) chalked on a board | a cart's buffer stop where the rails end at the drop; the cult's skulls on stakes |
| IV. The Deep | a kerb of obsidian with a crack glowing along it | a fence of bones fused into black glass | crystals grown along the lip; at the throne's gallery a balustrade banded in gold |

**Stairs, by run** (samples): run II, slate steps worn hollow, water running down one side; run III, timber treads
pegged into the rock with a handrope; run IV, steps cut in black glass with gold at their edges and ash drifted in the
corners.

**E1. The edges.** (a*) As the table, the owner marking up; (b) one plain kind for every run.

---

# Part B: the runs' hazards

## 4. How they come

- **Never at once fatal** (as asked): no hazard takes more than a quarter of your health in one stroke (*default*;
  most take an eighth to a fifth), and none can strike twice inside its own warning.
- **Two kinds** (*default*): **fixed** hazards placed with the level, in a room or hall of their own, with a rhythm you
  can learn (a censer swinging, a fissure breathing); and **arising** ones that come up near you as omens do now.
- **More as the run goes** (*default*): fixed hazards, one on the run's first level, rising to about five by its ninth;
  arising ones from its third level, every 120 s, down to every 40 s by its ninth; the omens on its last four levels as
  now, which become a part of this. Twice as many on the lower storey. None in pockets, her hall, or in a boss's
  arena but its own.
- **Earlier runs come back** (as asked, *default* numbers): of a level's hazards, about one in seven is the run
  before's, one in fifteen the run before that's, one in thirty the one before that's. So run II sees a little of run
  I; run IV a little of all three.
- **Elements**: each hazard's harm is an element, resisted as that element is; Premonition warns of every one a second
  sooner; the autopilot keeps out of their warnings and harm (`hz_dmap`).

**H1. The rhythm.** (a*) As above. (b) Only arising hazards, as omens are. (c) Only fixed ones.

**H2. The cap.** (a*) A quarter of your health a stroke at most. (b) A fifth. (c) Another (say which).

## 5. Samples, by run

To pick from, change or add to (H3). Each says its element, whether it is fixed or arising, how it warns, and what it
does. The four marked **soul-sapping** each draw souls while you stay inside them: the owner asked for one, and may
keep one or one a run.

**Run I, the Crypts** (the Order's vaults, bone and ashlar):

- **Falling masonry** (Physical, arising; as now): grit, a hardening shadow, a block, rubble after.
- **The Swinging Censer** (Physical, fixed): a great iron censer on a chain swings across a hall on a slow arc,
  creaking, its shadow and its trail of smoke ahead of it. Struck, you lose a share and are knocked a cell aside.
- **The Embalmers' Breath** (Poison, fixed): a burial slab cracks with a hiss and a seam of light, and a grey-green
  vapour settles over the cells before its niche for four seconds; standing in it poisons.
- **Votive Spill** (Fire, arising): a rack of votive candles topples with a clatter; burning wax runs along a line of
  floor cells and burns there three seconds.
- **The Cold Vault** (Frost, fixed): a crack breathes cold; rime creeps over the cells before it, and on them you are
  chilled and slowed.
- **The Hungry Reliquary** (Spirit, fixed, **soul-sapping**): a gilded reliquary on a plinth whose lid lifts when you
  come near; a violet haze two cells round it draws your souls, a soul a second, and a little health, and slows you a
  touch, with motes streaming from you into it. It shuts when you leave its haze, or after eight seconds.

**Run II, the Catacombs** (drowned slate, silt, saints, webs):

- **Bile** (Poison, arising; as now): the sheen creeping from a grate.
- **Sluice Burst** (Frost, fixed): a sluice in a canal's wall clanks open; a surge runs down the canal or hall and
  pushes you two cells along it, chilled and slowed, a little harm.
- **The Galvanic Pool** (Lightning, arising): lightning comes down a grate into standing water; the water's cells glow
  and crackle a second and a half, then everyone in water joined to it is shocked, and a wind-up breaks. (Lightning has
  no resist; this would give it one, or leave it so: H4.)
- **Web Snare** (Physical, fixed, from the run's sixth level): a web across a doorway you can just make out; walking
  into it holds you a second and a half and wakes the spiders near.
- **Silt Gas** (Poison, arising): bubbles rise from silt; a cloud hangs over the cells and poisons; a fire spell flying
  through it bursts it into flame, a share of Fire to anyone inside, and it is gone.
- **The Weeping Saint** (Spirit, fixed, **soul-sapping**): a saint in an alcove weeps; the pool spreading before it
  draws souls while you stand in it, and the saint's eyes glow brighter the more it takes.

**Run III, the Hollows** (the mine, its timber and rails, the charnel pit, the cult):

- **Hellrent** (Fire, arising; as now).
- **Timber Collapse** (Physical, fixed): props creak and split and dust sifts down; then the roof comes down along a
  strip of a gallery, and rubble closes that strip for twenty seconds.
- **Firedamp** (Poison, fixed): gas pools in a stope, a shimmer over the floor; breathing it poisons; a torch flaring
  or a fire spell bursts it (a share of Fire), and it gathers again later.
- **The Runaway Cart** (Physical, fixed): a cart comes rolling down the rails, sparks off its wheels and a rumble ahead
  of it; struck, you lose a share and are thrown a cell. Creatures can be struck too.
- **The Lodestone Seam** (Lightning, fixed): a green-veined seam in the rock crackles, then arcs to anything within a
  cell and a half; a wind-up breaks.
- **The Tithe Circle** (Spirit, fixed, **soul-sapping**): a ring of the cult's chalk marks on the floor glows as you
  step in, and takes its tithe of souls for every second you stay.

**Run IV, the Deep** (black glass, crystal, magma, ash, and the dark at its end):

- **The void** (no element, since nothing resists it; arising; as now).
- **Fissure Breath** (Fire, fixed): a fissure in a wall glows brighter, then breathes flame across a line before it,
  on a rhythm.
- **Ashfall** (Poison, arising): ash comes down in a room; the fog thickens under it and you choke while you stand in it.
- **Singing Crystal** (Lightning, fixed): a crystal hums higher and brighter, then discharges in arcs to the three
  nearest things within three cells, you or creatures.
- **Shatterglass** (Physical, arising): the glass floor cracks in a star round a cell, and spikes stand up there for a
  second.
- **The Dark Cold** (Frost, fixed, the run's last levels): where the fire has gone out of the rock, rime lies over the
  floor; on it you are chilled and slowed.
- **The Black Mirror** (Spirit, fixed, **soul-sapping**): a sheet of obsidian set in a wall; while you stand in its
  sight within three cells, your reflection in it grows clearer as it drinks your souls.

**H3. The samples.** (a*) The owner marks up the list: keep, change, cut, or add. (b) We pick four a run.

**H4. Lightning's resist.** The Galvanic Pool, the Lodestone Seam and Singing Crystal would be the first Lightning to
hurt you. (a*) Leave it unresisted, as 1.0.0b11 has it. (b) Add Resist Lightning, rolled where the others roll, with a
line in the points.

**H5. Soul-sapping.** (a*) One a run, as drafted. (b) One in all (say which run).

---

# Part C: logic puzzles

## 6. How they work

**When** (P1).
- (a*) Even levels from level 6, never a boss's: 6 and 8, 12 to 18, 22 to 28, 32 to 38, and round again. One puzzle
  a level.
- (b) Only the sixth and eighth level of each run.
- (c) Odd levels from 7: 7 and 9, 11 to 19, and so on.

**What it opens** (P2).
- (a*) A sealed wing of the level, two to six blocks of rooms and halls, behind a great door, a portcullis or a fallen
  passage: a hoard as rich as a vault's, and now and then something more (a font of health, a Cursed Coin, a Black
  Altar of its own). It is never the only way to the exit, so a level can always be left without it.
- (b) As (a), and one level in four the exit itself lies inside the wing.

**No screen of its own, and no spell as a key** (as asked). Everything is done with E and the body, and read in the
world: carvings, banners, epitaphs, murals and stained glass give the clues, read with E as a caption at the top of
the view (the line the game already uses). A puzzle answers with sound and movement: a click, a chime, a dull knock
for a wrong move, stones sliding back to reset, and when it is solved a rumble, dust, a choir chord and the wing's
door grinding open. Nothing about it depends on what you carry, what you have learnt or what your class is.

**The ways to touch the world** (P3; each new, all with E):
- **Take and set down**: carry one thing at a time (a skull, a relic, an urn, a weight, a fossil bone); E takes it,
  and E sets it down on a place that takes it, or on the floor.
- **Turn**: E turns a thing a quarter (a statue's head, a prism, a culvert, a dial).
- **Pull**: a lever or a sluice wheel, two positions (or held a second for a wheel).
- **Light and snuff**: a candle, a lamp, a brazier (as the Wraith's are kindled).
- **Strike**: a bell, a gong, a crystal: a note, and a glow so it is seen as well as heard.
- **Step**: a flagstone that sinks under you, or under a weight set on it.
- **Push**: walk into a block, a sarcophagus or a cart to shove it a cell along.
- **Read**: a carving or a plaque, as above.

**P3. Which ways to build.** (a*) All but push: pushing can wall the autopilot in, and a stuck block needs undoing. (b)
All of them. (c) Take, turn, pull and read only.

**P4. Carrying.** (a*) The thing is held in the hand, drawn where the hand rises; nothing can be cast while it is held,
and a cast sets it down first, where you stand. (b) You can cast; what you carry shows small in a corner.

**P5. Help.** (a*) After three wrong tries a whisper gives a nudge, once, as a caption. (b) None.

**P6. The dream.** (a*) The dream solves them, slowly and with a wrong try now and then, when it has time on the level:
the screensaver shows a puzzle being worked out. It reads the answer from the game, never the clues. (b) The dream
leaves them.

**What a puzzle pays** (P7). (a*) The wing's hoard, experience as a chain's end at the level (60 times the level's
weight), and a Cursed Coin once a level for solving. (b) Only what lies in the wing.

Every puzzle is made with its level from the level's seed: its answer chosen, its pieces and clues laid near the
wing's door (never in a vault, a pocket or behind a secret), and checked to be solvable before the level opens.

## 7. Puzzles, by run

The owner asked for a good number. Each run's are themed to it; the type (P8) is one per run, perhaps more. Each says
the way it is worked, what lies in the world, and where its clue is.

**Run I, the Crypts** (the Order's bones, bells, effigies and candles):

1. **The Ossuary Ledger** (take and set). Niches carved with the Order's ranks (a mitre, a helm, a tonsured head, a
   crown); skulls lie about the halls with the same marks scratched on their brows. Set each skull in its niche.
   Three to six niches. The gentlest.
2. **The Knell** (strike). Four bells of four sizes in a frame; a banner over them shows the Order's knell as a row of
   bell shapes. Ring them in its order; a wrong bell and all four hum down to silence and it starts again.
3. **The Effigies' Gaze** (turn). Five knights lie on their tombs with heads that turn; their epitaphs say whom each
   looks to (*I LOOK TO MY BROTHER*, *I TURN FROM THE ONE WHO BETRAYED ME*, *I WATCH THE DOOR*), and a tablet lists who
   was whose brother and who betrayed whom. Turn every head as its epitaph says.
4. **The Balance of Souls** (take and set). A great scale with two pans; reliquaries marked with one to five beads.
   Load the pans so they hang level with the marked reliquary in the left pan. A harder form: nine reliquaries alike,
   one false and heavier, two weighings to find it, and set it on the altar.
5. **Votive Lights** (light and snuff). A rack of nine votive candles, three by three; touching one lights or snuffs it
   and the ones beside it. Make the rack match the pattern in a stained-glass window over it.
6. **The Abbot's Urns** (take and set). Three burial urns of three sizes on the left of three plinths; move them all to
   the right, one at a time, never a larger on a smaller (the plinth refuses it with a knock). Four urns in the run's
   later levels.
7. **The Book of Hours** (turn). A dial of the eight canonical hours; a lectern's book says *WE SANG WHEN THE SHADOW
   FELL ON THE THIRD STONE*, and a sundial's shadow in a shaft of light lies across numbered stones. Set the dial to
   that hour.

**Run II, the Catacombs** (water, sluices, saints, drains):

1. **Three Cisterns** (pull). Cisterns of eight, five and three measures, their flood lines marked on the walls, the
   largest full; sluices between them pour until one is full or the other empty. A culvert opens when one holds exactly
   four. The old puzzle of the three jugs.
2. **The Saints' Attributes** (take and set). Saints stand in alcoves with empty hands; their emblems lie in the halls
   (a key, a wheel, arrows, a tower, a lamb, a lamp). Under each saint is a line of how they died (*BROKEN ON THE
   WHEEL*, *SHOT FOR THE FAITH*, *SHUT IN A TOWER*); put each emblem in its saint's hands.
3. **The Culverts** (turn). Culvert sections set into a floor in a grid, each turning a quarter; route the water from
   the spout to the mill wheel that lifts the sluice gate. The water runs along each channel as it joins.
4. **The Ferryman's Stones** (step). A flooded hall with stepping stones; some hold, some sink and send you splashing
   back to the start, cold but unhurt. A drowned tablet at the near end draws the safe path in the saints' signs, and
   each stone bears a sign.
5. **The Sluice Lock** (turn). A sluice door with three wheels of saints' signs; the order is the order the saints
   stand along the flood line, from the highest water mark to the lowest.
6. **The Drowned Bells** (strike). Bells hung at the water line, each sounding lower the deeper it hangs; a carving
   shows a phrase as a row of ripples, long and short; ring the bells in its order. Seen as well as heard.

**Run III, the Hollows** (the mine, its carts and hoist, the fossil hall, the cult):

1. **The Points** (pull). Rails branch through three switch levers to four buffers; let a cart go from the top (E at
   its brake) and it must reach the buffer by the winding gear, not a dead end (it clangs and rolls back). The mine's
   plan, chalked on a board, shows which way each switch sends it.
2. **The Hoist** (take and set). A hoist cage and its counterweight box; ore sacks marked in tallies from I to V. Load
   the box to the weight chalked on the shaft (*XII*) and the cage rises, opening the way below.
3. **The Tally Board** (take and set). The miners' tags hang on a board for those who came up from the last shift; the
   dead in the galleries still hold theirs. A shift list nailed by the board says who went down; hang the tags of
   those who never came up.
4. **The Fossil Hall** (take and set). The bones of a great fossil lie scattered; each fits one socket in the rock by
   its shape. Set them all and the fossil's jaw opens on the way through.
5. **The Cult's Ring** (turn). Seven standing stones, each with a mark that turns; the cult's litany on the chapel
   wall gives their order (*THE EYE BEFORE THE HAND, THE HAND AFTER THE CROWN, THE CROWN FACING THE DOOR*). Set the
   ring as the litany says.
6. **The Lamps of the Lost** (light). Five lamps down a shaft, one for each gallery; light the lamps of the galleries
   that never reopened, as the props' chalked dates and the collapsed galleries tell.

**Run IV, the Deep** (crystal, glass, magma, gold, the throne):

1. **Prisms** (turn). A beam of the magma's light comes through a fissure; crystal prisms on plinths each bend it a
   quarter; turn them to carry the light to a dark geode. Lit, it sings and the glass wall beside it melts away. The
   beam is drawn as it goes.
2. **The Court of Glass** (take and set). Statues of a dead court (a king, a queen, a priest, a headsman, a fool) and
   five plinths round the throne. The frieze above says *THE FOOL STANDS FARTHEST FROM THE CROWN; THE PRIEST AT THE
   KING'S RIGHT HAND; THE HEADSMAN NEVER BESIDE THE QUEEN*. Set each where the frieze allows only one way.
3. **The Glass Floor** (step). A floor of glass tiles; each lights when stepped on. Light every tile without stepping
   on any twice; a lit tile stepped on cracks, and the floor goes dark and starts again.
4. **Singing Crystals** (strike). Crystals sing a phrase in light and sound; strike them back in the same order, the
   phrase one note longer each time, five notes in all.
5. **Gold Veins** (turn). Tiles of the wall inlaid with gold veins, each turning a quarter; join the veins from the
   idol's eyes to the door's lock.
6. **The Cooling Vents** (pull). Vents that crust the magma in a channel for a while; open them in an order that lays a
   path of cooled crust to the far door before the first cools no more. The vents' order is cut on a gold tablet as
   the moons of a calendar.

**For any run** (if the owner wants more kinds to mix):

- **The Magic Square** (take and set): nine stones numbered one to nine set in a three-by-three so every line sums to
  fifteen, the corners given.
- **The Sealed Letters** (turn): dials of letters set to a word cut into the wall in pieces round the room.
- **Sokoban** (push, if push is built): sarcophagi pushed onto the marked flags.

**P8. The kinds, a run.** (a*) The owner picks one a run from the list (or two that go together), and the rest wait.
Our suggestion: run I *The Ossuary Ledger* (with *The Knell* as a second), run II *Three Cisterns*, run III *The
Points*, run IV *Prisms*. (b) We pick.

**P9. Harder deeper.** (a*) Within a run, the kind grows harder level by level: more pieces, a longer order, fewer
clues given outright; and a run's later levels may draw on the run's second kind. (b) Each kind at one difficulty.

---

# Part D: trials

## 8. How they work

- **The door**: a skull and crossbones on it, the run's own (D1). It is seen from down the hall: two red candles or
  their like stand by it.
- **Where** (*default*): a room of two to four blocks off the main way, never the only way to the exit; at most one a
  level, from level 3, about one level in three, never on a boss's level. From run II it may be on the lower storey.
- **Going in**: one cell past the door it shuts and seals behind you, as a boss's arch does, and the room turns: its
  light changes to the trial's colour, the fog thickens or clears, the walls take on the trial's look (the run's), the
  music gives way to a quicker, driving version of the run's march, and an hourglass with sixty seconds stands at the top
  of the view (T3). In the last ten, a ticking.
- **Falling** inside is a death (as asked): in a person's run it costs, as any death; in the dream the old mercy.
  Second Wind works as anywhere.
- **Winning**: the hourglass breaks, the room turns back, the door opens, and you are restored: full health, full
  spirit (to your vessel), bones to the most you carry; and **THE TRIAL'S BOON** for the rest of the level, a fifth
  more experience from combat (creatures slain and a boss). It shows on the experience lines and the tally, and ends
  with the level or a death.
- **The dream** (as asked): it takes every trial it finds with two thirds of its health or more, and the trials are
  tuned so that it wins nearly all of them (at least nineteen in twenty) with a few seconds to spare (a median of five
  to fifteen left). Each run's trials tuned by runs of the dream, with the numbers kept in the plan.

**T1. Time running out.** (a*) The hourglass empties, the door opens, the room turns back, and nothing is gained; its
creatures crumble; the room is spent for the level. (b) As (a), and the room's creatures stay, hunting. (c) The trial's
curse: the rest of the level pays a fifth less experience.

**T2. How often.** (a*) About one level in three from level 3, as above. (b) One a level from run II. (c) Rarer: one
level in five.

**T3. The clock.** (a*) An hourglass glyph and the seconds at the top of the view, where a boss's bar goes. (b) Only the
hourglass, its sand running; no numbers.

## 9. The doors, by run (D1)

- **Run I**: a skull carved over crossed thighbones on the oak door's iron plate; two red candles in sconces either side.
- **Run II**: a verdigris plate on the sluice door, a skull over crossed oars; two drowned lanterns burning red.
- **Run III**: painted on the timber door in red ochre, a skull over crossed picks, the miners' mark for a bad gallery;
  a red lamp hung on a nail.
- **Run IV**: a skull in black glass over crossed bones of gold, glowing from within, set in the iron door.

**D1. The doors.** (a*) As drafted, the owner marking up. (b) One door for all, the run's colour only.

## 10. Trials, by run

Each lasts sixty seconds, and each run picks two to four (T4). Each says what you must do, what makes it hard, and how
the room turns.

**Run I, the Crypts**:

1. **The Ossuary Wakes** (slay). The bones in the walls come down and stand up, in three waves; put every one down.
   The room: bone walls, a sick green light.
2. **The Vigil** (hold). Three candles on a bier; skeletons come to snuff them; keep at least one burning to the end, and
   relight with E. The room: dark but for the candles.
3. **Last Rites** (gather). Four relics lie about the room; carry each to the bier while the dead wake around you. (Needs
   P3's take and set.)
4. **The Knight's Wager** (duel). One skeleton knight two ranks above the level's best, alone, its plate whole; best it.
5. **The Toll** (reach). Every ten seconds a bell tolls, and on the toll the dead stand still for two seconds; between the
   tolls they hunt. Ring the four small bells about the room.

**Run II, the Catacombs**:

1. **Rising Water** (slay). The water rises a step every fifteen seconds, slowing you more each time (never drowning
   you); slay the drowned before it reaches the top.
2. **The Brood** (slay against a timer). Egg sacs ripen on the walls, each on its own time; destroy every one before it
   hatches. A hatched sac lets out its spiderlings.
3. **Shut the Sluices** (reach). Five sluice wheels, each turned by holding E a second, while the drowned grab at you and
   the water climbs.
4. **The Saints' Lamps** (light). The room goes dark; light the five saints' lamps while shades hunt you in the dark.

**Run III, the Hollows**:

1. **The Cave-In** (reach). The roof comes down from the far end, a strip at a time; brace the four props (E held a second
   each) before it reaches them. Rocks fall where the roof is coming.
2. **The Charnel Feast** (slay). Ghouls feed on the heap and heal while they feed; drive them off it and slay them all.
3. **The Rite** (hunt). Four deacons chant in the room's corners; each chant finished raises a skeleton; silence all four.
4. **Firedamp** (reach). Gas fills the room from its low end, poisoning; open the three vents at its high end (E).

**Run IV, the Deep**:

1. **The Molten Rise** (slay). Magma creeps over the floor from the vents; slay the imps that feed it, and each one slain
   crusts its vent.
2. **Dark Hours** (hold). The braziers gutter one by one; keep four of six alight for the minute, while a shade of the
   Wraith comes for them (as in its arena).
3. **The Glass Court** (hunt). A court of black-rank dead round a crowned one who raises them again; strike down the crowned
   one, who keeps behind the rest.
4. **Singing Glass** (reach). Crystals light one after another; strike each before its light goes out, while the glass
   spikes under you.

**T4. Which trials.** (a*) The owner picks two to four a run; our suggestion: I *The Ossuary Wakes* and *The Vigil*; II
*Rising Water* and *The Brood*; III *The Cave-In* and *The Rite*; IV *The Molten Rise* and *Dark Hours*. (b) All of them.

**T5. A record of them.** (a*) The record counts puzzles solved and trials won, with achievements (a first, five, twenty
puzzles; a first, ten, thirty trials; every trial of a run won once). (b) Not counted.

---

## 11. What else changes

- **The Codex** gains the hazards met (as it has the omens), the puzzle kinds solved, and the trials.
- **Premonition** marks the new hazards; Grave Sense and the map's numbers count trial rooms' creatures only once the
  room is entered.
- **The map at a walk** marks a puzzle wing's door and a trial's door once seen, and the wing once open.
- **Debug switches** (*defaults*): `CRAWL_HZ=k` (hazard k on every level, often), `CRAWL_PUZZLE=k` (puzzle kind k on every
  level, `CRAWL_TPPUZZLE=1` before its first piece), `CRAWL_TRIAL=k` (trial k on every level, `CRAWL_TPTRIAL=1` before its
  door), `CRAWL_LOWER=f` (a lower storey of that share on any level), `CRAWL_TPSTAIR=1` (before the first stair down).
- **Saves**: puzzles and trials belong to their level and aren't saved; the record's new counts go in a ledger section
  (`LG_DEEDS`, *a draft name*), so the version needn't move. The lower storey needs nothing saved.
- **Housekeeping on the 1.0.0b11 branch**: two debug logs (`b.txt`, `dream.txt`) were committed at the top of the tree
  on `claude/eloquent-carson-1x049c`; they should go before it is squashed onto main.

## 12. Art, for the owner's sheets first

- `hazards.png`: each chosen hazard warning, harming and fading.
- `puzzles.png`: each chosen kind's pieces and places (niches and skulls, bells, effigies, cisterns and sluices, rails
  and levers, prisms and the beam), the wing's doors, and the hand holding a carried thing.
- `trials.png`: the four doors, each run's turned room, the hourglass.
- `storeys.png` (1.0.0b13): edges, balustrades and stairs for runs II-IV, a gallery and a well seen from above and below,
  and the low ceiling of a passage under the floor.
- Frames: a hazard in each run, a puzzle solved, a trial won and one lost, and, for the lower storey, the test level.

## Building it

As two releases (G2 a); each step a commit, tested in the harness.

**1.0.0b12**

1. **Hazards**: fixed and arising hazards, the run's mix with earlier runs', the cap, the chosen samples with their
   elements; Premonition and the autopilot; their sheet.
2. **Ways to touch the world**: take and set, turn, pull, light, strike, step, read (as P3 chooses); the hand carrying.
3. **Puzzles**: the sealed wing, each chosen kind with its maker, its clues and its check, the help, the dream solving
   them; their sheet.
4. **Trials**: the doors, the seal, the room's turn, the hourglass, the reward and the boon; each chosen trial with the
   dream's way through it; tuning runs to the target.
5. **The record, the Codex, the README and the page.**
6. **Checks**: ASan; the Windows build; eight dream runs from levels 1, 11, 21 and 31 against 1.0.0b11 on the same
   seeds; the trials' win rate and time left a run; puzzles solved by the dream.

**1.0.0b13**

7. **The test**: the two-storey renderer on a hand-made level; frames and its time a frame, for the owner.
8. **Two storeys**: the generator (galleries, wells, passages under, stairs), collision and edges, the hunt field and
   fliers across storeys, missiles with a height (L5), the lightmap and fog for each storey, sprites clipped.
9. **Edges and stairs**, a run at a time, sheets first.
10. **The autopilot on two storeys**, the map's two storeys, secrets, vaults and the exit below, its danger.
11. **Checks**, as above, and the frame time on the deepest levels.

As always, all of it tested in the harness only until the owner plays it.

## The questions, together

Answer by number; anything left alone takes the starred answer. The samples (hazards, edges, doors, puzzles, trials)
can be marked up line by line.

| | Question | Recommended |
|---|---|---|
| G1 | Which release | (a) 1.0.0b12, after 1.0.0b11 |
| G2 | One release or two | (a) two: hazards, puzzles and trials first; the lower storey in 1.0.0b13 |
| L1 | How the lower storey is drawn | (a) two storeys on one grid, a test first |
| L2 | A boss's level | (a) no lower storey |
| L3 | What makes it more dangerous | (a) a rank higher, a third more, hazards twice as often |
| L4 | Creatures between storeys | (a) walkers by stairs, fliers by wells, spirits through floors |
| L5 | Striking across storeys | (a) yes, through wells and galleries |
| L6 | Ladders | (a) stairs only |
| E1 | The edges | (a) as the table, marked up |
| H1 | The rhythm of hazards | (a) fixed and arising, more as the run goes |
| H2 | The most a stroke takes | (a) a quarter of your health |
| H3 | The hazard samples | (a) the owner marks them up |
| H4 | Lightning's resist | (a) none, as now |
| H5 | Soul-sapping hazards | (a) one a run |
| P1 | When puzzles come | (a) even levels from 6, never a boss's |
| P2 | What a puzzle opens | (a) a sealed wing, never needed for the exit |
| P3 | Ways to touch the world | (a) all but push |
| P4 | Carrying | (a) in the hand; casting sets it down |
| P5 | Help | (a) a whisper after three wrong tries |
| P6 | The dream and puzzles | (a) it solves them, slowly |
| P7 | What a puzzle pays | (a) the hoard, experience, a Cursed Coin |
| P8 | Puzzle kinds a run | (a) the owner picks; ours: Ledger, Cisterns, Points, Prisms |
| P9 | Harder deeper | (a) yes, within the run |
| T1 | Time running out | (a) the door opens, nothing gained |
| T2 | How often trials come | (a) one level in three from level 3 |
| T3 | The clock | (a) an hourglass and the seconds |
| D1 | The doors | (a) as drafted |
| T4 | Trials a run | (a) the owner picks; ours: two a run as listed |
| T5 | A record of them | (a) counts and achievements |
