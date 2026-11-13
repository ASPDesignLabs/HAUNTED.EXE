# Drawing: the renderer, the palette, the map

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **Rendering.** Raycaster over the cell map with a sliding secret box, floor/ceiling casting, a
  lightmap (`LMR` texels per cell; static lights cached, dynamic lights per frame; `torch_flicker`, phase
  below -100 means steady), fog, sprites with sway, the hand, HUD (Wolfenstein status bar for a player,
  a small readout for the autopilot). The bar slides away 5 s after anything it should show (`G.bar`,
  `G.bar_t`, `show_bar`, `BAR_T`): a hurt, a spell chosen, a tap of H, Tab or B bring it back, and it
  stays up at the merchant's and when dead. While it is away, a person gets the autopilot's readout
  (`pop_readout`) for souls gained or spent, treasure and keys. H held during the screensaver sends the
  scene `CTRL_PEEK` (from `app.c`), which raises the readout above the HOLD H plate (`G.peek`, `G.lift`).

- **Art** (`crawl_art.c`). The 256-colour palette is fully allocated in 16-entry ramps (`R_STONE` ...
  `R_GLOW`, `C_WHITE`, 13 gold, `C_CLEAR`); `R_FLAME` and `R_GLOW` are full-bright.

- **The map** (1.0.0b9, docs/PLAN-b9.md section 1; at a walk since 1.0.0b11, docs/PLAN-b11.md section 1; `RUN.has_map`,
  kept through death and her claim; `G.mapped` per level and per pocket: 1 its shape known, 2 been near it, 4 seen, 8
  walked; `map_note` marks a ring of `MAP_REACH` (1.5 cells) through walls, the cells the body passes through, and what
  64 rays across the view cross out to `MAP_SIGHT` (16), stopped by walls, shut doors and shut secrets). Not a menu:
  `G.map_on` (`map_up`, `map_keys`, `map_follow`, `map_shown`), the crypt runs under it and you walk, turn and cast as
  in play; the arrow keys pan it (a cell a tap, smoothly held; the pad's D-pad a cell a press), it drifts back to you
  three seconds after (`G.map_pan_t`) or at once on Space; M, Esc or Tab close it (the pad's B, which then doesn't
  dodge); any other screen closes it (`open_ui`), and a new level. A parchment (`g_scroll`, made once a level by
  `map_scroll_make`: torn in from top and bottom and burnt through near the middle, about a quarter gone, stains)
  between two rollers across the middle 240 pixels, drawn over the crypt (no dimming); ink goes through `ink_px`:
  on paper in brown, over a hole into `map_ghost`, drawn last by `map_ghostly` in `R_GHOST` with a violet edge,
  drifting a pixel now and then. Five pixels a cell (`MAP_PX`), north up, walls wherever open meets solid (`map_open`;
  a shut secret's room, wall and doorway count as stone, `map_hidden`), walked cells washed darker and hatched, doors as
  bars (a vault's in its key's colour once near), the exit, opened secrets' doorways and a pocket's (`map_face`), keys
  seen, the altar, the visportal or cooler, the caster's red arrow. Across its foot the level's numbers (`map_numbers`:
  `map_creatures`, `map_gold` rounded with the Treasure bonus, secrets found, `map_key_mark` solid if held). The status
  bar comes up over its foot as it does without it; the target's readout waits. The merchant sells it for `MAP_PRICE`
  (1,000) as a floating `SP_MAPWARE` at `MAP_X/Y`, her left (`G.map_look`, a card and a tag; not one of the visit's
  wares; the autopilot never buys it). Debug: `EVENT map_bought`.
