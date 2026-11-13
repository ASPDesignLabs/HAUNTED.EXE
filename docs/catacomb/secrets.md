# Secrets, pockets, wisps and the Black Altars

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **Secrets** (`place_secrets`, `open_secret`, `update_slide`; docs/PLAN-b6.md). A run's levels hold
  3, 3, 4, 4, 4, 5, 5, 5, 6 and 6 links (`G.nlinks`, `G.links_found`: every link of every chain counts, in the caption,
  the tally, the record and the altar's gold), split into chains, longer deeper into the run. Each chain
  begins in a room (`Secret`: `x0..y1`, `len`, `links`) carved from solid plan blocks beside a hall: one
  block, two side by side, or two by two (`secret_block_free`); `Cell.secret` 1 room, 2 the cracked
  wall, 3 a doorway. The cracks show only within about two cells. E/Space pushes the wall right through
  the room in `SLIDE_TIME` (2.3 s). Loot by the room's size, off the stone's path, flagged
  `Item.secret` and kept out of the level's totals. A chained room has a doorway (`secret_door`: the
  back wall, else a side, solid rock behind it, `T_PASSAGE`). Links that find no room go deeper into a
  chain. The debug build prints `SECRETS level n: ...` with each room and doorway.

- **Pockets** (`PK`, `G_LEVEL`, `gen_pocket`, `pocket_build`, `pocket_return`, `pocket_door`,
  `carry_caster`). Walking into (or E at) a chained room's doorway saves the level whole in `G_LEVEL`
  (it waits: its clock, creatures and Wraith stop) and builds the chain's next link into `G` from the
  chain's seed: blocks in a square maze with loops and halls, 13 cells across early in the Crypts to 45
  at the end of a long chain in the Deep, a doorway back in its west wall, a doorway on in an outer wall
  or the chain's reward (for now a hoard). `carry_caster` copies the caster's spans of `G` across (keep
  `G`'s field order in mind when adding fields). What is picked up is remembered (`Item.gen`,
  `PK.taken`). The first time into a link is a secret found. Dying in a pocket throws you back to the
  chain's room at full health (`pocket_return(1)`) and the chain's creatures come back; sleepwalking takes
  the autopilot straight out (`human_end`). In a pocket the exit wisp goes to the way back; no Wraith, no
  wakers. The debug build logs `POCKET ...`, `EVENT pocket_in`, `pocket_out`, `pocket_thrown`.

- **Pocket creatures** (step 7, end of `gen_pocket`). The level's own kinds (`random_type`, with their
  escorts), away from the way in: about one per four blocks, a quarter more in the Backrooms, 16 at most;
  each a rank higher for each link past the first, up to black; health and blow raised by
  `theme_boost` (the Backrooms 1.25, Severance 1.3, whose also strike a fifth sooner: `m->cool` runs
  faster in `update_mob`). `Mob.gen` and `PK.slain` keep the slain dead, as `PK.taken` does items. Their
  kills go into the record, not the level's tally.

- **The Backrooms** (pockets in runs I and II, levels 1-20 of each descent; `G.ptheme` `PT_BACKROOMS`; docs/PLAN-b6.md
  step 5; built, the art approved; creatures in pockets come in step 7). `crawl_art_theme` swaps
  `R_FLOOR` (dark brown, mustard, pale yellow: carpet and wallpaper) and `R_CEIL` (off-white) on the way in
  and back on the way out; nothing in the crypt but its floor, ceiling and fog uses those two ramps.
  Walls `T_PAPER` (and `_DAMP`, `_OUTLET`), doorways `T_BDOOR`, and at the chain's end `T_VISPORT0..3`
  (`G.vis_x/y`, animated in `render_walls`) with the hoard to either side. `render_floor` draws
  `T_CARPET` (turned about per cell) and the ceiling tiles, with a tube panel over every block
  (`Cell.lamp`: 1 + its light, 255 dead; `T_CEILLAMP`/`T_CEILDEAD` by the light's state). A failing tube
  is a light with phase between -100 and 0 (`torch_flicker`: steady, then a burst of blinking).
  `make sheets` draws them as `backrooms.png`, with the glitch for creatures. `T_PASSAGE` from
  the crypt shows a pale yellow room on these levels. Things out of place are decorations with `DF_FLIP`
  (upside down on the ceiling), `DF_FLOAT`, `DF_SINK` (half into the floor, or with `DF_FLIP` the
  ceiling; drawn with `DR_CLIP`) and `DF_LOOP` (the level's own kinds walking the same few steps, glitching
  most of the time: `DR_LOOPED`). Every creature in a Backrooms pocket is drawn with `DR_GLITCH`
  (`glitch_rows`: in bursts, bands of rows torn sideways, some slipping red one way and blue the other).
  Sound: `snd_set_room(1)` fades the music and its drums, adds echo to the game's sounds and a faint
  mains hum. E at the visportal (`visportal_use`, `PK.vis_used`) gives full health, or fifty souls if
  whole, once a chain; then it stops drifting. The debug build logs `EVENT visportal`.

- **Severance** (pockets in runs III and IV, levels 21-40; `PT_SEVERANCE`; docs/PLAN-b6.md step 6; built, the art
  approved). `R_FLOOR` a muted green (carpet, doors, partitions), `R_CEIL` a cool white
  (walls, ceiling, the cooler's cabinet). `gen_pocket` carves no blocks at first: once the far end is
  known, a fifth of the blocks (and always the first and the far) are offices (`office`), the rest
  narrow halls from block middle to block middle, turning corners, with a hall out to the door on. Walls
  `T_WHITE` (`_SIGN`, `_ART`), doorways `T_SDOOR`, floor `T_SCARPET`, ceiling `T_SCEIL`/`T_SCEILLAMP`
  (every block lit and steady; ambient 0.16). Offices hold `SP_DESKS` (a solid prop) where nothing
  lies. At the chain's end the water cooler (`SP_COOLER`, a solid prop at `G.vis_x/y`, `G.vis_deco`)
  stands against a wall with the hoard beside it; `visportal_use` gives its one drink and turns it to
  `SP_COOLER_LOW`. Creatures are drawn `DR_LIT`. Sound: `snd_set_room(2)` fades the run's piece into
  `muzak_step` (eight bars of major sevenths: `PAT_EPIANO` comping, `PAT_BASS` root and fifth,
  `PAT_VIBES` on the tune; voices flagged `muz`, kept at 0.6, about 9 dB under the old march, as the owner
  asked) over the same hum and echo. `make sheets` draws
  `severance.png`.

- **Will-o'-wisps** (`Wisp` in `G.wisps`: [0] the one summoned, [1] the Black Altar's; `summon_wisp`,
  `update_wisps`, `render_wisps`, `wisp_target`, `wisp_route`). Things in the crypt, not on the hand: a
  wisp kindles before the caster (or over the altar), flies down its own BFS field (`wisp_route`: floor
  and doors, not the door of a vault you have no key for) at `WISP_PACE`, looking ahead along the way
  where the line is clear (`wisp_line`; a wisp that strays off its field over a prop steps back onto
  it), and hovers at its target until you come within 1.5 cells (exit, key); the altar's waits at its
  secret's door until it is sent home (below). Kinds `WK_EXIT` (blue, the exit's doorstep, or first the
  key the exit's vault needs: `past_vaults`), `WK_KEY` (the key's colour, `KEY_RAMP`) and `WK_ALTAR`
  (violet, a secret's door, or home to the altar's edge). It leaves motes that hang for `WISP_MOTE_T` (12 s) to show
  the way round corners, lights four cells round itself (`light_dynamic_ex(..., flat)`, which shows
  shades), and drives the Wraith back within three cells. A soul a wisp (`WISP_COST`); casting another
  replaces the first; the altar's is free. Reset per level. The debug build logs `EVENT wisp kind colour`.

- **The L menu** (`UI_WISPS`: `wisp_choices`, `update_wispmenu`, `render_wispmenu`). A tap of L opens
  it (paused): the exit wisp, a wisp to each key still lying with Keymaker, the altar's wisp while it is
  out at a secret (`AW_SEEK`); W/S, Enter (or E, or a click) casts, Space turns *L ALWAYS CASTS THIS*.
  With that on (`RUN.wisp_auto`, `RUN.wisp_kind`, `RUN.wisp_color`, saved), a tap casts and holding L
  0.6 s opens the menu (`G.l_t`, `G.l_down`). A click that closes any menu casts nothing until the
  button is let go (`G.click_spent`).

- **Black Altars** (`place_altar`, `update_altar`; `G.altar_*`). One a level from 6, on a
  level with secrets: in the exit room, in a block corner off the lanes and the way and two cells from
  the exit, otherwise in the room farthest from the start (`SP_BALTAR`, a solid prop; the flames are
  `SP_BFLAME_V*` and `_G*`). E opens its menu (`UI_ALTAR`: `altar_rows`, `update_altarmenu`,
  `render_altarmenu`), three gifts with a note on each: `altar_wake` (or `altar_send`), `altar_bless`
  and `altar_anoint`, which the autopilot calls directly. Dormant, then awake (ten spirit:
  `ALTAR_SOULS`), then gold once every secret on the level is found, every link of every chain
  (`G.links_found >= G.nlinks`), then spent. Waking it sends out its own violet wisp (`WK_ALTAR`, free) to
  the nearest secret still shut (`G.altar_seek`, `nearest_shut`), through the keys of any vault it lies in
  (`past_vaults`, shared with the way to the exit). Its round is `G.altar_wisp`: `AW_SEEK` (waiting at
  the secret's door, the cell before the cracked wall), `AW_HOME`, `AW_REST` (over the altar's edge). It
  waits while you are in the secret's room or a pocket beyond; once you have been in
  (`G.altar_inside`, `in_secret`) and come out, E facing it within 2.2 cells sends it home
  (`altar_callable`, `altar_call_home`). Resting, the altar's first row sends it on to the next secret
  still shut for `altar_price` (ten and five for every secret found on the level). Gold, it anoints one
  worn piece at random that isn't yet (`piece_anoint`: its stats rise by the quality's share, never
  past their caps). The blessing (`ALTAR_BLESS`, 25, in any state, `G.blessed`, cleared by
  `place_altar`, so it ends with the level or a death) makes every exit wisp turn the undead
  (`turn_undead`: within `BLESS_REACH`, 4 cells, in sight of it, as Dread does, fleeing from where the
  wisp was, `Mob.wfeared`/`wfx`/`wfy`, for 3 s after it passes; the Wraith is driven back); a blessed
  exit wisp has a pale gold halo, and the altar's fire burns with a white edge (`DR_EDGE`). The debug
  build logs `EVENT altar at`, `wakes`, `altar wisp home`, `altar sends price=`, `blesses`, `anoints` and
  `turned`.

- **Cursed Coins** (`RUN.coins`; `gain_coin`). One at the end of each chain, with the visportal's gift or the
  cooler's drink, and one more when every link on the level is found (`COIN_ALL`). Each is taken once a
  level whatever deaths come between (`RUN.coin_level/coin_bits`, saved), and comes in as a pop-up headed
  FOR THE SHRINE. Shown under the keys on the status bar (in `small_digits`), in the readout, and at the
  top of the caster's page. The debug build logs `EVENT coin`.
