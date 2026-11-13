# The caster: gear, spells, souls and what is earned

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **Items** (`crawl_gear.c`, docs/PLAN-items.md). A `Piece` has a kind (`IK_*`: bracer, boots,
  amulet, hood, robes, ring, bag), a quality (`Q_*`, white to gold: `QUAL_RAMP`), its level, Anointed or
  not, an ability (`AB_*`) and up to six rolled stats (`ST_*`, in tenths; health restored in
  hundredths). `piece_roll` follows the plan's tables (`KIND_STATS`, `QUAL_STATS`, `ABIL_CHANCE`,
  `stat_base`); no piece rolls past its stat's cap (`STAT_CAP`). `gear_total` sums what is worn, lets
  Swiftness and Arcane Knowledge raise the summed stat once, then caps it. `piece_name` gives the two
  name lines; `piece_worth` and `gear_value` are the autopilot's sense of what helps (it counts only stats
  and abilities the game uses so far, `FELT`, and never buys a bag); `gear_take` wears
  a piece if it is better and bags or loses the old one (`take_piece` says which in a caption).
  In play: Armor, Dodge, Health, Restores; the resists (poison and slowing last shorter and bite less,
  `slow_factor`; burning hurts less); Movement (and Swiftness); Quiet (`footstep`: each step alerts
  creatures three hunt-field steps off, less with Quiet); Spell Haste; Damage; the elements
  (`hit_mult`: each adds its share of every spell's damage, by the creature's weakness to Fire Bolt,
  Frost Shard or Chain Lightning); Spell Effect (`spell_effect`: Soul Drain's healing, Frost's slow,
  Hellfire's burning floor and every rune's strength); Thorns; Spirit Cost (below). The abilities
  (`update_abilities` and where each acts): Swiftness and Arcane Knowledge raise their stat's total;
  Treasure Hunter adds to treasure picked up and whispers from an unopened secret wall within three
  cells; Keymaker makes keys within its reach sparkle through walls (`DR_GLINT`), and puts key
  wisps in the L menu; Commune shows creatures within its reach as red shapes through the
  walls every 15 s (`DR_COMMUNE`); Thorns hurts what strikes you; Mirrored may send a missile back,
  homing on its thrower round corners (`Bone.from`, `reflect_field`). The hand shows the robe on the sleeve, the right bracer
  as an iron cuff inlaid in its colour, and the ring's stone; the hood frames the portrait. Tab's
  second page lists every total, gold for what an ability adds, blue at a cap (`gear_capped`).

- **Spirit and bones.** Spirit is the souls (`RUN.souls` and `RUN.soul_frac`, `spirit()`), spent to
  the tenth by `spend_spirit` and shown with one decimal (`spirit_text`). In `cast_spell`: tier 1 and
  below (and the starting fireball) is free; above, `spell_cost` (damage over 150, 0.1 to 1, less Spirit
  Cost; Seeker Wisps 1 a wisp); with too little, the cast goes at tier 1 (`Shot.tier` carries it to the
  hit) and a caption says so once a level (`G.spirit_said`). Soul Drain's healing costs `heal_cost` a
  point (half a soul at tier 1, a quarter at 11) or doesn't happen. The BFS9000 keeps its price in
  souls. Every spear takes a bone (`Gear.bones`, up to `BONES_MAX` 30; the spear brings `SPEAR_BONES`
  6 when bought or improved); skeletons, shield-bearers, knights and hounds drop them (`IT_BONE`, about
  one in eight, more by rank). The status bar shows spirit, and the bones over the spear. The autopilot
  weighs each spell at the strength it can pay for, grows thrifty under 15 spirit and skips spears
  without bones (`choose_spell`). The debug build logs `EVENT weak_<spell>` for each weak cast, `EVENT commune` and
  `EVENT mirrored`.

- **Runes** (`Gear.def[NDEF]`: 0 none, 1 Common to 5 Exquisite, `RUNE_EXQ`; docs/PLAN-b6.md). One of each
  at most, the best, all worn; `def_trigger` picks the moment and `cast_defense` works at `rune_tier`
  (2, 5, 8, 12, 18: about the tier each quality used to be bought at), with `def_cooldown` by quality
  (16 s at Exquisite, otherwise as the tier had it). An Exquisite rune does a second thing: the Bone
  Ward bursts into shards when it breaks (`G.ward_shards`, `ward_burst`; six shots of tier -1 that only
  show); Blink leaves a double for `DECOY_T` (`G.decoy_*`, `SP_DOUBLE`) that creatures within six cells
  go for and don't fight from (`lured`, `fights` in `update_mob`); the Mirror Veil sends missiles home
  to their thrower at twice the veil's strength; Mend past full becomes a small ward; Purify mends 2%
  a second; the frozen take double; Dread reaches the living (`Mob.dreaded`); what Time Slip slows
  takes a quarter more; Soul Reaping heals a twentieth of your health a kill. The caster's page shows
  each rune in its quality's colour with a tag (C, U, R, UR, EX). The debug build logs `EVENT
  ward_burst`, `decoy`, and `exq_*` for the rest.

- **The Alignment** (1.0.0b11, docs/PLAN-b11.md sections 7-10; being built). `ALIGN[NELEM]` (each line's nodes, `ND_*`:
  the opening, Efficiency, Insight, the ability, path A's three, path B's three; costs and values), `RUN.al[e][n]` the
  points in each node, `RUN.al_points`; `align_open`, `align_full`, `align_has`, `align_rank`, `align_sum`, `align_add`
  (a Mastery's share of its element added to other spells); `align_sync` hands the lines to `crawl_gear.c`
  (`spell_set_line`, `spell_set_atone`). Bought at the Lampstand (`LAMP_X/Y`, the room north of hers; `UI_ALIGN`), shown
  on Tab's fourth page, saved as `LG_ALIGN`. A spell is its line's (`LINE_ATK`, keys `KEY_SPELL`): strength by the Level
  of Atonement and rank (`spell_damage_at`), spirit by rank (`spell_cost`; the spear none), a wind-up (`spell_windup`)
  a blow may break (`EVENT windup_broken`). Charging (`G.chg_on/chg_sp/chg_t` in `human_cast`; `spell_charges`,
  `spell_tethers`, `cast_charged`; `render_charge` left of the palm and a light in the element's colour) and the
  ultimates: Firewall (`G.fwall_t`, `FWALL_R`, `SP_FSPIRIT_*`), Flamewalker (`G.fwalk_t/fwalk_cd`, armour in
  `caster_sync`), Winter's Lance (`SHOT_LANCE`; `rime`, a `Venom` with `rime` set, `SP_RIME`), Shatter (in
  `damage_mob`), Arc (`chain_lightning`), Amperage (`SHOT_ORB`, `daze`, `SP_ORB_*`), Plague Pit (a flask whose
  `target` is 1000 + the charge), Contagion (`kill_mob`), Soul Tether (`G.tether`, 1 + the creature, `tether_hold`),
  Wisp Host (in `cast_spell`'s drain), the Phalanx and the Ballista (`SHOT_BALLISTA`, `SP_BALLISTA`). `make sheets`
  draws their sprites on `spells.png`. Debug: `EVENT ultimate`, `orb_hits`, `shatters`, `contagions`, `flamewalks`,
  `wisp_host`, `tether_ticks`, `plague_pit`.

- **Gear's elements** (1.0.0b11 step 11, section 5). `ST_RES_SPIRIT`, `ST_VESSEL`, and `ST_POW`/`ST_EFF`/`ST_INS` + element
  (`NSTAT` 38); `KIND_STATS`' `PICK_POW/EFF/INS` choose the element as rolled (`lean_elem`: a third by `gear_set_lean`, the
  lines' ranks, set in `align_sync`). Felt in `spell_damage_at`, `spell_efficiency`, `spell_insight` (`crawl_gear.c`),
  `bone_spared`, `hurt_ex` and `windup_blow` (Resist Spirit). `piece_worth` weighs them by the element's share. Tab's
  fifth page (`NINV`, `render_elements`); the Codex's words for them in `stat_does`; Resist Spirit's caster line
  `LN_SPIRIT` (`NLINE_B9` the old count).

- **Souls** (1.0.0b11 step 12, section 12). `vessel()`, `soul_cap`, `gain_souls` (capped), `soul_add` (whole souls in);
  `G.vslip_t`, `render_vslip`. Kills leave `SoulMote`s (`G.motes`, `spawn_mote`, `update_motes`, `MOTE_LIFE`, `MOTE_REACH`,
  `SP_MOTE`; `RANK_SOULS` halved); the dream's `mote_goal` in `pick_goal`. Her `MM_OFFER` (`OFFER_SOULS`, `offer_xp`). The
  Dead Bell and the Weeping Font (`BELL_X/Y`, `FONT_X/Y`, `SP_DBELL(_C)`, `SP_FONT(_C)`; `RUN.bell_cursed`, `font_cursed`
  kept through death, `bell_next`, `bell_secrets`, `font_next`; `UI_HALL2`: `hall2_rows`, `update_hall2`, `render_hall2`,
  `bell_toll`, `wfont_drink`). `start_level` turns them into `G.corrupt`, `G.cshown`, `G.fonted` for one attempt: more
  wanderers, `xp_gain` halved (a tenth more for the font), gold at pickup, `boss_adds`, the scrip, `fogged`'s green, the
  card and the tally; `render_wisps` and the map mark the shown secrets. Replays: the Portal's levels row (`PortRow` both
  -1, `G.port_lvl`, `replay_step`, `replay_begin`, `RUN.replay_from`, `G.replay`; `gain_coin` gives none). Saved in
  `LG_HALL2`. `SHOP_BEYOND()` is her room's way on, by the door's row. Debug: `EVENT bell_toll`, `bell_corrupted`,
  `font_cursed`, `font_drink`, `offer_souls`, `corrupted`, `replay`, `souls_slipped`.

- **The classes** (1.0.0b11 step 10, docs/PLAN-b11.md section 11; `CLASSES`, `CLASS_SHORT`, `CLASS_MARK` in
  `crawl_art.c`, `RUN.cls` 1 + its element, `cls_el()`). Its line is open free (`align_open`); the gifts sit where their
  numbers are used: `align_sync` (a fifth of Efficiency, the Witchfinder's Power, the Galvanist's Insight), `permafrost`,
  `mob_poison`, `bones_max`, `bone_spared`, `caster_sync` (the Headsman's armour). The Witchfinder's three points for
  Fire are `RUN.cls_pts` (`align_funds`, spent first; `RUN.cls_spent` so unbinding doesn't pay them back as points). A
  class given up keeps its line with a gift opening (`RUN.al_gift`). `class_take`; her question `UI_CLASS`
  (`update_class`, `render_class`, `VOW_COINS`, `VOW_ATONE`), opened by `shop_human` while `RUN.cls` is 0; her menu's
  `MM_VOW`. The dream takes one at random in `begin_shop`. Saved at the end of `LG_ALIGN`; a stone's class and paths in
  its spell bytes (`Stone.lined`). Debug: `EVENT class`.

- **Experience** (section 3; `xp_gain`, `xp_kill`, `xp_rooms`, `KIND_XP`, `RANK_XP`, `BOSS_XP`, `xp_level`).
  `RUN.xp` is all earned since the last death; `atone_need(n)` is `ATONE_K` (42) x (n-1)^2 to 50, then 4,074 a level;
  `atone_reach` the level it would buy. Kills by kind, level and rank (a pocket's by `Mob.rank0`, the rank it
  had before the pocket raised it, x1.25 on the first link, +0.1 a link, 1.6 at most; a boss's adds half); a
  secret 25, a chain's end 60 a link, a boss 40 x `BOSS_XP`, a level cleared 30, a room first entered 3 (all
  x `xp_level`). Every gain a line at the lower left (lower right in the dream; `XPF`, `render_xp`), the status
  bar's top edge fills towards the next level (gold when one waits), and the moment one is banked
  `SFX_ATONE` (a choir chord, bells, a whisper) and *LEVEL OF ATONEMENT n* across the view. The tally has an
  EXPERIENCE row (`G.level_xp`, carried into pockets). Debug: `EVENT xp`.

- **Levels of Atonement and points** (sections 4-5; `RUN.atone`, `RUN.points`, `RUN.steps[NLINE]`, `LINES`,
  `LN_*`, `caster_sync`, `ascend`, `line_buy`, `UI_ASCEND`). Her menu's *ASCEND* (`MM_ASCEND`) takes every level
  the experience reaches, ten points each to the fiftieth. Twelve lines: health +5 a point, damage, resist all
  (+5 each) or one (+15), spell haste and dodge 1% for 3, treasure and movement 1% for 2, luck 0.1% and
  attunement (Spirit Cost) 1% for 1. `caster_sync` (every frame) hands them to `gear_set_own`: the caster's own
  joins the worn total in `gear_total` after Swiftness and Arcane Knowledge, under the same cap (`ST_TREASURE`
  and `ST_LUCK` are new stats no piece rolls: Treasure is Treasure Hunter's and the caster's, cap 100; Luck 2%
  base, cap 25). A buy past a cap asks twice (`G.asc_arm`); steps bought while the screen is open may go back
  (`G.asc_buy`). The dream's caster spends by itself (`auto_ascend`, at its greeting; never a person's run,
  `LEDGER.yours`). A death that costs takes it all (`death_fall`); her claim takes none. Debug: `EVENT ascend`,
  `line_buy`.

- **Luck** (section 5; end of `kill_mob`). A kill rolls `gear_total(ST_LUCK)` for gold of its rank
  (`Item.value`, 12% of `gear_haul` x the rank's share) and half the time a bone, potion or urn; out of the
  level's treasure. Debug: `EVENT lucky`.

- **Cornerstones** (section 6; `RUN.corner` bits `CS_*`, `corner_due`, `UI_STONES` and Tab's third page:
  `update_stones_core`, `render_stones`, `corner_take`; the autopilot takes them in order). Second Wind
  (`hurt_ex`: the fatal blow leaves half health, `G.wind_t` 2 s untouchable, `render_wind`; `RUN.wind_spent`,
  back at each run of ten via `RUN.wind_run` in `start_level`, or for a coin at `MM_WIND`, `wind_offer`).
  Premonition (`Hazard.pre`, a second before each hazard but the arc, in `hz_new`; `hz_pre_covers` for
  `hz_dmap`; `omen_marks`: embers' landings, a drowned one under a quarter, the Litany's sight; drawn as
  `FM_OMEN*`, a dashed purple edge, in `fm_pixel`). Cartographer (`map_carto` at each level and pocket; a shut
  secret's wall dotted once passed). Lantern of the Dead (a flat dynamic light round the caster; the ordinary
  Wraith won't step within 2.1 cells). Grave Sense (creatures within ten cells on the map).

- **Overlevels** (section 7; `RUN.over[NLINE]`, `over_due`: one at 55, 60 ...). Space (Y) on a line of the
  caster's screen, asked twice: +5 past the cap and the cap with it (Luck 1), not health or resist all. An
  overlevelled number is drawn by `eerie_text` with `eerie_eye` beside it (`stat_over`). The Codex has THE
  CASTER'S OWN part (`CX_CASTER`). Debug: `EVENT overlevel`.

- **One line to begin with** (1.0.0b16, docs/PLAN-play.md section 3a; step 1). No line is open to everyone: `align_open` is
  the class's line, a line lent (`LEDGER.lent`, `line_lend`: the dream on a kept page with no class, or a run met in the crypt
  without one; never saved) or an opening paid; Fire's opening costs 3 like the rest. The class's first rank costs nothing
  (`spell_set_own`, in `spell_cost`). Physical is shut till the Lampstand's first gift (`RUN.gifts`, `phys_locked`,
  `class_offered`, `class_random`). Whatever reached for the fireball takes `own_spell()` (or `own_missile()`, something that
  flies with no target). Splinters (`SHOT_SPLINTER`, `SPLINTER_*`, `splint_left`, `spear_bare`, `spear_ready`, `cast_cycle`,
  `splint_tick`, `RUN.splint_used/splint_t`, not saved): the spear's line with no bones throws them, eight at most, one back
  every six seconds, at 1.6 times its interval and two fifths of its blow, through nothing; the status bar shows them under a
  sliver. Any spell drives the ordinary Wraith back 3 s (`wraith_back_for`), fire, spirit and wisps 5 s. An old page's Fire
  bought on the free opening stays open as a gift (in `LG_ALIGN`'s reading). Debug: `EVENT cast_SPLINTER`, `line_lent`.

- **The Lampstand's gifts** (1.0.0b16 step 2, docs/PLAN-play.md section 3). `gifts_owed` (one from level 5, two from 13, by
  the level her room leads to), `gift_due`, `gift_take`, `gift_settle` (in `begin_shop`: a gift with every line open comes as
  points; the dream takes its own), `RUN.gifts` and `RUN.gifted` (a byte each at the end of `LG_ALIGN`); a person's way on
  holds at her door till it is taken (`shop_human`), the Lampstand breathes (`build_lightmap`, `G.gift_glow`), and its screen
  takes a closed line's opening free on two Enters (`AL_GIFT`; the pane's cost reads *A GIFT*). Debug: `EVENT gift`,
  `CRAWL_NOGIFT=1`.

- **The record and achievements** (`Record` in `RUN.rec`, saved with the run, so each save and grave
  has its own). `rec_kill`, `rec_seen`, `rec_secret` and `rec_cleared` keep it, the autopilot's
  doings included. Achievements are bits (`ach_bit` layout: kind x rank, then secrets, levels,
  descents): 20 kills of a kind's lowest rank, then 25, 20, 15, 10; gilded 5 (`slayer_need`; 60, 40, 25, 15, 8 and 3 till
  the portraits' step 1, docs/PLAN-portraits.md; `slayer_sweep` earns what a record read already reaches); the portraits' own
  from bit 345 (`ACH_POR0`: the Spectre's and the Wraith's three each; `ACH_FEAT0`: a feat a kind, `FEATS`);
  secrets 1-50; each of levels 1-40 (`ach_level`: 21-40 from bit 256); each descent after the first
  (level 80, 120 ...). `toast` queues the
  pop-up at the upper right (`render_toasts`), with a progress note halfway.
