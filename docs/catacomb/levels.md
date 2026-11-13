# Levels: state, the generator, runs and their looks

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **State.** `RUN` carries across levels, deaths and scene switches (level, hp, gold, souls, gear,
  shop flag). `G` is the level in play; `crawl_enter` clears it, `start_level` builds a level,
  `begin_shop` the merchant's room.

- **Generator** (`gen_level`). A plan of 11x11 blocks of 3x3 cells with one-cell walls between (plan
  grid `MG` = 23, map `MAPN` = 45, `mstart(m)` maps plan to map). Rooms span several blocks; halls are a
  maze of blocks whose stretches are drawn 1, 2 or 3 cells wide (`mz_widths`: a trunk from the start
  room to the farthest room is widest). Every hall keeps its middle **lane**: cells with x or y = 2
  (mod 4). Then: doors and colour-coded vaults (gold from level 1, silver from 3, crimson from 5) with
  keys, `place_secrets`, wall textures by room theme (`theme_tex`), colonnades, torches,
  `dress_level` (chains, hooks, gibbets, chandeliers, roots, web sheets, graveyards, fire pits, totems;
  every room gets a light), lightmap, fog.

- **Runs and sets** (`crawl.h`: `crawl_run`, `crawl_set`, `crawl_step`, `crawl_boss`, `crawl_loop`,
  `crawl_decay`, `crawl_depth`; `RUN_LEN` 10, `DESCENT_LEN` 40; docs/PLAN-b7.md). Levels come in four
  runs of ten, a set each (I the Crypts 1-10, II the Catacombs 11-20, III the Hollows 21-30, IV the
  Deep 31-40); the tenth of each is its boss's, and past level 40 the runs repeat. Each set has two
  tints that the stone, floor and ceiling move between over its ten levels. Decay (`decay()`: grime,
  moss, drips and cracks by `decay_tex`, and more of the dressing) starts fresh at each run and reaches
  1 on its last level. Depth (`depth()`: fewer torches, thicker fog, a darker base light, older bones)
  grows run by run and stays at 1 from the Deep on. The rank mix (`pick_rank`) and the merchant's
  qualities follow the sets, so they climb over ten levels; a set's newest rank comes in on its run's
  fifth level. Pockets are the Backrooms in runs I and II, Severance in III and IV. The Wraith never
  comes on a boss level. `level_name` gives each of the forty its name, with a numeral after the first
  time down. `crawl_art_level` regenerates the wall textures with a level seed. Textures
  and sprites are 64x64, column-major.

- **Each run's look** (1.0.0b10, docs/PLAN-b10.md; all four runs are drawn and in play; `crawl_drawn` can hold one back). `crawl_art_run(level)`
  (after `crawl_art_level`) makes the run's roles into their slots: `T_MATV(m, v)` (`NMAT` materials, `NVAR` looks
  each, on one 16-pixel course grid, so any two meet without a step; `g_mid` keeps their blocks for the blends),
  `T_BLEND` (`NBLEND`, one material into the next across a cell, the first on the left and in front: whole blocks
  fall away, a skin frays), `T_FEAT` (`NFEAT`, overlays with `C_CLEAR` where the material shows; the torch's three
  frames last), `T_FLOORS`, `T_CEILS`, `T_RDOOR`/`T_RJAMB`/`T_RLOCKED*`, and the keys' sprites. Rot: `decay_wall`
  (its grime and moss from a field shared by the run's textures, so it runs on across a seam; drips and cracks
  each texture's own), `plaster_flake`, `decay_flat`, the banner's tatter. Names and blend pairs are in
  `crawl_looks`. `make sheets` draws them (`crypts.png`, `cryptrooms.png`: the room types laid as stretches).
  The specials (the sign, the Repository, the Portal, the gate, the fonts, the grate) are drawn again on the
  run's ashlar (`specials_on`, `rebase`); other runs keep `specials_plain`. `crawl_art_run` picks the run's own
  (`crypts_run`, `catacombs_run`, `hollows_run`: run II's slots are named `M2_*`, `F2_*`, `FL2_*`, `CE2_*` in
  `crawl.h`, run III's `M3_*` ..., in run I's roles: floor 1 lies as laid, floor 4 is floor 0 with the rot,
  ceiling 0 the rooms', 1 the halls'); run II's rot is `decay_catacomb` (the flood line, `flood_y`) and
  `decay_flat2`, run III's `decay_hollows` (scree at the foot, claw marks on one look of each), run IV's
  `decay_deep` and `decay_flat_deep` by `deep_glitter`, `deep_heat` and `deep_dark` (glittering, then cracks
  glowing, molten runs on one look of each, then dark; the fissures, geodes, gem seams, cinders and the doors'
  grilles glow in `R_FLAME`/`R_GLOW` by the same curve); each draws its own keys (`spr_key_ring`, `spr_key_tag`,
  `spr_key_glass`) and treasure (`spr_coins_silt`, `spr_reliquary`, `spr_pearls`, `spr_drown`; `spr_nuggets`,
  `spr_rawgems`, `spr_strongbox`; `spr_molten`, `spr_glowgems`, `spr_idol`), which `treasure_plain` puts back at
  every level. Run IV's torch is a magma fissure (`F4_FISSURE`'s three frames). `SHEET_LAST=n` makes a run's sheets
  show level n as their second (run IV's burning is at 37). `make sheets` draws any run's pages from `RUNSHEETS` in
  `tools/sheet.c` (`draw_runlook`, `draw_runrooms`).

- **Laying it** (`G.faced`: a drawn run's level, and her hall always; never a pocket; `G.look` whose stone, the
  run's set, her hall 0). A room's type (`RT_*`,
  `RTYPES`: its floor, ceiling, dressing style and the old `TH_*` theme its dressing and dwellers follow; the
  chapter house is `TH_BRICK` so it keeps none) comes from `pick_rtype` (its size in blocks and the run's step;
  two ossuaries a level, three late in the run; one chapter house) and rides on `Room.rtype`; `dress_level`
  takes its style from it. At the end of `gen_level` (after the arena) and `gen_shop`, `lay_faces` walks each
  straight stretch of wall on each side (`faceable`, `face_open`; a stretch ends where the place it looks into
  changes, `face_key`) in the face's own left-to-right order (+y, -x, -y, +x for faces looking east, south, west,
  north) and `lay_stretch` gives it materials by its room type (`cell_rtype`: a secret's closet by the run,
  `CLOSET`, an undercroft, a silted nave, a stope or a cinder field; the arena `ARENA`, the knights' vault, the
  spinners' gallery, the chapel or the throne, its burrows `BURROW`, a niche, a flooded niche, an ore chute or the
  fused dead): the halls, undercrofts and the plot from a smooth field (`mat_field`, six cells),
  so whole stretches agree; a blend where two meet (`blend_of`, `Cell.fflip` mirrors it); what stands on the wall
  by rule (niches in rows, banners centred, slabs spaced, a tablet, an undercroft's shrine once). `Cell.face[4]`,
  `fover[4]` (the overlay; a torch's on `ftorch`, its face + 1, set where the torch is placed), `flo`, `cei` (the
  floor and ceiling; flags break with the rot), `fflip` bits 5-7 the floor's turn (4 across), bit 4 laid as it
  is (the lozenges, the gallery's rails along their room). Run II's water (`FL2_WATER`: a cistern's middle, a
  canal's length) is walked as any floor, but `footstep` plays `SFX_SPLASH` on it (`on_water`); a saint carries
  candles and a geode glows (`wall_lights`, ten a level, none in an arena); a magma vent has up to three of the old
  Deep's magma pools in its corners (`dress_level`). `render_walls` takes `face[fd]` for an ordinary wall and mixes the overlay in; a secret's cracked
  wall carries no overlay, and the sliding box wears the face it shows the hall. `render_floor` steps along a row
  in 16.16 fixed point texels (`U`, `V`: the cell by `>> 22`, the texel by `>> 16`, so `TS` must stay 64) and
  looks the cell up once per change. Debug: `ROOMS level n: ...` (each room's type, then the water's cells and the
  lights in the walls), `CRAWL_TPROOM=k` (in a corner of the
  first room of type k, `RT_*`, looking across it, once you take control).
