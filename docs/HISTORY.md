# Release by release: what each plan and release was

The opening of `CLAUDE.md` until it was slimmed (docs/PLAN-tidy.md, step 6), moved here word for word and in the order it
stood in: what each release and plan was, who settled what, on which branch, how it was tested, and where its log is.
Add a paragraph as each release goes out. `CLAUDE.md` has where the work stands today, and a table of the plans.

**1.0.0b18.1** was planned as an update of its own at the owner's word (7 November), in
[`docs/PLAN-b18-1.md`](PLAN-b18-1.md), settled in one round and built (steps 1-8, harness only) on
`claude/funny-lamport-onhkrp` from main at 1.0.0b18 while PLAN-combat's two tracks built 1.0.0b19 on theirs. At the owner's
word (8 November) it goes out with **1.0.0b19** instead: merged into `claude/inspiring-archimedes-dbj5n6` without a conflict,
J19's checks ran again over the two together (PLAN-combat's section 19), and it was released in 1.0.0b19. It mends the trial's door that shut on a person half in its doorway, and
brings the wheels at the merchant's, a reward left in a trial's room, the bell puzzles' bells told apart, a soul-lamp that bars the
Wraith from a challenge, the controller laid out by the player (View and Menu together to look at it), and props that take
half a cell or a quarter by their art. It changes none of the functions PLAN-combat's section 17.2 gives either track; its
section 10 lists what it touches beside them, and its sections 14 and 15 are the merge notes and the hand-off log: read them
first.

The next planned work (themes every five levels, wall tiles, enemies, bosses, bonus levels) is written
up in [`docs/PLAN-descent.md`](PLAN-descent.md). The creatures, their colour ranks and the order
in which creatures and decorations appear are in [`docs/PLAN-bestiary.md`](PLAN-bestiary.md),
which replaces the descent plan's section on enemies. The owner has accepted the recommended answer
to every question in both; the bestiary's status line says how far the work has got. Read both
before starting on any of it. The overhaul of items, abilities, spirit and the merchant is planned in
[`docs/PLAN-items.md`](PLAN-items.md); the owner has answered its questions, and its status line
says all seven steps are built (items as data, save version 3, everything worn felt in the game,
spirit and bones, the abilities, the merchant, the Black Altars, and the balance runs), released as
1.0.0b4 at the owner's word before they had played it; it was tested in the harness only.

1.0.0b5 was planned in [`docs/PLAN-b5.md`](PLAN-b5.md): a status bar that slides away after five
seconds (and a mini bar for pickups and souls), wisps as things in the world that fly to their target (L
opens a menu of them, a soul each), the Black Altar's blessing that makes exit wisps turn the undead, and
the console. All seven steps were built and released as 1.0.0b5 at the owner's word, tested in the
harness only. It went to main as one squashed commit, so that an early commit on the work branch that
listed the console's codes never reached main; keep it that way.

1.0.0b6 was planned in [`docs/PLAN-b6.md`](PLAN-b6.md): secrets in chains of
carved rooms and pocket areas themed as the Backrooms (levels 1-10) and Severance (11-20), Cursed
Coins, and the Rune Shrine in the merchant's room as the only source of runes, which now come by quality.
The owner answered nine rounds of questions; the plan marks the defaults taken.
Built (harness only): the hand fix (casting from rest while the bar is away), runes by quality
(step 2: Common to Exquisite, the nine Exquisite second effects, save version 5, the merchant's rune
pedestal now another piece to wear), the Rune Shrine (step 3) and the secret areas (step 4: counts by
set, chains of carved rooms, pockets, and the altar's wisp that goes to one secret and back for a price)
and the Backrooms (step 5) and Severance (step 6), their art approved. Step 7 is
built too: Cursed Coins (at a chain's end, and for every secret found), creatures in the pockets, and the
autopilot going through chains, and step 8, the checks. Released as 1.0.0b6 at the owner's word, tested
in the harness only, and squashed onto main as one commit like 1.0.0b5.

1.0.0b7 was planned in [`docs/PLAN-b7.md`](PLAN-b7.md): runs of ten with a boss on each tenth level
(the knight, the spider, the Deacon, the Wraith), their hazards, omens and waltzes; death that costs for a
person's run, the merchant's ledger in place of the graves and her claim; her hall (pedestals bought, the
Hexed Totem and the caster's boss abilities, the Accursed Portal, the Corpus Compendium, the Wretched
Repository, the grave plot) and the Codex. The owner answered two rounds of questions. All eighteen steps
were built (1-8 on `claude/magical-cori-s471gj`, 9-18 on `claude/trusting-pasteur-ceyxan`), tested in the
harness only (not under Wine), and released as 1.0.0b7 at the owner's word, squashed onto main as one
commit like 1.0.0b5 and 1.0.0b6. Still to do from the plan: the bestiary's boss pages, new art for the
Deacon and the Wraith, and tuning (the Wraith's fight runs long; the dream finds the first knight hard).

1.0.0b8 is a game controller, planned in [`docs/PLAN-pad.md`](PLAN-pad.md) (the owner took every
recommended answer): XInput loaded at run time, any button held three seconds takes over, the whole game
played by the pad, the hints in its words, rumble, the display kept awake while a pad plays, and a setting.
Built on `claude/trusting-pasteur-ceyxan`, tested in the harness with a scripted pad only (no real
controller, no Wine), and released at the owner's word, squashed onto main as one commit.

1.0.0b9 is planned in [`docs/PLAN-b9.md`](PLAN-b9.md) (the owner answered its questions the same day): a
map on a scroll bought from the merchant (M, D-pad up), the target's health with its weakest and strongest
spell, and Levels of Atonement for the caster (experience, *ASCEND* with the merchant, ten points a level,
Luck, overlevels past 50) with five cornerstones (Second Wind, Premonition, Cartographer, Lantern of the Dead,
Grave Sense), one chosen at each tenth level. All of it was built on `claude/gracious-newton-pqyd4u`, tested in
the harness only (no Wine, nothing by hand), and released at the owner's word, squashed onto main as one commit.

1.0.0b10 is planned in [`docs/PLAN-b10.md`](PLAN-b10.md): a look of its own for each run (the Order's
crypts, the drowned catacombs, the charnel mine, the black glass), with room types, materials laid by the
stretch and blended where they meet, features drawn over them, floors and ceilings, and each run's doors,
torches, keys and treasure. The owner answered the first questions; it goes one run at a time, sheets first,
then frames from the game. Run I's art is drawn (`crawl_art_run`, on `make sheets`' `crypts.png` and
`cryptrooms.png`) and approved, with the owner's defaults for the rest (the four themes as the plan has them,
her hall rebuilt once in run I's stone). Step 2 put run I in the game (walls by face, room types, floors and
ceilings, its doors, keys and torches, her hall), and its frames from levels 1, 5 and 10 were approved. Run II's
art (`catacombs_run`, `catacombs.png` and `catarooms.png`) was approved and is in the game, with the owner's
answers: the water only a look but splashing underfoot, webs from the run's sixth level, the spider's arena the
spinners' gallery. The owner then left runs III and IV to my judgement without approvals; run III (`hollows_run`,
`hollows.png`, `hollowrooms.png`) and run IV (`deep_run`, `deep.png`, `deeprooms.png`) are drawn and in the game.
Tested in the harness only, and released as 1.0.0b10 at the owner's word, squashed onto main as one commit. The
README's previews of the four runs, the crypt, the merchant and the bosses were taken again; the strips of menus,
creatures, wisps and pockets still show the old walls, and say so.

1.0.0b11 is planned in [`docs/PLAN-b11.md`](PLAN-b11.md), settled: the owner answered both rounds of its
questions (30 October). All sixteen steps were built on `claude/eloquent-carson-1x049c`, tested in the harness only
(the Windows build compiled and smoke-tested by CI on the branch, by a run started by hand), and released as 1.0.0b11 at
the owner's word, squashed onto main as one commit like the betas before it. The new art (`elements.png`, `spells.png`,
`hall2.png`) still waits for the owner's eye. The README's new strip, `preview_catacomb_alignment.png`, was taken for the
release; the map-and-Atonement strip shows the map and the target's readout as they were before. Part one: the map at a walk (the crypt runs while it is open, a holed parchment with eerie ink, walked cells apart
from seen ones, the level's numbers on the scroll), the target's weaknesses and harm as glyphs of six elements, and
sockets for the runes as they fire. Part two: spells out of the shop and into elemental lines bought with Alignment
Points at a Lampstand in a new room of her hall (the owner wrote Electric and Flame; our Frost, Poison, Spirit and
Physical were accepted), a wind-up and charging, gear that rolls the elements, Resist Spirit and bats that latch on
and sap souls, six classes with a past (the Witchfinder, the Anatomist, the Galvanist, the Plague Doctor, the
Sin-Eater, the Headsman), and a soul economy that can run dry: a vessel, motes to gather, a bell that corrupts the
next level (and shows its secrets, for more, if it was cleared before), a font for experience, levels replayed at
the Accursed Portal, and spectres on every level by Luck, slain by Spirit Mastery for souls. Blows can break a
spell's wind-up.

1.0.0b12 is a small release at the owner's word, from their first look at 1.0.0b11: Esc's *BEGIN A NEW RUN* (the
screensaver takes a class for a run it starts, so a person taking over a fresh run was never asked; a new run is a
person's, and her question comes at once) and the souls' line on the status bar moved under its figures. Built on
`claude/eloquent-carson-1x049c` on top of the old work history (a force-push to restart the branch from main was
refused), tested in the harness only, squashed onto main as one commit. Opening a ledger page from inside a pocket
may leave the pocket's state behind, as the new run did before `run_new` called `pocket_reset_level`; untried.

1.0.0b13 and 1.0.0b14 are planned in [`docs/PLAN-b13.md`](PLAN-b13.md), settled with the owner over two rounds
(30 October). It was written as 1.0.0b12's plan on `claude/pensive-bardeen-71ke4e`; since 1.0.0b12 went out as the small
release above, the owner moved it on a number, staged as its G2 (a) says: 1.0.0b13 the runs' hazards (fixed and arising,
every sample, soul-sappers from each run's sixth level, Resist Lightning), logic puzzles worked with things in the world
behind a sealed wing (a hidden message, a Black Shrine, Tab's documents), timed trials behind skull-marked doors, and a
reckoning of what is left before a person leaves a level; then the lower storey, its edges, stairs and ladders, now
**1.0.0b23** (renumbered seven times at the owner's word; a note at the plan's head says so). Being
built on `claude/epic-dirac-n8uosr` from main at 1.0.0b12, in the plan's steps, sheets before art in the game. 1.0.0b13's
steps 1-8 were built (harness only; the Windows build compiled and smoke-tested by CI on the branch, by a run started by
hand) and released as 1.0.0b13 at the owner's word, squashed onto main as one commit like the betas before it; the squash
also took out `b.txt` and `dream.txt`, debug logs that had come onto main with 1.0.0b11. The new art (`hazards.png`,
`puzzles.png`, `documents.png`, `trials.png`) still waits for the owner's eye. The README's new strip,
`preview_catacomb_trials.png`, was taken for the release (no hazard shows well in a still frame, so it has none).
The lower storey (steps 9-13, now 1.0.0b23) is not begun. See
*1.0.0b13 so far* below, and the numbers under *Known issues*.

1.0.0b14 is now the first art pass, planned in [`docs/InitialArtPass.md`](InitialArtPass.md) (31 October 2026; the
owner found many creatures basic, the skulls, the wall skulls, the imps' and gargoyles' heads "like a child's drawing",
the Drowned goofy): hand-pixelled skulls and faces in a gothic hand, **eyes bright and visible from anywhere, in any
light and fog, for every creature that has them (the Shade alone excepted)**, rank told by tatters, battle damage and
spirit vapour as well as colour, and the Drowned redrawn. Settled the same day: the owner answered A1-A17 (every creature
re-proportioned, all four bosses redrawn, every skull in the game including the bone walls), and asked for the work to go
**creature by creature, then each run's assets, each one committed, pushed and logged** so that it can be handed on between
sessions. Built on `claude/dazzling-dirac-20cr2y` and `claude/stoic-allen-sok1pe`, and merged into main, unreleased, at the
owner's word on 1 November, to be taken up again later; released, as far as it had come, in 1.0.0b14. **Its section 13 is the hand-off log: read it first.** It is
visual only, so nothing in the simulation, the saves or the autopilot's numbers may move. After the Black Deacon the owner
turned to the props and pickups of runs I and II (its section 14, with their answers): P1-P6 are done; runs III and IV's own
props (P7, P8) come next when the pass resumes, then the Wraith Unbound.

How the dead think is planned in [`docs/PLAN-ai.md`](PLAN-ai.md) (1 November; the draft written on
`claude/laughing-cori-a3jk29`, the work branch `claude/youthful-mayer-deaa11`, from main with the art pass so far):
senses and memory, packs of a kind and mixed bands, posts, patrols, callers, scavengers, hunters on the caster's trail,
lures and ambushes, cunning by rank, an encounter budget for the generator, and three new kinds for the roles. The owner
answered most of the first round the same day: it is **1.0.0b15**, ahead of the lower storey (now 1.0.0b23), with the
same minds for the dream and a person, the old ones behind `CRAWL_MIND=0`; ambushes from level 7 and a new kind of
interaction every two levels (the plan's section 23 tallies them and drafts a rollout); and the owner's own additions,
a sprint with a meter and a recovery that quickens with health, a line multiplying what a potion restores, and the
Lantern of the Dead shuttered on a long press of L. The second round settled it (1 November): the art pass waits until
this is done; new kinds (the Whipper-In, the Sexton, the Lightless); places to mend in the walls; doors that sound like
their run; `MAXMOBS` to grow. **Step 0, the caster's side, is built** (harness only; the Windows build compiled and smoke-tested by CI on the branch, by a run started by hand):
see *The sprint, PHYSIC and the lantern* below. **Step 1 is built** (harness only): the senses and memory, room for 64
creatures, doors that sound like their run, the places to mend; see *How the dead think* below. **Step 2 is built**
(harness only): posts, patrols and roamers. **Step 3 is built** (harness only): packs, their turns and morale, and the cap
on how many are in a fight at once; the file of the Order, the hounds, the roost, the wake. **Step 4 is built** (harness
only): callers (the wall skull's scream, the gargoyle's shriek, the deacon's call, the roost's cry) and scavengers (ghouls
drawn to deaths, in packs of one to three). **Step 5 is built** (harness only): the caster's trail, hunts let loose on it
(the Order's watch, hounds, hunting spiders), the shade's stalk, and the calm's change (H2). **Step 6 is built** (harness
only): lures and ambushes (kill rooms, teeth by run and room type, baits of three sorts, the gilded as bait, the signs, the
spectres' drift). **Step 7 is built** (harness only): cunning by rank (falling back, cover, dodging, doorways, flanking and
the loops it needs, spreading, feints, the charge read), hazards that hurt the dead, the throw over a corner, the dog at
your back, the cap passed for a person doing well. **Step 8 is built** (harness only): one encounter budget spent on set
pieces in place of today's placements, the rules, room types choosing, the special places, the mixed bands and the packs
still to come (the nest, the imps' brood, sentinels), and the deep's combinations, by section 23's rollout. **Step 9 is built**
(harness only): the three new kinds, their sheets approved by the owner (`whipper.png`, `sexton.png`, `lightless.png`,
`sides5.png` on `make sheets`), in the game with their ways and bestiary pages. **Step 10 is built** (harness only): the
dream's counters (a glance back, ambush signs read and a spell through the door, callers and leaders first, a corner held).
Steps 0-10 were folded into main at the owner's word on 3 November, squashed as one commit as the art pass was, and released in
1.0.0b14; the work goes on on `claude/youthful-mayer-deaa11`.
Step 11, showing it (sounds, captions, the bestiary's lines, the Codex, achievements, art), comes after the pass over the screens below, at the owner's word.

A pass over the screens is planned in [`docs/PLAN-ui.md`](PLAN-ui.md) (3 November, a draft on
`claude/gifted-rubin-q8up37`; nothing built): every menu cut in weathered marble (ledger stones, headstones, tablets),
three faces of our own at today's sizes, a kit of structured pieces (`src/art/crawl_ui.c`), the Alignment redrawn as a skill
tree with an icon to each node and a pane beside it, and the other screens after it. Visual only. Settled the same day (the
owner took every answer marked but U7, this pass whole before PLAN-ai's step 11, and U10, full screens keeping the dimmed
crypt in a 4 px margin); step 0 is done (`tools/uishots.sh`, the before sheets in `media/sheets/uipass/`), and step 1's sheet is
drawn (`src/art/crawl_ui.c`, `src/art/ui_art.c`, `tools/uisheet.c`; `media/sheets/uipass/sheet-ui.png`) and approved. Step 2 is built
(harness only; the Windows build compiled and smoke-tested by CI on the branch): every menu, card and the tally cut in the stone through `gfx_hook8/16` and `ui_on`/`ui_off` (menus lend
the floor's and ceiling's ramps for the frame; cards over the live crypt use the run's stone), with the fit checks'
`UI overflow` lines in the debug build. Step 3 is built (harness only; CI compiled the Windows build): the Alignment at the Lampstand and on Tab's
fourth page is `ui_tree`, filled by `render_align` from the line's state. Step 4 is built (harness only; CI compiled the Windows build): the cornerstones, the Totem, ascend and the class
(`ui_screen`, `ui_pane`, `ui_pane_wide`, `ui_socket_mark`). Step 5 is built (harness only; CI compiled the Windows build): Tab's pages (`tab_frame`; the wardrobe as sockets on a figure,
icons for the kinds of piece), the bestiary (`ui_niche`) and the Codex. Step 6 is built (harness only; CI compiled the Windows
build): her hall's screens (`hall_frame`, `hall_keys`, `ui_row`, `ui_scroll`). Step 7 is built (harness only; CI compiled the Windows build):
the crypt's screens, the cards as headstones (`card_box`), the tally as a memorial tablet. Step 8 is built (harness only; CI compiled the
Windows build): the HUD in the run's stone (the bar a tablet, `bevel` a well, `ui_band` behind words over the crypt). Step 9, the checks, is done
(`PAD=1` and `HOST=` for `tools/uishots.sh`; the README's menu previews taken again): the pass is built, harness only. The page has *Then and now* (`tools/gallery.sh`, `media/gallery/`).
This pass, the art pass so far and the minds' steps 0-10 were released together as **1.0.0b14** at the owner's word (3 November),
a pre-release as before with nothing pinned as Latest, squashed onto main as one commit; `docs/CHANGES-1.0.0b14.md` is the full
record of everything since 1.0.0b13, and the release notes were cut from it to what a player needs to know (no background work,
technicals or debug switches: keep later notes that way). The plans' numbers move down one: the minds' showing (PLAN-ai's step 11)
and the rest of the art pass come next.
Its section 10 is the hand-off log: read it first.

A browser build for the page is planned in [`docs/PLAN-web.md`](PLAN-web.md), settled, **built** and deployed (3 November,
on `claude/vibrant-mccarthy-9hk3fi`, squashed onto main as one commit at the owner's word; tested in headless Chromium and
the harness only, and by CI's smoke test): Emscripten and a
JavaScript shell (`src/platform/web_main.c`, `web/`) beside `win_main.c` and `tools/host.c`, at `/play/` on the page, saves in
`localStorage`. The owner took every recommended answer but W3 (Space casts in the browser, Ctrl does nothing: a few
lines behind `HVS_WEB`, the only change to the game's code) and W7 (it goes out as part of 1.0.0b14: CI uploads the web zip to that
release from main without a bump, and Pages serves `/play/` from it). The link is the *BROWSER* cell under *EVERYTHING*,
beside the download panel's small print, where the owner asked. Only 1.0.0b14 is
built and tested for the browser. Its *What was built* says what was checked and what wasn't.

The music is planned in [`docs/PLAN-music.md`](PLAN-music.md) (3 November, settled with the owner the same day; on
`claude/vibrant-keller-bdeun7` from main at 1.0.0b14 with the browser's build): a piece for each run, her hall's, the menus'
lullaby in each run's guise, a fight's layers and quickening, the failing's muffle and heartbeat, Music and Effects sliders;
the march retired. It is **1.0.0b15** (the owner, 4 November); the minds' showing and the rest of the art pass move up to
1.0.0b16, the lower storey to 1.0.0b17 (since moved on again, to 1.0.0b18, 1.0.0b19 and 1.0.0b20, by PLAN-play below and 1.0.0b18's release, and to 1.0.0b23 by PLAN-combat). Steps 1-2 (the engine and the pieces, in `src/core/snd.c` after the bosses' waltzes; `sh
tools/music.sh` renders them into `build/music/`) were sent to the owner, who said to wire them in; steps 3-4 (the game's side,
`music_frame` at the end of `scene_crawl.c`, and the sliders) and step 5's checks were done, harness only (CI compiled and
smoke-tested the Windows and browser builds on the branch), and it was released as **1.0.0b15** at the owner's word (4 November),
squashed onto main as one commit. Its *hand-off log* says where it stands.

**1.0.0b16** is planned in [`docs/PLAN-play.md`](PLAN-play.md) (4 November, on `claude/modest-knuth-gd9u51`; steps 1-8
built, harness only, with the pad's wheels, and released as **1.0.0b16** at the owner's word on 5 November, squashed onto main
as one commit; its steps 9-10, the lull's watch and its checks, were built on the same branch, harness only, and released as **1.0.0b17** at the owner's word the same day, squashed onto main as one commit), for the early levels' quiet stretches: no class starts with the fireball any more (each begins with its own line), a
second and third line given free at the Lampstand after levels 4 and 12, things in the crypt struck by the right spell (only
the lines held), creatures warded against a held line (60% to 90%, cracking), and the Mirror rune raised by hand on the right
mouse as a parry (Q turns between it and the boss gifts), a Common one free with the shrine's first curse. The owner answered
the first round the same day and put it first: the minds' showing and the rest of the art pass were **1.0.0b18**, the lower
storey **1.0.0b19** (moved on once more when 1.0.0b16 went out without steps 9-10). The minds' showing (PLAN-ai step 11, with
every creature's back, the roost and the ambushes' signs) was built on the same branch, harness only, and released as **1.0.0b18**
at the owner's word (5 November), squashed onto main as one commit; the rest of the art pass (InitialArtPass's P7, P8, the Wraith
Unbound) was then 1.0.0b19 and the lower storey 1.0.0b20; PLAN-combat (below) took 1.0.0b19-b21, so they are now **1.0.0b22** and
**1.0.0b23**. The second round (N1-N5, from the fireball's going) was answered the same day; steps 1-8 and the
wheels are built (harness only): one line each, the gifts, the Mirror by hand, things that want a spell in every run (their
sheets approved), warded creatures, sequences and things for fights. The Plague Doctor is still the weakest past in the dream's runs; its flask strikes a quarter harder.

An overhaul of harm is planned in [`docs/PLAN-combat.md`](PLAN-combat.md), settled with the owner (7 November) on
`claude/inspiring-archimedes-dbj5n6`, from a review of how damage works today (its section 1) and the owner's guide (its
appendix, as given): a status for each element on the caster and on the dead (On Fire, Chilled and Frozen, Stunned, Poisoned,
Unbound, Knocked Down), the dead weak to one element and strong against one (more with rank) and born to elements past rank I,
new attacks (the Bone Thrower, the shield-bearer's charge, the Drowned's spew), a starting sheet for each class, the Unholy
Confessions in place of the ascend screen's lines (weighted by class, out of sight), six augments for each spell line earned by
using it, and a merchant that leans to the lines held. It comes **before the art pass's rest and the lower storey, in three
releases**: **1.0.0b19** the statuses, the elements and the new attacks; **1.0.0b20** the classes and the Confessions; **1.0.0b21**
the augments and the merchant; so the art pass is now **1.0.0b22** and the lower storey **1.0.0b23**. The owner took every default
in its section 16 but a few (noted there). **It is built by two agents at once on that one branch**, track C (harm that reaches the
caster) and track D (harm that reaches the dead): **read its section 17 before touching anything**, and its tracks' logs (sections
18 and 19). Built so far (harness only): C1 to C4 (one road for harm; the caster's statuses; their elements on you; the new
attacks), D1 to D4 (the bestiary's elements, elemental creatures, the dead's statuses, the elements on your spells): 1.0.0b19's
steps are all built, and J19, the joint checks, is done (8 November: track D's half whole, track C's from level 1 for 3,000 s
only, at the owner's word, then from levels 1 and 31 again over 1.0.0b18.1, merged in at the owner's word; the numbers in both
logs, the README and the page). Released as **1.0.0b19** at the owner's word (8 November), a pre-release as before, squashed
onto main as one commit with 1.0.0b18.1's work. 1.0.0b20 is begun (8 November): C5 (the Confessions) and D5 (the classes' own); C5's contract is in (its 17.2 b and 17.3:
`conf_rank`, `conf_value`, and D5's hooks `class_own`, `class_extra`, `blow_survived`), and C5 is built and in play (the
Confessions' sheet approved; harness only); D5 is built (harness only): the classes' sheets, the killing blow, Toughness's cracks and
Restoration (both sheets approved, `media/sheets/combat/d5-cracks.png`, `d5-restoration.png`), and the owner's caps by class
(docs/BRIEF-caps.md, given to D). All of it was released as **1.0.0b19.1** at the owner's word (9 November), a point
release before J20's runs (the owner asked for b19 again; CI publishes only a version with no release, so a new number),
squashed onto main as one commit. J20, the joint checks against 1.0.0b19, is done (9 November; the Galvanist's Frost at -10 and
the buried shambler's stall mended at the owner's word first): the dream dies half again as often early on, the Confessions
giving its early caster less than the old lines did, and the owner kept the harder start. Released as **1.0.0b20** at the
owner's word (9 November), a pre-release, squashed onto main as one commit. **1.0.0b21** (C6, the augments' frame and Fire's;
D6, the merchant by class; D7, the other lines' augments) is built (harness only; every sheet approved, all of it in play), and J21,
the joint checks against 1.0.0b20, is done (9 November): the dream dies a third more often early on, drawing at random openings
that trade its one early spell's hit (Firecracker, Siphon), and the owner kept it as it is. Released as **1.0.0b21** at the owner's word
(9 November), a pre-release, squashed onto main as one commit: PLAN-combat is done. The art pass's rest (1.0.0b22) and the
lower storey (1.0.0b23) come next. Found in the
review and mended by the plan: `HURT_MAGIC` is resisted by Resist Burn whatever its element; poison is the weak element across
the bestiary (0.55 on average); skeletons' throws have no floor; flat armour grows as the blow does.

The music work is planned and logged in [`docs/PLAN-music-work.md`](PLAN-music-work.md) (9 November, on
`claude/beautiful-einstein-tzed36` from main at 1.0.0b21; it was `PLAN-music2.md` until the owner named it): the owner asked for
the game's music (not the screensaver's) to be better, with more chords, more voices and the rest of what an AdLib could do,
her hall first and then run I with its fight. The engine has the chip's tremolo and key scaling, its rhythm mode, glides, the
tracker's arpeggio, twelve new instruments, 64 voices and pieces written as text (`ScSec`). Her hall's five attempts were heard
and all placed by the owner: the habanera is her hall's (`MU_MERCHANT`; 1.0.0b15's retired), the alley cat plays there after a
perfect level (`MU_PERFECT`, `RUN.perfect`), the lament ground while the ledger is opened (`MU_TITLE`, for a title screen when
there is one), the waltz (`MU_SPECIAL`) and the nocturne (`MU_ENDING`) wait unused. Run I's five attempts, each written with its
fight, were heard and three placed: the ill-fated is run I's piece (`MU_CRYPTS`), the descent run II's (`MU_CATACOMBS`), the
estampie plays while the Lampstand or ascending is open (`MU_LAMP`); the office and the ossuary are kept, unused. 1.0.0b15's
run I and run II pieces are retired. Harness only. Merged into main, unreleased, at the owner's word (9 November), so that
a title screen can be built with the music in: `snd_set_menu(3)` is the title's piece, 5 the waltz, 6 the nocturne (the
plan's status says how). Its hand-off log is at its end: read it first.

**1.0.0b21.1, "The Title Screen Update"**, is planned and logged in [`docs/PLAN-title.md`](PLAN-title.md): read its hand-off
log first. Taking over (H held, or any pad button) now opens a title screen before anything is the person's. Its three
scenes (A Beneath, B The Walk, C The Risen) come round once per start of the program, through a 24th byte of the prefs
record (`PL.title`, `PL_BYTES_TITLE`). PRESS ANY KEY / PRESS ANY BUTTON follows the input, and any key or button counts,
but not a stick. A menu then comes into the middle while the scene goes on: CHOOSE A GRAVE (the ledger's pages as graves,
from `page_info`, which now reads a page's class from `LG_ALIGN`; `ledger_open`), BEGIN A NEW RUN (`run_new`), THE
CONTROLS (the keys' list, or `ui_pad` read-only), HELP (a page still to write). The music is the lament ground
(`snd_set_menu(3)`). 90 s untouched on the prompt gives the screen back to the dream. The art is `src/art/crawl_title.c`
(`title_draw` with a `TitleView`), drawn with a sculpt kit (`sc_*`) and the game's palette, textures and sprites. The logic
is the `TI` block in `scene_crawl.c` before "death's cost". `tools/titlemock.c` (`make build/titlemock`) drives the same art
for mock-ups, sheets and a bench. It went out with main's music pass (docs/PLAN-music-work.md), squashed onto main as one
commit, harness only. In the harness, holding H now meets the title: `CRAWL_TITLE=0` (debug) gives the dream's run straight
away, as before, and the debug teleports (`CRAWL_TP*`) want it. `tools/uishots.sh` and `tools/gallery.sh` set it.
`CRAWL_TITLESCENE=k` (0-2) picks the scene. A title frame is 8-14 ms against the crypt's 1.8. `sc_shade` keeps what it
shaded under a hash of the piece (`SCC`), and the still earth, crypt and ground are drawn once (`layer_get`, `layer_put`).
`tools/web_smoke.js` passes the title (a key, then BEGIN A NEW RUN) and judges its frame apart, under 40 ms.

**The portraits of the slain** are planned in [`docs/PLAN-portraits.md`](PLAN-portraits.md) (10 November, settled, being
built on `claude/adoring-hopper-h9ivcg` from main at 1.0.0b21.1, at the owner's word; its section 11 is the hand-off log): a portrait for each bestiary page on her walls, under a
tapestry till one is slain, then its shadow, then each rank unlocked by its slayer achievement walking in its colours on the stone
of the run it was unlocked in; the frame's colour the highest rank, a candle beside each in one of seven layouts; each rank's gold
paid once a life, at every visit to her room; the first two thirds in her room, the last in the back room; the tapestry left
tied at the side once drawn back; and the bosses' four, two faces wide, by the Accursed Portal. The owner settled every
question and the numbers (in a Sheet on claude.ai that its status links: starts of 100, 200 and 300, the Spectre's, the
Wraith's and twenty feats added, the bosses' rows, every row paying but the Wraith's fourth gem, the shade at rank V, and the
kills each rank asks lowered to 20, 25, 20, 15, 10 and 5 gilded, for the game's own achievements too). **Step 1 is built**
(harness only): the record's part, `RUN.por_*` saved as `LG_PORTRAIT` (`por_fill`, `por_guess` for an older page), a life
(`por_life` from `death_fall`, `POR_CLEARED` from `rec_cleared`), the counts lowered in `slayer_need` and `slayer_sweep` for a
record already past them; debug `PORTRAITS` lines as a run is read and at each visit to her room. **Step 2 is built** (harness
only): `src/art/crawl_portrait.c` in the game's build, the twenty-two hung at section 7's faces by `por_hang` (`POR`, `Cell.por`),
drawn by `por_view`/`por_frame` into 128-texel overlays (`POR_OV`, by columns, mirrored as the walls are) while in view, laid over
their faces by `render_walls`; each run's stone kept by `portrait_keep_runs` through `crawl_art_stone`; `CRAWL_PORTRAITS=n`,
`CRAWL_TPPORTRAIT=k[,d]`. **Step 3 is built** (harness only): the layouts dealt in `por_hang` (no neighbour alike, tapestries gathered
left along a run, `portrait_drape_side`), each flame a static light (`light_static_ph`, its phase given so the dice aren't touched;
`POR_LIGHT`, `POR_REACH`) and a glow on the wall about it (`POR.wf`, `PorFlame`; gathered per column in `render_walls`, `POR_GLOW`);
`CRAWL_PORLIGHT=light,reach,glow`, `PORTRAITS hung ...`. **Step 4 is built** (harness only): facing one (`por_facing`, `G.pt_look`; the
band's prompt in `render_shop_card`), E draws a tapestry back (`por_draw_back`, `POR.veil_t`) or opens its card (`UI_PORTRAIT`,
`update_portrait`, `render_portraitcard`, A/D through all of them), the gold (`por_gold` from `por_third`/`por_strength`, matching
`ranks.csv`; `por_take` into `RUN.treasure`), the dream's tapestry now and then (`por_dream`, no dice); `CRAWL_PORGOLD=1`, `EVENT
portrait_drawn`, `portrait_paid`. **Step 5's first half is built** (harness only): the Spectre's and the Wraith's achievements and the twenty feats
(`ACH_POR0`, `ACH_FEAT0`, bits 345-370; `FEATS`, `NPFEAT`, `por_ach`; each kind's count `RUN.por_feat`, at the end of `LG_PORTRAIT`;
`por_reckon`, `por_bump`, `por_feat_kill` from `kill_mob`; the deeds as `Mob.feat`'s `FT_*` and `Mob.feat_t`, set where each happens), the
feat a seventh gem (`POR_FEAT`) paid as rank III and hung as a medallion (`medallion` in the art), the card's and bestiary's rows, `ach_lines`;
`EVENT feat`. **Its second half is built** (harness only; the knight's mock-up seen by the owner, its name plate taken off): the bosses'
four in the plot, two faces each (`PorAt.boss`/`half`, `NPOR` 30, `por_head`, `por_boss_view`, `portrait_boss_face`, `POR_BOSSBIT` for their
tapestries, `boss_gold`, `render_bosscard`, `POR_BOSS_NAME`; `media/sheets/portraits/bosses.png`, `knight.gif`). **Step 6, the checks, is done**, and all
of it was released as **1.0.0b21.2**, "The Portraits of the Slain", at the owner's word (10 November), a pre-release, squashed onto main as one
commit, harness only (CI compiled and smoke-tested the Windows and browser builds on the branch first). The README's strip is
`media/previews/preview_catacomb_portraits.png`. Its art is `src/art/crawl_portrait.c`, driven
by `tools/portraitmock.c` (`make build/portraitmock`); the settled numbers are `docs/portraits/ranks.csv` and `bosses.csv`, written by
`tools/portrait_sheet.py` from `KINDS`.

**1.0.0b21.3, "Her Wares, Weighed"**, from the owner's first look at 1.0.0b21.2 (10 November; on `claude/adoring-hopper-h9ivcg` with main
at 1.0.0b21.2 merged in, harness only; CI compiled and smoke-tested the Windows and browser builds on the branch; released at the owner's
word the same day, a pre-release, squashed onto main as one commit; the strips' frames that showed the old cards were taken again: the
first of `preview_catacomb_items.png` and `_shrine.png`, the second of `_portraits.png`, the third of `_play.png`). *The candles* (they could not find the skeletons' portrait): a portrait's candles are cold till its tapestry
may come down (`por_lit`: `por_can_draw` or `por_is_drawn`), so a lit candle by a tapestry still hung says there is one to draw back;
cold, `por_hang` gives it no light and no glow, and the art draws no flame (`PortraitView.unlit`, `BossPortraitView.unlit`; `g_unlit` in
`crawl_portrait.c`: the wicks bare, the hung lamp's glass dark, no smoke). The debug line `PORTRAITS hung` marks a cold one `c`. Her
room's urn had stood before the skeletons' portrait (at the left of her south wall as you face it, by the arch to the plot), hiding its
sconce; it stands by the way on now, under the east wall's torch. *Her
rooms' cards* (they covered too much of the view): a person facing anything in her rooms gets a slim prompt, not a card
(`render_shop_prompt`, `shop_prompt`: the key on a cap, what it is in its colour, a note where one matters: a ware's price, gold waiting,
a gift, the shrine's takes); the dream keeps its ware cards. E at a ware, or at the map, opens its buying (`UI_BUY`: `buy_open`,
`update_buy`, `render_buy`, `G.shop_buy`, `BUY_MAP`; `buy_live`, `buy_next`, `map_buy`, `buy_worn`, `buy_stats`, `abil_count`): the ware
and its price, what is worn there now, each of the caster's numbers that would change, now and after (a copy of the gear with it worn,
`gear_take_wear`), the abilities gained and lost first, what becomes of the old piece and how the visit stands; E or Enter buys (twice
where the old would be lost, `G.shop_confirm`), Q, Space or Back holds (`shop_hold`, its words by `shop_say`), A/D the other wares and
the map, Esc closes. Two faults found on the way: `shrine_note` printed its line into itself, which this C library clears first, so the
shrine's, Totem's and Compendium's passing words were blank in the harness (on Windows, unknown); and `pk` read the word BACK as a key
anywhere (1.0.0b21's X in a menu), so a pad saw *ESC BACK* as *B X*: it is a key now only where a group begins (*BACK UNBINDS*).
Tested in the harness only: the dream from level 1 for 2,400 s line for line 1.0.0b21.1's; buying by keys and by a scripted pad (opened,
warned, bought, held, the map, too poor, round the wares); ASan clean on that and on the dream for 1,500 s; the smoke test passes.

**Touch, the mouse in menus and asking how you play** are planned in [`docs/PLAN-mobile.md`](PLAN-mobile.md) (10 November,
on `claude/funny-meitner-yn953c`, with main at 1.0.0b21.3 merged in; settled: the owner answered T1-T14 the same day):
**1.0.0b21.4**, for the browser only. **Built: the touch controls and the plate's question; the owner tried them on their Pixel 10a
and said they work well enough to publish. Released as 1.0.0b21.4, "By Touch", at the owner's word (11 November), a pre-release,
squashed onto main as one commit from `claude/brave-albattani-sqvb5n` (with the stones' art pass, tried in headless Chromium only);
the dream from level 1 for 2,400 s line for line 1.0.0b21.3's. What is still to do is the plan's *Start here* list's last paragraph.**
Part A (a pointer in the browser's menus) is not built. Step 0, the probe: `?perf`/`#perf` shows a readout of each frame's cost by
what is on the screen, `?take`/`#take` holds H after 20 s, `window.HAUNTED.probe()` the text; `tools/probe_page.py` makes `build/web`
into a page for claude.ai's private pages (the owner's test page is in the plan's log). Touch, in brief: `Finger` and the stage in
`Input`, `Scene.stage`, a held finger taking over (`app.c`, `app_present_stage`, the picture in the safe area's middle by
`app_stage_place`), `web_touch`/`web_stage`/`web_stage_frame`; in `scene_crawl.c` `TCH` and `touch_keys` (the walking stick on the
left, the view following a finger slid on the right through `touch_follow`'s 1-euro filter, the stones' taps and holds), `touch_geo`,
the stones drawn by `crawl_stage`, Esc's TURNING and SMOOTHING by touch (`PL.tturn`, `PL.tsmooth`, `PL_BYTES_TOUCH`); in
`web/haunted.js` `cfg.controls` asked on the plate, `fitStage`, `finger()` on Pointer Events. **The stones' art pass** (the owner's,
the same day; on `claude/brave-albattani-sqvb5n`, fast-forwarded from the touch branch, which is now the one to release from): each
stone a monument cut with the graves' sculpt kit (*Touch's stones* at the end of `src/art/crawl_title.c`: `TStone`, `TsInfo`, `tstone_draw`,
`tstone_ring`), weathered, broken and grown over, with places cut for what the game says (a socket, a ring for a time, a tablet with
handles, a groove), filled each frame by `crawl_stage` (`use_word` for what E would do); the sheet and the contrast check are
`build/titlemock <dir> touch` (`media/sheets/touch/`). The first private test page is kept at the owner's word (this session couldn't write to
it); the art pass is on a second, https://claude.ai/artifact/L28ZMsohLh2N4vjdx9dnuB. The plan's section 11 is the hand-off log.

**The tutorial** is planned in [`docs/PLAN-tutorial.md`](PLAN-tutorial.md) (11 November, on `claude/sweet-carson-qxvyfi` from main
at 1.0.0b21.3, with 1.0.0b21.4 merged in since; nothing built; its section 14 is the hand-off log): levels 1 and 2 of the first run
laid out the same every time from the owner's drawings, for everyone, the dream too; a paged screen that pauses the crypt and teaches
as you go (`UI_TUTOR`), on a person's first run on the computer, no skip, again from the title's HELP; the view taken and turned at a
few moments; triggers in the floor that teach or turn a person back. It is **1.0.0b21.5**. Level 1 (settled: the owner answered
T1-T19) teaches walking, a burning floor and mending, the class's spell, souls, keys and the way out; level 2 (read, its questions
T20-T24 open) an ice field taken down by a switch struck three times, a one-way gate, two keys, a secret, and the map, without which
the way out won't open while the lessons run. The drawings are `media/sheets/tutorial/level1-master.png` and `level2-master.png`, their
readings in whole cells beside them `level1-read.png` and `level2-read.png` (`tools/tutorial_read.py` reads such a drawing). The
game's walls are cells and the drawings' are lines, so thin walls became stone and doors the neck cells beside them; the drawings are
off the generator's 4-cell lanes, so each gets a lane mask drawn with it. `open_door` spends a key on the door it opens, so level 1's
vault's two gold doors are made one lock (T19 a). The prefs byte for the lessons is the 27th, after touch's. Settled (T20-T24
answered); **step 2 is built** (harness only): level 1 drawn in play, everyone's (`g_drawn`, `DRAWN`, `gen_drawn` and its kin before
`start_level`; `gen_level`'s tail is `gen_dress`, shared; `lane()` reads `g_lane` on a drawn level; `CRAWL_DRAWN=0` the generated level
back), **step 3** (level 1's burning floor, `DF`, `drawn_fire_tick`) and **step 4's sheet** of the lessons' pages (`src/crawl/crawl_tutor.c`,
`media/sheets/tutorial/pages1.png`, `pages2.png`, approved), **steps 5 and 6** (the lessons in play on level 1 and in her room after it: `TU`,
`tu_tick`, `tu_steer`, `UI_TUTOR`, `WK_MEND`, `PL.tut`, the title's HELP; `CRAWL_TUT`, `CRAWL_TUTBEAT`, `CRAWL_TUTKEY`, `CRAWL_TUTCALM`,
`CRAWL_TUTAUTO`), level 2's art sheet (`media/sheets/tutorial/level2-art.png`, `make sheets`' `tutorial2`: the rimed seal `SP_MK_RSEAL0..2`,
the gate's face `T_GATE_SEAL`, the map lying `SP_MAPFLOOR`, the ice `T_ICE` laid by `crawl_ice_at`; sent for approval) and **step 7**
(level 2 in play: `DRAWN2_MAP`, `IC` and `ice_*`, `MKK_RSEAL` and `MKK_GSEAL` that any spell answers, `IT_MAP`, the secret laid by
hand, `drawn_marks`, `drawn_clear`; `CRAWL_ICE=0`, `CRAWL_WHERE=1`) and **step 8** (level 2's lessons: the turnings a queue, `tu_move`,
`tu_hold`, `tu_lead`, nothing hurting while they hold you; `TB2_*`; the map's moment, `tu_map_taken`; `tu_exit_barred`; `g_map_kept`).
Released as **1.0.0b21.5**, "First Steps", at the owner's word (12 November), before step 9's checks ("then we will gauge the
testing need"), a pre-release squashed onto main as one commit; harness only. An older settings record, or any page in the ledger,
counts the lessons as given (`pl_read`), so a player who had played before isn't met by them. Still to do: step 9's checks (a pad and
touch through the lessons in play, the dream against 1.0.0b21.4 over many seeds), the title's HELP paging through the lessons, the
scorched floor, and the owner's look at level 2's art; the plan's log says where it stands.

**The tidy-up** is planned and logged in [`docs/PLAN-tidy.md`](PLAN-tidy.md) (13 November, settled with the owner the same
day, on `claude/peaceful-ritchie-fmeoal` from main at 1.0.0b21.5; its section 9 is the hand-off log: read it first). The owner
found code and pictures intermingled, and sessions short of context for it. Nothing of the game changes and nothing is released:
pictures go out of `docs/` into `media/`, `src/` is sorted into folders by kind, the Makefile keeps object files, the two great
files (`scene_crawl.c`, `crawl_art.c`) are cut into parts that they `#include`, and this file is slimmed to a map with the rest
moved word for word into `docs/`. Each step must leave every object the same file, byte for byte, as 1.0.0b21.5's:
`sh tools/tidy_check.sh` proves it (`all` adds the binaries, the sheets, the site and the dream's runs). The plan's section 6
says where each thing went. ("This file" was `CLAUDE.md`, where this account stood till that last step.) All seven steps were
done the same day, each proved: 289 pictures to `media/`, `src/` in six folders, objects kept (all seven binaries from nothing in
43 s, 226 s before), `scene_crawl.c` in 70 parts and `crawl_art.c` in 31, `CLAUDE.md` from 250 KB to 27. CI built the branch: the
Windows binary is 1.0.0b21.5's but for the five bytes of its link time, the browser's build the same eight files, and the win64
zip 9.6 MB where it was 73.7. Clean under the sanitizers. It waits on its branch for the owner's word to go to main.
