# The portraits of the slain

*Status: a draft (10 November 2026), at the owner's request, on `claude/adoring-hopper-h9ivcg` from main at 1.0.0b21.1. Nothing is in
the game. The mock-ups are drawn with the game's own textures, sprites, palette and light by `src/crawl_portrait.c` (not in the
game's build yet) and `tools/portraitmock.c` (`make build/portraitmock`, then `build/portraitmock <dir>`); the sheet of
achievements is `docs/portraits/achievements.xlsx`, written by `tools/portrait_sheet.py` from `KINDS` itself. The questions in
section 9 wait for the owner; each has the answer I would take marked first.*

## 1. What was asked

Her walls are bare ashlar between the torches. The owner would hang a picture of each creature there, one to a bestiary page:
covered by a dark tapestry until one of its kind is slain; then drawn back by the player to show its silhouette walking, as an
unmet rank walks in the bestiary; then, rank by rank as the slayer achievements are earned, the creature in that rank's colours,
on the stone of the run where the rank was unlocked. Each unlocked rank pays gold once a life, taken at the picture in her room
after a level is cleared, so that the record kept through death becomes something felt after it. The frame's colour tells the
highest rank unlocked (black the highest, gilded just under it); a candle in a holder lights each one, laid out differently
from its neighbours. Gold by thirds of the creatures: 250 to 1,000 a rank for the first third, 500 to 2,000 for the second, 750
to 3,000 for the last, by the rank and the creature's strength.

Asked for here: a sheet of every achievement for kills, suggestions for more, a mock-up of the frame and its background, and
the skeleton's frame with ranks I and II unlocked, each walking five seconds in turn.

## 2. How it goes, in play

- **Twenty-two portraits**, one for each bestiary page (`best_kinds`), on her four walls (section 7). Twenty of the kinds have
  slayer achievements today; the Spectre and the Wraith have none, since nothing counts as slaying them (section 5 A).
- **Veiled.** Until one of its kind is slain (any rank: `Record.kills`), an oxblood tapestry hangs over it from an iron rod,
  pleated, a border and a skull worked in gold thread, a fringe at its foot.
- **One slain.** In her room the tapestry waits for you: facing it, *E DRAW BACK THE TAPESTRY* (A on the pad). It gathers to
  the side away from the candle, is tied back a moment and goes (a dither, as the game's fades go). Behind it is the creature's
  shadow, walking on the spot and turning about exactly as the bestiary shows a rank not yet met, on bare old wood.
- **A rank unlocked** (its slayer achievement earned): the picture shows the creature in that rank's colours, its dressing and
  its eyes, standing on the floor of the run where the rank was unlocked, that run's stone behind it. With more than one rank
  unlocked it shows each in turn, five seconds each, dissolving between (`skeleton.gif`).
- **The frame** takes the colour of the highest rank unlocked: bare wood, then bone (I), green (II), blue (III), red (IV), gold
  (gilded) and black (V). Set in its foot is a gem for each rank the kind comes in, in the order the bestiary lists them, dark
  until that rank is unlocked and then in its colour. A gem whose gold is waiting twinkles, so a wall can be read from across
  the room: which have gold for you this visit.
- **The candle.** Seven layouts (section 6), dealt so that no two neighbours share one. Each candle is a real light in her room
  and lights the frame and the stone about it.
- **E at a portrait** opens its card (`card.png`): the picture up close, each rank's slain and needed, its gold and whether it
  was taken this life, waits, or isn't yet earned. E takes all that waits on that portrait at once; A/D go to the next
  portrait without walking to it; Esc leaves.
- **Paid once a life.** Each unlocked rank pays its gold once a life: on any visit to her room that follows a level cleared in
  that life. A life runs from waking after a death that costs (`death_fall`), or a new run, to the next such death. So the
  waking visit pays nothing; the first visit after clearing a level pays everything unlocked; a rank unlocked in the middle of a
  level is ready on the next visit. Her claim takes no portrait's gold and doesn't start a life.
- **The dream** leaves them alone, as it leaves the Repository: no tapestries drawn back, no gold taken. Its balance runs stay
  comparable with the builds before, line for line. A person who takes over a dream's run finds its kills already counted, its
  tapestries waiting for them.

## 3. The sheet: every achievement for kills

**The slayers.** One achievement for each rank of each kind (`slayer_need`): 60 slain of its lowest rank, then 40, 25, 15 and
8; 3 gilded; the Deep's kinds start at rank II, the shade has one rank, bats stop at IV, and spirits and egg sacs have none. Each
is named by its rank's word and the kind (*ROTTEN SKELETONS*, *COLD SKELETONS*, *BLOODY*, *BLACK*, *GILDED*; chitin's rank V is
*PALE*). 104 achievements on twenty kinds. Each cell is *slain needed: gold* by the formula in section 4.

| # | Kind | Third | Strength | I | II | III | IV | V | Gilded | All |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Skeleton | 1 | 0.43 | 60: 310 | 40: 460 | 25: 610 | 15: 760 | 8: 910 | 3: 840 | 3,890 |
| 2 | Spectre | 1 | - | - | - | - | - | - | - | - |
| 3 | Bat | 1 | 0.00 | 60: 250 | 40: 400 | 25: 550 | 15: 700 | - | - | 1,900 |
| 4 | Shambler | 1 | 1.00 | 60: 400 | 40: 550 | 25: 700 | 15: 850 | 8: 1,000 | 3: 930 | 4,430 |
| 5 | Wall Skull | 1 | 0.14 | 60: 270 | 40: 420 | 25: 570 | 15: 720 | 8: 870 | - | 2,850 |
| 6 | Shield-Bearer | 1 | 1.00 | 60: 400 | 40: 550 | 25: 700 | 15: 850 | 8: 1,000 | 3: 930 | 4,430 |
| 7 | Spider | 1 | 0.29 | 60: 290 | 40: 440 | 25: 590 | 15: 740 | 8: 890 | - | 2,950 |
| 8 | Drowned | 2 | 0.60 | 60: 680 | 40: 980 | 25: 1,280 | 15: 1,580 | 8: 1,880 | 3: 1,730 | 8,130 |
| 9 | Bone Hound | 2 | 0.28 | 60: 580 | 40: 880 | 25: 1,180 | 15: 1,480 | 8: 1,780 | - | 5,900 |
| 10 | Wraith | 2 | - | - | - | - | - | - | - | - |
| 11 | Hunting Spider | 2 | 0.44 | 60: 630 | 40: 930 | 25: 1,230 | 15: 1,530 | 8: 1,830 | - | 6,150 |
| 12 | Ghoul | 2 | 0.52 | 60: 660 | 40: 960 | 25: 1,260 | 15: 1,560 | 8: 1,860 | 3: 1,710 | 8,010 |
| 13 | Skeleton Knight | 2 | 0.92 | 60: 780 | 40: 1,080 | 25: 1,380 | 15: 1,680 | 8: 1,980 | 3: 1,830 | 8,730 |
| 14 | Brood Mother | 2 | 1.00 | 60: 800 | 40: 1,100 | 25: 1,400 | 15: 1,700 | 8: 2,000 | - | 7,000 |
| 15 | Spiderling | 2 | 0.00 | 60: 500 | 40: 800 | 25: 1,100 | 15: 1,400 | 8: 1,700 | - | 5,500 |
| 16 | Shade | 3 | 0.29 | 60: 880 | - | - | - | - | - | 880 |
| 17 | Ember Imp | 3 | 0.00 | 60: 750 | 40: 1,200 | 25: 1,650 | 15: 2,100 | 8: 2,550 | - | 8,250 |
| 18 | Deacon | 3 | 0.57 | 60: 1,010 | 40: 1,460 | 25: 1,910 | 15: 2,360 | 8: 2,810 | 3: 2,580 | 12,130 |
| 19 | Gargoyle | 3 | 0.43 | 60: 940 | 40: 1,390 | 25: 1,840 | 15: 2,290 | 8: 2,740 | - | 9,200 |
| 20 | Whipper-In | 3 | 0.71 | 60: 1,070 | 40: 1,520 | 25: 1,970 | 15: 2,420 | 8: 2,870 | 3: 2,640 | 12,490 |
| 21 | Sexton | 3 | 0.29 | 60: 880 | 40: 1,330 | 25: 1,780 | 15: 2,230 | 8: 2,680 | 3: 2,460 | 11,360 |
| 22 | Lightless | 3 | 1.00 | - | 60: 1,650 | 40: 2,100 | 25: 2,550 | 15: 3,000 | 3: 2,780 | 12,080 |

All of them, once a life: **136,260** gold (the first third 20,450, the second 49,420, the last 66,390).

**The other achievements that are kills**, today:

| Achievement | What | Now | For the portraits |
|---|---|---|---|
| *THE ILL-FATED KNIGHT*, *THE WANDERING SPIDER*, *THE BLACK DEACON*, *THE WRAITH UNBOUND* | each boss slain (`ACH_BOSS0 + 6b`) | achievements | a boss's own portrait (5 F); the sheet's second tab suggests 1,500, 2,500, 3,500 and 4,500 |
| the same, *SLAIN AT STEP 1* to *5* | each challenge step at the Accursed Portal (`ACH_BOSS0 + 6b + step`) | achievements | the boss's portrait's gems, a fifth more gold a step |
| *CUT OFF THE HEAD*, *SHEPHERD OF NONE*, *THE LEADERLESS DEAD* | a pack's leader slain first: 1, 10, 50 | achievements | none: they belong to no one kind |
| *BEFORE THE CRY*, *A HOUSE OF SILENCE* | a caller slain before it calls: 1, 10 | achievements | none, or as the feats in 5 B |

The workbook (`docs/portraits/achievements.xlsx`) has all 104 slayer rows with the formula live: change a third's starting gold,
the share each rank adds, whether strength counts, a kind's strength or the rounding on its first tab and every figure follows.
`python3 tools/portrait_sheet.py` writes it again from the code (`--grid` prints the table above, `--md` the rows).

## 4. The gold, and what it weighs

**The formula.** Gold = the third's start x (1 + 0.6 x (rank + strength)), to the nearest 10.

- *The third*: the bestiary's pages in the order met, in three. The spiderling goes with its mother, so 7, 8 and 7: Skeleton to
  Spider (levels 1-6), Drowned to Spiderling (7-14), Shade to Lightless (16-32). Starts 250, 500 and 750.
- *Rank*: I 0, II 1, III 2, IV 3, V 4; gilded 3.5, between IV and V as its frame is. A rank II is worth the same step whatever
  kind it is (the Lightless, which starts at II, begins at step 1).
- *Strength*: the kind's worth in experience (`KIND_XP`, the game's own measure) against the others in its third, from 0 for the
  weakest to 1 for the strongest. So the weakest kind's rank I pays the start exactly and the strongest kind's rank V pays four
  times it: 250 to 1,000, 500 to 2,000, 750 to 3,000, as asked. Each rank adds 60% of the start; strength adds up to 60% more.
  With strength left out (the workbook's switch) every kind of a third pays alike and rank V only 3.4 times the start (2,550
  in the last third); 0.75 a step would bring it back to four times.

**What it weighs.** A level's haul is `gear_haul`: about 700 gold on level 2, 5,500 on 10, 19,600 on 20, 42,700 on 30 and 74,800
on 40, and wares are priced against it. A death that costs leaves a person with nothing on level 1. The portraits then pay
everything unlocked after the first level cleared: with the first third whole and half the second, about 45,000, a level-30
haul in the hand on level 2; with everything, 136,260, nearly two of the Deep's. Her wares are limited to one to three a visit,
but the Corpus Compendium sells back every piece worn before at 1.2 times its price, as many as you can pay for. So a person who
dies deep will buy much of their kit back on level 2: that is the progression asked for, and it is large. If it proves too
large, the levers, in the order I would reach for them: a portrait pays only its highest rank's gold each life, not every
rank's; or one portrait's gold a visit; or the starts halved. Nothing in the dream's runs measures it, since the dream leaves the
portraits alone; it can only be judged by playing.

**The shade** has one rank, so its whole portrait pays 880. Its one achievement asks 60, the most of any rank. Q9 asks whether
it should count as its third's rank V instead (2,680).

## 5. Suggestions for more

**A. The two that cannot be slain**, so that all twenty-two portraits can open. *The Spectre*, laid to rest: only Spirit
Mastery's Spirit reaches it, and it is uncounted (`KF_UNCOUNTED`); count it apart, 5, 15 and 40 laid to rest, three gems, paid as
a first-third kind of strength 0 (250, 400, 550). *The Wraith*, driven back: 10, 40 and 100 times (`wraith_back` already knows),
and the Wraith Unbound's fall as a fourth gem; paid as a second-third kind of strength 1 (800 to 1,700). Its tapestry would come
down the first time it is driven back.

**B. A feat for each kind**: one deed particular to it, a gold medallion on a ribbon hung from the frame's foot when earned,
paid once a life as its rank III is. Those marked * are counted already in some form.

| Kind | Feat | Kind | Feat |
|---|---|---|---|
| Skeleton | *IT NEVER THREW*: 20 slain before their first throw | Ghoul | *AT ITS MEAL*: 20 slain feeding * |
| Bat | *SHAKEN OFF*: 20 latched bats shaken off * | Skeleton knight | *IN FULL PLATE*: 10 slain before shedding any |
| Wall skull | *BEFORE THE SCREAM*: 20 silenced before they scream * | Brood mother | *NONE HATCHED*: 5 slain before a sac hatches |
| Shield-bearer | *THROUGH THE SHIELD*: 20 slain after breaking the shield | Spiderling | *THE WHOLE BROOD*: 200 slain * |
| Spider | *STILL IN ITS WEB*: 20 slain before they drop | Shade | *FACED DOWN*: 10 slain while fading from your light |
| Shambler | *BACK IN THE GROUND*: 20 slain before they stand | Ember imp | *QUENCHED*: 25 slain by Frost * |
| Drowned | *NOT A DROP*: 10 slain without their burst reaching you | Deacon | *NONE RISE*: 10 slain before any rises for him |
| Bone hound | *BOTH OF THEM*: 10 pairs slain within five seconds | Gargoyle | *STILL STONE*: 10 slain within two seconds of waking |
| Hunting spider | *OFF THE CEILING*: 20 slain overhead | Whipper-In | *NO HORN*: 5 slain before his horn * |
| | | Sexton | *UNRUNG*: 10 slain with his bell never rung * |
| | | Lightless | *IN THE DARK*: 10 slain with the lantern shuttered |

**C. The whole line.** Every rank of a kind unlocked, gilded too: the frame's rosettes take gold leaf and it pays once (not a
life) twice its rank V. **D. A wall whole.** Every portrait in a third showing rank I or better: a candle in her chandelier for
each third, and a Cursed Coin each, once. **E. Born to an element.** 25 slain of the dead born to each element, six achievements
over all kinds (`Mob.elem`): the cartouche's skull takes that element's colour in its eyes, one at a time. **F. The bosses' own
portraits**, two faces wide, over the arches to the plot and the Lampstand's room: the existing boss achievements as their gems,
the gold on the workbook's second tab. **G. The pockets.** 50 slain in the Backrooms, 50 in Severance: a portrait may then be hung
with that pocket's look behind it, chosen at its card, for the joke of it.

I would take A with the portraits, and B after them as a step of its own; the rest wait for the owner's word.

## 6. The art

![The tapestry, then the frame by the highest rank unlocked](portraits/states.png)

- **The frame**, 42 by 48 texels of a wall face, the picture inside 32 by 38. A moulding five texels wide, shaded across its
  profile (lip, flat, cove, bead, the step down to the picture) by where the candle is, its bead worked into pearls; a block at
  each corner with a four-petalled rosette; a cartouche standing over its head with a skull in it; the gems in its foot. Bare
  wood is grained and wormed; black is lacquer, grey where the light catches it; gilded throws a spark now and then.
- **The picture**: the run's stone a step behind the creature, its floor going away under it, a pool of painted light about it
  and the frame's shadow at its edge. The creature is drawn as the crypt draws it: its rank's recolouring (`crawl_rank_lut`), its
  rank's dressing (`crawl_dress`), and its eyes put back after in the rank's colour and form, as `render_eyes` does. It walks on
  the spot and turns about as in the bestiary (towards you, its right, towards you, its left, 1.6 s each), and blinks.
- **The run behind it** is that run's main stone (dressed ashlar, bedded slate, rough granite, obsidian with its glowing seams)
  and first floor, made once as the game starts and kept. Her room is always drawn in level 1's palette, so the Catacombs', the
  Mine's and the Glass's stone keep their shapes but take the Crypts' tint there; the mock-ups show it so.
- **The tapestry**: oxblood with a gold border, the skull worked in it, a fringe; drawn aside to the side away from the candle,
  tied with a gold cord, then gone. `tapestry.gif` is the moment.

![Seven candles](portraits/candles.png)

- **The candles**: a sconce to its right, one to its left, a pair, a pierced iron lamp hung on a chain over it, a ledge of
  church candles under it (one gone out and smoking), a skull on an iron bracket with a candle on its crown, a stand of three on
  the floor. Flames flicker each their own way. In the game each is a static light of a third of a torch's power on its cell and
  a glow on its own face (the mock-ups add a small round glow about each flame to the crypt's own light; the game would do the
  same for those faces).
- **Resolution.** A wall face is 64 texels across. At 64 the creature is drawn at half its size, which reads from across the
  room and is blocky up close; at 128 (one branch in `render_walls` for these faces alone) it is drawn at its own size, as the
  bestiary's niche shows it. `runs.png` puts the two side by side from a cell and two thirds and from three cells. I would take
  128.

![Behind it, the run; the walls' own 64 against 128](portraits/runs.png)

![Her north wall from the middle of the room](portraits/room.png)

![The card](portraits/card.png)

The animation asked for, the skeleton's frame with ranks I and II unlocked, I on the Crypts' ashlar and II on the Catacombs'
slate, each walking five seconds, the frame green for II, II's gold waiting: `portraits/skeleton.gif`.

## 7. Where they hang

Her room is eleven cells by six; there are exactly twenty-two bare faces on its four walls once the torches, the sign, the door,
the arches and what stands against them are left out:

- **South wall** (faces looking north, the first thing seen on turning from the wares): x 10, 11, 12, 13, 17, 18, 20. The
  shrine and its candles stand before 14-16, the arch to the plot is 19, the urn stands before 20. *The first third.*
- **North wall**, behind the pedestals' arc: x 10, 11, 13, 14, 16, 17, 19 (12 and 18 the torches, 15 her sign, 20 the arch to
  the Lampstand). Seven of *the second third*.
- **East wall**, by the door: y 12, 14, 16, 17 (13 a torch, 15 the door). **West wall**, by her: y 12, 14, 15, 16 (13 the arch to
  the back room, 17 a torch; 15 and 16 are behind her). The spiderling and *the last third*.

What stands in front: the pedestals hide the north wall's lower half from the middle of the room, she sits before two of the
west's and the urn before one. Walking up to a portrait to read it is the way, and the card's A/D makes walking to each needless.
Twenty-two candles brighten her room; their power can come down until it looks as it does now.

## 8. What it touches

- **The record.** `Record.kills` and the slayer bits are there already, kept through death and with the page. New: for each
  kind and rank, the run it was unlocked in (a byte: set in `rec_kill` beside `ach_earn`, from `crawl_set(G.level)`); for each
  kind, its tapestry drawn back; for each kind and rank, its gold taken this life; and whether a level has been cleared this life.
  A ledger section of its own (`LG_PORTRAIT`, about 200 bytes), so no save version. An older page reads with each unlocked rank's
  run as the first run that rank is common in (I the kind's own, II the Catacombs, III the Mine, IV and V the Glass), its
  tapestries down where kills are counted, and every rank's gold waiting.
- **A life.** `death_fall` (a death that costs) and `run_new` clear the gold taken and the level cleared; `rec_cleared` sets it.
- **The art.** `src/crawl_portrait.c` into the build; the runs' stone and floor kept at start; an overlay a portrait (22 x 64 x 64,
  or 128 x 128 each at 128: 360 KB), drawn again only while its face is in view, its creature's frame changing four times a second
  and its flames twenty; `render_walls` takes them as overlays (`fover`), or at 128 by their own branch.
- **Her room.** `gen_shop` hangs them (the faces in section 7, layouts dealt so no neighbours match) and lights their candles.
- **The card.** `UI_PORTRAIT`: the prompt through `pk`, E and the pad's A, A/D, the gold through `RUN.treasure` with
  `SFX_COIN`, a caption (*THE PORTRAIT PAYS 440*).
- **The Codex** could gain a part, *THE PORTRAITS*, with the rule written down; the bestiary's ranks could show their gold.
- **Debug**: `CRAWL_TPPORTRAIT=k` (before portrait k), `CRAWL_PORTRAITS=r` (every kind unlocked to rank r), `EVENT portrait_drawn`,
  `portrait_paid`.

## 9. Questions for the owner

1. **The thirds.** (a) The bestiary's order, the spiderling with its mother: 7, 8, 7 (as the sheet). (b) Straight thirds, 7, 7, 8,
   the spiderling in the last.
2. **The formula.** (a) Start x (1 + 0.6 x (rank + strength)), strength by `KIND_XP` within the third. (b) By rank alone,
   start x (1 + 0.75 x rank). (c) Yours: the workbook takes any.
3. **What a portrait pays.** (a) Every rank unlocked, once a life each. (b) Only its highest rank's, once a life.
4. **When.** (a) On a visit to her room after a level cleared in that life; not on waking after a death. (b) At any visit,
   waking too.
5. **The dream.** (a) Leaves them alone: no tapestries, no gold. (b) Draws back tapestries but takes no gold. (c) Both.
6. **Resolution.** (a) 128 texels a face, the creature at its own size. (b) The walls' own 64.
7. **Where.** (a) All twenty-two on her four walls, as section 7. (b) The last third in the back room with the Totem.
   (c) A long gallery of their own off her room.
8. **The tapestry, drawn back.** (a) Gone. (b) Left gathered and tied at the side.
9. **The shade's one rank.** (a) Pays as its third's rank V (2,680), since it is all there is. (b) As the formula (880).
10. **The Spectre and the Wraith** (5 A). (a) Achievements of their own, so their portraits open. (b) Their portraits stay
    veiled; a joke in itself. (c) Twenty portraits, not twenty-two.
11. **Feats** (5 B). (a) Yes, a step after the portraits. (b) Not now.
12. **The bosses' portraits** (5 F). (a) Later, with the boss pages the bestiary still lacks. (b) With these.
13. **The card's A/D through every portrait.** (a) Yes. (b) Only E at each, walked to.

## 10. Steps, once it is settled

1. The record's new section, a life, and older pages read (harness: pages from 1.0.0b21.1 opened, a death, a new run).
2. The art in the build: the overlays, `render_walls`, the runs' stone kept; frame time in her room (it is 1.8 ms in the crypt).
3. Her room: the faces, the layouts dealt, the candles' light.
4. The tapestry and the card, the gold, the prompt in the pad's words.
5. What section 5 adds that the owner takes.
6. The checks: the dream's runs line for line against 1.0.0b21.1 on the same seeds (it leaves the portraits, so nothing may move);
   ASan; a person's run in the harness taking gold at the right visits; the README's preview strip and the page's table.

## 11. Hand-off log

- **10 November.** Drafted. Read the achievements (`slayer_need`, `ach_lines`), the bestiary's drawing, the eyes, the ranks'
  colours and dressing, her room (`gen_shop`) and the runs' looks. `src/crawl_portrait.c` draws a portrait at any scale;
  `tools/portraitmock.c` sets them in her north wall straight on with the crypt's projection (`FOCAL`, `PLANE_LEN`), textures,
  palette, colourmap and light formula (`light_static`'s), and writes `skeleton.gif`, `tapestry.gif`, `states.png`,
  `candles.png`, `runs.png`, `room.png` and `card.png` into `docs/portraits/` (`PM_SCALE=2`; `all` or one of `skeleton`,
  `veil`, `room`, `sheets`, `card`). `tools/portrait_sheet.py` writes the workbook (LibreOffice can fill its formulas'
  values: `soffice --headless --calc --convert-to xlsx`). Not looked at in the game, nor by the owner: the next step is theirs.
