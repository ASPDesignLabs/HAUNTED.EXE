# How the dead think

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **How the dead think** (1.0.0b15 step 1, docs/PLAN-ai.md Part A and sections 27-30; `CRAWL_MIND=0` in the debug build
  keeps the old minds, for the balance runs). `minded` (everything but spectres, wall skulls, web spiders, the Wraith,
  sacs, spiderlings, bosses and their adds, allies and a trial's) and `mind_on`. Sight (`mind_sees`): a facing (`Mob.face`,
  turned by `mind_face`: to you while it sees you, to a sound while wary, the way it walks, its head `Mob.look` sweeping
  while calm or searching), nine cells within `in_view`'s 120 degrees and 2.5 behind (a hound smells 4; bats see all
  round); in the dark (`caster_dark`: `G.lit_here`, the light at the caster without its own glow, measured in
  `build_lightmap`, under a third, and no cast's `G.flare_t`) five cells, but not for bats and hounds, and the shade the
  other way about. Hearing (`noise`, `NZ_*`): a footstep 3 (5 sprinting or in water; Quiet less), a cast 8 (the spear 5,
  Hellfire and the ultimates 10, the BFS9000 the level), a blast where it lands 6 (`mind_frame`), a door opening 4, a
  secret wall 8, a death 4 (8 to its own kind), a blow on you 5, a place to mend 16; distances down `mind_field` (a few
  cached fields to a place, a second each) or the hunt field for the caster's own. States (`Mob.aware`, `MA_*`, with
  `aw_t`): calm; wary (`mind_wary`, its tell `mind_tell`; stops, turns, goes to look at half pace, calm after 8 s);
  hunting (the live field while `alert`, refreshed to `MIND_LIVE` 3 s while seen; a blow gives 6); tracking
  (`mind_track`: to where you were, three cells on the way you went, `kx/ky/kh`); searching (`mind_arrived`,
  `search_point`: places six to ten steps round, a room in `G.lost_rg` first; 15-25 s, then calm and `edgy` 30 s, hearing
  half as far again). A calm resident out of its room walks back to its post (`hx/hy`). `mind_goal` and the `going`
  branch of `update_mob` move it; `chase` needs the live field. Caught unaware (in `damage_mob`): calm or wary, struck
  from outside its view, half again and a stagger. Side views in play (`mob_side`, `DR_FLIPH` mirroring the body, its
  dressing, eyes and vapour): facing across your line, its side; away, its side too until backs are drawn.
  `MAXMOBS` is 64 (the masks that count them 64 bits: `Shot.pierced`, `Trail.hit`, the lightning's). Doors by run
  (`door_sfx`, `SFX_DOOR1_OPEN` ... `SFX_DOOR4_SHUT`, played in `update_doors` as one starts to open and as it shuts).
  **Places to mend** ("rests" in the code, as `G.mend_*` is the Mend rune's: `place_rests`, `T_REST` drawn per run by
  `gen_rest`, `G.rest_*`, `rest_facing`, `rest_begin`, `rest_tick`, `rest_end`, `rest_cap`, `REST_T`): one a level, two
  from run III, on a room's wall facing its way in; E begins (not held: a pad's A is a press), standing still facing it
  keeps on, moving, turning away, casting, E again or a blow ends it; the dream's `rest_goal` (under two fifths, not
  `in_combat`, within 30 steps) and `PS_MEND` (`at_rest`). Debug: `EVENT noticed`, `wary`, `lost_you`, `tracked`,
  `searches`, `gave_up`, `unaware_hits`, `pool_full`, `mend_begin`, `mend_end`; `MEND level n: ...`; `CRAWL_TPMEND=1`,
  `CRAWL_HP=n` (health as you take over), `CRAWL_STAGEFACE=deg` (the stage's creatures turned). `make sheets` draws
  `rests.png`. **Step 2** (section 9): a calm creature's part (`Mob.role`, `RL_*`, given by `role_give` the first time
  it thinks): a resident or a vault's guard keeps a post (`hx/hy`, facing `pf`, the room's way in by `room_way_in`; it
  stands there, its head turning, and walks back after a search); bats and hunting spiders roam room to room
  (`roam_next`, a pause in each); the rest walk a round (`route_make`: three or four lane cells five to sixteen steps
  about where they came, spread apart and taken in order of their bearing; `G.route`, `G.nroute`, reset with a level's
  creatures; `Mob.route`, `wp`), pausing 1.5-3 s at each, one that came with another on its round. In `mind_goal`'s
  calm case, `mind_arrived` and the hold in `update_mob`. Debug: `EVENT routes`, `posted`. **Step 3** (sections 5, 6, 8):
  packs (`Pack` in `G.packs`, eight a level, `G.npacks`; `Mob.pack` 1 + its index; `PACK_*` kinds, `PAS_*` states;
  `pack_new`, `pack_join`, `pack_of`). A pack has a rank: its leader's roll, the rest that or one under (`pack_join`). A member that sees you (or is struck) calls (`pack_call`: the kind's tell,
  `pack_tell`, and 0.4 s); the call heard, all of it hunts from wherever it is (`pack_hunt`); the caller slain first, the
  call dies with it. While hunting, what one sees all know (`pack_frame` refreshes the live field). Turns (`pack_turns`):
  its melee members engage one at a time, two when its leader is rank III or more, each for 4-6 s or until hurt by an
  eighth; the rest hold off (`Mob.hold` 2) and circle at three cells, a hound at four. Broken (its leader down, or half of
  it: `pack_break`, `SFX_ROUT`): the file only loses its order; the roost scatters (`Mob.flee`, then a roamer); a hound
  runs to the nearest other creature that thinks (`Mob.fetch`, `fetch_tick`), tells it where you were and both go
  there; the wake doesn't notice. The cap (P6 a; `engage_pick`, `engage_cap`, called from `minds_frame` after
  `pick_hunters`, which now lets any number of the new minds hunt): within eight cells and in sight, the nearest 2, 3, 4,
  5 by run for the dream and 4, 5, 6, 7 for a person (the dream's one less since step 8, A3) are `Mob.engage`; those in stay in while they are, and a blow or throw under way is
  let finish; the rest hold (`Mob.hold` 1) at three and a half cells, throwing nothing (a brood mother held lays nothing
  either: she had sat at the edge laying sacs until spiderlings filled the 64 places). The buried are outside it.
  Packs placed (only with the new minds): the file of the Order (`place_file`, from 5, always on 5 and one level in two
  after: three, four, four or five by run, one or two shields leading; on a round, the rest following in a line,
  `RL_FOLLOW`, `follow_of`, or two times in five holding a room as residents; its throwers keep a cell behind its
  shields; `SFX_DRUM`), the hounds (`spawn_wanderer_type`: a pair becomes a pack of two, three in run III, four in IV;
  `SFX_HOWL`), the roost (`place_roost`, from 11, always on 11 and one level in two after: three to six bats asleep on the
  ceiling of the dimmest room, `Mob.sleep`, `roost_doze`, drawn `DR_FLIPV`; a noise reaching it, a step within 2.5 cells,
  you in its room in the light, or a blow, and it calls; all of it drops and comes), the wake (`place_wake` in
  `place_risers`, from 13 where the buried are: three or four under one room's graves or puddles; one rising stirs the
  rest, nearest you first, 0.6-1.2 s apart, `wake_stir`; earth and water heard now and then near it). The file and the
  roost each take a wanderer's place; a level holds one to four more creatures than it did. Debug: `CRAWL_PACK=k` (1 a
  file, 2 hounds, 3 a roost asleep, 4 a wake, a few cells ahead five seconds into each level), `PACKS level n: ...`,
  `EVENT pack_calls`, `pack_cuts`, `pack_hunts`, `pack_broken`, `packs_broken`, `fetches`, `roosts_woken`, `wakes_rise`,
  `turns`; the state line's third line counts the minds (calm, wary, hunting, tracking, searching, engaged of the cap,
  held, asleep, packs). **Step 4** (section 10): callers and scavengers. A caller's alarm (`alarm_at`, once a creature,
  `Mob.called`; `NZ_CALL` through `noise`, which only the new minds hear): whatever hears it is wary of the place, a pack
  (or one already about) goes there tracking. The wall skull's scream 0.6 s after it wakes, ten steps (kill it first
  and nothing hears); the gargoyle's shriek as it comes loose (`SFX_SHRIEK`), eight; the deacon's call the first time he
  sees you (`Mob.call_t`, 0.8 s standing to chant it, a moan and a whisper), fourteen; the roost's cry as it drops, eight
  (from `pack_hunt`). Scavengers: a ghoul hears a death three times as far (twelve steps) and, while it isn't hunting,
  goes there (`Mob.scav`, `scx/scy`, the goal before any other in `mind_goal`); beside it it feeds (`scav_feed`: the old
  feeding, `sub` 4, 5-8 s, bent over the remains, mending and taking double), until it sees you within six cells. Seeing
  you on the way, it forgets the meal. A ghoul past its first level comes with none, one or two more (`PACK_SCAV`; a
  groan and bones for its tell); broken, they scatter as the living do. Debug: `EVENT alarm <kind> steps n`, `alarms`,
  `alarms_heard`, `scavenge`, `feeds`. **Step 5** (section 11): hunters. The trail (`G.scent`, the level's clock when the
  caster was in each cell, stamped each frame, none on water; `SCENT_LIFE` 120 s; `on_trail`). The hunt (`place_hunt` in
  `start_level`, in a wanderer's place; `HK_*`; H4's chance: a quarter of run I's levels from 5, a third of II's, half of
  III's, two thirds of IV's, every corrupted level, never a boss's or a pocket): a band at the far end (24 steps or more
  from the start), waiting at its post (`Mob.hunter` 1), let loose at `G.hunt_at` (150 s less 10 a step of the run, give
  or take 15) by `hunt_tick` with a far-off tell (`hunt_sfx`). The watch (`HK_WATCH`, from 5: a file) goes to the last loud
  noise (`G.noise_x/y/t`: any of the caster's but a step) and to each new one (`hunter` 3); hounds (`HK_HOUNDS`, from 9) and
  hunting spiders (`HK_SPIDERS`, from 15, along the ceiling: `PACK_SPIDERS`, scattering when broken) go to where the caster
  was a minute ago (`G.hunt_nx/ny`) and follow the trail (`hunter` 2; the `trailing` branch of `update_mob`: the freshest
  neighbour, a tenth quicker; a shut door or one of its own ahead, it waits), and within four seconds of you hunt as anything
  does (`hunter_tick`, the nearer tell once); losing you, they take the trail again, not a search; a trail that ends (water,
  a pocket, gone cold) ends a hunter's hunt (`hunter_lost`: it searches there). `MF_FAR` is 160, so a field to a place
  reaches across the level. The shade's stalk (from 16): while it has you it holds five cells off, behind you
  (`update_mob`'s hold 3), and strikes only while you stand still 1.5 s, fight something else within six cells or have a bat
  on your face (`shade_may_strike`, `G.still_t`); lit and in front of you it fades back (`Mob.shy`, 1.5-2.5 s, afraid). The
  calm (H2): with the new minds a person gets no calm wanderer (the hunt is its stand-in); the dream's comes from 16-30
  steps and tracks you in. Debug: `CRAWL_HUNT=s` (a hunt on every level from 5 that can have one, let loose at s seconds),
  `CRAWL_HUNTK=k` (its kind: 1 the watch, 2 hounds, 3 spiders), `CRAWL_HUNTTRACE=1` (each hunter and shade once a second:
  where, how it knows you, the scent under it, the steps to you and to the pick-up), `PACKS level n: hunt of ...`, `EVENT
  hunt_begin`, `hunt_scent`, `hunt_lost`, `shade_fades`, `shade_strikes`; the minds line's `hunt=kind/loose/trailing`.
  **Step 6** (section 12, and section 30's glint): ambushes (`Ambush` in `G.amb`, two a level at most, `G.namb`; `place_ambushes`
  in `start_level`, in a wanderer's place; from level 7, half of run I's and II's levels and two thirds of III's and IV's,
  a second one time in four from run III; never a boss's or a pocket). A kill room (`amb_make`): not a vault, the wing or
  the trial's room, no exit, key, altar or place to mend in it, its nearest cell ten steps from the start and eight from
  the exit, one or two ways in, and with it shut (`amb_bfs`) the exit and every key still reachable (rule 3); its way in
  the opening nearest the start (`ox/oy` outside, `ix/iy` inside), its far side (`fx/fy`). Its teeth (`TEETH_*` by run and
  room type, `amb_teeth`: run I shield-bearers, or the buried in a burial vault; II the Drowned in a cistern or canal (a
  puddle each), a knight and a shield-bearer past 13, or shield-bearers; III hunting spiders overhead (a gallery or stope)
  or ghouls (a charnel pit); IV gargoyles on plinths (a glass hall) or shades), along the wall either side of the way in,
  `Mob.amb` 1 + the ambush, posted and turned to the doorway, in a pack: they wait doing nothing (`update_mob`'s early
  return; the buried don't rise of themselves) until it springs (`amb_tick`: you two cells in, a tooth in your sight
  within nine cells, a gargoyle within 2.5; or a tooth struck, or the bait slain in the room: `amb_spring`, `amb_release`
  (out of the earth, off the plinth, down from the ceiling), `SFX_STINGER`, *AN AMBUSH* once a run of ten, `RUN.amb_runs`,
  not saved). The bait (two times in three; `Mob.bait_st`, `B_*`): the run's straggler (a skeleton that throws once,
  `Mob.bthrew`), a hound (a yelp), a bat, an imp, in a hall six to twelve steps from the way in; seeing you it shows itself
  (`B_SHOW`, its tell) and waits; come within six cells or strike it and it backs away (`B_LURE`: to the far side at 1.8
  cells a second, facing you) and turns there (`B_HOME`, the fight). Lose it for eight seconds (or six while it leads)
  and it goes back to tease from where it saw you last (`Ambush.teases`); the third time the whole set piece comes hunting
  (`amb_out` with hunting). Slain outside the room, the teeth come out to search where it fell (`amb_out`). Else a glint:
  a chest or a potion two cells in from the way in, out of the level's totals. The level's gilded, one time in two
  (`amb_gild`, `B_GILD`), runs to the kill room when hurt instead of away. The sign (`TELL`: run I bones, II a puddle, III
  and IV rubble) by the door outside; a run's first ambush a torch by its door snuffed (`Ambush.snuff`, a tenth of its
  light in `build_lightmap`) or a second sign. A spectre drifts to an unsprung ambush's door first, one time in two
  (`update_ghost`'s `sub` 7), and hangs there harmless. Debug: `CRAWL_AMBUSH=1` (one on every level that can have one,
  from level 1), `CRAWL_AMBT=k` (its teeth, `TEETH_*`), `AMBUSH level n: ...` (the room, the way in, the teeth, the bait, the
  signs; or why none fitted), `EVENT ambush_sprung <teeth> <why>`, `ambushes_out`, `ambushes_refused`, `bait_seen`,
  `bait_run`. **Step 7** (sections 13-15, 27, 30): cunning by rank (`cunning`: the rank, the gilded as white; only the new
  minds). Green: alone and hurt to a quarter, it falls back once to the nearest of its pack beyond three cells (`fall_mate`,
  `Mob.fell` 1 + whom, `fall_t`, ten seconds at most; it strikes nothing while it goes and isn't counted in the fight); a
  skeleton or an imp, after a throw, steps behind a corner a cell off if there is one (`cover_near`, `Mob.cover`, `cvx/cvy`)
  and out again a step's time before its next; three times running at most, then it stays out fifteen seconds (`cov_n`,
  `cov_t`; step 8's checks found one peeping at the dream from a corner for ten minutes). Blue: sees a slow spell coming (the fireball, Hellfire, the flask, the orb, a
  spear from beyond five cells; `mind_dodge`: within its view, 0.15-0.8 s from passing it, or a flask's landing within 1.6
  cells) and steps a cell aside at 3.4 cells a second (`Mob.dodge_t`), once in four seconds (`dodge_cd`); keeps out of what
  warns or harms (`hz_shuns` 1: it waits) and walks round what is always there when another step is as good (2: 2.5 off the
  score); won't step into a doorway you hold (`doorway`: a door, or a gap a cell wide where the region changes;
  `held_door`: you stand, under 1.2 cells a second, looking at it within 25 degrees, 1.6 to 7 cells off, a spell ready, for a
  person no cast under way and the wait under 0.35 s), but steps aside out of your sight and waits (`door_wait`,
  `door_beside`, `Mob.dwx/dwy` the doorway, `door_t` its patience: eight seconds near it, shut or open, then it comes and
  won't wait again for six; while it waits it knows where you are). Red: the second and later of a hunting pack take another
  way to you (`pack_flank`, `FL[pack]`, `flank_field`: the lead's way from two steps out to ten blocked with a cell either
  side, the steps to you round it, made each second; taken by a red or black member more than four steps off when it is at
  most six longer; `Mob.flank` 1 + the pack); with a splash in hand (`splash_in_hand`: Hellfire, the flask, the frost
  spell with Shatter, lightning with Arc) none steps within a cell and a half of another (`spreads`, `crowded`). Black:
  feints one wind-up in three, never two running (`feint_roll` as it begins, `feint_held` as it ends: no blow, and the next
  0.35 s on; the shield-bearer, the shambler and the Drowned, the knight, the gargoyle, the shade, the skeleton's and the
  imp's throws; not the hound's quick bite); reads the charge (`charge_read`: a charging ultimate held past 0.25 s, within
  six cells: it holds at 4.5, `hold` 4, off the line you face). D2: with the new minds `gen_level` makes two lairs a level,
  three from run III (`G.lairs`, rooms of two blocks or more), gives each a second way in where the plan allows (`con_carve`,
  the old opening code made a function) and opens the extra loops within two blocks of them one time in six, not twelve
  (`lairnear`); the roost takes the dimmest lair, a file holding a room a lair. D3: the hazards that hurt only the caster now
  hurt the dead at half the share (`hz_mobs`, `hz_harm`: rocks, Hellrent, bile, the void, the gases, the wax and the
  fissure, the cold, the sluice, the ash; harm, not a blow, so nothing learns where you are from it; not bosses, their adds,
  allies, spirits, the buried, a roost asleep, nor what is over a floor's harm); the runs' own that hurt them already keep
  their 1.5 x share. The throw over a corner (`corner_aim`, `throw_to` with `lob`; `Mob.lsx/lsy/lst` where and when it last
  saw you): a green or better skeleton or imp out of your sight, having seen you within six cells in the last three
  seconds, lobs to that place and up to a cell and a half on the way you went while it can see there; a lobbed bone comes
  down there (`bone_land`, within 0.9 cells) and arcs as an ember does. The dog at your back: in a hound pack's turns, one
  more than 110 degrees from your facing goes in first. The cap passed (section 27; `WELL`, a person's last two minutes in
  tens of seconds, `doing_well`, `well_tick`): a fight begun after five quiet seconds, while doing well, takes one more one
  time in three, for a minute or until a hurt. Debug: `CRAWL_CUNNING=r` (every creature thinks as rank r, 1-5),
  `CRAWL_TPHOLD=k` (once you take control: in a room three cells in from an open door, facing it, and a creature of kind k
  three cells beyond it, after you; `TPHOLD` line), `CRAWL_SPLASH=1` (as if a splash were always in hand), `CRAWL_WELL=1`
  (as if a person were doing well), `LAIRS level n: ...`, `EVENT dodged`, `flank`, `spread` (at most once a second),
  `feints`, `fell_back`, `covers`, `corner_throws`, `door_waits`, `charge_reads`, `hz_mob_hits`, `cap_over`.
  **Step 8** (Part E, sections 7, 16-19, 21, 23): with the new minds `start_level` calls `place_creatures` (today's placements
  are `place_creatures_old`, for `CRAWL_MIND=0`). A level's budget (`budget_of`, a table by run and step fitted to what the old
  minds' levels held, so the heads on a level stay near today's; a boss's level half, outside its arena; a corrupted one two
  in five more, in place of the extra wanderers) is spent on set pieces (`SET_*`, tagged on `Mob.set` by `set_tag`): the vaults'
  guards and a newcomer's first showing first, then the watch, the webs, the buried, the imps, the gargoyles, the brood, the
  file, the roost, the ambush, the hunt and the band in a shuffled order (the one this level shows first, `set_first`, at the
  front; the ambush always before the hunt), each told how many heads it may place (`set_cap`, from what is left at the
  level's usual rank; every placement takes `imin` of it), then posts and wanderers with the rest. A head costs by kind
  (`KIND_COST`) and a fifth more a rank (`rank_cost`; `set_spent`). Rules: 48 heads at most; nothing set within six steps of
  the altar, a puzzle's or a trial's door or a place to mend (`set_clear`); what killed you on a level (`last_set`, from
  `hurt_by` and a thrower's bone) comes back a head short each time (`short_n`, by the level), and a hunt that killed you isn't
  let loose again (`short_hunt`). Rollout gates (section 23): D2's lairs from 25, D3's harm to the dead from 31. Mixed bands
  and packs (`PACK_IMPS`, `PACK_SENTINELS`, `PACK_BAND`; `MAXPACKS` 12; a broken pack's members go by their kind now: the
  living scatter, hounds run for help, the dead hold their drill): the watch and the dead (from 3, `watch_dead`: two or three
  skeletons holding the nearest room within three to nine steps of a wall skull, which its scream reaches); the nest (15,
  `nest_make`: two or three hunting spiders overhead by a web spider's corner, which is its tripwire, `pack_call` when it
  drops); the knight and his hounds (17, a band from `spawn_wanderer_type`: the hounds hold at four cells, not biting, till he
  is within four of you, `pack_turns`; kill him and they run for help); the deacon's congregation (18, a band, a
  shield-bearer in front); the brood's guard (19, two hunting spiders with her, leashed through the falling-back path: six
  cells, two and a half once she is hurt); the roost and the shade (21, a shade keeping the roost's room); the imps' brood
  (23, by one vent, splitting to either side to lob and falling back into the vent's heat); sentinels (27, `garg_wake` wakes
  the others within twelve cells; grit falls from them now and then); the Wraith's chill (9, `wraith_chill` at its coming and
  each wail: packs within sixteen cells keep to their posts fifteen seconds, `Pack.chill_t`, till struck); bands of every kind
  met (29, `place_band`: a knight, deacon or ghoul with escorts of another kind, in a fossil hall or the throne if there is one,
  holding it one time in two) and of three kinds (33); a hunt that drives you into an ambush (37: with an ambush on the level,
  hounds or spiders wait ten to eighteen steps behind the start and are let loose at a minute or so); and what you passed by
  gathers in the exit's room once you see it (39, `gather_exit`, `G.gathered`). Room types choose (section 18): a file holding
  a room takes an ossuary, chapter house or fossil hall first, then a lair; a roost an undercroft or geode, then a lair, then
  the dimmest; a nest a gallery or stope's webs first; a knights' vault's posts are shield-bearers (knights once the descent
  comes round). Pockets: their creatures near one another of a kind join a pack, ranks kept (`pack_add`). Debug:
  `CRAWL_BUDGET=f` (the budget times f), `SETS level n: budget, spent, heads, each set piece's heads and cost`, `PACKS` lines
  for each new piece, `GATHER level n`, `EVENT nest_calls`, `chills`, `sentinels_woke`, `leashed`, `gathered`. Two faults of
  the dream's found in its checks and mended for both minds: a stope's firedamp (`HZ_DAMP`, which hangs till fire sets it off)
  is marked as what is always there, not harm, so the dream crosses it when it must (it had waited at the doorway it filled
  till it gave the level up); and a censer's swing marks the dream's map only 0.9 s ahead whatever is held (Premonition's
  further second had kept the lane under it marked the whole swing, and the dream never crossed). `hz_dput` keeps the worse
  of two marks, a warning or harm over what is always there.
  **Step 9** (section 22): the three new kinds made for the minds' roles, at the end of `KINDS` (`MOB_WHIPPER` 20, `MOB_SEXTON`
  21, `MOB_LIGHTLESS` 22; `REC_KINDS` 24 leaves room for one more), and with the new minds only (`random_type`, the newcomers'
  list, the hunts). Sprites `SP_WHIP_*`, `SP_SEXTON_*`, `SP_LIGHT_*` (docs/InitialArtPass.md's log); sounds `SFX_HORN`,
  `SFX_HANDBELL`, `SFX_GLASS`, `SFX_WHIPCRACK`. The Whipper-In (from 22): never without three hounds (four from run IV) in his
  pack (`PACK_WHIP`, from `spawn_wanderer_type`); its call is his horn (`pack_call` gives it 1.1 s, `Mob.call_t` his pose,
  `pack_tell`), and as the pack sets on you it is heard ten steps off (`alarm_at`); each hound his turns send in gets the whip's
  crack (`pack_turns`); he keeps off (`keeps_off`) and lashes what comes within two and a half cells; the pack breaks only when
  he falls, and then his hounds run wild (`Mob.wild`: out of the pack, a third quicker, careless: `cunning` RK_I). Three hunts of
  hounds in five from run III are his (`place_hunt`, `G.hunt_whip`; the hunt's tell is the horn). The Sexton (from 26;
  `sexton_ring`, `Mob.ring_t`, `rings`, `ring_cd`, `ice_t`, `seen_t`): never hunts (`mind_tick` keeps him calm), walks his round
  and rings once at each turn of it (`mind_arrived`); seeing you, struck, or come round having seen you within 25 s, he raises the
  bell half a second, then rings it (`NZ_BELL` through `noise`, fourteen steps: everything that hears goes to his place), every
  eight seconds at most; killed while it is raised, caught unaware (2.5 s) or a frost hit (`ice_t` 6 s, `SP_SEXTON_ICE`) and it
  isn't rung; he keeps off, and the dream aims at him first as at the deacon. The Lightless (from 32; `light_feel`): no eyes
  (`has_facing` 0), so it sees all round, nine cells in the light and five in the dark (the caster's own glow); your lantern lit
  or a cast's flare within sixteen cells is as good as sight, a wisp or a fire on the floor within ten cells of you somewhere to
  go; quick while it knows (1.4 times its pace, 0.6 calm), a roamer (`RL_ROAM`), hearing at half the reach, silent but for a
  glassy chime now and then; finding nothing where it went it stands and listens five to seven seconds (`SP_LIGHT_LISTEN`); it
  puts out a wisp it reaches (`wisp_out`). A fault found on the way and mended for the new minds: a creature whose way somewhere
  was stood in (a post in a hall a cell wide, a statue) had waited there for ever; after three seconds it now gives the place up
  (`Mob.blocked`, `EVENT ways_blocked`). The bestiary's pages run to the Deep's kinds (`best_kinds`, to `DESCENT_LEN`). Debug:
  `CRAWL_PACK=5` (the Whipper-In and his hounds), `CRAWL_KINDTRACE=k` (each of kind k once a second, and now and then its way
  somewhere), `PACKS level n: the huntsman and his hounds ...`, `EVENT horns`, `whip_cracks`, `whip_lashes`, `wild_hounds`,
  `bell_rounds`, `bell_raised`, `bell_alarms`, `bell_heard`, `bells_frozen`, `light_felt`, `lantern_felt`, `lightless_lost`,
  `lightless_hits`, `wisps_struck`.
  **Step 10** (Part G): the dream's counters, never a person's. A glance back (`dream_glance`, from `pack_call`, `hunter_tick`'s
  nearer tell and `scav_feed`: a sound behind it within twelve cells, once in six seconds; `G.glance`, then at the next lane cell
  `dream_looks` turns it to face the sound, `PS_LOOK`, and it fights what `find_target` finds there or turns back after half a
  second). An ambush's sign read (`amb_read_tick`: within seven cells of its way in and in sight; `G.amb_read`) makes the cells by
  its way in soft to `decide` (`amb_soft`, as `hz_soft`: chosen against only between ways on), and within two cells of the way in,
  outside the room, it turns and sends a fire bolt through to the far side first (`PS_PROBE`, `G.amb_probed`, once an ambush).
  `find_target` takes a caller about to call from any side within nine cells (`Mob.call_t`, the Sexton's raised bell, a pack's
  caller while its call is under way), first of all, and a hunting pack's leader before the rest. Hurt under half with a pack
  hunting it within ten cells (`pack_on_me`), it makes for the nearest lane corner it has walked within eight steps (`corner_goal`
  in `pick_goal` after the trial's; `lane_corner`: a bend or a dead end) and holds it (`at_corner`, from a step's end or a turn's;
  `PS_GUARD`: it faces and fights the nearest that knows it is there, `guard_target`, the aim coming back to the guard) while it is
  under three fifths and the pack is on it, thirty seconds at most (`corner_keep`, `corner_leave`: twenty seconds before another;
  `G.corner_*`). It never chased bait, and its engaged cap is one less since step 8. Debug: `EVENT glances`, `glance_finds`,
  `amb_read`, `amb_probes`, `corners`, `corners_held`. For any fault in the dream's runs: `STALL level n ...` lines (each
  creature within twelve cells, what it is doing and whether the dream can aim at it) once the dream has stood 40 s within a
  cell of one place, and again at 120 s; `CRAWL_RUNID=n` gives a fresh run that id (each run prints `RUNID n`; otherwise it
  comes from the clock, and the trials' order with it), so a run that went wrong can be run again.
