# Hazards, puzzles, trials and things that want a spell

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **Hazards and omens** (docs/PLAN-b7.md, section 6; `Hazard` in `G.haz`, `hz_new`, `update_hazards`,
  `boss_hazards`, `update_omens`). Each warns (`warn`, never under 0.9 s, `hz_warn`), harms (`on`) and fades
  (`off`), and hurts only the dweller, by a share of its health (`share`). `HZ_ROCK` (grit and a hardening
  shadow, then a rock: `HURT_STONE`, no dodging; rubble after), `HZ_FIRE` (Hellrent: cracks along `hz_line`
  in order, glowing up to the burst, fire a second, embers), `HZ_BILE` (a sheen creeping cell by cell from an
  outlet, foul once settled: poison and a fifth slower; drains when shut; `hz_bile_wet`), `HZ_VOID` (a ring and
  a thin column, then a purple volume: `HURT_VOID`, 60% pace, `G.silenced`: no spells or runes, the hand
  drops and greys, `snd_set_muffle`), `HZ_ARC` (the knight's Sundering Arc) and `HZ_GRASP` (the Wraith's
  Grasping Dark). The floor's marks are made each frame (`hz_marks`, `FMark`, `fm_pixel` in `render_floor`);
  what stands up is drawn with the sprites (`SP_GRIT0/1`, `SP_ROCK`, `SP_RUBBLE`, `SP_ERUPT0/1`, `SP_VOIDCOL`,
  `SP_HANDS`). `hz_dmap` (`hz_danger`) tells the autopilot which cells warn or harm: `decide` keeps out of them
  and it stops aiming to step out of one. A boss's own (`boss_hazards`): the knight's rockfall and arc, the
  spider's outlets (`G.bout_*`, from the map's `v` and the well, two open, one changing every 20 s, one more
  in a wave), the Deacon's lines, rings and crosses, the Wraith's voids and hands; each a fifth more often a
  challenge step. Omens (`omen_every`): the run's hazard near you, ahead if it can be, at half the harm, on
  its last four levels (every 90, 60, 40 and 30 s), never in a pocket or the arena; the Catacombs' bile
  comes from a wall turned to a grate (`T_GRATE`) for the while. The first of each kind is named
  (`RUN.omens`, saved).

- **1.0.0b13 so far** (docs/PLAN-b13.md, steps 1-7). *Hazards*: each run's own, fixed (`HzSite` in `G.site`, `place_sites`,
  `update_sites`) and arising (`update_arising`), older runs' now and then, by element (`hz_seep`, `G.hz_el_acc`), the
  sappers (`hz_sap`, `HZ_SAPPER`) from each run's sixth level; `hz_dmap`'s `HZ_SOFT` marks what is always there, which the
  dream walks round only when another way is as short (`decide`). Resist Lightning is `ST_RES_LIGHT`. *Things*: `Thing`
  in `G.thing` (`TR_*` roles, `thing_new`, `thing_use`, `thing_facing`, carrying in `G.carry`, `render_carry`); the dream
  uses them from `use_spot`. *Puzzles*: even levels from 6 but a boss's (`pz_level`, `pz_kind_of`, `PZ_RUN`); `pz_make`
  lays the kind's things near the sealed wing's door (`Door.locked` `WINGED`, `G.pz_room`, `in_wing`), the exit inside one
  puzzle level in three (`G.pz_exit`); `pz_touched` answers, `pz_next` and `pz_goal` are the dream's way. *The hidden
  message* (`pz_hide`): `G.msg_deco` among things of the run's kind (`HIDES`, `SP_HIDE0`), searched with E in a second
  (`hide_facing`, `search_begin`, `update_search`), its glint (`msg_glint`, `Draw.mg`); Tab's sixth page *DOCUMENTS*
  (`DOCS`, `doc_add`, `render_documents`, the pages from `crawl_doc_page` and `crawl_doc_ink`); the Black Shrine
  (`G.bsh_*`, `SP_BSHRINE`, `UI_BSHRINE`, `bsh_price`, its showing `msg_shown` through the walls and on the map). Debug:
  `CRAWL_HZ=k`, `CRAWL_TPHZ=1`, `CRAWL_THINGS=1`, `CRAWL_PUZZLE=k`, `CRAWL_TPPUZZLE=1`, `CRAWL_WINGEXIT=1`, `CRAWL_TPMSG=1`,
  `CRAWL_TPBSHRINE=1`, `CRAWL_BSHOW=1` (the showing from the start); `SITES`, `PUZZLE` and `MESSAGE` lines, `EVENT
  pz_solved`, `thing`, `msg_found`, `bshrine_found`, `bshrine_show`. `make sheets` draws `hazards.png`, `puzzles.png`
  and `documents.png`. *Trials* (step 5): `G.tri` (`TRI_*`, `TRI_RUN`, `tri_kind_of`) in a one-door room chosen as the wing is
  (`mz_vault`, `mz_max`; `G.tri_room`, `G.tri_door`, drawn `T_TRDOOR`, `SP_TRLIGHT0` by it, `in_trial`), placed by `tri_place`;
  `update_trial`: `TRI_IN` (1.5 cells) past the door's middle with the body clear of its cell (1.0.0b18.1; one cell in, it
  had shut on a person half in the doorway), `tri_begin` (`Door.locked` `TRIALED`, `tri_turn`'s faces, `tri_floor`'s water, `fog_tint`,
  `snd_set_trial`, the kind's creatures `tri_spawn`/`tri_wave` with `Mob.tri`, things `tri_thing` with `Thing.tri`),
  `tri_run`, `tri_end` (`G.tri_boon`: the boon in `xp_kill`, the curse in `xp_gain`, the tally's note). Slow things:
  `Thing.slow` in tenths of a second, `G.work_i`, `update_work`. The hourglass: `crawl_hourglass`, `render_trial`,
  `trial_shown`. The dream: `tri_goal` (after `boss_goal`), `tri_next` (sticky on its choice, `G.tri_a[12]`), `G.tri_hold`.
  Debug: `CRAWL_TRIAL=k`, `CRAWL_TPTRIAL=1` (before its door; 2 the dream started there), `TRIAL` lines, `EVENT trial_begin`,
  `trial_won`, `trial_lost`, `trial_died`, `TRIAL end` (what was left); `make sheets` draws `trials.png`. *The reckoning*
  (step 6): `human_leave` in place of `human_exit` for a person at the exit; `reckon_lines` (creatures, secrets, the wing,
  `map_gold`, the trial; on a boss's level only outside the arena), `UI_RECKON`, `update_reckon` (NOT YET steps back a
  pace; `G.rk_fresh` spends the press that opened it), `render_reckon` on `crawl_doc_page`'s slip (run + 4). Not in a
  challenge, a replay or a pocket. Debug: `EVENT reckon`, `reckon_stay`, `reckon_descend`. *The record* (step 7):
  `RUN.deed_pz`, `deed_tri`, `deed_lost`, `pz_kinds`, `tri_kinds` (kept through death; `LG_DEEDS` grew to 18 bytes after
  the hazards' four, an older one read as none), `rec_puzzle`, the trial's in `tri_end`; achievements from `ACH_DEED0`
  (324: puzzles at 1, 5, 20, trials at 1, 10, 30, each run's trials all won) in `ach_lines`; the Codex's `CX_PUZZLE` and
  `CX_TRIAL` parts (`PZ_DOES`, `tri_run_of`).

- **Things that want a spell** (1.0.0b16 steps 4-6, docs/PLAN-play.md section 4; every run, section 4.3's from a run's fifth level). `crawl_mark` (`crawl_art.c`:
  the seal, its motes, pips; `MK_WAIT/HIT/WRONG/DONE`), the sprites `SP_MK_*` (each waiting and done; `make sheets`' `marked.png`,
  approved). In play: `Marked` in `G.mk`, the kinds `MKK`, `place_marked` (the way from the start to the exit, `MK_STRETCH`,
  `mk_spot`, `mk_pick`), `mk_shot`, `mk_burst`, `mk_aimed` (lightning and Soul Drain with no creature), `mk_strike`,
  `mk_finish`, `mk_reward`, `mk_tick`, `render_marks`, the lights in `build_lightmap`; the dream's in `dream_looks` and
  `PS_PROBE` (`G.probe_mk`). Step 6 (`marked2.png`, approved): `MkKind.tier/runs/own`, `mk_can`, `mk_place`; moving things
  (the corpse-light, the rolling skull, run III's cart: `Marked.x/y` move and `mk_deco` moves the decoration), the
  heat-and-shatter seal (`st` hot, its `el` turning to frost), three bells (`grp`, `BELL_RING`; `Shot.mkhit` lets the spear
  pierce them, a ringing one lets other shots under), the gates (`GATED`, a door's lock: `mk_gated` finds a short cut by
  `mk_flood`; the winch's `mk_gate_up` for `GATE_UP`, the hearth's for good), the steam vent (`VENT_*`, `mk_dmarks` for the
  dream), run II's web, run III's lodestone, run IV's prism and geode. Debug: `MARKED level n`, `EVENT mk_dream`, `mk_hit`,
  `mk_wrong`, `mk_done`, `mk_heat`, `mk_gate up`, `mk_lost`, `CRAWL_TPMARK=k` (and its line readied), `CRAWL_MARKK=k`.
  Step 8 (`marked3.png`, gothic, approved): the sequences (tier 3, from 13: `mk_tablet` with `MKK_PLINTH`s and `Marked.els`, the
  reliquary's turning, the ward-stone through `mk_gated`, `mk_relay` and `relay_check`), `mk_wrong`; the fight things (tier 4,
  `place_fights`: `mk_fight` for the oil, the column, the font, the drum, the ice; `mk_water_shock`, `mk_wet_hit`; firedamp on
  `G.site`); the flask answers what wants poison as it passes. Debug: `CRAWL_FIGHTK=k`, `FIGHTS level n`, `EVENT mk_crush`,
  `mk_drum`, `mk_water_shock`.

- **The lull's watch** (1.0.0b17, PLAN-play step 9, section 5; released). `lull_tick` (a person's quiet walking, `G.lull_t`, reset by a
  fight, a find, a secret, a strike or a creature seen; `LULL_T` 30 s, `LULL_CD` 60 s), `lull_spot`, `lull_offer` (a corpse-light out
  of sight, a pricket or coil lit a moment in sight, `Marked.st` 3, or a handbell and a wanderer tracking you). Never the dream's.
  Debug: `CRAWL_LULL=s`, `CRAWL_LULLK=k`, `CRAWL_LULLTRACE=1`, `EVENT lull`.
