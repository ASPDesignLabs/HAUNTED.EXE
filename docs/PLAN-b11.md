# 1.0.0b11: the map at a walk, glyphs, runes in sight, and the elements

**Status: questions for the owner (30 October 2026); nothing built.** The owner asked for three adjustments (a map
that moves with the caster while the crypt goes on, the target's weaknesses as glyphs, and a sign of a rune firing)
and a review of how the caster grows: elements in place of a list of spells, an Alignment screen of elemental lines
bought with points, perhaps classes, and a soul economy that can run dry. This plan maps that onto the game as it
stands at 1.0.0b10, says where it rubs against what is there, drafts the four lines the owner left to us (Frost,
Poison, Spirit, Physical), and asks what only the owner can answer.

Each question has a recommended answer (*); a question the owner leaves alone takes it. Answers go into
**Decided** below, as 1.0.0b9's did. **Whoever builds this: build nothing until the owner's answers are written
there.** Read it with `CLAUDE.md`, [`PLAN-items.md`](PLAN-items.md) (spirit, bones, the stats) and
[`PLAN-b9.md`](PLAN-b9.md) (the map, the target's readout, Levels of Atonement). The questions are gathered at the
end so they can be answered in one go.

## Decided

*Nothing yet.*

## What the owner asked for (30 October 2026)

**Part one: adjustments.**

1. **The map moves with you.** The crypt doesn't wait while it is open; you can walk with it out. It shows where you
   have walked apart from the walls you have only seen, and what you can see ahead of you fills in as you look at it.
   It shows the level's own numbers: creatures left, an estimate of the gold still lying, secrets found and
   remaining, and the keys.
2. **Glyphs, not words**, for what a creature takes more or less from, each with a + or a - beside it, so the screen
   is less cluttered; and under its name a glyph for the harm it does, once learnt. Learnt means after five slain or
   a few hits; what it is weak to, what it shrugs off and what it does to you are each learnt apart, per creature
   and per type.
3. **A sign of the runes.** The hand is good, but something on screen should say what fired, as the boss abilities'
   sockets do, and only for a moment: when a rune fires its mark and cooldown show for three seconds; two seconds
   before it is ready again it comes back, flashing; once ready it holds solid for a second, then goes.

**Part two: how the caster grows** (items, abilities and damage; more ways to make a build of one's own).

4. **Elements.** Damage types become Elements, each carrying its own numbers for the caster (with an innate
   Efficiency for a class).
5. **An Alignment screen** in place of spells as wares: a tree for each element, where its spell is bought and
   raised. Fire is open from the start. Alignment Points come one a level. Opening a line costs three points (the
   examples say five); then two augments of two points each, bought a point at a time at half their bonus a point;
   then an ability that changes the spell; then two paths, each with its own bonuses and requirements, one locking
   out the other; then an Ultimate that changes the spell radically. Each point spent ranks the line's spell up
   one, a little at first and more in its later ranks. A new trait, **Insight**, shortens casting time.
6. **The owner's two lines**, Electric and Flame, with four abilities and ultimates of their own (section 7). The
   other lines are for us to draft: Frost (it has a spell), Spirit (Soul Drain), Poison (no spell yet: an arcing
   projectile whose first upgrade leaves residue on the floor, the next making the residue more: holding fire to
   charge a wider pool that lasts longer) and Physical (limited by ammunition, so it may grow stronger later on; it
   adds nothing to the other elements, but may turn a share of armour into a bonus or an ability).
7. **Perhaps classes**, shaping what certain spells cost and how well they work.
8. **The soul economy.** Souls never run short today; there should be a need to manage them, even to go without.

## What it touches in the game as it is

**The map** (1.0.0b9; `UI_MAP`, `update_mapmenu`, `render_map`, `map_note`):

- It pauses the crypt, as every menu does (`G.ui`). While it is open W/A/S/D and the left stick pan it, the mouse
  drags it, Space recentres it, M, Esc or Tab close it, and the status bar is hidden.
- It fills by distance only: every cell within 3.2 cells (`MAP_REACH`), through walls, marks `G.mapped` (bit 1 the
  shape known, bit 2 been near). Nothing records where you have walked or what you have looked at. The Cartographer
  sets bit 1 everywhere.
- The parchment covers the middle 240 pixels (`SCROLL_X0`-`SCROLL_X1`), top to bottom; the dimmed crypt shows in the
  strips either side.
- The level's numbers already exist, for the tally and the autopilot: `G.kills`, `G.items_got`/`G.items_total`,
  `G.value_total` (its treasure in gold), `G.links_found`/`G.nlinks`, `G.has_key[]`, and the keys still lying as
  items.

**The target's readout** (1.0.0b9, section 2; `render_target`, `weak_word`):

- Words either side of its bar (*FIRE*, *CHAIN*, *DRAIN* ...) in each spell's colour, `?` and `--`.
- Learnt per kind and **spell**, at three kills with that spell (`Record.spell_kills[kind][NATK]`, kept through
  death). Nothing is learnt from hits, and nothing records what a creature does to you.
- What creatures do to the caster is one of `HURT_BLOW`, `HURT_MAGIC`, `HURT_POISON`, `HURT_STONE` and `HURT_VOID`:
  how it is resisted, not what element it is.

**Runes** (`update_defense`, `def_trigger`, `cast_defense`): they fire by themselves. The only sign is the hand's
glow (`G.rune_glow`, `G.rune_lift`) and `SFX_RUNE`. Their cooldowns run 40, 27, 20 and 20 s by quality, 16 s
Exquisite (`def_cooldown`). The boss abilities' sockets (`render_sockets`, lower left over the status bar, for a
person only) are a 17-pixel bevel, an 11x11 mark and a line along the foot filling as it comes round; the readout
moves over beside them.

**Spells** (`crawl_gear.c`):

- Nine, each bought by tier, about the level it is sold on: Fire Bolt (the fireball you start with is its tier 0),
  Frost Shard, Chain Lightning, Bone Spear, Soul Drain, Hellfire, Seeker Wisps, Rapid Sparks and the BFS9000. Damage
  is a base of 4 to 18 times 1 + 0.25 a tier. The time between casts is a base of 0.7 to 3 s, 7% quicker a tier,
  less Spell Haste (capped at 50%). There is no casting time apart from that: the hand rises for 0.22-0.5 s and the
  spell leaves a third of the way through.
- Where tiers come from and go: the merchant's first pedestal (`POOL_ATTACK`, with a jackpot now and then), the
  Compendium's spells page (`RUN.comp_atk`), death (every spell but the fireball), her claim (a spell down a tier),
  the graves (`Stone.atk`) and the save (`gear_pack`). The autopilot buys three attack spells first (`shop_choose`)
  and picks a spell by the creature's weakness and what it can pay (`choose_spell`).
- Elements already on gear: the Fire, Frost and Lightning additions (`ST_FIRE`, `ST_FROST`, `ST_SHOCK`, `hit_mult`:
  that share of every spell again, weighed by the creature's weakness to Fire Bolt, Frost Shard and Chain
  Lightning). The resists are to poison, burning and slowing, not to elements.
- Nothing hurts a creature over time but Hellfire's burning floor (`Burn`: 2.5 s, eight at once). Frost Shard slows
  (`Mob.slowed`) and Frost Nova freezes (`Mob.frozen`). Nothing poisons a creature.
- Holding the button repeats the cast. Nothing charges.
- Chain Lightning already leaps: two to four times, within 3.5 cells, a quarter less each time.

**Spirit** (PLAN-items): the souls, one to six a kill by rank (three times as many under Soul Reaping). A cast above
tier 1 costs its damage over 150 (0.1 to 1), less Spirit Cost; tier 1 and below are free; with too little, a spell
goes out at tier 1. Soul Drain's healing costs half a soul a point, down to a quarter. Spending: wisps a soul each,
the Black Altar 10, 25 and 40, the BFS9000 40 to 100. At 1.0.0b4 the autopilot netted 14 to 71 souls a level on
levels 1 to 8, and of about 10,700 casts 51 went out weak. Nothing caps what is carried, so souls pile up.

**Points** (1.0.0b9): ten a Level of Atonement, spent on twelve lines (health, damage, the resists, spell haste,
dodge, treasure, movement, luck, attunement), lost at a death that costs, with a cornerstone at each tenth level.
Alignment Points would be a second kind of point.

**Saves**: the record is packed by `NATK` (`REC_BYTES`), and the gear carries spell tiers, so taking spells out of
the shop changes both: this is save version 7. New things of the caster's own go in ledger sections, as
`LG_CASTER` did.

## Decide first

**G1. One release or two.**
- (a*) One, 1.0.0b11, as asked, built in the steps at the end. Part one comes first and stands on its own, so it
  could still go out alone if part two runs long.
- (b) Part one as 1.0.0b11 (the map, the glyphs, the runes' sockets, and the elements the glyphs need); the
  Alignment, the classes and the souls as 1.0.0b12.

Part one is small; part two is about the size of 1.0.0b7 (a new spell, six trees, twelve ultimates, classes, a new
economy, the autopilot taught all of it). The glyphs need the six elements either way (section 4),
so the elements come in part one.

---

# Part one

## 1. The map at a walk

M opens the scroll as now, but the crypt goes on: creatures move, missiles fly, the level's clock runs. You walk,
turn, open doors and cast as in play. The map stays centred on you as you go, sliding after you, north up as now.
Other screens (Tab, B, L, Esc) close it as they open.

**M1. Panning, now that the keys walk.**
- (a*) The arrow keys pan it (a cell a tap, smoothly while held); it drifts back to you three seconds after the last
  pan, or at once on Space. The pad doesn't pan: its sticks walk and turn. W/A/S/D, the mouse and the pad play as
  they do without the map.
- (b) Two ways in: a tap of M is the map at a walk, following you; M held half a second is today's scroll, the
  crypt waiting, panned with W/A/S/D as now.
- (c) No panning: it always follows you.

**M2. How much of the crypt shows.** The scroll covers three quarters of the view, and with the crypt running
something can come at you behind it.
- (a*) The scroll as now, but its bare parchment thinned to every other pixel (as `dim_view` thins the crypt behind
  a menu), so the crypt shows through faintly; the ink, the marks and the numbers stay solid. The status bar comes up
  over its foot when you are hurt or a spell is chosen, as it does without the map.
- (b) Solid, as now: the strips either side are all you see.
- (c) A smaller window on the same scale across the top half of the view, the crypt below it.

**Seen and walked.** Two new marks for each cell in `G.mapped`:

- **Walked** (bit 8): every cell the caster's body passes through.
- **Seen** (bit 4): the cells the view's rays cross before they meet a wall, out to where the fog closes, and the
  wall faces they meet. The raycaster walks those cells for every column already, so it costs a store a cell. A shut
  door stops sight; an open one doesn't. A shut secret still looks like stone (`map_hidden`).

Walls are inked as now, wherever a known cell meets stone. A walked cell's floor is marked (M4), so where you have
been reads apart from what you have only seen.

**M3. What fills in.**
- (a*) What you see ahead, and a ring of 1.5 cells round you through walls (so a corner you brush past shows). The
  3.2-cell reach through walls goes.
- (b) What you see ahead, with the 3.2-cell reach through walls kept (the owner's answer in 1.0.0b9).
- (c) What you see ahead only.

**M4. How a walked cell looks.**
- (a*) A wash: its floor a shade darker on the parchment, with a faint hatch.
- (b) A dotted trail of footsteps through the middles of the cells walked.
- (c) Both.

With the Cartographer the whole level is drawn on arrival, as now; the walked cells then show what is left to see.

**The level's numbers**, in a band of ink across the scroll's foot, above the hint:

```
   CREATURES 14     GOLD ABOUT 1,400     SECRETS 2 OF 5     KEYS [gold] [silver] [crimson]
```

- **Creatures**: those the tally counts, still alive, the buried risers and the gargoyles on their plinths among
  them; never the spectres, the Wraith, egg sacs, a boss or its adds. An exact count.
- **Gold**: the level's treasure not yet taken (M5). What creatures may drop isn't in it, as it isn't in the tally.
- **Secrets**: links found of all of them, as the tally counts (`G.links_found` of `G.nlinks`).
- **Keys**: a mark for each colour the level has: solid if you hold it, an outline if it is still lying, none if the
  level has no such key.
- In a pocket: its own creatures and gold, and the chain's links found.

**M5. "An estimate."**
- (a*) Rounded, *ABOUT 1,400* (to the nearest hundred; fifty under a thousand), with your Treasure bonus in it, so it
  is what you would get.
- (b) Exact.
- (c) A share: *MOST OF IT*, *ABOUT HALF*, *A LITTLE*, *NONE*.

**M6. Who sees the numbers.** (a*) Everyone with the map. (b) Creatures left only with Grave Sense, the gold only
with Treasure Hunter or Treasure bought with points.

Unchanged: the map is bought from her and kept through death; the dream never opens it.

## 2. Glyphs on the target

```
                    COLD SKELETON
                         [bone]                       its own harm, once learnt
   [bolt]+ [bone]+  [##########......]  [flake]- [drop]- [wisp]-
                        (34/60 in the bar, as now)
```

- **Six glyphs**, 7 by 7 pixels, one an element (section 4), each in its element's colour, drawn as the
  cornerstones' and the boss abilities' marks are: a flame, a six-armed flake, a forked bolt, a drop with a bubble in
  it, a wisp's teardrop flame, a bone.
- **Left of the bar**: the elements it takes more from, the most nearest the bar, a + after each. **Right**: those it
  takes less from, a - after each; an element that does nothing to it at all is struck through. Up to four a side,
  the most any creature has.
- A dim **?** at the outer end of a side while any element is still to be learnt on this kind. When every element is
  learnt and none differs, that side is empty (today's `--`).
- **Under its name**, centred, the glyph of the harm it does, once learnt; a dim ? before that; nothing for a
  creature that does no harm of its own (the deacon calls others to fight).
- Spirits and bosses as now: a spirit's line says nothing you carry touches it; a boss has its own bar.
- The bestiary's pages and the Codex say the same in glyphs (the Codex gains a page on the elements).

**T1. The signs.**
- (a*) One sign each, as asked: + more, - less, struck through for nothing.
- (b) Two for the strong ones: ++ at 1.5 times or more, -- at half or less.

**T2. What learning takes.** Per kind and element, kept in the record through death, as now.
- (a*) What an element does to a kind is learnt after five of that kind are slain by it, or twelve of its hits land
  on that kind, whichever comes first (so an element it shrugs off, which seldom deals the killing blow, is learnt
  too). What the kind does to you is learnt after it has hurt you three times. Whether an element hurts it more or
  less is one fact, so the three things learnt apart are: what hurts it more, what hurts it less, and what it does.
- (b) As (a), but only kills teach the elements.
- (c) A reading of its own: weaknesses by five slain, resistances by a few hits landed, its harm by a few hits taken
  (say how many each).

An old record's kills by spell carry into their elements (Fire Bolt's and Hellfire's into Fire, and so on), so a
kind learnt at 1.0.0b10 with three kills needs two more with that element at most. `CRAWL_LEARNT=1` learns all of it.

## 3. Runes in sight

As asked:

- When a rune fires, its socket comes up: a 17-pixel bevel like the boss abilities', the rune's 11x11 mark in the
  rune's colour, the bevel's edge in its quality's colour, and a line along its foot that fills as the cooldown comes
  round. It shows for three seconds, then slides away.
- Two seconds before the rune is ready it comes back, flashing (four times a second). The moment it is ready it holds
  solid, its line bright, for a second; then it goes.
- Several at once stand in a row, the newest outermost, and a socket keeps its place while it shows.
- In the void (silenced) a socket shows struck through, as the boss sockets do, and no rune fires.

Nine marks to draw: Bone Ward (crossed bones), Blink (an eye and a step), Mirror Veil (a hand mirror), Mend (a
stitched heart), Purify (a drop in a ring), Frost Nova (a burst), Dread (a skull), Time Slip (an hourglass), Soul
Reaping (a scythe).

**R1. Where.** (a*) The lower right, over the status bar (or in the corner when the bar is away), mirroring the boss
sockets at the lower left. (b) Beside the boss sockets, one row from the left. (c) The upper left.

**R2. Its name.** (a*) The mark only. (b) The rune's name in small letters under it for its first three seconds.

**R3. The dream.** (a*) For a person only, as the boss sockets are. (b) In the dream too, for the screensaver.

**R4. While a rune works** (the Bone Ward's bones, the Mirror Veil, Time Slip and Soul Reaping last 3 to 30 s).
(a*) Not shown: the hand and the crypt show them already. (b) The socket stays up, glowing, while it lasts.

---

# Part two

## 4. The elements

Six. Each spell belongs to one; creatures take more or less from each; each creature's harm is one of them; and
each carries a set of numbers for the caster.

| Element | Colour | Glyph | Its spell | By nature | Paid in | What deals it to you |
|---|---|---|---|---|---|---|
| Flame | `R_FLAME` | a flame | Fire Bolt (the fireball) | burns | spirit | wall skulls, ember imps; Hellrent |
| Frost | `R_GLOW` | a flake | Frost Shard | slows | spirit | the Wraith's touch |
| Electric | `R_GOLD` | a bolt | a bolt that strikes at once (E2) | strikes at once | spirit | nothing yet |
| Poison | `R_MOSS` | a drop | the Blight Flask (new, section 8) | sickens over time | spirit | spiders and hunting spiders; the drowned's pool, bile |
| Spirit | `R_PURPLE` | a wisp | Soul Drain | heals as it hurts | spirit | shades; the Deacon's Litany |
| Physical | `R_BONE` | a bone | Bone Spear | pierces | bones | the rest: thrown bones, blows, bites, claws, falling rock |

Electric moves from blue to gold (Chain Lightning shares `R_GLOW` with Frost today; Rapid Sparks is already gold).

**E1. The words in the game.** The owner's lines say Flame and Electric; the gear already says FIRE and LIGHTNING
(*of Cinders*, *of Storms*).
- (a*) FIRE, FROST, LIGHTNING, POISON, SPIRIT, PHYSICAL.
- (b) FLAME, FROST, ELECTRIC, POISON, SPIRIT, PHYSICAL.

(This plan uses the owner's words for the lines either way.)

**E2. The spells left over.** One spell a line, as asked, leaves four.
- (a*) Hellfire's burning floor becomes Firestarter's (section 7); Chain Lightning's leaps become Arc's, so the
  Electric spell strikes one creature until then; Rapid Sparks goes, its quickness now Insight's; Seeker Wisps
  become the Spirit line's ultimate (section 8); the BFS9000 stays apart, the one spell the merchant still sells,
  as rare as now, for souls.
- (b) A line has two spells, the second opened by its ability: Fire Bolt and Hellfire, the bolt and Rapid Sparks,
  Soul Drain and Seeker Wisps.

**What creatures take, by element.** A first table for the owner to change. Flame, Frost, Electric, Physical and
Spirit are today's Fire Bolt, Frost Shard, Chain Lightning, Bone Spear and Soul Drain columns unchanged. Poison is
new: the bony and the stone take little, the living most, the spiders least of their own venom. Spectres and the
Wraith still take nothing.

| Kind | Its harm | Physical | Flame | Frost | Electric | Poison | Spirit |
|---|---|---|---|---|---|---|---|
| Skeleton | Physical | 1.25 | 1 | 0.75 | 1.25 | 0.5 | 0.75 |
| Bat | Physical | 1 | 1 | 1.5 | 1.25 | 1.5 | 1 |
| Wall skull | Flame | 1 | 1 | 1 | 1 | 0.5 | 1 |
| Shield-bearer | Physical | 1.5 | 0.75 | 1 | 1.5 | 0.5 | 1 |
| Spider | Poison | 1 | 1.25 | 1.5 | 1 | 0.5 | 1 |
| Shambler | Physical | 1 | 1.5 | 0.75 | 1 | 0.75 | 1.25 |
| Drowned | Physical (its burst Poison) | 1 | 1.5 | 0.5 | 1.25 | 0.75 | 1 |
| Bone hound | Physical | 1 | 1 | 1.5 | 1.25 | 0.5 | 0.75 |
| Hunting spider | Poison | 1 | 1.25 | 1.5 | 1 | 0.5 | 1 |
| Ghoul | Physical | 1 | 1 | 1.5 | 1 | 0.75 | 1.25 |
| Skeleton knight | Physical | 1.5 | 0.75 (1 bare) | 1 | 1.5 | 0.25 | 1 |
| Brood mother | Physical | 1 | 1.5 | 1 | 1.25 | 0.5 | 1 |
| Spiderling | Physical | 1 | 1.5 | 1.5 | 1.5 | 0.75 | 1 |
| Shade | Spirit | 0.75 | 1.25 | 0.5 | 1 | 0 | 1.5 |
| Ember imp | Flame | 1 | 0.5 | 1.5 | 1 | 1 | 1 |
| Deacon | none (he calls others) | 1.25 | 1.25 | 1 | 1 | 0.75 | 0.75 |
| Gargoyle | Physical | 1.5 | 0.5 | 0.5 | 1.5 | 0 | 0 |

**E3. A creature with two harms** (the drowned's grab and its burst; a spider's bite poisons and slows).
(a*) One glyph, its commonest harm, as in the table. (b) Up to two, side by side.

**The caster's numbers for each element** (the owner's "each element carries its own set of stats"):

- **Rank**: its spell's level; each point spent in its line is a rank (A2).
- **Power**: more damage of that element (the Masteries, a Vow).
- **Efficiency**: less spirit for its spell (for Physical, bones; E4).
- **Insight**: less casting time for its spell.
- **Addition**: a share of it added to every other spell (Electric Mastery's 20%; the gear's Fire, Frost and Lightning
  additions are this already).
- **Innate**: what the Vow gives (section 9).

They are shown on the Alignment screen and as rows on Tab's totals page.

**E4. What Efficiency means.** The owner names three things a line changes: casting time (Insight), damage and
spirit cost; the Masteries give damage.
- (a*) Spirit: each point of a line's Efficiency takes that share off its spell's spirit cost (for Physical, a chance
  that a spear spends no bone).
- (b) Damage: its spell does that much more for the same spirit.

**How the general and the elemental numbers meet** (*default*): they multiply. A cast's spirit is its cost times (1 -
Spirit Cost) times (1 - Efficiency), never under 0.1. Its time is its cycle divided by (1 + Spell Haste) and times
(1 - Insight), never under 40% of its base. Damage (worn and bought) and Power add to each other.

**E5. The caster's resists.** No creature deals Electric, and today's resists are to poison, burning and slowing
(which a grab and frost both do).
- (a*) As they are: Armour for Physical, Resist Burn for Flame, Resist Poison for Poison, Resist Slow for any slowing;
  Spirit and Electric have none (the void already can't be resisted). The glyph under a creature's name tells you
  which of them matters.
- (b) Six resists, one an element, Resist Slow becoming Resist Frost, and the gear's and points' lines renamed;
  Electric's waits for something that deals it.

**E6. Gear and the elements.**
- (a*) Gear keeps its stats this release: the general ones count for every element; the Fire, Frost and Lightning
  additions stay; Poison and Spirit additions come only from the Masteries.
- (b) Pieces may roll an element's Power, Efficiency or Insight too (new stats and names, *of Kindling* and so on).

## 5. Spells as lines: strength, rank, casting time, charging

**A spell's strength.** A tier today is about the level it was bought on, so damage keeps up with creatures, whose
health grows 4% a level. Points come at about one a level (A3), and a caster spreads them over lines, so a rank
can't carry that alone.

**A1. Strength.**
- (a*) A spell's strength follows the Level of Atonement, as a tier followed the crypt's level, and its rank adds to
  that: +4% a rank at first, rising to +8% a rank from rank 12 ("a little each, more in its later levels"). A line
  opened late still hurts, and a line fed is clearly stronger. Tuned so that a caster with one line full and a second
  half full keeps pace with 1.0.0b10's dream on the same seeds.
- (b) Rank alone, on a steeper curve: a line opened late is weak until it is fed.

**A2. What a rank is.**
- (a*) A line just opened is rank 1, and every point spent in it after the opening is a rank more (a line taken all
  the way is rank 17 to 25, by its path; table in section 8).
- (b) Every point, the opening's included: a line just opened is rank 5.

**Casting time.**

**K1. What Insight shortens.**
- (a*) Its spell's whole cycle (the hand's rise and the wait before the next cast), as Spell Haste does.
- (b) A wind-up spells gain: the hand gathers a spell for 0.3 to 0.6 s before it goes, and Insight shortens that;
  the wait between casts stays Spell Haste's.

The owner's Insight comes to -30% before the paths and -40% more on Electric's second path: 0.85 x 0.85 x 0.8 x 0.8 =
0.46 of the time, before Spell Haste. Hence the floor above: never under 40% of the base.

**Physical** (*defaults*, from the owner's notes): the spear costs bones only, never spirit; its rank's curve climbs
a little steeper past rank 10; nothing in its line adds to the other elements; armour can become its damage
(section 8).

**K2. Holding to charge.** Amperage, Firewall and Poison's wider pool are charged by holding the button; today
holding repeats the cast.
- (a*) Once a line has a charging ultimate, holding its spell's button charges it: a ring closes round the hand over
  a second, the hand trembles, and letting go releases it. A tap (let go within a quarter of a second) casts the
  ordinary spell. Repeating by holding is lost for that spell only. The pad's RT the same.
- (b) Charging on a button of its own (the right mouse is the boss abilities', so it would need a new key).
- (c) The ultimate replaces the spell: every cast of it is charged.

## 6. Alignment: points, the screen, death

**A3. Where Alignment Points come from** ("one a level").
- (a*) One with each Level of Atonement claimed from her (*ASCEND*): about 14 by the knight, 27 by the spider, 42 by
  the Deacon and 49 at level 35 at the dream's pace (1.0.0b9's curve). Past 50, one with each overlevel's choice.
- (b) One for each level of the crypt cleared for the first time in this life: 10 by the knight, 40 by the Wraith.
- (c) Both.

**A4. Opening a line.** The owner's words say three points, the examples five. (a*) Five. (b) Three.

**A5. Where they are spent.**
- (a*) With her, as *ASCEND* is: a row in her menu, *ALIGN*, opens the screen. Tab gains it as a page to look at, not
  to buy from.
- (b) Anywhere, from Tab.
- (c) At a thing of its own in her hall (say, six iron bowls on a stand, one burning for each line open).

**The screen.** Across its top, the six elements' glyphs, each with its rank, lit if open, and the points to spend.
A/D (the bumpers) choose an element; below, its tree, from the top down:

```
  [flame] 7   [flake] -   [bolt] 12   [drop] -   [wisp] -   [bone] -          3 POINTS

                                 ELECTRIC   RANK 12
                                     (OPEN)
                       (EFFICIENCY 2/2)    (INSIGHT 2/2)
                                  (IONIZATION)
                         /                          \
              (EFFICIENCY II 1/2)               (INSIGHT II)       chained: the path not taken
              (ELECTRIC MASTERY)                (ELECTRIC MASTERY)
              (ARC)                             (AMPERAGE)

   EFFICIENCY II: ELECTRIC SPELLS COST 10% LESS SPIRIT A POINT.  1 OF 2.  ONE POINT
```

- W/S and A/D move between nodes; Enter buys a point. Each node is a mark: lit when bought, outlined when it can be
  bought now, dim when it can't yet, chained over when the other path was taken.
- A card at the foot says what the node does, its numbers, its price, and what it changes in the spell.
- Taking the first node of a path asks twice: *THE OTHER PATH WILL CLOSE*.
- Points not spent are kept for later.
- The order (*default*): the opening; Efficiency and Insight in either order, both full; the ability; then one path,
  its nodes in order, its ultimate last.

**A6. "Its own unlock requirements"** (each path has its own bonuses and requirements, the owner said).
- (a*) Points and the nodes before it, nothing more.
- (b) A deed as well: a Mastery needs a hundred creatures slain with its element in this life, an ultimate three
  hundred, counted on the screen.
- (c) A Level of Atonement as well: a path from 15, an ultimate from 25.

**A7. Taking it back.**
- (a*) Never: a path is chosen for the life, and a death that costs clears everything anyway.
- (b) She unbinds a line for a Cursed Coin, its points back.
- (c) She unbinds it for souls, 5 a rank (a sink, section 10).

**A8. Death.**
- (a*) The lines go with the Levels of Atonement, as the spells go now; Flame stays open at rank 1, and the Vow's own
  line too.
- (b) The lines stay; only the points not spent go.

**Her claim** takes a spell down a tier today. (*default*) She takes the last node bought in a line chosen at random,
its points lost (never an opening or an ultimate), or a worn piece, as now.

## 7. The owner's lines: Electric and Flame

As the owner wrote them, with what is unclear asked after each.

**Electric** (open 5) > Electric Efficiency (+5% a point, 2) > Electric Insight (-15% a point, 2) > **Ionization** (3) >
one path:

- **Ionization**: Electric attacks leave a trail from the hand to what they strike; it hangs for half a second and
  hurts anything that crosses it for half the element's damage (*default*: once a creature a trail).
- Path A: Electric Efficiency II (+10% a point, 2) > **Electric Mastery** (+30% Electric damage; 20% Electric damage
  added to every other attack and spell; 5) > **Arc** (5): the spell leaps on to the creature nearest the one it
  struck, up to four leaps, each a quarter weaker.
- Path B: Electric Insight II (-20% a point, 2) > **Electric Mastery** (+20%; 10% added; 5) > **Amperage** (5): hold
  to charge an orb that flies straight from the hand. It hurts what it strikes, and an arc from it reaches a creature
  just out of its reach; it leaps on to more (EL1); whatever it hurts can't strike or throw for 1.5 s.

**EL1. Amperage's leaps.** The tree says up to four bounces, the list of ultimates one more creature hit.
(a*) One more (the list, written later). (b) Four.

**EL2. The Electric spell before Arc.** (a*) A bolt that strikes the creature aimed at at once (today's Chain
Lightning, without its leaps), as quick as the fireball. (b) Rapid Sparks's quick, light shot.

**EL3.** Path B's Mastery is weaker than path A's (20% and 10% against 30% and 20%). (*default*) As written: path B
buys Insight and the orb instead.

**EL4.** (*default*) Amperage's orb leaves Ionization's trail along its flight.

**Flame** (open from the start) > Flame Efficiency (+5% a point, 2) > Flame Insight (-15% a point, 2) >
**Firestarter** (3) > one path:

- **Firestarter**: the fireball sets the floor alight along its flight, the flames lasting 1.5 s. A creature that
  walks through them catches fire and burns for a while (FL4).
- Path A: Flame Efficiency II (+10% a point, 2) > **Burning Intensity** (+50% burning time a point, 2) >
  **Flamewalker** (5): walking through Firestarter's flames gives you half your armour again for five seconds; it
  can come again ten seconds after that ends, a wait Spell Effect shortens, never under three seconds (FL1).
- Path B: Flame Insight II (-15% a point, 2; FL2) > **Flame Mastery** (+25% Flame damage, 5; FL3) > **Firewall** (5):
  charging the fireball calls three flame spirits that circle you half a cell out for five seconds; a creature they
  touch burns for half the fireball's damage over five seconds, a burn that doesn't stack with itself.

**FL1. Flamewalker's wait.** Spell Effect is capped at +50% today: taking its share off ten seconds leaves five at
best; dividing by it, 6.7. Neither reaches three.
- (a*) Ten seconds less Spell Effect's share (five at the cap); the three seconds a floor for anything that raises
  Spell Effect later.
- (b) Spell Effect counts 1.4 times over here, so its cap brings the wait to three seconds.

**FL2.** Flame's Insight II is -15% a point, Electric's -20%. (a*) As written. (b) -20%, as Electric's.

**FL3.** Flame Mastery adds no Flame to other spells, Electric's does. (a*) As written: Flame's strength is its
burning. (b) It adds 10%, as Electric's path B does.

**FL4. Burning** (*defaults*). A creature alight burns for two seconds, a quarter of the fireball's damage a second;
Burning Intensity makes that three and four seconds, Spell Effect longer still. Walking through flames again starts
it over; it doesn't stack. Firewall's burn and Firestarter's can't both run on one creature: the stronger does.

**FL5. Firewall's charge.** (a*) A charged cast throws no fireball: the spirits come instead; let go early and an
ordinary fireball goes. (b) Both: the fireball and the spirits.

## 8. The other four lines, drafted

Built to the owner's pattern: open (5) > Efficiency (+5% a point, 2) > Insight (-15% a point, 2) > an ability (3)
that changes how the spell behaves > path A, of control or upkeep (an augment of 2, a node of 2, an ultimate of 5) or
path B, of damage (Insight II at -20% a point, 2; a Mastery of 5; an ultimate of 5). Every ultimate changes what the
spell is. Names and numbers are first drafts.

**P1. These drafts.** (a*) Mark them up: keep, change or replace each. (b) The owner writes these lines too.

**Frost**: Frost Shard, a shard that slows what it hits.
- **Deep Chill** (3): every shard chills; the third on one creature within four seconds freezes it for a second (a
  boss is slowed harder instead).
- Path A, the binding: Frost Efficiency II (+10% a point, 2) > **Permafrost** (+50% slowing and freezing time a point,
  2) > **Winter's Lance** (5): hold to charge a lance of ice that goes through every creature in its line, freezes
  each for two seconds (bosses slowed), and rimes the floor along its way for four, slowing whatever crosses it.
- Path B, the shattering: Frost Insight II (-20% a point, 2) > **Frost Mastery** (+25% Frost damage; 10% Frost added
  to every other spell, whose hits then slow a little; 5) > **Shatter** (5): a frozen creature struck by any spell
  shatters for double damage, its shards cutting everything within a cell and a half for half a shard's damage and
  chilling them.

**Poison**: the **Blight Flask** (new). A stoppered flask lobbed in an arc to where you aim, over low things in the
way, as the ember imps throw; it bursts into a splash a cell across, a little harm at once and the rest as poison
over four seconds. The bony and the stone take little of it (the table). Poison on a creature doesn't stack: a new
dose starts it over if it is stronger.
- **Seep** (3; the owner's residue): the splash leaves a pool of venom for two seconds, and whatever walks in it is
  poisoned again.
- Path A, the pool: Poison Efficiency II (+10% a point, 2) > **Lingering** (+50% to the pool's time a point, 2) >
  **Plague Pit** (5; the owner's charged residue): hold to charge, and the pool grows with the charge, to two and a
  half cells across at full; it lasts six seconds and slows whatever wades in it.
- Path B, the sickness: Poison Insight II (-20% a point, 2) > **Poison Mastery** (+25% Poison damage; 10% of every
  other spell's damage added as poison; 5) > **Contagion** (5): a creature that dies poisoned passes what is left of
  its poison to everything within two cells, a fresh dose at 60%, so a pack goes down together.

**Spirit**: Soul Drain, a beam that hurts and heals you for part of it, the healing paid in spirit. This is the line
of the soul economy (section 10).
- **Harvest** (3): a creature slain by Soul Drain gives a soul more (two more from rank III).
- Path A, the leech: Spirit Efficiency II (+10% a point, 2) > **Communion** (+25% healing a point, 2) > **Soul
  Tether** (5): hold to keep the beam on. It drains four times a second while held, slowing what it holds, and when
  that dies it leaps to the next within four cells; it costs spirit by the second, not the cast.
- Path B, the reaper: Spirit Insight II (-20% a point, 2) > **Spirit Mastery** (+25% Spirit damage; 10% Spirit added
  to every other spell; 5) > **Wisp Host** (5; Seeker Wisps folded in): every drain also looses three wisps that hunt
  the nearest creatures round corners, each for 40% of the drain.

**SP1. Spirit and the spirits.** (a*) Nothing touches the spectre or the Wraith still; Spirit damage drives the
Wraith back, as fire does. (b) With Spirit Mastery, Spirit can kill spectres (the Wraith still can't be killed).

**Physical**: the Bone Spear, through everything in a line, a bone a spear, no spirit.
- **Physical Efficiency** (2): +5% a point that a spear spends no bone, and five more bones carried (`BONES_MAX`).
- **Impale** (3): the first creature a spear strikes is pinned for 0.6 s and can't move or strike (a boss staggers).
- Path A, the bulwark (the owner's armour): **Bone Plate** (+10% armour a point, 2) > **Ossified** (spears add half
  your armour to their damage, and each spear that strikes gives a point of armour for six seconds, up to ten; 5) >
  **Phalanx** (5): hold to charge, and a fan of spears goes, three at a short charge and five at full, for one bone.
- Path B, the marksman: Physical Insight II (-20% a point, 2) > **Physical Mastery** (+30% Physical damage, adding
  nothing to other spells; 5) > **Ballista** (5): hold to charge a heavy spear that flies twice as fast, knocks each
  creature it passes a cell back and stuns it for a second, up to three times the damage at full charge.

**P2. Physical's Efficiency as bones.** (a*) Yes. (b) The spear costs spirit too, and Efficiency is spirit as for the
rest.

**What a line costs, all the way** (points after the opening; the opening is 5, A4):

| Line | Before the paths | Path A | Path B | All of A | All of B |
|---|---|---|---|---|---|
| Flame (open from the start) | 7 | 9 | 12 | 16 | 19 |
| Electric | 7 | 12 | 12 | 24 | 24 |
| Frost, Poison, Spirit | 7 | 9 | 12 | 21 | 24 |
| Physical | 7 | 12 | 12 | 24 | 24 |

("All of" counts the opening's five; for Flame, which comes open, it doesn't.) At A3 (a), the dream's caster holds
about 49 points at level 35: Flame and one more line all the way, or Flame halfway and two more lines.

## 9. Vows (classes)

**V1. Classes.**
- (a*) Yes: six Vows, one an element, the class taken as a vow to the Order's dead.
- (b) Three classes of two elements each.
- (c) No classes; the trees are enough.

Drafted for (a):

| Vow | Its element | What it gives from the start |
|---|---|---|
| Vow of Embers | Flame | Flame Efficiency +20% and Flame Power +10%; five points in Flame (which is open for everyone) |
| Vow of Rime | Frost | Frost open; Frost Efficiency +20%; slowing and freezing 20% longer |
| Vow of Storms | Electric | Electric open; Electric Efficiency +20%; Electric Insight +10% |
| Vow of Blight | Poison | Poison open; Poison Efficiency +20%; poison 20% longer |
| Vow of Souls | Spirit | Spirit open; Spirit Efficiency +20%; the vessel (S1) holds 20 more |
| Vow of Bone | Physical | Physical open; +20% that a spear spends no bone, ten more bones carried, +10% armour |

**V2. What a Vow costs.** (a*) Nothing but the choice. (b) Its opposite costs more: two more points to open, and its
Efficiency halved (Flame and Frost, Electric and Physical, Poison and Spirit).

**V3. When it is taken.**
- (a*) At a fresh start and after every death that costs, in her room before level 1; she asks before the wares:
  *WHAT DO YOU VOW?* A life keeps one Vow. The dream vows at random each time it starts fresh, so the screensaver
  shows every line in time. An old page vows at its next visit.
- (b) Once for the ledger's page, kept through death.
- (c) As (a), and changed at her for a Cursed Coin.

The Vow's mark shows on the caster's page, the Alignment screen and the grave's stone (*default*).

## 10. Souls

The aim, as we read it: a caster who casts its best spell at every chance runs dry in a long fight and has to fall
back on weak casts, the free fireball or the spear; one who spends with thought seldom does; and a full purse of
souls is a choice, not a given. Before setting numbers, measure 1.0.0b10: souls gained, spent and carried at each
level's end, and casts gone out weak, over eight seeds from levels 1, 11, 21 and 31.

**S1. A vessel.**
- (a*) Spirit has a ceiling: 20 at first, two more a Level of Atonement (120 at 50), more with the Vow of Souls and the
  Spirit line. Souls past it slip away: a pale wisp rises from the hand, and once a level *THE VESSEL IS FULL*.
- (b) No ceiling, as now.

**S2. What a cast costs.**
- (a*) By rank: about 0.2 a cast at rank 1, 1 at rank 10 and 2.5 at rank 20, less Efficiency and Spirit Cost; a full
  charge 2.5 times a cast; Soul Tether by the second. The fireball (Flame at rank 1) is free. Any spell cast with too
  little spirit goes out at rank 1, free, as now; with A1 (a) that is weak, not useless.
- (b) By damage, as now, on a steeper scale.

**S3. What comes in.**
- (a*) As now, one to six a kill by rank, the numbers left to the runs.
- (b) Souls are gathered: a kill leaves its soul as a mote for six seconds, taken by walking within a cell of it (the
  autopilot takes those on its way).
- (c) Fewer a kill, Harvest the way to more.

**S4. A sink that is a choice.**
- (a*) *OFFER SOULS* at her: ten souls for experience (scaled by the level), so a full vessel in her room is power now
  or Atonement later.
- (b) None beyond wisps, the altars and the BFS9000.
- (c) The unbinding of a line (A7 c).

**S5. A tithe at the stair** (a tenth of what is carried, taken at each descent). (a*) No. (b) Yes.

## 11. What else changes

- **The merchant** sells no spells. Her first pedestal is a piece to wear, or now and then the BFS9000, as rare as now
  (E2). The autopilot's shopping loses its three attack spells first.
- **The Compendium** loses its spells page.
- **Graves**: a stone keeps the Vow and each line's rank and path, in place of the spellbook.
- **Old saves.** O1: (a*) a 1.0.0b10 page loads with Flame open, no Vow (asked at the next visit), its spells forgotten,
  and Alignment Points for every Level of Atonement it holds; said once: *THE OLD SPELLS ARE FORGOTTEN. ALIGN WITH
  HER.* (b) Each spell held at tier t becomes its line opened, with t points in it (down a path chosen for it), the
  rest as points.
- **Keys** (*default*): 1 Flame, 2 Frost, 3 Electric, 4 Poison, 5 Spirit, 6 Physical, 7 the BFS9000; the wheel and the
  bumpers skip what isn't open. The status bar's SPELL shows the element's glyph and its rank.
- **The hand** casts in the element's colour, as it does by spell now.
- **The autopilot** chooses by element and cost (`choose_spell`); spends its Alignment Points by its Vow (its own
  line to the end of a path, Flame's ability, then a second line); charges an ultimate when three creatures or more
  are in reach or a boss is; calls Firewall when something comes to blows; holds Soul Tether on the strongest. It
  never spends a person's points.
- **Debug switches**: `CRAWL_VOW=k`, `CRAWL_ALIGN=` (lines opened to a node and path), `CRAWL_ALPTS=n`; `CRAWL_GEAR`
  loses its spells; `CRAWL_LEARNT` learns the elements and the harms.
- **The Codex**: the elements, each line and node, the Vows. **The controls card**: charging, *ALIGN*.
- **Save version 7**: the gear's spell tiers go; the record's spell columns become elements, with hits landed and
  harms learnt; a ledger section `LG_ALIGN` (the Vow, points, each line's nodes and path). Old versions still load
  (O1).

## 12. Art, for the owner's sheets first

As 1.0.0b10 went: sheets from `make sheets`, approved, then frames from the game.

- `elements.png`: the six glyphs (at size and doubled), the nine rune marks, the tree's node marks (an opening,
  augments, each ability, the Masteries, each ultimate), the six Vows' marks.
- `spells.png`: the Blight Flask in flight, its splash and pool and Plague Pit's; Firestarter's flames on the floor; a
  creature alight; the three flame spirits; Ionization's trail and Amperage's orb; Arc's leaps; Winter's Lance and its
  rime; Shatter's shards; Soul Tether's held beam; Wisp Host's wisps; a pinned creature; the Phalanx's fan; the
  Ballista's spear; the charging ring on the hand.
- Frames: the map at a walk, the target with its glyphs, a rune's socket, the Alignment screen, the Vow's question.

## Building it

Each step a commit, tested in the harness; part one can be released alone (G1).

**Part one**

1. **The elements as data**: `NELEM`, each spell's element, the creatures' table by element and their harm, the
   record by element (kills, hits landed, harms learnt) with old records carried over, save version 7's record.
2. **Glyphs**: the six glyphs (after the owner's sheet), the target's readout, learning (T2), the bestiary and Codex.
3. **The runes' sockets** (section 3).
4. **The map at a walk**: the crypt running under it, walking with it out, seen and walked cells, the reveal, panning
   (M1), the scroll (M2), the level's numbers.

**Part two**

5. **Spells as lines**: tiers out of the shop, the Compendium, the claim and the save; ranks and strength (A1, A2);
   Insight (K1); Efficiency; each line's spell, the Blight Flask with it (a lob, a splash, poison on creatures);
   burning on creatures; the Electric bolt; the BFS9000 alone at the merchant's.
6. **The Alignment screen** and Alignment Points, *ALIGN* in her menu, Tab's page, death, the claim, old saves.
7. **The abilities**: Ionization, Firestarter, Deep Chill, Seep, Harvest, Impale.
8. **The paths' nodes and Masteries**, with the Additions.
9. **Charging** (K2) on the mouse and the pad, then the ultimates, a line at a time: Arc and Amperage; Flamewalker and
   Firewall; Winter's Lance and Shatter; Plague Pit and Contagion; Soul Tether and Wisp Host; Phalanx and Ballista.
10. **Vows** (V1-V3).
11. **Souls** (S1-S5), set from the measurements and tuned by runs.
12. **The autopilot**: Vows, spending, choosing by element and cost, the ultimates.
13. **The Codex, the controls card, the hints, the README and the page.**
14. **Checks**: ASan over the dream and a scripted session through every new screen and ultimate; the Windows build;
    eight 100-minute dream runs from level 1 (seeds 1-15 odd) and eight 50-minute runs from 11, 21 and 31 against
    1.0.0b10 on the same seeds, with souls gained, spent and carried a level; a dream run with each Vow.

As always, all of it tested in the harness only until the owner plays it.

## The questions, together

Answer by number; anything left alone takes the starred answer.

| | Question | Recommended |
|---|---|---|
| G1 | One release or two | (a) one, part one first |
| M1 | Panning the map at a walk | (a) arrow keys, drifts back; the pad doesn't pan |
| M2 | How much of the crypt shows | (a) the parchment thinned, the crypt through it |
| M3 | What fills in | (a) what you see, and 1.5 cells through walls |
| M4 | How a walked cell looks | (a) a wash with a faint hatch |
| M5 | The gold left | (a) *ABOUT 1,400*, with Treasure in it |
| M6 | Who sees the numbers | (a) everyone with the map |
| T1 | + and - | (a) one sign each, struck through for nothing |
| T2 | What learning takes | (a) five slain or twelve hits; its harm after three hurts |
| R1 | Where the runes' sockets go | (a) the lower right |
| R2 | The rune's name | (a) the mark only |
| R3 | In the dream | (a) a person only |
| R4 | While a rune works | (a) not shown |
| E1 | The elements' words | (a) FIRE, FROST, LIGHTNING, POISON, SPIRIT, PHYSICAL |
| E2 | The spells left over | (a) folded into the lines; the BFS9000 stays at the merchant's |
| E3 | Two harms | (a) one glyph |
| E4 | Efficiency | (a) spirit (bones for Physical) |
| E5 | The caster's resists | (a) as they are |
| E6 | Gear and the elements | (a) as it is, this release |
| A1 | A spell's strength | (a) the Level of Atonement, ranks on top |
| A2 | What a rank is | (a) rank 1 when opened, a rank a point after |
| K1 | What Insight shortens | (a) the whole cycle |
| K2 | Holding to charge | (a) hold to charge, tap to cast |
| A3 | Where points come from | (a) one a Level of Atonement |
| A4 | Opening a line | (a) five |
| A5 | Where points are spent | (a) *ALIGN* in her menu; Tab to look |
| A6 | A path's requirements | (a) points only |
| A7 | Taking it back | (a) never |
| A8 | Death | (a) the lines go; Flame and the Vow's line stay open |
| EL1 | Amperage's leaps | (a) one more creature |
| EL2 | The Electric spell before Arc | (a) a bolt that strikes at once |
| FL1 | Flamewalker's wait | (a) ten seconds less Spell Effect's share |
| FL2 | Flame's Insight II | (a) -15% as written |
| FL3 | Flame Mastery's addition | (a) none, as written |
| FL5 | Firewall's charge | (a) the spirits, no fireball |
| P1 | The four drafted lines | (a) marked up by the owner |
| P2 | Physical's Efficiency | (a) bones |
| SP1 | Spirit against spirits | (a) still nothing; it drives the Wraith back |
| V1 | Classes | (a) six Vows |
| V2 | What a Vow costs | (a) nothing but the choice |
| V3 | When a Vow is taken | (a) at each fresh start; the dream at random |
| S1 | A vessel | (a) yes, 20 and two a Level of Atonement |
| S2 | What a cast costs | (a) by rank |
| S3 | What comes in | (a) as now, tuned |
| S4 | A sink | (a) *OFFER SOULS* for experience |
| S5 | A tithe at the stair | (a) no |
| O1 | Old saves | (a) spells forgotten, points for the Levels of Atonement |

Questions the owner may want to add their own answer to, beyond the options: the four drafted lines (P1), the Vows'
names and gifts (V1), and the souls (S1-S4), where the owner's sense of "starvation" decides how hard the numbers
should bite.
