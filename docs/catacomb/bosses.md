# Bosses, the gifts they leave, and the Accursed Portal

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **Bosses** (docs/PLAN-b7.md; `BOSSES[]`, `BossDef`, `boss_of_level`; the fight in `update_boss`). The last
  level of each run (`crawl_boss`) holds its boss: the knight (10), the spider (20), the Deacon (30), the
  Wraith (40), then round again. `gen_level` keeps plan rows from `ARENA_PLAN` (cells from `BAND_Y0`, 29)
  out of the level (`K_PILLAR` on the plan), keeps a doorway at plan column `ARENA_DOOR_MX` open
  (`MCell.keep`), moves the start room as far from it as it can (`swap_rooms`), and stamps the boss's
  map last (`stamp_arena`, maps `MAP_LISTS`, `MAP_WEB`, `MAP_PULPIT`, `MAP_BOTTOM`: an antechamber, the arch,
  the arena; the legend is above `BOSSES`), so nothing of the level spills into it; `in_band` keeps the
  level's placements out. A boss is one of its run's creatures with `Mob.boss` (1 + `BOSS_*`): health in
  rank-I skeleton knights of the level (`BossDef.hp`), size, pace and blow. It waits untouchable
  (`BS_WAIT`) until you are a step into the arena; then the arch seals (`Door.locked` `SEALED`), its card
  shows (`G.bcard`), and its bar runs along the top (`render_bossbar`; toasts wait). At 75, 50, 25 and 10%
  (`WAVE_MARK`) `boss_hurt` stops its health at the mark and calls a wave (`BS_WAVE`): two seconds later
  `boss_adds` brings the wave's adds (`BossDef.waves`, `Mob.add`, at `add_rank`: the highest rank of each
  kind met since the last death, `RUN.top_rank`) out of its gates, burrows or heaps; when they are down it
  rises (`BS_RISE`) and fights on. Untouchable, it wears a pale shell (`DR_EDGE`) and spells ring dully on
  it (`G.bring`). Three fonts in the walls (`T_FONT`, `T_FONT_DRY`; `font_drink`, a third of your health)
  fill at the start and after the first and third waves. It falls (`boss_fall`): its scrip drops
  (`IT_SCRIP`, `SP_SCRIP`, to `RUN.scrip` at once), the exit (barred, `T_GATE`, while it lives) opens, the
  tally gains a BOSS row, the record (`Record.boss`) and achievements (`ACH_BOSS0`) note it, and on the way
  down `RUN.runs_done` counts the run. The autopilot (`boss_goal`) keeps three to six cells from it, away
  from its adds, drinks under 40% and takes the scrip; a boss level waits 1500 s, not 600, before it gives
  up on one. The bosses' own ways (runes, abilities) come in steps 5-8 of the plan.

- **The Ill-Fated Knight** (step 5). Sheds plate by waves, not health (`knight_stage`: full, then after
  wave 1 half, after wave 3 the breastplate), 15% quicker each time (`knight_pace`, which keeps the boss's
  pace). Drawn `DR_SHINE`: its plate's R_IRON raised, a glint sliding across, and while its Mirror Rune
  flashes, silver all over with a ring going out. Poses (`G.bpose`, `BP_PLANT` with `SP_KNIGHT_P0..2` for
  Rockfall, `BP_RAISE` then `BP_STRIKE` for the Sundering Arc) hold it still (`update_mob`'s boss hold).
  Swift Dodge (`knight_boss`, `knight_dash`, `dash_room`): 0.5 s untouchable (`G.bdash_t`; shots pass,
  `damage_mob` refuses), 2-5 cells, after-images from `G.bdash_x0/y0` for 0.8 s; to close in (5 s), to flee
  when `G.bhurt` (harm fading over 3 s) passes a tenth of its health or it stands in fire (9 s), aside from a
  shot (16 s). Mirror Rune (`knight_turns`): one missile in seven, not twice in 2 s, comes back as
  `MIS_TURNED` (`Bone.atk` the spell, drawn as it) at half damage, capped at a fifth of your health, with
  `SFX_MIRRORBELL`. The autopilot values missiles at 0.8 against it. Debug: `EVENT knight_dash close|flee|
  dodge`, `knight_turned`, `turned_hits`. A boss's hit radius grows with its scale.

- **The Wandering Spider** (step 6; `spider_boss`, `web_strike`, `egg_shot`). Sprites `SP_BSPIDER_A/B/LEAP`,
  `_REAR`, `_D` (her own since 1.0.0b14's art pass: `spr_bspider`, an old matriarch). Up to the ceiling every 4-7 s
  (`G.bsp_t`; `sub` 5 climbing, 0 overhead: out of reach, shots pass under, not `targetable`), across it to
  two cells short of you, down (`sub` 1) and a leap. Mend Rune: an eighth over 4 s (`G.bmend_t`) after a
  fifth lost since `G.bmend_base`, every 20 s at most, capped at the last wave mark, drawn `DR_MEND` (green).
  Tangled Web: rears 0.6 s (`G.web_rear`, `SP_BSPIDER_REAR`), spits `MIS_WEB`; held, `slow_factor` is 0 and
  `cast_spell` sends each spell's damage into `G.web_hp` (a third of your health, fire double) instead;
  `render_web` draws the strands; next 12 s after you are free; the autopilot casts a fire bolt at nothing
  every 1.2 s while held and idle (`G.web_idle`). Egg Shot every 14-18 s (`G.bz2_t`): a sac (`Mob.small`) by
  a wall in your sight that hatches four small hunters 5 s later. Every 15 s one small hunter comes out of a
  burrow (`Mob.add` 2, four at most; only `add` 1 counts for a wave). Debug: `EVENT spider_ups`,
  `spider_mends`, `webbed`, `web_torn`, `egg_shots`. The BFS9000 takes 8% of a boss's health.

- **The Black Deacon** (step 7). Drawn in his own sprites at twice a deacon's size (`SP_BDEACON_A/B/CHANT/D`, 1.0.0b14's
  art pass: `spr_bdeacon`, a profane bishop). Bone
  Ward (`G.bward`, 4% of his health, `G.bward_cd` the Rare rune's cooldown to grow back): blows go to it
  first; six bone shards wheel about him while it holds. Grave Call is the deacon's chant from the
  chapel's heaps (`SP_BONES2` in his arena): one at a time, 12 s apart, three of his (`Mob.add` 3) about
  him at most; a tenth of his health in damage mid-chant (`G.bchant_dmg`) breaks it off. Litany (`BP_LITANY`,
  every 18-24 s): two seconds of green light (`DR_MEND`, the chant sprite), then a fifth of your health if he
  can see you; timber props (`K_PILLAR`) hide you, pews don't. Five chandeliers light the chapel. Neither he
  nor any boss's adds back away (`keeps_off`), and shamblers called as adds rise at once. Debug: `EVENT
  ward_broken`, `ward_grown`, `chant_broken`, `litany_hits`, `litany_missed`; `BOSS fight ...` every 10 s of a
  fight (health, ward, distance, sight, whether it is targetable and aimed at) and `BOSS adds left ...` when a
  wave's adds linger past 20 s.

- **The Wraith Unbound** (step 8; `wraith_boss` in place of `update_wraith`, `wraith_lit`, `wraith_open`,
  `brazier_set`, `brazier_facing`). Its arena's eight braziers (`G.bbz_*`) are props lit by a dynamic light
  each frame, so they can go out (`SP_BRAZIER_OUT`): it gutters one it passes within a cell, one in 8 s at
  most (`G.bgutter_cd`), and a void over one puts it out. E facing one kindles it in a second (`G.bbz_at`,
  `G.bbz_t`; walking off cancels); a wisp within 1.5 cells kindles it at once; the autopilot kindles any
  within two cells, and while fewer than four burn `boss_goal` takes it to the nearest one out. It can be
  hurt only within four cells of a lit brazier or three of a wisp, and not while in the stone
  (`wraith_open`: `damage_mob` refuses, shots pass, not `targetable`); it is drawn faint in the dark. The
  ordinary Wraith's kind has no weaknesses, so `weak_to` gives the boss 1, fire 1.2. Time Slip (`G.bslip_*`,
  the Rare rune's cooldown): a missile about to strike it, in light, slows to a fifth within three cells for
  1.5 s (`update_shots`), a grey ring (`DR_RIPPLE`) and `SFX_DEEPCHIME`, and it steps aside. Wraithwalk
  (`G.bww` 1 fading, 2 gone 1.5 s, 3 out of a wall two to three and a half cells from you; every 16-20 s).
  Its touch: 1.2 blows, every 2.5 s, chill and slow. Nothing drives it back. Debug: `EVENT brazier_out`,
  `brazier_lit`, `wraith_slips`, `wraithwalks`, `wraith_touches`. `in_arena` now stops short of the map's
  rim (it had none, and a Wraithwalk could read and land past the edge; found by ASan).

- **The caster's boss abilities** (step 9; `RUN.abil[b]` 0-3 by boss, `RUN.abil_sel` the right mouse's,
  `BA_NAME`, `BA_WAIT`, `ab_wait`, `ab_readied`; `G.ab_cd`, `ab_update`). Unlocked and raised at the **Hexed Totem**
  (`SP_HEXTOTEM` at `TOTEM_X/Y` in the merchant's back room, x 4-8, through an arch at (9,13); `UI_TOTEM`:
  `totem_rows`, `update_totemmenu`, `render_totemmenu`; a rank costs its number in that boss's scrip, `totem_extra`
  sells for `SCRIP_PRICE` 20,000, asked twice; the scrip hang on its hooks, drawn in `render_sprites`). **Swift
  Dodge**: W/A/S/D tapped twice within 0.25 s (`G.tap_key/tap_t`), `swift_dodge`, `dash_reach` (stops before walls,
  doors, props, creatures; under 2 cells nothing), `G.dash_t/dash_vx/vy` 0.25 s, `hurt_ex` refuses while it lasts,
  missiles pass (`passing`); the autopilot's `auto_dodge` lands on a lane cell 2-5 off. **Tangled Web**: a
  `SHOT_WEB` glob, `web_spit`, `web_land`, `web_holds` (not spirits, shades, wall skulls, sacs, web spiders or
  bosses), `Mob.webbed/webmax` (a third of your health a rank), `web_claw` (its `mob_blow` at half its rate),
  drawn `DR_WEBBED`; the wait is 2 s after a miss. **Grave Call**: `note_remains` keeps the last twelve remains of
  what you slew (`G.remains`), `remains_near` (aimed, or fresh for the autopilot), `grave_call` raises it as
  `Mob.ally` for `ally_t` (12/16/20 s), `update_ally` strikes the nearest foe or follows the hunt field to you;
  foes turn on the nearest within six cells (`ally_near`, `ally_hurt`, `Mob.acool`); allies are skipped by every
  spell, rune and count (`damage_mob` refuses them), drawn `DR_ALLY` (a violet edge). **Wraithwalk**: `wraithwalk`,
  `G.wwalk_t`; creatures lose you (`sees` and `fights` false, `alert_near` deaf), missiles pass, you walk through
  them; the view greys (`tint`) and the hand thins. Right mouse `use_readied`, Q `ready_next`; silenced, none of
  them. The sockets: `render_sockets` (`BA_MARK` glyphs, a filling line, pips for webs and risen dead, the
  readied one outlined), `sockets_shown`, `SOCK`; the readout moves over beside them. The autopilot
  (`auto_abilities`): dodges a missile or a warning, webs a creature come to blows, raises fresh remains in a
  fight, wraithwalks when cornered under 35%. Debug: `EVENT dodge`, `web_spit`, `web_hold`, `web_miss`,
  `web_free`, `gravecall`, `ally_falls`, `wraithwalk`, `totem_raise`, `totem_sell`.

- **The Accursed Portal** (step 10; `PORTAL_X/Y` (16,27), `T_PORTAL0..3` turning in `render_walls`, in the far wall
  of the grave plot: x 11-21, y 20-26, through an arch at (19,18) past the shrine; the stones come in step 12).
  `UI_PORTAL` (`portal_rows`, `update_portalmenu`, `render_portalmenu`): the runs opened (`RUN.run_top`, raised in
  `boss_fall` on the way down: the boss's level over ten) and the challenges (bosses slain, at `RUN.bstep`,
  `step_text`). `portal_run(k)`: the merchant's room before level 1 + 10k, as you are. `challenge_begin(b)`: the
  boss's level this time down (`boss_level_of`), `G.bchallenge`, the caster in the antechamber, the way down into
  it fallen in (rubble), the autopilot set leaving; `RUN.chal_from` is the level whose room it was begun from: the
  tally, a save and a scene switch all go back there; a death begins it again; `begin_shop` clears it. Debug:
  `EVENT portal_run`, `challenge`.
