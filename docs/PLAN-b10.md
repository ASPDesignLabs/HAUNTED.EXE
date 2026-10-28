# 1.0.0b10: a look of its own for each run

**Status: run I is in the game (28 October 2026), tested in the harness only; its frames from levels 1, 5
and 10 and her hall wait for the owner's approval before run II's sheets. Run I's art was approved on its
sheets (`make sheets`: `crypts.png`, `cryptrooms.png`), and the owner took the defaults for the three
questions below.** The owner asked for the crypt's
walls to flow together, for floors and ceilings, and for each run of ten to look like its own place.
This plan sets out a theme for each run, the room types that carry it, and how the generator lays
them; then the work, one run at a time.

## What the owner asked for

- Each run visually distinctive: a theme for each, with more textures for variation and blending in
  each. The red brick is out of place in a structure buried so far below anyone's reach.
- Sets and room types for each run. The variety is tied to the set's rooms, but the generator may
  blend some of them together outside their usual rooms.
- The descent plan's damage progression is the starting point, with more visual variety going deeper.
  Doors, torches, keys and the look of treasure change between runs, to match the run's theme.
- Skull walls in rooms only.
- A clean reset at the start of each run, as now.
- Floors and ceilings: rooms and halls differ, and the rot reaches them.
- One set at a time: sheets first, then frames from the game, then the next set.

## What is wrong today

The texture review (October 2026) found it. A wall cell takes its texture by a hash of its position,
with no regard for its neighbours. The halls have nine-cell zones, two in ten of them ossuary, so skull
walls run along ordinary halls. A room's wall shows the room's texture on every face. Room themes are a
coin toss. The materials sit on different course grids, so every change is a hard edge. The niche,
banner and torch bring their own stone. The crypt has no floor or ceiling textures. Every run is the
same ten levels in a new tint, and the brick and the doors ignore the set.

## The four runs

The level names already tell the story: an order's crypts, the drowned catacombs below them, a mine
the dead were tipped into, and the black glass at the bottom.

| | I. The Crypts (1-10) | II. The Catacombs (11-20) | III. The Hollows (21-30) | IV. The Deep (31-40) |
|---|---|---|---|---|
| What it is | The burial vaults of a forgotten order of monks and knights, cut deep and kept | Tunnels cut into wet slate beneath them, flooded and drained, saints in its walls; webs by its end | Caverns and an abandoned mine where the dead were dumped; a cult's chapel at the end | Volcanic depths: black glass, crystal, magma, ash; bones fused into the rock |
| Materials | Dressed ashlar, coursed rubble in lime, lime plaster, cut bedrock, bone wall | Bedded slate, slate lining, silt and rubble, calcite flowstone, bone in silt | Rough granite, a pick-cut mine face, timber shoring, basalt columns, packed backfill | Obsidian, basalt columns, ash-crusted rock, crystal crust, bone in glass |
| Rooms | Ossuary, burial vault, chapter house, knights' vault, undercroft | Saints' catacomb, cistern, silted nave, canal, spinners' gallery | Timbered gallery, stope, charnel pit, fossil hall, the chapel | Geode, glass hall, cinder field, magma vent, the throne |
| On the walls | Burial niches, the Order's banner, knights' slabs, wall shrines, tablets | Flooded niches, saints' alcoves, culverts, flood lines and handprints, web curtains | Ore veins, fossil ribs, timber frames, chutes, the cult's marks | Geodes and gem seams (glowing), magma fissures, gold veins, fused bones |
| Floor / ceiling | Flags, a lozenge floor, setts, bone earth / rib vaults, barrel vaults, rough rock | Wet flags with puddles, silt / dripping rock, slate vaults | Gravel, rails, trodden earth / rough rock, timber lagging, roots | Cracked glass, ash / glinting rock, crystal |
| Doors | Oak under iron; vaults strapped in their key's metal | Sluice doors, iron gone green | Rough mine doors, chains | Black iron, a glowing grille |
| Light | Iron sconces and pitch torches | Lanterns on hooks, candles at the saints | Oil lamps hung on the timber | The walls themselves: crystal and magma |
| Keys | Church keys with a trefoil bow | Corroded keys on a ring | Heavy keys with a tally tag | Keys of glass with a metal bow |
| Treasure | Coin, chalices, gems, crowns, chests (as now) | Silt-black coin, a saint's reliquary, pearls | Nuggets, raw gems in the rock, a strongbox | Molten gold, glowing crystal, an idol of glass |
| Its rot, first level to tenth | Kept, then damp: moss, plaster falling, blocks loose, cracks | Damp, then the floods: water lines climb, silt, algae, then webs | Worked, then abandoned: props broken, cave-ins, claw marks | Glittering, then burning, then molten, then dark |

The keys keep their three colours (gold, silver, crimson) in every run; only their shape changes. A
run's room types replace today's ossuary, crypt, brick and moss. The halls are a room type of their
own. The pockets (the Backrooms and Severance) keep theirs. The merchant's hall is outside the runs
(question 3).

## Run I, the Crypts, as drawn

On `crypts.png` and `cryptrooms.png`. Everything is made by the code (`crawl_art_run` in
`crawl_art.c`), each level from its own seed, at its step's rot.

- **Materials**, two looks each that sit side by side without a seam: **dressed ashlar** (blocks on
  16-pixel courses, the mason's tooling, a mark here and there); **coursed rubble** in pale lime (it
  takes the brick's place: what an order would build with below ground); **lime plaster** with a
  painted dado of red ochre and its crosses, sometimes a consecration cross, falling away with the rot
  to the rubble behind; **cut bedrock** dressed with the pick; and the **bone wall**, skulls between
  banks of long bones laid end out, rooms only.
- **Blends**, each a cell wide, its two edges matching the plain materials: ashlar into rubble (blocks
  fall away whole), plaster into ashlar and into rubble (the plaster frays, its lip catching the light),
  ashlar and rubble into the bedrock, the bone wall into ashlar.
- **On the walls**, drawn apart and laid over whatever material is there: burial niches, the Order's
  banner (tattering with the rot), a knight's slab, a wall shrine with a candle, a memorial tablet, and
  the torch in its sconce.
- **Floors**: flagstones, the chapter house's lozenges, a hall's setts, an ossuary's bone earth, and
  flags broken through for the later levels. **Ceilings**: a rib vault over rooms, a barrel vault over
  halls, rough rock over an undercroft.
- **Doors and keys**: oak under iron with a grille, vaults strapped in their key's metal with the
  Order's cross on the plate; church keys with a trefoil bow. The treasure stays as it is.
- **Rooms**: the halls (ashlar, patched with rubble, cut into the rock in places), the ossuary (bone
  walls facing in), the burial vault (niches in the ashlar), the chapter house (plaster, banners,
  tablets, the lozenge floor), the knights' vault (slabs and banners), the undercroft (rubble giving
  way to the rock, a shrine cut in it).

## How the generator will lay it

- **Faces, not cells.** Each wall cell gets a texture for each of its four faces (`Cell.face[4]`: a
  material or a blend, a flip, and an overlay). The raycaster picks the face it hit. A room's walls
  show its own materials inside, and the hall outside keeps its own.
- **Stretches.** The generator walks each straight stretch of wall between corners and doorways, on
  one side of it, and gives it one material from its room type's mix. Where two stretches meet flat
  and their materials differ, the cell between takes the blend (flipped as the order needs). Now and
  then a hall's stretch takes a neighbouring room's material for a few cells, blended at both ends: the
  owner's "outside their usual rooms".
- **Features** are placed along a stretch by rule: niches in rows, banners centred on a wall,
  slabs spaced, torches every so often (where the light is wanted, as now). A secret's cracked wall
  stays plain material, so its cracks show.
- **Room types** by what a room is, not a coin toss: its size, how far it lies from the start, whether
  a vault or the exit is in it, and the run's step (the ossuary grows likelier deeper in the run, say).
- **Floors and ceilings** come from the cell's room type (rooms) or the halls', turned about cell by
  cell as the pockets' carpet already is (patterns like the lozenge floor lie as laid). With the rot,
  some cells take the broken variant. The pockets already draw theirs per pixel, so the cost is known;
  it will be measured with `build/host bench`.
- **Per run**: `crawl_art_level` calls `crawl_art_run`, which makes the run's roles into the same slots
  (`T_MAT`, `T_BLEND`, `T_FEAT`, `T_FLOORS`, `T_CEILS`, `T_RDOOR`...), and its keys, torches and
  treasure into their sprites. The rest of the game keeps using its slots unchanged.
- **The palette**: each run retints `R_STONE` (its main rock), `R_FLOOR` (its second stone, the floors,
  the mortar) and `R_CEIL`, as now. The ramps the creatures use stay as they are (rank II is green from
  `R_MOSS`, and so on), so growth and glows come from the ramps that already suit them: moss and algae
  green, lichen from `R_BONE`, crystal from `R_GLOW` and `R_PURPLE`, magma from `R_FLAME`. Run IV's glows
  may use palette cycling, as the descent plan proposed.
- **The boss arenas** take their run's room types (the knight's lists in the knights' vault, the
  spider's in the spinners' gallery, the Deacon's in the chapel); their gates, fonts and grates become
  overlays on the arena's material.
- **Saves**: untouched. Every texture comes from the level and its seed.

## The work, one run at a time

1. **Run I's art and its sheets** (done, approved).
2. **The engine**: faces, overlays, stretches, blends, room types by rule, floors and ceilings, run I's
   doors, keys and torches in play. Frames from the game on levels 1, 5 and 10 for approval. (Built; the
   frames wait for approval. See "Run I in the game" below.)
3. **Run II**: sheets, then frames. 4. **Run III**. 5. **Run IV**, with its glows.
6. **Checks**: `bench` against 1.0.0b9; ASan; eight-seed dream runs against 1.0.0b9 (the lights move,
   so the autopilot's levels may play a little differently); the README's previews and "What has been
   verified"; the page.

## Questions for the owner (answered: the defaults)

1. Do the four themes fit? Each run's column above is a proposal; say what to change. *Taken as they
   stand.*
2. Run I's sheets: anything to change before it goes into the game? *Nothing.*
3. The merchant's hall is red brick too. Keep it (it is her room, not the crypt's), or rebuild it once
   in run I's ashlar and rubble, the same in every run? *Default, taken: rebuild it once, the same in
   every run.*

## Run I in the game (step 2)

- **Which levels.** Levels 1-10 of each time down (`crawl_drawn`) and her hall, always. Runs II-IV keep the
  old look until they are drawn; the pockets keep theirs.
- **Room types** (`pick_rtype`): an ossuary at most twice a level (three times from the seventh level), a
  chapter house once and only in a room of four blocks or more, a knights' vault from the third level in rooms
  of two blocks or more (one, then two from the sixth), the undercroft likeliest in the smallest rooms, the
  burial vault the rest. Each type keeps the old dressing that suits it (graves in the burial vault and the
  knights' vault, fire or cages in an undercroft) and the old theme's dwellers: the ossuary's and the vaults'
  skeletons as before; the chapter house has none.
- **The halls** take ashlar, rubble or the bedrock from a field that changes over about six cells, so a whole
  run of hall is dressed, patched or raw, and the change between them is a blend. A hall's stretch of ashlar
  may carry a tablet now and then. The undercrofts and a secret's closet are rubble and rock, the plot ashlar
  and rubble.
- **Floors and ceilings**: setts and the barrel vault in the halls, flags (breaking through as the run goes
  on, to about two in five by its tenth level) under a rib vault in the burial and knights' vaults, the
  lozenges in the chapter house, bone earth in the ossuary, rough rock over an undercroft.
- **Doors, jambs, keys and torches** are run I's: oak under iron, vaults strapped in their key's metal, the
  trefoil keys, and the torch's sconce as an overlay on whatever stone it is on.
- **The boss's arena** takes the knights' vault (the lists): ashlar, slabs and banners, its burrows niches;
  its gate, fonts and grate are drawn again on the ashlar, as are the sign, the Repository and the Portal. A
  challenge's fallen way is run I's rubble.
- **Her hall** is ashlar with the Order's banners under a rib vault, the plot ashlar and rubble under the
  barrel vault, whichever run it comes before.
- **Checked** (28 October 2026), in the harness only: nothing played by hand or run under Wine.
  AddressSanitizer clean over the dream from level 1, the knight's level, levels 9 to 11 (the change back to
  the old look), a person's pocket, her hall, a challenge from the Portal and an opened secret. Eight
  100-minute dream runs from level 1 (seeds 1-15 odd) against 1.0.0b9 on the same seeds: a median of 11
  levels cleared (9.5), 17 deaths (17; 13 on level 10, against 16), the knight beaten in 5 runs of 8 (4) in
  19 tries (21), a level's median time 358 s (351), its treasure share 0.84 (0.86); the old build's numbers
  are the ones its release recorded. `build/host bench crawl 60`, three runs each in turn: 3.64-3.82 ms a
  frame against 3.25-3.90 ms. The floor and ceiling now find their cell and texel in fixed point along a row
  (a first version that worked them out per pixel in floats cost a fifth more than 1.0.0b9).
