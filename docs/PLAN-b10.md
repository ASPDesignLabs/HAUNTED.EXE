# 1.0.0b10: a look of its own for each run

**Status: all four runs are in the game, tested in the harness only, and released as 1.0.0b10 (29 October 2026,
at the owner's word). Run I was approved on its sheets (`crypts.png`, `cryptrooms.png`) and in its frames (28
October 2026); run II on its sheets (`catacombs.png`, `catarooms.png`, 29 October 2026), with the owner's answers
to its questions below. The owner then left runs III and IV to my judgement, without sheets or frames to approve
first: both are drawn (`hollows.png`, `hollowrooms.png`; `deep.png`, `deeprooms.png`) and in the game.** The owner asked for the crypt's
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

## Run II, the Catacombs, as drawn

On `catacombs.png` and `catarooms.png`, in the same slots and roles as run I's, so the engine lays them the
same way (`catacombs_run` in `crawl_art.c`, each level from its own seed).

- **Materials**: **bedded slate** as it was cut, beds of every thickness, the partings waving a little, pieces
  broken out of the face, the cleavage at a slant (the halls' rock); **slate lining**, dressed slabs a hand
  thick laid dry in courses of 16; **silt and rubble**, river stones of several kinds in banded silt;
  **flowstone**, lobe over lobe like wax down a candle, tan where the water ran most; **bone in silt**, the dead
  the floods brought down, long bones every way, skulls, vertebrae, some under the mud again (rooms only).
- **Blends**: lining into slate and into silt (slabs fall away), slate into silt (pieces fall away),
  flowstone over slate and over lining (its edge frays), bone in silt into plain silt.
- **On the walls**: flooded niches (water standing in them, a skull afloat), a saint's alcove (a saint in
  faded blue under a gilt halo, candles on the sill), a culvert (bars, the water coming out over the foot of
  the wall), handprints in pale silt, a web curtain, and a lantern on an iron hook in the torch's place.
- **Floors**: wet slate flags, silt in ripples (laid as laid), slate set on edge (the halls), standing water,
  and flags with puddles for the later levels. **Ceilings**: a slate vault over rooms, the bedded rock over
  halls, dripping rock hung with calcite straws.
- **The rot**: a flood line at one height on every wall of the run, so it runs on across seams, climbing from
  the foot of the wall on level 11 to about three fifths up on level 20; under it the stain, older lines and
  silt, algae along it in patches. Webs are things on the walls (and the dressing's sheets), not in the stone,
  so they don't repeat cell after cell.
- **Doors and keys**: sluice doors of riveted iron, rusted, green with slime where the water stood, a grille
  and a wheel; the vaults banded in their key's metal with a plate and cross; keys on a ring, pitted.
- **Treasure**: coin gone black in the silt (some still bright), a saint's reliquary (a little gilt house,
  blue enamel, a bone behind crystal) in the chalice's place, pearls out of a rotted purse in the gems',
  and the crown and the chest as the water gave them back. Only the look changes; their worth stays.
- **Rooms** (`catarooms.png`): the halls (the slate as cut, lined in places, patched with silt; lanterns), the
  saints' catacomb (lining, saints and flooded niches), the cistern (flowstone over the lining, culverts,
  water underfoot), the silted nave (silt banked up the slate, handprints), the canal (lining both sides,
  culverts, lanterns over the water), the spinners' gallery (bone in silt, webs; the spider's arena too).

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
3. **Run II**: sheets, approved; in the game (see "Run II in the game"). 4. **Run III**: drawn and in the game
   without approvals, as the owner asked (see "Run III in the game"). 5. **Run IV**, with its glows: drawn and in
   the game the same way (see "Run IV in the game").
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

## Questions for the owner on run II (answered)

1. Water underfoot in the cistern and the canal: only a look, walked through as any floor, or should deep
   water slow you as the bile does? *Only a look, but it should splash: a sound that tells you you are walking
   in water.*
2. Webs outside the spinners' gallery: from the run's sixth level, more by its tenth, as the descent plan's
   "then webs" has it? *Yes.*
3. The spider's arena on level 20 dressed as the spinners' gallery (bone in silt, webs, the culverts for its
   bile)? *Yes.*

## Run II in the game

- **Which levels.** Levels 11-20 of each time down. `G.look` says whose stone a level is laid in (`crawl_set`;
  her hall is always run I's), so the room types, the laying rules and the arena follow it.
- **Room types**: the saints' catacomb (lining, saints every third cell, flooded niches between; candles at a
  saint's feet, ten lit a level at most) the commonest, the silted nave likeliest in small or long rooms, the
  canal only in long rooms (water down its middle, three cells wide in a wide one), the cistern in square rooms
  of four blocks or more (water wherever no wall or door is next to it, flags round the edge), the spinners'
  gallery from the run's third level (one, two from the seventh). The saints' catacomb keeps the crypt's
  residents; the nave keeps its graves but no dwellers; the spinners' gallery is an ossuary's.
- **The halls** take the slate as cut, the lining and the silt from the same smooth field as run I's; handprints
  now and then on a stretch that isn't slate. From the run's sixth level, webs on the walls, more each level.
- **Water**: on the standing water a footstep is `SFX_SPLASH` (a slap, a slosh, a drop or two) in place of the
  step; it is walked through as any floor.
- **The spider's arena** is the spinners' gallery, its burrows flooded niches; its outlets are culverts, and the
  grate the omens open is the culvert drawn over the lining.
- **Checked** (29 October 2026), in the harness only. AddressSanitizer clean over the dream from level 11, the
  spider's fight, levels 19 and 20, the spider's fall and on into 21, a pocket on 13, her hall before 11 and a
  cistern on 15. Eight 50-minute dream runs from level 11 at `CRAWL_GEAR=3` (seeds 1-15 odd) against 1.0.0b9 on
  the same seeds: a median of 16.5 levels reached (16), no deaths in either. Sixty levels gave 45-101 cells of
  water a level.

## Run III, the Hollows, as drawn

On `hollows.png` and `hollowrooms.png` (`hollows_run` in `crawl_art.c`; slots `M3_*`, `F3_*`, `FL3_*`, `CE3_*`
in `crawl.h`, in run I's roles).

- **Materials**: **rough granite**, worn lumps lit from above, cracks along some joints, pale feldspar and dark
  mica, one look with a band of quartz; **a pick-cut face**, the pick's strokes in patches all over it, a drill
  hole half left in it, one look with the scar of a charge fired; **timber shoring**, boards two to a course
  nailed against the rock, half a post at each edge so that two cells make one post, one look with a board gone;
  **basalt columns**, iron grey, three faces of each showing, joints across, one look with a piece broken out (the
  chapel's); **packed backfill**, earth and broken rock tipped in with bones in it, one look with a skull (rooms
  only).
- **Blends**: the face into the granite and into the backfill (it frays), the timber into the face and into the
  granite (the boards end at a post), the basalt and the backfill into the granite.
- **On the walls**: an ore vein glinting in its quartz, fossil ribs (some great beast's, in the rock), a timber
  set (two legs and a cap), an ore chute, the cult's marks (a red eye in a ring), and an oil lamp on its bracket
  in the torch's place.
- **Floors**: gravel trodden flat, rails on their sleepers (laid along the gallery), trodden earth with worn
  hollows (the halls), the charnel floor (bone in the earth), and gravel under fallen scree for the later levels.
  **Ceilings**: timber lagging over the gallery, rough rock over the halls, the stopes and the chapel, roots
  through the rock over the charnel pit and the fossil hall.
- **The rot**, worked then abandoned: dust and grime, scree banked at the foot of every wall and rising through
  the run, cracks, boards split and fallen, and late in the run claw marks dragged down one look of each stone (so
  they don't repeat cell after cell).
- **Doors and keys**: rough mine doors with a Z brace; the vaults chained, the padlock in their key's metal; heavy
  keys with a tally tag.
- **Treasure**: nuggets in the coins' place, raw gems still in their rock, a strongbox in the chest's; the
  chalice and the crown as they are.
- **Rooms** (`hollowrooms.png`): the halls (the cave as it was, cut through with the pick, shored in places; a
  timber set every six cells on the rock; lamps), the timbered gallery (timber, sets along it, lamps on the posts,
  rails down it, carts by them), the stope (a face worked out, ore still in it, a chute; a fire), the charnel pit
  (backfill and the dead tipped in with it, the rock at its ends), the fossil hall (granite and ribs), the chapel
  (basalt, the cult's marks, its altars and candles).

## Run III in the game

- **Room types**: the stope the commonest (about two rooms in five), likeliest in the smallest rooms and the
  largest; the timbered gallery in long rooms (one in five), with its rails along the room's middle (`FL3_RAILS`,
  drawn along y and turned across for a room that runs along x); a charnel pit once a level (twice from the sixth);
  a fossil hall in a room of two blocks or more (twice from the seventh); the chapel once, from the seventh level,
  in a room of four blocks or more. The gallery, the charnel pit, the fossil hall and the chapel keep resident
  dead (about three rooms in five, as in run I); the stope none.
- **The halls** are granite, the face and timber from the field; a timber set every six cells along the rock.
- **The Deacon's arena** is the chapel: basalt, the cult's marks, its burrows ore chutes (`F3_CHUTE`).
- **Light**: the old Hollows' lights where they were (lanterns hung in the big rooms, the mine lamps), each
  torch an oil lamp on its wall. The stone was lifted a little after the first frames, so that the levels' average
  picture level matches 1.0.0b9's on the same seeds (about 10% from level 21 to 25).
- **Checked** (29 October 2026), in the harness only. AddressSanitizer clean over the dream from level 21, the
  Deacon's fight and fall, levels 28 to 31 (into run IV, still the old look), a pocket on 23, the chapel walked
  into and a gallery. `build/host_debug bench crawl 60` from levels 21, 25 and 30: 2.2-2.5 ms a frame, as
  1.0.0b9. Eight 50-minute dream runs from level 21 at `CRAWL_GEAR=5` (seeds 1-15 odd) against 1.0.0b9 on the same
  seeds: a median of level 27 reached (27), one death (none; on level 21), 31.5 creatures slain a level of 40 (31
  of 40), a level's median time 362 s (362).

## Run IV, the Deep, as drawn

On `deep.png` and `deeprooms.png` (`deep_run` in `crawl_art.c`; slots `M4_*`, `F4_*`, `FL4_*`, `CE4_*`). Its rot
runs its own way, as the plan's table has it: glittering on the run's first levels (mica and glass dust catching
the light), burning through its middle (cracks heating through every stone and glowing, the floors' seams and
the shattered glass alight, the iron doors' grilles bright), molten on its seventh and eighth levels (the rock
running down from a crack on one look of each stone), and dark at the end, the fire gone out of the rock and the
cracks cooled red-black, the crystal's light nearly out, for the Wraith. `make sheets` shows the first and the last
level; `SHEET_LAST=37 build/sheet deep out.png` shows the burning.

- **Materials**: **obsidian**, black glass broken in shells, each facet ribbed about the point it flaked from and
  the light sliding over its curve, one look with a great flake struck off; **basalt columns**, as the Hollows' but
  in the run's own stone, ash going violet; **ash-crusted rock**, grey and full of gas holes, ash settled on every
  ledge and run down from it, one look with a vent's mouth gone yellow with sulphur; **crystal crust**, amethyst
  grown point after point, each a pyramid of six faces seen end on, a light down in a few, one look with a great
  crystal; **bone in glass**, the dead the fire caught, fused into the obsidian with its sheen across them (rooms
  only).
- **Blends**: the obsidian into the ash (facets fall away), the basalt into the obsidian and into the ash (columns
  fall away), the crust into the obsidian and into the ash (crystals fall away), bone in glass into plain glass.
- **On the walls**: a geode broken open, its rind banded like agate and its crystals pointing in, their points
  alight (a light in the room, as the saints' candles are, ten a level at most); a seam of gems, some with a fire in
  them; threads of gold through white quartz; one of the dead fused into the rock, an arm raised as if it clawed
  at it; the throne's frieze, a crowned dead king on his seat inlaid in gold; and in the torch's place a fissure
  with the molten rock in it, its three frames pulsing, which is the run's light ("the walls themselves").
- **Floors**: black glass with a crack or two (rooms), the throne's great flags with gold in their joints (laid as
  laid), drifted ash (the halls), cinders and crust with their seams alight while the rock burns, and the glass
  shattered for the later levels, the fire showing under it. **Ceilings**: crystal over the geode and the throne,
  glinting rock over the halls and the glass halls, rock sooted black over the cinder fields and the vents.
- **Doors and keys**: black iron in riveted plates with a grille, the fire behind it; the vaults banded in their
  key's metal with a lock plate; keys of glass with a bow and collar of their metal.
- **Treasure**: coin run molten, a few still whole at its edge (the coins); a cluster of crystal with the light in
  it (the gems); an idol of black glass, a seated king with gold eyes and crown (the chalice). The crown and the
  chest stay.
- **Rooms** (`deeprooms.png`): the halls (ash, glass and columns, fissures for light, gold here and there, the
  dead in the glass), the geode (crystal crust, geodes alight), the glass hall (obsidian, gem seams, the fused
  dead), the cinder field (ash rock, the dead burnt into it, cinders underfoot), the magma vent (basalt giving way
  to ash, gold, magma welling up in its corners), the throne (bone in glass, the frieze over its seat; the Wraith's
  arena).

## Run IV in the game

- **Room types**: the glass hall the commonest; the geode likeliest in small rooms; the cinder field in rooms of two
  blocks or more, likelier as the run burns; a magma vent once a level (twice from the fifth), with up to three
  pools of magma (`magma`, the old Deep's prop, lit) in its corners; the throne once, from the fifth level, in a
  room of four blocks or more. The glass hall and the throne keep the crypt's resident dead, the cinder field the
  ossuary's (its skull heaps too); the geode and the vent none. The old Deep's crystals, glass shards, altars and
  gargoyle perches still dress them by their style.
- **The halls** are ash, obsidian and basalt from the field, with gold veins now and then and, on the glass, one of
  the dead.
- **The Wraith's arena** is the throne: bone in glass, its burrows the fused dead (`F4_FUSED`). The geodes' lights
  are kept out of it, so its darkness is the braziers' to break.
- **Light**: the torches are fissures; the geodes add a light each (ten a level, with the saints'
  candles' rule: `wall_lights`); the vents' magma lights itself. The levels' average picture level matches
  1.0.0b9's on the same seeds (means of 8.4-13.5% against 8.9-14.1% from levels 31, 35 and 39).
- **Checked** (29 October 2026), in the harness only. AddressSanitizer clean over the dream from level 31, the
  Wraith's fight and fall and on into level 41 (run I's look again), levels 38 and 39, a pocket on 33, the throne
  walked into and a magma vent. The Windows build compiles without a warning. Eight 50-minute dream runs from
  level 31 at `CRAWL_GEAR=8` (seeds 1-15 odd) against 1.0.0b9 on the same seeds: a median of level 36.5 reached
  (37), no deaths (one, on level 31), a level's median time 347 s (342). The Deep holds a little more than it did:
  39 creatures a level in those runs against 35.5, and 21.6 at a level's start against 19.9 over forty-five
  levels, since the glass halls, the throne and the cinder fields keep resident dead. Taking the cinder fields'
  away changed that by 0.2, so they keep them; the runs show no harm, but a person will meet a few more.
