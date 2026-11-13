# Combat: PLAN-combat's two tracks

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **PLAN-combat, the caster's track (C)** (docs/PLAN-combat.md, sections 3, 4, 8, 10 and 11; its log is section 18). Only
  track C writes in this bullet. **C1 is built** (harness only): `ATTACKS[]` beside `KINDS` (`FoeAtk`, `FA_*`: each attack's
  creature, kind `AK_*`, element, share of the blow `k` and `add`, `wind`, `cool` (below 0 the skeleton's `throw_interval`),
  `cool_rnd`, `go`/`go2`, `reach`, `slow`; `fa_main`, `fa_every`), read by `update_mob`'s branches, `update_leap`,
  `update_spider`, `update_wskull`, the Wraith's touches and `mob_blow`. `foe_hit(m, fa, k)` is the one road for a creature's
  blow, bite, landing or touch (its element through `harm_next`, `HURT_SPELL` unless physical); `bone_hurt(b)` for what flies
  (`Bone.el1`, 1 + the element; a spell the knight's Mirror turns carries its own). `HURT_SPELL` (it was `HURT_MAGIC`, still an
  alias) is cut by its element's own resist alone (`el_resist`) and by half the armour; armour keeps off a share (`armour_keep`:
  a / (a + 4 + 0.8 x level), half as much against the elements; P2); `throw_interval` never under 0.6 s (P1). `CRAWL_OLDHARM=1`
  puts back 1.0.0b18's harm. `make combat` (`build/host_debug combat`, `crawl_combat_sheet`) writes the balance sheet to
  `build/combat.txt`. **C2 is built** (harness only; its sheet `media/sheets/combat/c2-statuses.png`, approved): the caster's statuses,
  `G.af[NAF]` (time left), `af0` (what it began at), `af_n` (Chilled's stacks), `af_by` (1 + the knocker), `af_lock`/`af_hardt`
  (S1: a hard control can't land within 3 s of the same ending, nor any within 1 s), `af_burn`/`af_acc`/`af_fire_by` (On Fire's
  harm still to come, a point at a time), `af_cold`/`af_cold_t`, all in `carry_caster`'s span, cleared by `af_clear` (deaths,
  `reset_caster_state`, Second Wind). The rules sit before `hurt_ex`: `af_can` (off, dead, at her hall, a dart or a Second Wind;
  Purify running keeps fire, cold and poison off; S1), `af_rolls` (S11: the element's own resist halves the chance), `af_hold`
  (a hard one: the wind-up broken, a mend ended, its sound and rumble), `af_ignite` (E2), `af_chill` (four stacks freeze for
  3 s, held at three while Frozen may not land), `af_poison` (each dose adds, to a minute), `af_put`; whence: `af_struck` (from
  `foe_hit` and `bone_hurt`, a creature's elemental hit by its rank, `af_rank_chance`: 8% + 4% a rank; never a boss, a boss's
  fight, a turned spell or the Wraith), `af_stroke` (`hz_strike_el` and the rock: a frost stroke a stack, the rest `AF_BASE` 12%;
  Physical knocks down), `af_cold` (`HZ_COLD`, `HZ_DCOLD`: a stack every 1.5 s, in place of `hz_chill`), the Wraith's touches
  (two stacks, in place of the slow), bites, the Drowned's pool, bile and `hz_poison` (`af_poison` by what `poison_t` rose).
  What they do: `slow_factor` (rooted 0; 8% a stack), `sprint_recovery` (Chilled: two fifths the pace), `human_walk` (stunned,
  no sprint), `cast_spell` (`af_hushed` returns; Chilled 1.5 degrees astray a stack, not lightning or the drain), `af_slow_cast`
  (Poisoned: `cast_cycle` and the three wind-ups x1.5), `hurt_ex` (Frozen x1.33 on everything; down, every creature's harm but the
  knocker's halved, `harm_from`), `no_cast` (the void or `af_hushed`) in place of `G.silenced` at every gate of spells, gifts,
  Swift Dodge and the dream's aim, `rune_barred(k)` for runes (Blink fires frozen or down, Purify frozen; `af_break`, `af_purify`
  in `cast_defense`, and their `def_trigger`s), E refused unbound, frozen or down, `hush_say`'s words. The dream waits out a
  freeze or a knock-down at the head of `update_player`. Sounds `SFX_AF_STUN`, `_HUSH`, `_FREEZE`, `_THAW`, `_DOWN`, `_SICK`
  (and `SFX_IGNITE`, `SFX_FROST`). The look: `render_af_row`, `ui_status`/`UI_AFMARK`, `render_af_view`, `af_roll`, the horizon
  and eye in `crawl_render`, `render_hand`, `tint`; the sockets struck through by `no_cast`/`rune_barred`. Debug: `CRAWL_AF=0`
  (none: the previous build's events line for line), `CRAWL_AF=<fire|chill|frozen|stun|poison|unbound|down|all>` (that one on
  the caster every ten seconds), `CRAWL_AFSHOW=k` (the look only: `AF_*`; `1:n` Chilled's stacks, `7` the grapple, `all`),
  `EVENT af <status> on by what/type/rank n=stacks player|autopilot`, `EVENT af <status> off`; `build/uisheet status`.
  **C3 is built** (harness only): an elemental creature (`Mob.elem`, D's) carries its element on one attack in three (M1). It is
  decided as the wind-up begins (`elem_arm` beside each `feint_roll`, in `start_leap`, the web spider's drop, a bat settling on
  you; `Mob.elem_on` while it holds, `Mob.elem_t` how long, run down by `elem_tick` in `update_mob`), or as a quick bite lands;
  never for a boss, an add or an ally, and an Unbound one carries none (E5). `elem_take` spends it: `foe_hit` makes a physical
  blow that element (resisted by its own resist and half the armour), `throw_to` makes the bone its element (`Bone.el1`), `bite`
  gives the web spider's. Its status by rank as any elemental hit (`af_struck`, M2: 12% at rank II, 4% more a rank), the
  shambler's grip two and a half times. The bone shimmers (`DR_ELEMX`, bit 24, the element's ramp in `remap`, fire kept low
  and orange) with three motes trailing at its height; D's eyes flare while `elem_on`. Ordinary physical blows knock nobody
  down: Knocked Down comes from physical hazards (C2), and the charge and the grapple (C4). `CRAWL_AF=0` takes C3 out too
  (C2's events line for line); `CRAWL_ELEMON=1` (every attack of an elemental creature carries it, for frames); `EVENT
  elem_hits`, `elem_throws`. **C4 is built** (harness only; its sheet `media/sheets/combat/c4-attacks.png`, approved): the new attacks,
  in play (`c4_on`; the debug build's `CRAWL_C4=0` or `CRAWL_AF=0` takes them out, and nothing is drawn from the dice then, so play
  is the previous build's line for line). `FA_VOLLEY`, `FA_CHARGE`, `FA_SPEW` in `ATTACKS`; `Mob.c4` (`C4_*`), `c4n`, `c4_t`, `c4_cd`, `c4a`, `c4x/c4y`;
  `c4_ready`, `c4_wait`, `c4_stop`. The Bone Thrower (`c4_thrower`, in the skeleton's branch: rank II up, 2.5-9 cells, a 0.6 s
  flash through `Mob.c_flags`, the contract's, then three bones lobbed 0.5 s apart, ten degrees either side; 12-18 s apart). The
  charge (`c4_charge`, `c4_lane`: with its shield, 2-5 cells, a clear lane; the stamp 0.5 s, `SP_SHIELD_RAISE`, dust; the rush at
  seven cells a second up to five, `SP_SHIELD_RUSH`, dust kicked up and two after-images; struck, 1.2 blows and knocked down 1.2 s
  unless dodged, warded or out-armoured twice over, M4; 8-16 s apart). The spew (`c4_spew`, in the Drowned's branch: 1.2-2 cells,
  `SP_DROWN_SWELL` 0.5 s, then `SP_DROWN_SPEW` and six gouts, `SP_SPEW_A/B`, in a thirty-degree cone; at 0.15 s, eight tenths of
  its blow as Poison and Poisoned; 6-9 s apart). The grapple (`c4_grapple`, after a shambler's grip with no element, `foe_el`:
  2.5 times M2's chance, held 1.2 s with no fall, `G.af_shake`), and a Spirit shambler's grip taking 2 + its rank in souls (M6).
  Stunned or Unbound, none begins. The art in `crawl_art.c` (`spr_shield_charge`, `spr_drowned`'s poses 6 and 7, `spr_spew`,
  `spr_dust`), on `make sheets`' `c4`, `c4drown` and `c4fx`. Debug: `CRAWL_C4=1`, `CRAWL_C4SOON=1` (a few seconds apart, for
  frames), `CRAWL_FACEFOE=1` (a person's view turned to the nearest creature and the controls card put away, for frames); `EVENT
  c4_volleys`, `c4_charges`, `c4_charge_hits`, `c4_downs`, `c4_spews`, `c4_spew_hits`, `c4_grapples`, `c4_souls`. The dream's
  counters (8.6): `c4_tells` (in `incoming`: a Bone Thrower's flash or a stamp in its sight counts as a missile on its way, for its
  runes and Swift Dodge) and `c4_dmarks` (in `update_hazards`, after `mk_dmarks`: the charge's lane and the spew's cone as warnings
  on `hz_dmap`, so `decide` steps off them and `auto_abilities` dodges out).
  **C5, the Unholy Confessions** (1.0.0b20, section 10; harness only; the screen's sheet `media/sheets/combat/c5-confessions.png`, approved;
  `conf_on`, and `CRAWL_LINES=1` in the debug build puts the old lines back): six Confessions (`CF_*`, `CONF`: each
  number at rank I and V, `ConfNum.zero`, `from1`) in place of the ascend screen's lines, bought a point at a time (`CONF_CUM` 10,
  30, 60, 100, 150; `RUN.conf`, `RUN.conf_over`), the class's share (`CONF_W`, 75% with no past) in every number; the contract's
  `conf_rank`, `conf_value` (`conf_at` for any points). `conf_own` (from `caster_sync`, with D5's `class_own` after it) adds the
  stats (`CONF_STAT`; Toughness's share of worn armour; past V the caps go up with the overlevel); `conf_cut(k)` takes Agility's
  share off Knocked Down, Stunned and Frozen, Spirituality's off Unbound and Survival's off the rest, their chance in `af_rolls` and
  their length in `af_hold`, `af_ignite` (harm too), `af_chill`, `af_poison`; `soft_pace` runs the old clocks down sooner (`slow_t`,
  `snare_t`, `poison_t`, the bat's latch, which also takes longer to land); `soul_share` on the motes in `kill_mob` (Spirituality
  and D5's `KX_SOULS`); `sprint_recovery` quicker by Constitution. The screen is `render_confess` and `update_confess` (D a point,
  held ten a second after a breath, A back what this opening gave, Space an overlevel on one at V, asked twice; `G.cf_buy`,
  `cf_arm`, `cf_rep0`, `cf_repn`), from `render_ascend`/`update_ascend`; `conf_buy`, `conf_overlevel` (an Alignment Point unless
  `RUN.ap_overs` says an old line's overlevel already gave it), `over_due` counts both. The dream's `auto_confess` (its class's two
  best to V, then the lowest, overlevels to the best whole). Saved as `LG_CONF` (points, overlevels, `ap_overs`); an older page's
  lines are paid back as points and overlevels as it is read (`lines_payback`, `RUN.old_lines`, `old_lines_said`); a death that
  costs takes them. Debug: `CRAWL_CONF=dddddd` (ranks; 6-9 V and overlevels) or six points `a,b,c,d,e,f`, `EVENT confess <name>
  <rank> by ...`, `EVENT lines_payback`. The screen shows each stat as the caster has it in all and as the next rank would leave it
  (`conf_total`, `conf_tfmt`; `gear_total_free` in `crawl_gear.c` is a total before its cap), a pale blue lid over a number at
  its cap (`cap_lid`, `conf_at_cap`; on the list's numeral too, `conf_capped`), *AT ITS CAP: ...* at the pane's foot, and asks
  twice before confessing one whose every number is capped (`G.cf_arm` the Confession). The caps and the owner's caps by class
  are in `docs/BRIEF-caps.md` (section 6 settled, built by D). Tab's caster page lists D's Restoration on 8 once confessed
  (`render_caster`: an ankh, its rank, gold while `G.rst_sel`).
  **C6, the augments' frame and Fire's six** (1.0.0b21, section 11; harness only; its sheet, `media/sheets/combat/c6-augments.png`,
  approved, and in play: `aug_on`, `CRAWL_AUG=0` in the debug build takes it out). `RUN.aug`/`aug_ord`/`aug_cr`/`aug_bank`, saved as `LG_AUG`;
  `aug_of`, `aug_rank`, `aug_k` (0 till rank I), `aug_credit` (kills 1, things 2 in `mk_finish`, a boss 10 in `boss_fall`),
  `aug_feed`, `aug_choose`, `aug_forget`, `auto_augment`; the dispatchers `aug_*` hand Fire to `fire_cost`, `fire_dmg`,
  `fire_cast`, `fire_af`, `fire_kill` (Volatility), `fire_tick` (Pyre, `Mob.pyre`), the other lines to D's `augd_*`. The
  Lampstand's slots (`ui_tree`'s `UiNode.aug*`, `UI_AUGMARK`), the pair's tablet (`ui_augpair`, `update_augpair`, `G.al_aug`,
  `G.al_augs`, `AL_AUG`); Space the augments, Back unbinds. Burning Intensity's held bolt: `fire_grows`, `hold_span`, `cast_held`,
  `g_fire_hold`, `FIRE_GROW_T`; `Shot.scale` drawn in `render_sprites`. The Fire node once called BURNING INTENSITY is SLOW EMBERS.
  The Codex's part *AN AUGMENT OF A LINE* (`CX_AUG`, a page for each chosen: what it does now and at III, its credits). Debug:
  `CRAWL_AUG=0`, `CRAWL_AUGSET` (eighteen digits, a line's three steps in `ELEM` order: 0 none, 1-3 the first at that rank, 4-6
  the second), `CRAWL_AUGAUTO=0` (the dream leaves them), `EVENT aug_choose`, `aug`, `fire_held`, `fire_splits`, `fire_bursts`,
  `fire_spreads`; `AUGCR` lines at each ascend (each line's credits, for the pacing).

- **PLAN-combat, the dead's track (D)** (docs/PLAN-combat.md, sections 5-7, 9 and 12; its log is section 19). Only track D
  writes in this bullet. **D1, the bestiary's elements** (7.1-7.3; harness only): `KINDS[].weak` is flat, x1.5 for the kind's
  one weakness and x0.5 for its one strength, the rest x1, as 7.2's table (spiderlings and sacs take the brood mother's, the
  sac's Spirit still nothing; the wall skull, the spectre and the Wraith as they were); `kind_weak` reads it. A rank's
  strengths are the wards folded in (`Mob.ward`, `wset`, `wdmg`, `wring`, `Pack.ward` kept): `ward_give` calls
  `strength_give` (`rank_strengths`: III and IV one, V two, the gilded one; `strength_pick`: lines held, never the weakness
  nor what the kind shrugs off, with two lines never the class's own, always a held line left that bites as usual; a pack's
  chosen once for its leader, two deep, first in each member's choice), x0.5 (`STRENGTH_K`) in `weak_el`, cracking at a
  fifth of its health as before. One is shown only once seen struck (`Mob.wseen`, `strength_shown`, `strength_seen`, its
  pack's too): the rim, `render_warded`, the readout (`shown_weak`, the barred glyphs), the caption *THIS ONE SHRUGS OFF
  ...*, and cracked *CRACKED: ... BITES IT AGAIN* (`G.ward_said`'s bits 8-13). The bestiary's rows read `kind_weak`, and say
  *SHRUGS OFF ONE MORE* (*TWO MORE*) by the rank shown; the Codex's elements part has a second page, *THE DEAD'S ELEMENTS*
  (`CX_ELEM` 1). Lore rewritten: the gargoyle, the Drowned. Debug: `CRAWL_ELEM=0` puts back 1.0.0b18's table (`OLD_WEAK`)
  and wards (`elem_mode`); the hazards' harm to the dead reads `KINDS` itself and keeps the new table either way. `EVENT
  strengths`, `ward_cracked`, `wardhits=`/`cracked=` on the state line. **D3, the dead's statuses** (section 5; harness only):
  `Mob.stun_t`, `unb_t`, `chl_n`/`chl_t` (Chilled stacks, all gone 3 s after the last), `stun_cd`/`frz_cd` (after a stun or a
  freeze, 3 s before the same lands again: `AF_RELOCK`, S1 for the dead too), `frz_was`; On Fire is `burn_t`, Poisoned
  `pois_t`, Frozen `m->frozen` (+33% in `damage_mob`). Given by `mob_ignite` (E2: added to what is left, four seconds again),
  `mob_chill` (stacks to `need`, then `mob_freeze` for `CHILL_FREEZE`; never lengthened while frozen), `mob_stun` (1.2 s, a
  boss a third; a wind-up broken), `mob_unbind` (1.5 s, no boss); `mob_takes_af` says who takes none (spectres, the Wraith,
  sacs, wall skulls, spirits, allies, the buried). The contract's `mob_stunned`/`mob_unbound` read them. `mob_gate`, the one
  gate at the head of `update_mob`, runs them down and returns the dt the rest runs on: chill takes 18% a stack
  (`CHILL_SLOW`) off the whole clock, a stun halves it and holds `cool`; a stun or Unbound holds the calls (`Mob.call_t`, the
  Sexton's raised `ring_t`, the pack's `call_t` when it is the caller) by adding back what the branches take off
  (`mob_clock` mirrors `mob_time`); Poisoned doubles wind-ups and waits the same way. Drawn: `DR_CHILLA`/`DR_CHILLB` (bits 30,
  29: the stacks, a frost dither of 2, 3, 5 in 16), `DR_ICE` frozen as before, a ring of sparks over the stunned (in
  `render_warded`), grey eyes while Unbound (`Draw.estate` 5). Debug: `CRAWL_MAF=k` (status `AF_k` on what is in sight within
  nine cells every 2.5 s, a chill stack every 0.7), `EVENT maf <status> <kind>`. The gate runs before `CRAWL_STAGE`'s early
  return, so staged creatures show them (but a staged one never thaws). **D4, the elements on your spells** (section 6;
  harness only): `el_status` from `damage_mob`, for a hit that leaves it standing (not a tick): `el_chance` is 10% at the
  line's `align_rank` 1 to 25% by 20, times `spell_effect`, halved if `weak_el` says strong, half again if weak; Fire
  `mob_ignite` (the hit again), Lightning `mob_stun` 1.2 s, Spirit `mob_unbind` 1.5 s. Reconciled, not doubled: Frost's
  `DM_SLOW` is a Chilled stack on every hit (not `m->slowed`), the flask's poison is Poisoned, neither rolls; Physical gives
  nothing. Deep Chill's shard count gives way to `chill_need` (three stacks, not four); Permafrost lengthens the freeze;
  Amperage's `daze` is a 1.5 s stun. The Codex's first elements page says what each leaves on them. `CRAWL_ELEM=0` keeps all
  of it out. **D2, elemental creatures** (7.4; harness only; its sheet `tools/eyesheet.py`): `Mob.elem` (1 + the element) given
  with its strengths the first time it moves (`elem_give` from `ward_give`): `elem_kind` (the kinds with eyes and a blow, B7 b),
  `elem_may` (never Physical, never its kind's weakness, never a boss, its adds or an ally), `elem_roll` (rank II 15%, III 25%, IV
  35%, V 45%, the gilded never; the run's two elements thrice as likely); a pack's rolled once for its leader (`Pack.elem`, -1
  none) and taken by its members past rank I where their kind allows; sacs and spiderlings take the nearest mother's. Born to it,
  it shrugs it off (`weak_el`, x0.5, shown at once since the eyes tell it; `strength_pick` counts it as shrugged). The eyes
  (`EC_EL0` + the element in `EYE_CORE`/`EYE_GLOW`, tones apart from the ranks': frost white in cyan, poison bile over green):
  `Draw.ecol` in its colour, the rank's form kept, a flicker and a mote moving each element's way (`render_eyes`); striking with
  it (`Mob.elem_on`, C's) `Draw.estate` 4, a full halo in its colour; Unbound (5) wins, grey; a web spider waiting in its web
  shows its two points in the colour. Debug: `CRAWL_ELEM=e` (1 Fire ... 5 Spirit: every one that can be, of e, mothers or
  none), `CRAWL_EYEFLARE=1` (every born one flaring, for the sheet), `EVENT born`.
  **D5, the classes' own** (1.0.0b20, section 9 and 10.3-10.5; harness only). The bodies of C's three hooks, in D's marked
  block before `xp_rooms`: `CLASS_OWN` (each class's health against the hundred and up to five stats in their units, `af_cut`,
  `souls`), `class_own` (into `caster_sync`'s own), `class_extra` (`KX_AF_CUT`, the Plague Doctor's tenth off every mark's
  chance, read by `af_rolls`; `KX_SOULS`, the Sin-Eater's tenth more souls, read by C5's `soul_share`), `class_sheet_row`,
  `class_sheet_words`. Her question has a second page (Space: `class_page`, *WHAT THEY BRING*), the Codex's class pages a sentence
  of it; a class taken feels its health at once (`class_take`), and a debug start with `CRAWL_CLASS` has it full. The killing blow
  (10.4): `blow_survived` (after Second Wind; not what is stood in, `lastblow_can`, as `windup_blow` has it), Constitution's
  chance (`conf_value(CF_CONST, CV_LASTBLOW)`), once a level (`G.lb_spent`, cleared in `start_level`; a pocket is a place of its
  own): 1 health and `G.wind_t` 3 s, drawn by `render_wind` with *UNBROKEN* while `lastblow_shown`. Toughness's cracks (10.5):
  `render_cracks` (from the end of `render_warded`), motes off each shut secret's crack face in the run's colour within
  `conf_value(CF_TOUGH, CV_CRACKS)`, more by rank (its sheet `media/sheets/combat/d5-cracks.png`, approved; `cracks_on`). Restoration (10.3): in hand as `G.rst_sel` (id `RS_ID`, not an `AT_`; 8, the wheel, the spells'
  wheel's ankh `RESTORE_MARK`), `restore_cast` (its channel, from `human_cast`), `restore_tick`, half pace in `slow_factor`,
  `restore_hud` (the status bar), `dream_restore` (from `update_player`), and the glow `render_restore` (from `render_hand`,
  `G.rst_k`; its sheet `media/sheets/combat/d5-restoration.png`, approved); `EVENT restore_begin`, `restore_end`. The owner's caps by class
  (docs/BRIEF-caps.md section 6): `CLASS_OWN`'s `strong`/`weak` (its own element's resist +15, one other's -15, the Galvanist's
  Frost -10) and `cap[3]`
  (`class_cap`, each lift in its stat's units), `armour_cap` in `armour_keep` (75%, the Headsman's 90%), `class_cap_row` and
  `cap_text` for her question's second page and the Codex's second class page (`CX_CLASS` with `i` past `NELEM`). Debug: `CRAWL_SHEET=0`
  (no sheets, lifts or armour's cap: the build before, line for line), `CRAWL_LASTBLOW=p` (its chance as given), `CRAWL_CRACKS=0` (no motes), `CRAWL_CRACKS=2` (and a `CRACKS`
  line a second: the face, the distance, the reach), `CRAWL_RESTSHOW=1` (the glow, for the sheet), `EVENT lastblow`.
  **D6, the merchant by class** (1.0.0b21, section 12; harness only). In `crawl_gear.c`: `gear_set_class(el, on)` (from
  `align_sync`, and before her wares roll in `begin_shop`), `pick_elem` (an element's stat from the lines held by rank, G1; with
  `off`, one not held: a piece in six, `off_left` so never two), `on_line` and the reroll in `piece_roll` (Rare and up carry one
  for a held line, G2; boots and amulets have none to carry), `PICK_CHANCE` on bracers and rings and `ST_STEADY` on boots and
  amulets (G3), `st_adds(el)` ("ADDS %" for six, one shared cap: `adds_all`, G4), the class's ability one time in three on
  Uncommon and up (`AB_CLASS` + its element, G5; `CLASS_FOR`, *FOR THE ...* in `piece_ability_text` when it is another's),
  `piece_worth` weighing them (a class's ability only for its class). Felt, in `scene_crawl.c`: `st_odds(el)` (the status
  chances; in `el_chance` for Fire, Lightning and Spirit, a second chill in `damage_mob`'s frost, a dose half again in
  `mob_poison`), `cls_ab(e, which)` (a class's ability, felt by that class alone: Pyrebrand in `el_chance` and `mob_ignite`, the
  Slab in `damage_mob` and `mob_gate`'s chill, Live Wire in `el_chance` and `wire_pass`, Fever in `damage_mob`, Swallowed Sin in
  `el_chance` and a mote in `kill_mob`, the Block in `gear_cut` and a bone in `kill_mob`), `gear_cut` (Steadiness and the Block
  added to Agility's cut, under 0.9), `abil_short`, Tab's elements page with ADDS and ODDS (`render_elements`), the Codex's words
  (`stat_does`, `ABIL_DOES`). Debug: `CRAWL_D6=0` (the build before, line for line), `make items` with `ITEMS_LINES`,
  `ITEMS_CLASS`, `ITEMS_OLD`; `EVENT wire_arcs`, `sin_souls`, `block_bones`.
  **D7, the other five lines' augments** (1.0.0b21, section 11.4's drafts; harness only; its sheet `media/sheets/combat/d7-augments.png`
  sent to the owner). The contract's `augd_name`/`augd_does` (`AUGD_NAME`; the words with the rank's numbers) and the bodies of
  `augd_*` in D's marked block, each reading `dk(e, t, w)` (C's `aug_k`, the rank's share 0.5/0.75/1): `augd_cost` (Overcharge),
  `augd_dmg` (Hoarbite, Overcharge, Siphon), `augd_cast` (Glacial Weight's slower heavier shard, `target` -3; Forked and Grounding
  by `d7_bolt` from `G.d7_zap`; Straight Throw's flat flask; Siphon's heal and Lingering Hunger's hold, `G.d7_lg*`, from
  `G.d7_drained`; Twin Throw's second spear), `augd_hit` (Hoarbite's second chill, Static Build's `Mob.stb_n/stb_t`, Thunderclap,
  the drain's dealt, Barbed's `Mob.bld_*`), `augd_af` (Overcharge, Static Build, Hollowing), `augd_kill` (Soul Echo, Shepherd's
  `G.d7_shep` read by `spawn_mote` into `SoulMote.fly`, Recovery, Bone Rain), `augd_tick` (the bleed, the hold's beam and harm,
  Static Build's fading), `augd_fly` (Rime Trail's rime). In D's own functions: Splinterfrost (`frost_splinter`, `target` -2), Heavy
  Haft, Skewer and `G.d7_spear` (`update_shots`' hit cases; a splinter sets `G.d7_splint` so no bleed, bone or rain comes of it;
  a flask thrown flat bursts on a creature), Cold Snap (`mob_chill`, `cold_snap`), Brittle, Rot and Exorcism (`damage_mob`), Thick
  Brew (`mob_poison`), Conductor (`trail_life`, `update_trails`), Cluster and Miasma (`flask_burst`; a cloud is `Venom.rime` 2, drawn
  as `SP_VAPOUR`), Pestilence (`kill_mob`). Drawn: `DR_BLEED` (bit 28), Static Build's bolts in `render_warded`. The sheet approved
  (9 November): `d7_on` is on, so `augd_built` offers them wherever C's frame is (`aug_on`, on since C6's
  sheet was approved; `CRAWL_AUG=0` takes both out). Debug: `CRAWL_D7=0` (D's out),
  `CRAWL_AUGD=abc` (the choice at each step on all five lines, bypassing the frame) and `CRAWL_AUGDR=r`, `CRAWL_AUGSHOW=k` (1 bleed,
  2 bolts, 3 bursts and the cloud, 9 all); `EVENT aug_fork`, `aug_ground`, `aug_claps`, `aug_snaps`, `aug_splinters`, `aug_rains`,
  `aug_bones`, `aug_lingers`, `aug_twins`, `aug_echoes`, `aug_conducts`, `aug_clusters`, `aug_miasmas`, `aug_pests`.
