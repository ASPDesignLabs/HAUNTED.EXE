# Creatures: the kinds, their ranks and their ways

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **Enemies** (`MOB_SKEL`, `MOB_GHOST` (the spectre: harmless, through walls, gone after level 10),
  `MOB_BAT`, `MOB_WSKULL` from level 3, `MOB_SHIELD` from 4, `MOB_SPIDER` from 6; guardians in vaults).
  `KINDS[]` gives each its name, rank family, first level and weaknesses (damage per spell, which
  `choose_spell` plays to). On its first level a kind comes alone (`may_spawn`).
  - **Ranks.** Every creature takes a rank on spawning (`pick_rank`, by the set's mix; `set_rank`):
    HP, damage, pace, souls and size by `RANK_*`, and its colour through `crawl_rank_lut` when drawn.
    From level 7, one level in three has a gilded creature that runs when hurt and drops treasure.
    Ranks II and up sometimes drop coin, gems or a potion (not counted in the level's treasure).
  - **Captions.** `name_newcomers` names each kind and rank the first time it is seen on a level.
  - **Strength.** HP is the kind's base x 1.04 per level x the rank's; `mob_damage()` is
    (4 + 0.8 x level) x the rank's, less 15% per death on the level (`G.retries`, up to 45%, reset
    when cleared). `throw_interval()` 5% quicker per level. Bones thrown from beyond 4 cells spread
    wider (wide halls made long throws too deadly).
  - **Hunting.** `build_hunt` keeps a field of steps to the caster. A creature that sees the caster,
    is hit, hears a spell within 8 steps or a footstep within 3 (`alert_near`, `footstep`; Quiet boots
    shorten the footstep's reach) is alerted (`Mob.alert`) and follows the
    field round corners; the dead pull ordinary doors open. Only the nearest three hunt at once
    (`pick_hunters`). A thrower that finds you again waits a moment before it throws.
  - **Residents.** Ossuaries and crypts have residents (`Mob.home`) that keep to their room until
    alerted. In player mode the "something wakes ahead" respawn waits 35 s of quiet, not 11.
  - **Kinds' properties** (`KF_*` in `KINDS`, tested with `KIND_IS`): undead, solid, spirit (nothing
    hurts it), unique, opens doors, melee, uncounted. A newcomer's first level always brings one.
  - **The newcomers so far.**
    - The shambler (from 2) and the Drowned (from 7) wait under turned earth before a grave, or a
      stirring puddle (`place_risers`, `update_riser`), and rise when you come near. They grab, which
      slows you; the Drowned burst into a foul pool (`Pool`).
    - Bone hounds (from 8) come in pairs (`spawn_wanderer`), bite and dart back.
    - The Wraith (from 9; `update_wraith`) comes once a level after 240 s (`CRAWL_WRAITH=s` to
      test), from the far end, down the hunt field, opening doors, while torches gutter near it.
      It can't be killed: fire, a wisp's light close by and Dread drive it back for 5 s
      (`wraith_back`). The autopilot turns fire on it. Driven back into a corner it can't leave (nowhere further from
      you, or a door it can't open), it goes into the stone after a second and comes again from the far end
      (`wraith_cornered`, `Mob.corner_t`; `EVENT wraith_cornered`).
    - The hunting spider (from 11) waits upside down on the ceiling (`DR_FLIPV`), drops when you
      come close, and leaps (`start_leap`, `update_leap`); one leap in three poisons.
    - The ghoul (from 12) leaps too, and feeds on bones when it has lost you (`sub` 4: it mends, and
      takes double damage).
    - The skeleton knight (from 13, with two hounds after its first level): a 0.9 s wind-up and a
      heavy blow; `knight_stage` (by health) picks its plate, its pace, and whether fire bites.
    - The brood mother (from 14) lays egg sacs (`MOB_SAC`, `update_sac`) that split into spiderlings
      (`MOB_LING`); the sacs die with the last mother.
    - The shade (from 16) shows only in light (`shade_alpha`, `shade_hidden`: torches, fires, the
      wisp's pool, a spell going by, and for a moment after its touch); its touch takes souls, or
      more health when there are none. The autopilot can't target it in the dark.
    - Ember imps (from 17) start beside magma (`place_imps`), keep their distance and lob embers
      (`MIS_EMBER`) that burn the floor where they land (`Burn.foe`).
    - The deacon (from 18) walks with two or three skeletons, keeps back, and chants over remains
      or bone heaps (`DEACON_CHANT`) until a skeleton gets up from them, four at most. The autopilot
      aims at it first.
    - Gargoyles (from 19) are statues on plinths (`place_gargoyles`, `garg_wake`) until you pass
      within three cells or hit one; then they fly over the floor's things (`blocks_flight`) and swoop.

- **Ranks and the bestiary art.** `crawl_rank_lut[family][rank]` recolours a creature's body ramp
  (`PF_*`, `RK_*` in `crawl.h`). The new creatures' and decorations' sprites are drawn in
  `crawl_art.c`; `make sheets` shows them. See `docs/PLAN-bestiary.md` for what is in play.

- **Warded creatures** (1.0.0b16 step 7, docs/PLAN-play.md section 6). `Mob.ward[2]`, `wdmg`, `wring`, `wset`; `Pack.ward`;
  `ward_give` (the first time it moves), `ward_pick`, `ward_str` (in `weak_el`: 60% on level 5 to 90% by 30), `ward_struck`
  (from `damage_mob`: the dull ring, the caption, the crack at a fifth of its health), `Draw.ward` (the rim), `render_warded` (the
  barred glyph over its head), the readout's barred glyph, the bestiary's *SOME COME WARDED*. Debug: `CRAWL_WARD=0/1/2`, `EVENT
  warded`, `ward_cracked`, `wardhits=`/`cracked=` on the state line.

- **The dead's ways shown** (1.0.0b18, PLAN-ai step 11, Part H; harness only; released). `WY_*` (seventeen set pieces), `way_met` (named
  once a run of ten, `RUN.ways`/`way_said`/`way_run`), `pack_way`; the Codex's `CX_WAYS`; the record's deeds (`deed_note`,
  `DEED_*`, `caller_hushed`, `ways_ends`, `Pack.hunted/hurts0/done`, `Ambush.deaths0/done`, `G.hurts`, `G.hunt_shaken`) and
  achievements from `ACH_WAY0`; `LG_WAYS`; the bestiary's ways on Space (`G.best_ways`); sounds `SFX_CHEW`, `SFX_YELP`,
  `SFX_THRUM`, `SFX_RATTLE`, `SFX_SNUFF`. Debug: `CRAWL_WAYS=1`, `EVENT way`, `EVENT deed`.
  The art (Part H's): backs for every kind that turns (`*_BKA/_BKB` sprites; `mob_side` past 135 degrees, `is_back` without
  eyes; `g_back` while drawing, `cv_mirror` or `spr_turn`; `make sheets`' `backs`), the roost asleep (`SP_BAT_ROOST`, `_ROOST2`),
  the ambushes' signs by run (`SP_SIGN_*`, eyes in the dark by a snuffed torch, `Ambush.eyes`); `make sheets`' `minds`. Debug:
  `CRAWL_TPAMBUSH=1`, `CRAWL_STAGESLEEP=1`.

- **Soul Sap** (1.0.0b11 step 13, section 6). Bats of rank II up latch on instead of biting (`latch_begin` from the bat's
  touching branch, `LATCH_T`); `G.latch` (1 + the bat), `update_latch` (the drain as a seeping Spirit harm, souls, the
  shaking by the view's turns and the feet's strafes, `SHAKE_NEED`, `G.shake`), `latch_release` (shaken, thrown by Blink
  or Swift Dodge, or after ten seconds), `sap_slow` in `slow_factor`, `render_latch`. Untargetable while latched. Debug:
  `CRAWL_LATCH=1`, `EVENT latch`, `latch_off`, `shakes`.
