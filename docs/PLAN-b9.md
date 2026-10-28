# 1.0.0b9: the map, the caster's levels and the cornerstones

**Status: settled and built (28 October 2026) on `claude/gracious-newton-pqyd4u`, tested in the harness only
(no Wine, nothing by hand); not released. The steps at the end say what each did.** The owner asked for a map bought from the
merchant, a levelling system for the caster (experience, levels bought with her, points for stats, Luck,
overlevels past 50), three abilities the owner calls cornerstones, and the health of whatever you are
looking at. This plan maps that onto the game as it stands at 1.0.0b8, says where it rubs against what is
already there, and asked what only the owner could answer. They answered the same day (below); the
questions they left alone take the recommended answer (*).

## Decided (28 October 2026)

- **G1**: the caster's level is the **Level of Atonement** (*LEVEL OF ATONEMENT 12*; *ATONEMENT 12* where it
  is short). The crypt's levels keep their word.
- **G2**: one release, 1.0.0b9.
- **M1**: one map, north up. **M2**: everything within three cells fills in, through walls too.
- **M3**: 1,000 gold, the map floating beside the merchant, on her left.
- **S1**: one cap on the total, as proposed; but an overlevel gives Luck **+1**, not +5.
- **S2**: points buy health: **+5 for each point**.
- **S3**: Spell Haste **1% for 3 points**, its cap 50% (the cap on the total was already 50%; the 30 in the
  question was the points it took at the old price).
- **C1**: five cornerstones: Second Wind, Premonition, Cartographer, **Lantern of the Dead** and **Grave
  Sense**. None is given: each tenth level brings one choice, made on a screen of their own (the **Player
  Abilities** screen, *CORNERSTONES*).
- **C2** (a): Second Wind comes back with each run of ten, and a coin restores it between. **C3** (a):
  Premonition warns of everything the place does. **O1**: the first overlevel at 55.
- T1, X1, X2 and S4 take their recommended answers.
- **New**: an eerie fanfare the moment enough experience is banked to claim a level, so it is heard in the
  middle of play; and every experience earned is shown as it comes, with its value and what it was for.

## What the owner asked for

1. **A map.** Overhead, drawn in lines, north up, moving with the caster, on a tattered scroll. M opens it
   (up on the D-pad). It fills in the walls as you go, up to three cells out from you. You can move it
   about to look at places away from you; it draws only inside the scroll. The scroll fills most of the
   screen, with about a tenth of the view showing either side. The crypt waits while it is open. Bought
   once from the merchant's wares; it, and what it does, outlast death.
2. **Levels for the caster.** Experience from what you slay (by its kind and the level it is on; creatures
   early in a chain must not pay too much, but should pay more than the level's own), and more for clearing
   secret areas and trials. Experience is exchanged for levels with the merchant, once a level is
   completed: *Ascend to the Next Level* takes you as far as the banked experience allows. Fifty levels,
   the fiftieth reached at about level 35-39 of the crypt, or 29 for someone who clears many secrets and
   trials. Levels, experience, the points spent and the cornerstones are lost at death; gear adds to the
   caster's stats.
3. **The same start for everyone**: health, damage, each resistance, spell haste, treasure bonus, movement,
   dodge, Luck and spiritual attunement.
4. **Points**: ten a level, spent on:

   | Cost | Stat | Each step |
   |---|---|---|
   | 3 | Damage | +1% |
   | 3 | Resistance | +5% to all three, or +15% to one |
   | 3 | Spell Haste | +5% |
   | 3 | Dodge | +1% |
   | 2 | Treasure Bonus | +1% |
   | 2 | Movement | +1% |
   | 1 | Luck | +0.1% |
   | 1 | Spiritual Attunement | +1% (spirit cost) |

5. **Luck**, a new stat: the chance a creature drops its tier of gold and resources. 2% for everyone at the
   start; above 5% it should show in the gold. Never on anything bought or found.
6. **Overlevels.** From level 50, every five levels a bonus of 5% past the usual cap, on one stat of the
   player's choice. Overlevelled stats get an eerie mark wherever they are shown.
7. **Cornerstones**, free, one at a time, every ten levels:
   - **Second Wind**: back from death at half health, once a run; a Cursed Coin offered to the merchant
     restores it.
   - **Premonition**: a purple zone warns of the place's harm before it lands.
   - **Cartographer**: the map comes filled in, but for secrets and keys; locked doors and secrets show as
     you pass them, without being looked at.
8. **The target's health.** Look at a creature and see its health, with the spell it is weakest to on the
   left and the one it shrugs off on the right; `--` if there is none, `?` if it isn't known yet (the
   bestiary already learns these).
9. *Later, not in this plan:* innate attributes for creatures.

## What it touches in the game as it is

- **"Level" already means the crypt's level**: LEVEL on the status bar, *LEVEL 12 COMPLETE!*, level names.
  The caster's level needs telling apart (question G1).
- **The stats already exist, on gear.** Damage, the three resists, Spell Haste, Dodge, Movement and Spirit
  Cost are worn stats (`ST_*`), summed and capped by `gear_total`: Dodge 35%, each resist 75%, Spell Haste
  50%, Movement 40%, Spirit Cost 60%, Damage +100%. Treasure Bonus is what the Treasure Hunter ability does
  (+50% at most). Health starts at 100 and only gear raises it. Luck is new. So the caster's own stats
  join the totals every spell, rune and ability already reads, and the question is how the caps work
  (S1).
- **The merchant's room comes after every level**, and after a death (you wake there before level 1), so
  *speak to her after completing the level* fits as it is: a new row in her menu.
- **Death already costs** (`death_fall`): gold, pieces, spells but the fireball, souls, bones; it keeps runes,
  the shrine, the Totem, scrip and the record. The caster's levels join what goes; the map joins what stays.
  Death costs only a person's run: in the screensaver's dream, in a pocket or after a console code, the old
  mercy holds, so the dream's caster never loses its levels.
- **D-pad up opens the wisps' menu today**, and so does Y. Moving D-pad up to the map loses nothing; Y stays
  the wisps.
- **Hazards already warn** (grit and a shadow, glowing cracks, a creeping sheen, a ring and column), and the
  void is already purple. Premonition has to add something and look different (C3).
- **What hurts a creature most** is learnt per kind and spell, three kills each (`Record.spell_kills`), and
  the record survives death. The target's readout uses exactly that.
- **The status bar is full** (LEVEL, GOLD, HEALTH, the face, SOULS, SPELL, the keys). Experience has to show
  somewhere else.

## Decide first

**G1. What to call the caster's level.**
- (a*) **Caster level**: *CASTER LEVEL 12* in full, *LV 12* where it is short; the crypt's levels keep their
  word. Experience is *EXPERIENCE*, *EXP* short. The merchant's row is *ASCEND*.
- (b) A word of the Order's own: *degrees* (*THE TWELFTH DEGREE*, *rise a degree*), so "level" means only
  the crypt's.

**G2. One release or two.**
- (a*) One, 1.0.0b9, built in the steps at the end, as 1.0.0b7 was.
- (b) The map and the target's health first as 1.0.0b9, the levels and cornerstones after as 1.0.0b10.

## 1. The map

Decided: one map, north up; what is within three cells fills in through walls; 1,000 gold, floating on the
merchant's left.

### The scroll

A parchment scroll unrolled across the view, held by two dark wooden rollers at its left and right ends,
filling the middle eight-tenths of the screen (256 of 320 pixels), so a strip of the crypt, dimmed and
waiting, shows either side. Its top and bottom edges are torn and uneven, with a few burns and a tear or two
and old stains (drawn once a level from a seed, so they hold still). Ink is a dark brown on `R_BONE`
parchment; the caster is a small red arrow.

**M1. The map's style.** The request reads two ways.
- (a*) One map: lines seen from overhead (walls as strokes along their faces, not filled blocks), north
  always up, centred on the caster and moving as you walk until you move it yourself.
- (b) Two views, switched with a key: the wireframe turning with you (your facing up), and the flat north-up
  one.

### What it shows

- **Walls**, as lines along each face between floor and stone; doors as a bar across the doorway; the
  colour of a vault's door once you have been near it; the exit once seen; the caster.
- **Things seen**: keys (in their colour) until taken, the Black Altar, a secret once opened, a chained
  room's doorway, a boss's arch.
- **Not shown**: creatures, treasure (the map is the crypt's shape, not a list), the wisps.
- In a **pocket**, the pocket's own map; the level's comes back when you do. In the merchant's room M only
  says there is nothing to map.

**M2. What "three spaces out" fills in.**
- (a*) Walls within three cells that you could see from where you stand: not through stone, so a room fills
  as you cross it and the far side of a wall stays blank.
- (b) Everything within three cells, through walls too.

### Moving it

W/A/S/D or the arrows pan it, a cell at a tap and smoothly when held; dragging with the left mouse pans too;
Space or Enter brings it back to you; M or Esc closes it. With a pad: D-pad up opens and closes it, the left
stick pans, A brings it back, B or Menu closes it. It never pans past the level's edge. One scale only: five
pixels a cell, so a 45-cell level is a little bigger than the scroll and the far corners take a pan.

### Buying it

**M3. Where she sells it, and for how much.** Decided: as (a), but 1,000 gold and floating in the air at
her left, not on a stool.
- (a*) A rolled map in a scroll case on a stool of its own beside her pedestals, there every visit until it is
  bought: 800 gold (about the first level and a half's treasure). It doesn't take a pedestal or count as one of the
  visit's wares. E looks and buys as at a pedestal.
- (b) On one of the pedestals, in place of a ware, until bought.
- (c) A row in her menu, *BUY THE MAP*.

It is kept through death and her claim (she never takes it), written on the ledger's page with the run. The
autopilot never buys it (*default*): it has no use for it, and a person who takes over the dream can.
Pressing M without one: *YOU HAVE NO MAP. THE MERCHANT SELLS ONE.*

## 2. The target's health

While a person plays, the creature nearest the middle of the view (within a few degrees of it, in sight,
within twelve cells) is the target; it stays the target for a second and a half after you look away, and
anything you hit becomes the target for three seconds. At the top of the screen, under the boss's bar if one
is up:

```
            COLD SKELETON
   FIRE  [##########.....]  FROST
               34/60
```

- **Left**: the spell that hurts it most, in that spell's colour. **Right**: the one it shrugs off most.
- `?`: nothing learnt so far says more (or less), and some spell isn't learnt for it yet.
- `--`: every spell is learnt for it and none is more (or less) than plain.
- A tie: the best found so far; if several tie, they take turns, a second each (*default*).
- A spirit (the spectre, the Wraith): its name and *NOTHING YOU CARRY TOUCHES IT*, no bar. A boss has its own
  bar and isn't shown here. Not drawn for the autopilot.

**T1.** Show the health as numbers under the bar (as above), or the bar alone? (a*) Both.

## 3. Experience

### Where it comes from

- **Kills**: the kind's worth, times the level's (1 + 0.12 a level past the first), times the rank's (I 1,
  II 1.4, III 1.9, IV 2.5, V 3.2, gilded 4). The kinds' worth, by how much trouble they are: spiderling 1,
  bat 5, wall skull 6, spider 7, skeleton and bone hound 8, shambler, shield-bearer, hunting spider and ember
  imp 12, ghoul 14, drowned and shade 16, gargoyle 18, deacon 20, skeleton knight 24, brood mother 26. The
  spectre, the Wraith and egg sacs give nothing; a boss's adds give half.
- **In a pocket**, a creature is a rank higher for each link past the first. It pays the rank it would have
  had on the level, times 1.25 on the first link and 0.1 more for each further one (1.6 at most): more than
  the level's own, never a black rank's worth for a rotten one. (This is the owner's "early in chains".)
- **Secrets**: every link found, 25 times the level's factor; a chain's end reached (the visportal or the
  cooler), 60 times it for each link in the chain.
- **A boss**: forty of its kind's worth at its level (about two levels' kills); a challenge pays it again.
- **A level cleared**: 30 times its factor.
- **X1. Exploring.** (a*) Each room first set foot in, 3 times the level's factor, so walking the whole level
  pays a little and the map's filling is matched by experience; (b) none.

No experience for treasure. The numbers are first guesses; step 9 tunes them by simulation.

### Showing it

- **Every gain is shown** (decided): a short line for each, *+12 EXP  COLD SKELETON*, *+25 EXP  A SECRET*,
  stacking at the lower left above the bar and fading after a couple of seconds; gains that come together
  (a chain lightning's three kills) each get their line. The status bar's top edge becomes a thin line that
  fills with the experience towards the next Level of Atonement, gold when there is enough for one.
- **The moment there is enough for the next** (decided): an eerie fanfare (a low choir chord, a bell, a
  falling whisper) and *THE MERCHANT WILL HEAR YOUR ATONEMENT* across the view; again for each further
  level banked.
- The tally gains an EXPERIENCE row.
- The caster's page (Tab, A/D) heads with *CASTER LEVEL 12*, the experience and what the next needs.

### How much each level needs

To reach Level of Atonement *n* needs 42 x (n-1)² experience in all (`ATONE_K`): 42 for level 2, 672 for 5,
3,402 for 10, 15,162 for 20, 35,322 for 30, 63,882 for 40, **100,842 for 50**. Past 50, each level costs what
the fiftieth did (4,074).

**As built, measured** (28 October): the screensaver's caster, over levels it cleared without a death (eight
100-minute runs from level 1, and four 40-minute runs each from 11, 21 and 31 with `CRAWL_GEAR`), earned
about 260, 230, 360, 310, 500, 480, 760, 730 and 1,000 on levels 1-9, 3,700 on level 10 with the knight,
1,000-2,200 on 11-15, 3,400-4,200 on 22-25 and 4,200-7,500 on 31-36. At the first guess of 30 x (n-1)² that
caster reached 50 at about level 30, the thorough end of the target, so the curve was steepened to 42: it now
reaches about 15 after level 10, 28 after 20, 43 after 30 and **50 at about 35**; someone who clears every chain
(half as much again) about **29**. The figures below are the plan's first estimate, kept for the record.

From eight short autopilot runs (kills by kind and rank on levels 1-7, 11-16 and 21-25, extrapolated
past 25): a crypt level L pays about 100 x L^0.95 in kills, about a quarter more with the rest. That puts a
deathless caster at about caster level 3 after level 1, 9 after 5, 15 after 10, 29 after 20, 41 after 29
and **50 at about level 36**: two or three caster levels a visit at first, one or two later. Someone who
clears every chain earns about 1.65 times as much and reaches **50 at about level 28**. Past 50, a crypt level
pays about 4,000, so an overlevel's choice comes every three or four levels. All of it estimates, checked in
step 9.

## 4. Ascending

**Her menu** gains *ASCEND* (after *SIGN THE LEDGER*). With enough experience for a level or more, or points
still to spend, it opens the caster's screen; otherwise its note says how much more is needed. Ascending takes
every level the banked experience reaches at once, keeps what is over, and gives ten points a level. A dull
bell for each level, the cat's eyes catch the light.

**The caster's screen** (`UI_ASCEND`): a row for each thing points buy: Health, Damage, Resist all, Resist
poison, Resist burn, Resist slow, Spell Haste, Dodge, Treasure, Movement, Luck, Attunement. Each row shows the step's
price and size, the caster's own, what is worn, the total and its cap. W/S choose, D or Enter buys a step, A or
Backspace takes back a step bought on this screen; Esc leaves. Points left over are kept for later (*default*);
nothing is refunded once you leave (S4). A cornerstone due opens the Player Abilities screen first (section 6).

**X2. When experience can be spent.** (a*) Only with her, as asked; (b) also from the caster's page.

**The autopilot** ascends at every visit and spends by a fixed order (Spell Haste to its cap, Resist all to
45%, then Dodge, Damage and Attunement a step each in turn), and takes Second Wind first.

## 5. The caster's stats

### The start

Everyone starts alike, as today: health 100, Damage +0%, each resistance 0%, Spell Haste 0%, Treasure +0%,
Movement 100% (3 cells a second), Dodge 0%, **Luck 2%**, Spirit Cost 0%.

### How points and gear add up

**S1. Caps.**
- (a*) **One cap on the total**, as today: the caster's own and what is worn add up, and the existing caps
  hold (Dodge 35%, each resist 75%, Spell Haste 50%, Movement 40%, Spirit Cost 60%, Damage +100%), with two
  new: Treasure +100% (Treasure Hunter alone stays at +50%) and Luck 25%. Points past a cap are wasted, and
  the screen says so before you buy. Overlevels (section 7) raise one cap at a time. Reaching every cap from
  points alone would take about 1,050 points; fifty levels give 490, so a build is a choice, and gear decides
  where points are worth spending.
- (b) Caps of the caster's own, added on top of the gear's: the most a stat can be is the two caps together
  (Dodge, say, 35% worn and 15% your own).

**S2. Health.** Not in the list of things points buy.
- (a) It grows by itself: +2 a caster level (198 at level 50; a Common robe gives 120 on level 20).
- (b) Only gear raises it, as now.
- (c) Points buy it too: 2 points for +5.
- **Decided**: points buy it, +5 for each point. It has no cap, so it is not an overlevel's line (*default*).

**S3. Spell Haste's price.** At 5% for 3 points it reaches its cap of 50% for 30 points, three caster levels,
on the first or second level of the crypt; the same 30 points buy +10% Damage. More haste is more spells, so
the two are nearly the same thing.
- (a) As asked: 5% for 3.
- (b*) 1% for 3, as Damage (150 points to the cap). **Decided**, with the cap at 50%.
- (c) 2% for 3.

**S4. Taking points back.** (a*) Never, once you leave the screen; (b) she resets them all for a Cursed Coin.

Spirit Cost on gear and Spiritual Attunement are the same stat (*default*): the screen calls the row
*ATTUNEMENT* and the caster's page keeps *SPIRIT COST*. Swiftness and Arcane Knowledge go on raising only the
worn share, as now. Armor, Restores, Quiet, Spell Effect, Thorns and the elements stay gear's alone.

### Luck

On every kill that counts, Luck is the chance of a **lucky drop**: treasure of the creature's tier (rank I a
purse, II a chalice, III gems, IV a crown, V a chest, gilded two chests), worth about 12% of what the level's
treasure comes to (`gear_haul`) times the tier (I 0.6, II 0.8, III 1, IV 1.3, V 1.6, gilded 3), and half the
time a resource beside it: a bone from the bony kinds, a potion from rank III up, otherwise an urn of souls.
It glints like a closet's loot and isn't counted in the level's treasure; Treasure Bonus adds to it. A boss's
adds never drop.

Measured over the first four levels in autopilot runs (four seeds each, 8% at first, then raised to 12% so
that it shows): 2% Luck adds about 3% to the gold, 5% about 7%, 10% about 19%; deeper, with more kills and
higher ranks, more. Luck is never rolled on a piece or sold (`ST_LUCK` is not in any kind's table). The
lucky drop comes on top of what ranks II and up drop now, which stays.

## 6. Cornerstones

At Levels of Atonement 10, 20, 30, 40 and 50 a choice falls due: the **Player Abilities** screen
(*CORNERSTONES*) lists the five, those held, and how many choices wait; you take one, free, when you like.
It opens of itself after an ascension that brings a choice, and from the caster's screen and Tab's pages at
any time. They go with the levels at death.

**Decided**: the five are Second Wind, Premonition, Cartographer, Lantern of the Dead and Grave Sense.

**C1. Five unlocks, three cornerstones.**
- (a*) Two more, so there is one for each tenth level; candidates for the owner to pick from or replace:
  - *Lantern of the Dead*: a faint light of your own, two cells round you; shades show in it, and the Wraith
    keeps a cell further off.
  - *Grave Sense*: on the map, creatures within ten cells as red marks, while the map is open.
  - *Merchant's Favour*: one more ware a visit.
  - *Spirit Well*: kills that would give a soul give a tenth more spirit, and the first spell after a kill is
    free.
- (b) The three come at 10, 20 and 30; 40 and 50 raise one already held to a second rank (Second Wind at full
  health; Premonition a second earlier; Cartographer shows keys too).
- (c) Nothing at 40 and 50 until more are designed.

### Second Wind

When a blow would kill you: you come back at half health instead, with two seconds when nothing touches you,
a wail and a white flash, and *SECOND WIND* across the view. Then it is spent.

**C2. "Once a run."**
- (a*) As the game counts runs: it comes back at the start of each run of ten (levels 1, 11, 21, 31), and
  between, a Cursed Coin offered to the merchant (a row in her menu while it is spent) restores it.
- (b) Once for the whole life; only the coin brings it back.

It works anywhere the caster would fall, pockets included (it spares the trial), and for the autopilot.

### Premonition

**C3. What it warns of.**
- (a*) Everything the place does to you, not the blows of creatures: each hazard and omen shows a purple zone
  a second before its own warning starts (so it comes a second later than it would have, with more notice);
  where an ember or a wall skull's fireball will land, from the moment it is thrown; the burst of a drowned
  one at its last quarter of health; the Ill-Fated Knight's arc and the Black Deacon's Litany (the cells he
  can see). The zone is a pulsing purple outline with a faint fill, unlike the void's solid purple. The
  autopilot reads it too: the warned cells join `hz_danger` that much earlier.
- (b) Only the hazards and omens, a second earlier.

### Cartographer

The map comes drawn whole on each level and in each pocket: walls, doors, rooms, the exit. Not drawn until
you pass within three cells of them, seen or not: a vault's door's colour, a secret's cracked wall (marked
with a small crack), a chained room's doorway. Keys show only once seen, as without it.

Cartographer needs the map (*default*): until it is bought, the card lists it as *NEEDS THE MAP* and it can't
be taken.

## 7. Overlevels

Past caster level 50, levels go on (2,910 experience each) without points. At 55, 60, 65 and every fifth after,
you choose one line (Damage, a single resist, Spell Haste, Dodge, Treasure, Movement, Luck or Attunement): it
gains 5 (percentage points; Luck 1, decided) and its cap rises as much with it, so the bonus counts whatever
the cap. The same line
can be chosen again. They go with the rest at death.

**O1. When the first comes.** (a*) At 55, five levels past 50; (b) at 50 itself.

**The mark.** Wherever an overlevelled stat is shown (the caster's screen and page, the Codex), its number is
drawn in a pale violet that breathes slowly, now and then slipping a pixel sideways as the Backrooms' creatures
do, with the dead's small eye beside it and a quiet whisper the first time the page opens.

## 8. Death, the claim, the dream

- **Death** (a person's): the caster's level, experience, points, stats, cornerstones and overlevels go; the map
  stays. The stone records the caster level reached.
- **Her claim** takes none of it (*default*).
- **The dream** never loses its levels (death costs nothing there), so a long screensaver run climbs to 50 and
  overlevels; a person who takes over keeps them until they die.
- **Saves**: a new ledger section, `LG_CASTER` (caster level, experience, points, the steps bought in each line,
  cornerstones, Second Wind spent, overlevel picks, the map), so the save's version stays 6 and older builds
  skip it. An older page loads at caster level 1 with no map.

## 9. Keys, hints and the Codex

- M opens the map (`KEY_M`, a new key), D-pad up for the pad (Y keeps the wisps). The controls card, both
  versions, gains the map; the hint lines go through `pk` as before.
- The Codex gains pages for Luck, the cornerstones (once held) and overlevels.
- The caster's page lists Luck and Treasure with the rest, and the caster's own share of each total.

## Building it

All built (28 October 2026), each step a commit on `claude/gracious-newton-pqyd4u`:

- Steps 1-2: the map and the target's health, as planned.
- Step 3: experience and its feed (lower right in the screensaver, clear of the portrait), `SFX_ATONE`.
- Step 4: *ASCEND*, the caster's screen and the Player Abilities screen (also Tab's third page). The
  screensaver's caster ascends and spends by itself in a fixed order (spell haste, damage, health twice, dodge,
  resist all to 45%, attunement, health, luck); never a person's run, whose points are theirs.
- Step 5: Luck. A lucky purse was 8% of a level's treasure at first; over the first four levels 5% Luck added
  only about 4.5%, so it is 12%.
- Step 6: the cornerstones. The Lantern's light is a flat dynamic light of 0.9 over 2.6 cells; Premonition gives
  every hazard but the knight's arc (which keeps the blade's time) its extra second.
- Steps 7-8: overlevels (Space or Y on a line, asked twice), the eerie mark, the Codex's THE CASTER'S OWN.
- Step 9: ASan clean over a long screensaver run with everything on, a pocket, the Deacon's fight with
  Premonition and a scripted session through every new screen; the Windows build compiles. Against 1.0.0b8 on
  the same eight seeds over 100 minutes from level 1, the screensaver's caster cleared a median 12.5 levels (9.5),
  died 13 times (27) and beat the knight in 6 runs of 8 (4); Second Wind saved it in six. The experience curve
  was steepened from these runs (above).

1. **The map**: `KEY_M` and the pad; seen cells and walls per level and pocket; `UI_MAP` and the scroll's art;
   panning; the map on its stool at the merchant's; `LG_CASTER` with the map's bit; the harness.
2. **The target's health**: the target, the readout, the weakness words.
3. **Experience**: its sources, every gain shown with its value, the fanfare when a level is banked, the
   bar's line, the tally's row, saved.
4. **Ascending and points**: *ASCEND*, `UI_ASCEND`, the caster's own stats into the totals (`gear_total` takes
   them from a small table the scene keeps in step), health by level, death; the autopilot's spending.
5. **Luck and Treasure**: lucky drops, the Treasure Bonus.
6. **Cornerstones**: the Player Abilities screen; Second Wind and the coin; Premonition; Cartographer;
   Lantern of the Dead; Grave Sense.
7. **Overlevels** and their mark.
8. **The Codex, the caster's page, the controls card.**
9. **Checks**: the experience curve against the targets (autopilot runs from level 1 without deaths counting,
   and with the chain-following debug player for the thorough pace), the dream's balance with levels against
   1.0.0b8 on the same seeds, ASan, the Windows build, the README, the page and the release notes.

As always, all of it tested in the harness only until the owner plays it.
