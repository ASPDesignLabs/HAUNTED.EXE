# Known issues and loose ends

From `CLAUDE.md`, moved here word for word when that file was slimmed (docs/PLAN-tidy.md, step 6): what is known to be
wrong or untried, and the numbers from each round of the dream's runs. Add to it; keep the bad news.

- With the items (step 7 of the items plan), eight 50-minute autopilot runs from level 1 died 3 times
  (7 at 1.0.0b3; six of eight never died in both), levels 8-9 reached. Weak runs were the ones whose
  shopping skipped attack spells, so `shop_choose` takes three attack spells first, and saves only for a
  spell within two levels' gold (`gear_haul` x 0.6 x 2).
- Secrets, the graves, the items, spirit, bones, the merchant's room and the Black Altars have not been
  played by hand on Windows yet; nor has anything in 1.0.0b5 (the sliding bar, wisps in the crypt, the L
  list, the altar's menu and blessing, the console), nor anything in 1.0.0b6 (chains and pockets, the
  Backrooms and Severance, coins, the shrine, runes by quality).
- With 1.0.0b5's wisps and altars, eight runs on each start died 4 times from level 1 (3 at 1.0.0b4), 11
  from level 11 (12) and 28 from level 16 (25). The Wraith had come only once a session since 1.0.0b3;
  now that it comes once a level, it is behind most of the Deep's extra deaths.
- With 1.0.0b6's chains (both builds run again on the same seeds; the event log counts a death in a
  pocket as `died` too, followed by `pocket_thrown`): in the crypt 4 deaths from level 1 (5 at 1.0.0b5),
  7 from level 11 (11), 30 from level 16 (28), plus 2, 2 and 5 in chains. The autopilot reaches few
  chain ends past level 10 (4 from 11, none from 16) and takes about three runes from the shrine in
  eight runs (it bought 20 from the merchant at 1.0.0b5); it never saves the six coins to raise it.
  Exquisite runes (only through the debug switches so far) made level 18 deathless; untuned.
- Spectres in a pocket come back when you go in again: they can't be killed, so they never get a slain
  bit (`PK.slain`). Harmless, but worth knowing when counting creatures.
- `decay_tex` already draws cracks on ordinary walls at depth; keep secret cracks distinguishable if
  textures change (see the plan).
- CI warns that `actions/download-artifact@v6` targets Node 20; bump the action when a newer major is out.
- Past level 40 the runs come round with a numeral on the names; no loop moods yet, and the bosses are
  stronger only as their level makes them.
- With 1.0.0b7, eight 100-minute dream runs from level 1 (seeds 1-15 odd, against the step-10 build on
  the same seeds) cleared levels 9-13 (median 9.5, against 11.5), died 27 times (11), 21 of them on level
  10 (10 to the knight, 10 to its adds, 1 to its mirror), and beat the knight in 4 runs of 8 (6), 23 tries
  (13); four runs were still on level 10 at the end. None died to a hazard; rocks struck it 33 times. The
  cause is a fresh run's two pedestals and one ware a visit in run I (78 wares bought against 101). A
  variant that kept half a level's haul back when buying a pedestal bought fewer; not kept. Logs of these
  runs aren't kept in the repo; rerun with `CRAWL_SEED=s build/host_debug apl crawl 6000`.
- Before the release, `mz_vault` could make the room above a boss arena's doorway a vault and wall the
  doorway up (116 of 800 boss levels; three of the eight dream runs sat on level 10). It now skips any
  room with a `keep` exit; the debug map's `MAP lanes reachable` line shows such a cut-off at a glance
  (about 200 lanes short).
- 1.0.0b10's run I in play (docs/PLAN-b10.md, step 2) has been tested in the harness only. Against 1.0.0b9 on
  the same eight seeds (100-minute dream runs from level 1, seeds 1-15 odd): a median of 11 levels cleared
  (9.5), 17 deaths (17; 13 on level 10, against 16), the knight beaten in 5 runs of 8 (4) in 19 tries (21). The
  room types decide which rooms keep resident skeletons (the ossuary, burial and knights' vaults: about three
  rooms in five, against one in two before), so the dwellers' count moved a little; the runs show no harm.
  Run II in play: eight 50-minute dream runs from level 11 at `CRAWL_GEAR=3` reached a median of level 16.5
  (16 for 1.0.0b9), no deaths in either. Run III: eight from level 21 at `CRAWL_GEAR=5` reached a median of
  level 27 (27), one death (none), the same creatures slain a level. Run IV: eight from level 31 at `CRAWL_GEAR=8`
  reached a median of 36.5 (37), no deaths (one), but its levels hold about a tenth more creatures (39 against
  35.5), since more of its rooms keep resident dead. None of the runs' looks has been seen on a real screen yet.
- 1.0.0b9 (the map, the target's health, Levels of Atonement, Luck, cornerstones, overlevels) has been tested
  in the harness only. Against 1.0.0b8 on the same eight seeds (100-minute dream runs from level 1, seeds 1-15
  odd), the released build (`ATONE_K` 42) cleared a median 9.5 levels (9.5), died 17 times (27; 16 on level 10,
  against 21) and beat the knight in 4 runs of 8 (4) in 21 tries (23); Second Wind saved it in seven runs, Luck
  dropped 68 purses. At the first curve (`ATONE_K` 30) the same seeds cleared a median 12.5, died 13 times and
  beat the knight in 6 of 8: the knight is still the wall, and the steeper curve (set from those runs and from
  runs at 11, 21 and 31, so that the dream's caster reaches 50 at about level 35) gives it fewer levels to meet
  it with. None of it has been played by a person, so how a person's pace compares is unknown. A level replayed
  after a death in the dream pays its experience again.
- The game controller (1.0.0b8) has been tried only with a scripted pad in the harness: XInput itself, the
  dead zones, `PAD_TURN` and the stick's walk, the rumble's strength, the display kept awake and a pad
  pulled out mid-game are all untried on Windows. PlayStation and Switch pads (Raw Input) were left for
  later.
- Nothing in 1.0.0b7 has been played by hand or run under Wine: the bosses, hazards and omens, death's
  cost, the ledger and the claim (the mark is a registry value written from the main loop), the hall, the
  Codex, the C key and the right mouse's first use.
- The Deep (levels 16-20) is deadly for the autopilot even with every spell at tier 2: eight
  30-minute runs from level 16 died 25 times with the items (22 at 1.0.0b3, 25 before the Deep's
  newcomers came); it can seldom afford anything there. The Hollows at tier 1 died 12 times in eight
  50-minute runs from level 11 (18 at 1.0.0b3, 9 before their newcomers). `CRAWL_GEAR` gives different
  pieces than it did at 1.0.0b3, so those compare only roughly. Tune by hand.
- 1.0.0b11 has been tested in the harness only. Against 1.0.0b10 on the same eight seeds (seeds 1-15 odd; deep starts
  with `CRAWL_XPBANK` so the dream ascends and aligns at its first greeting): from levels 11, 21 and 31 (50 minutes, gear
  3, 5, 8) a median of level 16, 26.5 and 36 (16.5, 27, 36.5), no deaths (one); from level 1 (100 minutes) a median of
  9 levels cleared (11), 21 deaths (17; 19 on level 10, against 13), the knight beaten in 3 runs of 8 (5) in 22 tries
  (19). Souls run short in his fights (16% of casts weak from level 1, 4-6% deeper; under 1% before) and his blows break
  wind-ups. A change that sent the dream after motes mid-fight did worse and was reverted. The levers are in
  docs/PLAN-b11.md's step-16 note. The first runs found two stalls (a key at a block's corner past the dream's 1.5-cell
  reach, now 1.6; a Wraith fired at in a dead end, now let be once driven back); a look over a new batch's
  state lines for a minute or more spent within a cell of one place is worth the minute it takes.
- 1.0.0b13 has been tested in the harness only. Against 1.0.0b12 on the same eight seeds (seeds 1-15 odd; from 11, 21 and
  31 at gear 3, 5, 8 with `CRAWL_XPBANK` 9450, 35322, 77658): from level 1 (100 minutes) a median of 9 levels cleared (9),
  22 deaths (26; 19 on level 10, against 24), the knight beaten three times (twice), three deaths in trials (the Toll, the
  Wager, the Ossuary); from 11, 21 and 31 (50 minutes) a median of 16.5, 25.5 and 36 (16, 26, 36), no deaths in either. A
  level takes 10 to 35 s longer (median 360-385 s). The dream went into 163 trials and won 150 (10 lost, 3 died), 14 s
  left at the median from 11 and 20-29 s elsewhere, and solved 83 puzzles on 95 puzzle levels. It passed 74 trials by,
  67 of them unseen (`EVENT trial_passed`): `tri_goal` waits for the door to be seen or the room within sixteen steps.
  The table is in docs/PLAN-b13.md's *The checks*. A fault found there: `crawl_step` counts from 0, and the trials' picker
  had taken it as from 1, so each run's first trial came twice; whatever indexes by a run's step, check which it means.
  Runs started in the same second in the harness share a run id (`new_run_id` is the clock and the app's seed), so
  their trials come in the same order.
- 1.0.0b15's step 1 (the senses and memory, the places to mend; harness only), against `CRAWL_MIND=0` on the same build and
  the same eight seeds (seeds 1-15 odd; deep starts as 1.0.0b13's): from level 1 (100 minutes) a median of 10.5 levels
  cleared (9), 16 deaths (23; 15 on level 10, against 18), the knight felled 5 times (3); from 11, 21 and 31 (50 minutes)
  the same medians (levels 17, 27, 37) and no deaths (2 from 11); a level's median time 356-375 s (363-372). In the eight
  runs from level 1 the dead noticed the dream about 2,300 times, heard it 1,400, lost it 170, searched 75 times and gave
  up 51; 328 blows landed on creatures caught unaware. So the dream is better off: the dead now forget it, and it catches
  them unaware. The challenge is meant to come with packs, hunts, ambushes and cunning (steps 3-8). Blasts filled their
  eight places four times and the dead's missiles their ten once. Blasts are doubled (16: only their look and the noise
  of a landing). The dead's missiles stay at ten: doubled, the knight's level killed the dream more with either mind (the
  old minds 26 deaths against 23 on the same seeds), since throws a full pool refused had been part of its balance.
- 1.0.0b15's step 2 (posts, patrols, roamers), against `CRAWL_MIND=0` on the same build and seeds, from level 1 (100
  minutes): a median of 9 levels cleared (9), 30 deaths (30; 26 on level 10, against 24), the knight felled twice
  (never). From 11, 21 and 31 (50 minutes, with step 2 but the dead's missiles doubled): no deaths, levels reached 17, 26.5
  and 37. Neutral, then. But the old minds died 30 times on this build and 23 on step 1's, where nothing in their path
  changed but the size of the blasts' pool: one small difference and the runs part ways, so eight seeds carry noise of
  about that size (a quarter either way). Weigh a balance change against that, and use more seeds when it matters.
- 1.0.0b15's step 3 (packs, turns, morale, the cap; harness only), against `CRAWL_MIND=0` on the same build and on sixteen
  seeds this time (1-31 odd; deep starts as before): from level 1 (100 minutes) a median of 10 levels cleared (9), 46
  deaths (48), 33 of them on level 10 (39) and 13 elsewhere (9), the knight felled 9 times (8); from 11, 21 and 31 (50
  minutes) the same medians (levels 17, 27, 37), no deaths either way, a level's median time 362-366 s (365-369). Harder
  on ordinary levels, then, as the owner asked (O3), and no harder at the knight. In the sixteen runs from level 1 the
  packs called 319 times, hunted 297, were cut short 5, took 677 turns, broke 299 times and fetched help 76; 18 roosts
  woke and 16 wakes rose. Levels hold one to four more creatures (six seeds, levels 5-25; level 13 the most). The first
  version, where each member rolled its own rank, died 62 times (48 on level 10) and felled the knight 3 times: a level
  with packs offered more chances of a high rank, and a boss's adds come at the highest of each kind met since the last
  death (the knight's first-wave shield-bearers at rank III or above in 22 runs of 28, against 14 of 26). Whatever adds
  creatures feeds the adds' ranks too. The runs also found a brood mother held by the cap laying sacs without end (one
  seed's level 33 filled all 64 places with spiderlings); fixed after them, checked on that seed (2 sacs, against 32).
- 1.0.0b15's step 4 (callers and scavengers; harness only), against the old minds' sixteen runs from step 3 (nothing on
  their path changed; and no two harness runs started at different times are alike anyway, since a run's id, which orders
  its trials, comes from the clock): from level 1 a median of 10.5 levels cleared (9), 36 deaths (48), 28 on level 10 (39)
  and 8 elsewhere (9), the knight felled 10 times (8); from 11, 21 and 31 the same medians, no deaths either way, a
  level's median time 367-373 s (365-369). From level 1 there were 301 alarms, heard 71 times, 17 scavengers set off and
  one fed; deeper, 218-417 alarms a start, heard 87-231 times, 93-149 scavengers and 10-26 meals. So callers and
  scavengers make no fight harder yet; the gap at the knight is the noisiest number here (his level's deaths have run
  from 28 to 48 between builds that don't touch his fight, whose adds come as high as before: shield-bearers at a mean
  rank of 3.2-3.3). The challenge is still to come (hunters, ambushes, cunning). One run from 11 filled the forty places
  for the dream's own rime in a fight with seven hunting it; only the newest rime is lost.
- 1.0.0b15's step 5 (hunters, the shade's stalk, the calm; harness only), against the old minds' sixteen runs (seeds 1-31
  odd): from level 1 a median of 9 levels cleared (9), 60 deaths (48), 43 on level 10 (39) and 17 elsewhere (9), the knight
  felled 4 times (8); from 11, 21 and 31 the same medians, no deaths either way, a level's median time 363-368 s (365-369).
  Hunts were let loose on 25 levels in the runs from level 1 and 35-63 levels a deeper start; 5 of the 17 deaths off the
  knight's level came on a level whose hunt was loose, about twice the rate of the rest. Hunters seldom reach the dream
  by its trail alone (once in all the runs; their pack mostly sees it first) and lost the trail twice. The shade faded
  back 60-103 times a deeper start and never struck: the dream seldom stands still, so to it the shade is now almost
  harmless; to a person who stops to look about it is not. The calm's change makes the dream's life easier: with the old
  calm put back (`CRAWL_OLDCALM=1`) the new minds died 74 times on the same seeds, 27 of them off level 10. So 60 is a
  quarter above today's 48, the edge of D5's margin; steps 6 and 7 add to it, and step 8's budget is where to give some
  back. A death count "on a level whose hunt was loose" needs the flag kept until the level changes, not reset at a death:
  the first count here said 2, not 5.
- 1.0.0b15's step 6 (lures and ambushes; harness only), against the old minds' sixteen runs: from level 1 a median of 9
  levels cleared (9), 57 deaths (48), 37 on level 10 (39) and 20 elsewhere (9), the knight felled 7 times (8); from 11, 21
  and 31 the same medians, no deaths either way, a level's median time 366-377 s (365-369). An ambush was built on 40 of
  the levels played from level 1 (275 in all the starts); there 13 sprang (7 as the dream walked in, 6 as it saw the teeth)
  and 19 went out to search because the dream shot the bait at range. Deeper, the teeth were also sprung by a gargoyle
  come near, a tooth struck, the bait slain in the room, and once a refused set piece came hunting. 4 of the 20 deaths off
  the knight's level came on a level with an ambush, about the rate elsewhere: the dream mostly kills the bait from afar
  or sees the teeth first, so the ambush is a person's trial more than the dream's. Ten of the 20 were on level 3, where
  nothing of step 6 can be (seven of them to rank-I skeletons, spread over five seeds; step 5's build had three there):
  the noise of sixteen seeds on any one level is that large. About a fifth harder in all, inside D5's margin.
- 1.0.0b15's step 7 (cunning by rank, hazards that hurt the dead, the corner throw, the dog at your back, the cap passed;
  harness only), against the old minds' sixteen runs: from level 1 a median of 9 levels cleared (9), 60 deaths (48), 43 on
  level 10 (39) and 17 elsewhere (9), the knight felled 5 times (8); from 11, 21 and 31 the same medians, no deaths either
  way, a level's median time 359-367 s (356-369). So a quarter harder, D5's edge again (step 6 had 57; the difference is
  inside the noise, and the extra fell at the knight, whose fight step 7 doesn't touch). Cunning shows by rank, so mostly
  deeper: from levels 1, 11, 21 and 31 the dead stepped aside from 278, 82, 383 and 625 spells, spread out against a splash
  in 0, 8, 34 and 391 seconds, flanked 5, 2, 12 and 38 times, feinted 2, 1, 8 and 48 times, waited by a doorway 0, 1, 9 and
  26 times and read a charge 0, 3, 4 and 29 times; throwers took cover 244, 96, 113 and 100 times and threw over a corner
  47, 17, 26 and 91 times; 25-86 fell back to their packs. Hazards harmed the dead 707 times from level 1 (172-413 deeper).
  Of 450 lairs made from level 1, 144 had one way in and were given another (about a third deeper too). The dream seldom
  stands to hold a doorway or charges an ultimate, so those two are a person's more than the dream's; the cap passed was
  tried only with `CRAWL_WELL`.
- 1.0.0b15's step 8 (the budget, the bands, the deep's combinations; harness only). The heads on a level are within about a
  tenth of the old minds' on every level (six seeds, levels 1-39), a boss's level half. Against the old minds on the same
  build, sixteen seeds (1-31 odd) from level 1: three batches of the new minds died 58, 68 and 60 times (37, 49 and 33 on level
  10; median 9 levels cleared each), two of the old 53 and 47 (37 and 36): 62 against 50 a batch, a quarter's margin all but
  used; the knight's level went from 37 deaths to 49 between two batches whose difference doesn't touch his fight. The knight was felled 6, 3 and
  4 times (old 5 and 6). From 11, 21 and 31: one death in all the new minds' runs, one in the old's. The last batch (60) is the
  build as committed, with the dream's engaged cap one less (Part G's A3, taken early since the budget alone left the margin
  spent); the two before it had the old cap. Deaths off the knight's level ran 21, 19 and 27 (old 16 and 11): ordinary levels
  are harder, as the owner asked. In the first batch eight of the 128 runs (five of the new minds', three of the old's) spent
  over 200 s within a cell of one place: a censer's marks in five (both minds; mended), a green thrower peeping from a corner
  in one (now three covers running at most), a sluice in one and one not found; before it, a stope's firedamp had held a test run the same way (mended). After the
  mends the new minds' longest such wait was 217 s once (by a sluice, near a run's end) and under 90 s otherwise; the old
  minds' 256 and 299 s (one by a sluice), untouched. Of the new pieces, from level 31: 172 nest calls, 72
  sentinel wakings, 532 times a brood's guard went back to her, 482 packs chilled by the Wraith's wail. Mixed bands placed from
  level 1 (sixteen runs): 72 watches with their dead; the others come deeper.
- 1.0.0b15's step 9 (the Whipper-In, the Sexton and the Lightless; harness only), the new minds on sixteen seeds (1-31 odd) from
  each start: from level 1 a median of 9 levels cleared, 64 deaths (44 on level 10, 20 elsewhere), the knight felled 4 times,
  against step 8's batches of 58, 68 and 60 (21, 19 and 27 off level 10): nothing new comes before 22, so the change there is the
  blocked-way mend and the noise. From 11: 2 deaths, median level 17 reached. From 21 and 31: no deaths, median levels 27 and 37,
  a level's median time 371 s (359-367 at step 8). From 21 the Whipper-In was met on 22 of 113 levels and the Sexton on 17; from
  31 the Lightless on 16 of 107, the Whipper-In on 9. In those starts the huntsman sounded his horn 116 times, cracked his whip 395
  times and lashed 6, and 56 hounds ran wild; the Sexton rang at his turns 579 times and hard 107, heard 140 times, and his bell was
  frozen 11 times; the Lightless felt the dream's lantern (it takes the Lantern of the Dead with its deep starts' cornerstones)
  1,844 times, went to a wisp or a fire 6, stood lost 4, struck 35 times and put out 2 wisps. The new kinds come mostly as
  wanderers (and the respawns), so how often each is met follows the budget's remainder; untuned. The blocked-way mend fired about
  9,300 times in the runs from level 1 and 10,000 from 31: a creature stood in its way had been common, not rare. Two of the 64
  runs (from 11) spent over 200 s within a cell of one place, 337 s on level 11 and 206 s on level 14, both levels with a sluice,
  as step 8's longest were; a rerun of the first seed didn't reproduce it (harness runs part ways by their clock-born run ids).
  Still open. ASan clean on three sessions with the new kinds (the huntsman's pack, the Sexton met, a rank V Lightless).
- 1.0.0b15's step 10 (the dream's counters; harness only), the new minds on sixteen seeds (1-31 odd) from each start: from level
  1 a median of 9 levels cleared, 56 deaths (33 on level 10, 23 elsewhere), the knight felled 7 times (step 9: 64, 44 and 20, 4);
  from 11, 21 and 31 no deaths, median levels reached 17, 27 and 37, a level's median time 356-363 s (371 at step 9). From levels
  1, 11, 21 and 31 the dream glanced back 25, 38, 50 and 39 times (finding something 9, 15, 22 and 20 times), read an ambush's
  sign 9, 11, 11 and 8 times and sent a bolt through 6, 4, 4 and 3 times, and took a corner 5, 0, 0 and 5 times (held 5, 0, 0
  and 6). Easier for the dream, as Part G meant; how much is inside the noise of sixteen seeds. Two runs (one from level 1, one
  from 31) stood within a cell of one place about four minutes (236 s on level 9, 262 s on level 32), aiming and casting a fire
  bolt about once a second at nothing it killed while its souls ran down; in both a Wraith had just been driven back nearby.
  One cause found by reading the code: a pack broken while its call was under way stayed calling, and the leaders-first rule
  aimed at its caller from any side for ever; mended (`pack_break` ends the call). Whether that was all of it isn't known yet:
  the `STALL` lines and `CRAWL_RUNID` were added to catch the next one. They caught it (7 November, at the owner's word, on
  PLAN-combat's branch): a Wraith driven back into a corridor's end, a vault's locked door at its back, where the dream's way
  out went through the vault. It tried the door for ever, and the dream, blocked by it (`update_player`'s blocker, which
  never asked whether it was a driven-back Wraith), drove it back every 1.8 s until its souls ran out; about one run in four
  from level 11. Mended with `wraith_cornered` (under *Enemies*): the three seeds that showed it run clean. ASan clean on three sessions (corners held, an ambush
  read and probed).
- 1.0.0b16's steps 1 and 2 (one line to begin with, splinters, Physical shut till the first gift, the gifts; harness only),
  the dream from level 1 for 50 minutes on eight seeds for each of the five pasts against 1.0.0b15: 248 levels cleared and 57
  deaths against 261 and 32 (without the gifts 241 and 66). Fire, Lightning and Spirit are about where they were; the
  Anatomist died 13 times (7) and the Plague Doctor 25 (6), on levels 1-7, and its poison a tick half again as hard (the
  owner's) didn't move it (28). The table is in docs/PLAN-play.md, *The checks so far*.
- 1.0.0b16's step 5 (things that want a spell, run I; harness only), the same 40 runs as steps 1 and 2's: 1,409 things set,
  1,161 answered by the dream, one wrong spell; 246 levels cleared and 44 deaths (step 2: 248 and 57; the Plague Doctor 29),
  a level a few seconds longer. That build also has the stronger poison, the wheels and the Mirror by hand.
- 1.0.0b16's step 6 (section 4.3's things and the runs' own; harness only), the same 40 runs: 252 levels cleared and 40 deaths
  (step 5: 246 and 44), about 80% of 1,437 things answered by the dream; the rolling skull is mostly lost to it.
- 1.0.0b16's step 7 (warded creatures; harness only), sixteen seeds against `CRAWL_WARD=0`: from level 1, 99 levels cleared and
  16 deaths (98 and 19), a level 13 s longer; from 21, 98 cleared (94), no deaths either way. The dream switches spells and loses
  only time; how a person fares is untried.
- 1.0.0b16's step 8 (sequences, things for fights; harness only), the same 40 runs from level 1: 250 levels cleared and 47
  deaths (step 6: 252 and 40), the Plague Doctor 29 of them (15); from level 11 the dream answers the reliquary, the relay and
  the ward-stone, and keeps the tablet's order since it goes by the plinth next in it. The fight things are a person's.
- 1.0.0b19 (PLAN-combat's C1-C4 and D1-D4, with 1.0.0b18.1 merged in) has been tested in the harness only. From level 1, the
  first 3,000 s on sixteen seeds for each of the five classes against 1.0.0b18 on the same runs: 511 levels cleared and 78 deaths
  (495 and 94); the Plague Doctor 21 deaths (52), the rest within the noise (the Sin-Eater 14 against 8, the Anatomist 18 against
  14). From level 31, 25 minutes on eight seeds a class, no deaths either way. Before the merge, half the seeds to 6,000 s: 136
  deaths (122), 82 on the knight's level (64), the knight beaten 15 times (19). The tables are in docs/PLAN-combat.md, sections 18
  and 19. Not run over the merge: the knight's level and the starts from 11 and 21. A run of the harness cut short is the same,
  line for line, as the first part of a longer one on the same seed, so a longer batch can be cut to compare with a shorter.
- 1.0.0b20 (C5 and D5: the Confessions, the classes' own and their caps; released early as 1.0.0b19.1) has been tested in the
  harness only. From level 1, the first 3,000 s on sixteen seeds for each of the five classes against 1.0.0b19 (J20): 504 levels
  cleared and 117 deaths (511 and 78), on levels 3 to 6, a third to rank I skeletons; the Confessions are most of it (on half the
  seeds the old lines back gave 47 deaths against 69), since the dream's early caster gets no health or resists from them unless
  it confesses Constitution. Kept at the owner's word. The Galvanist died 45 times (18), the worst in every batch. From level 31,
  eight seeds a class: 119 cleared (114), one death (none). The tables are in docs/PLAN-combat.md, sections 18 and 19.
- 1.0.0b21 (C6, D6 and D7: the augments, the merchant by class) has been tested in the harness only. From level 1, the first
  3,000 s on sixteen seeds for each of the five classes against 1.0.0b20 (J21): 485 levels cleared and 158 deaths (504 and 117),
  on levels 1 to 7 (in track D's half 34 of 75 to rank I skeletons); the augments are most of it, and of them the openings the dream draws at random that
  take from its one early spell (on the seeds that drew them, Firecracker 27 deaths against 4, Siphon 23 against 4). Kept at the
  owner's word. From level 31, eight seeds a class: 117 cleared (119), one death (one). Every run that reached Level of Atonement 18
  had its class's line's 300 credits. 1.0.0b20 reads a piece with one of 1.0.0b21's new stats or a class's ability as nothing
  (`piece_unpack`), so going back loses such pieces; the notes say so. The tables are in docs/PLAN-combat.md, sections 18 and 19.
- The harness isn't quite deterministic between `shot` runs that end at different times or scales
  (the same seed and input can part ways after a minute or so); compare runs made the same way.
