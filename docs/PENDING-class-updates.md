# Pending class updates (3 November 2026)

Status: gathered, not built. These are the changes to the classes and their spell lines (the Alignment) that the owner
asked for in their first pass through the words editor (docs/PLAN-words.md). They wait to be folded into a later update;
that update's plan should put the questions below to the owner first, as with every big feature.

Two parts. **Part 1** is the owner's eight notes, quoted as written, each beside what the game does now (from the code
on main at 1.0.0b13 plus the art pass) and what needs deciding. **Part 2** is what the owner's new wording says the game
does where it doesn't do it yet, with no note: each is either a mechanic to change or a sentence to change, and the owner
should say which.

The source is the editor's database (`words/<id>`, read 3 November): the id after each heading is the row.

## Part 1: the owner's notes

### 1. Lightning: the opening spell, a chance to interrupt
`a_bolt_from_the_hand_that-6c41` (the Lightning line's opening node; the spell is Chain Lightning, `AT_CHAIN`)

Their words: *RELEASE A SPARK OF SPIRIT ENERGY THAT DEALS LIGHTNING DAMAGE, WITH A SMALL CHANCE TO INTERRUPT ATTACKS*

> We need to add in a mechanic that handles how lightning attacks can interrupt an enemy when they attack and for how
> long. It should be brief, only .5 seconds but it should require the NPC to re-run the loop it needs to in order to
> attack again. It should be somewhat randomized, between 3-6 hits over a period of 4 seconds set to a low chance if
> landed. There should be a cooldown that prevents the same NPC from being "shocked" for a period of time. This would
> permit NPCs with large health pools to be shocked multiple times without chaining them endlessly.

**Now.** The bolt strikes what you aim at at once and does nothing else. Nothing a person casts interrupts a creature's
attack but Amperage's orb (`daze`: no blow or throw for 1.5 s) and the knock-back skeletons and shield-bearers take when
hit (`stagger`). A creature's attack runs on `swing` (a melee wind-up, the knight's 0.9 s), `throw_t` (a thrower's
count-down) and `cool` (the wait between blows); the wall skull gathers fire for three seconds; the deacon chants.

**The change.** A lightning hit may shock a creature: half a second stunned, and whatever attack it was making starts
again from the beginning (`swing`, `throw_t`, a wall skull's or deacon's gathering all reset). Rare, a little random,
and a creature once shocked can't be shocked again for a while.

**To decide.**
- How to read "3-6 hits over 4 seconds". My reading: each creature counts the lightning hits it takes; when the count
  reaches a number rolled between 3 and 6 within four seconds, it is shocked, and the count and the roll start again.
- Which lightning counts: the bolt only, or also Arc's leaps, Ionization's trail, Amperage's orb, and the lightning
  share that Lightning Mastery and gear add to other spells? (Suggested: the bolt, Arc and Amperage; not the shares.)
- The cooldown (suggested four seconds) and whether bosses can be shocked (suggested: no, or only between waves).
- Lightning already interrupts the caster: creatures' and hazards' lightning can break a spell's wind-up, and Resist
  Lightning makes that rarer. This is the same idea turned the other way.

### 2. Frost: Deep Chill, four hits from any frost
`every_shard_chills_the_third_on-bb43` (Frost, the ability node)

Their words: *STRIKING ANY SINGLE ENEMY WITH A FROST SPELL FOUR TIMES WITHIN A FIVE SECOND WINDOW FREEZES THEM IN PLACE
FOR A PERIOD OF TIME.*

> We need to adjust this mechanic so it freezes every four hits with frost spells over a five second window, from all
> sources.

**Now** (`deep_chill`). Only Frost Shard's hits count. The third on one creature within four seconds freezes it for a
second (longer with Permafrost and for the Anatomist); a boss is slowed for three seconds instead.

**The change.** Four hits within five seconds, from every source of frost.

**To decide.** Which sources: Frost Shard, Winter's Lance, Shatter's shards, the Frost Nova rune, gear's Frost, and the
tenth of Frost that Frost Mastery adds to every other spell? (Suggested: every hit whose own element is frost, but not
Mastery's share riding on other spells, or every spell would count.) The freeze: keep one second, raised by Permafrost?
Bosses slowed as now?

### 3. Frost: Shatter, a chance, not a certainty
`a_frozen_creature_struck_shatters_for-63f3` (Frost, path B's ultimate)

Their words: *A FROZEN CREATURE HAS A CHANCE TO SHATTER, EXPLODING INTO A SHOWER OF DAMAGING ICICLES IMPACTING ALL AROUND
IT, DAMAGING AND SLOWING.*

> We need this ability to be retooled so it imparts a chance to shatter, the 100% rate we have now is comically
> overpowered. For enemies with more than 400HP this ability should only offer the chance to shatter when below 100HP,
> freezing as usual.

**Now** (in `damage_mob`). Any spell but the BFS9000 that strikes a frozen creature (not damage over time) always
shatters it: double damage, it thaws, and shards worth half a Frost Shard cut and slow everything within a cell and a
half.

**The change.** A chance to shatter. A creature of more than 400 health can shatter only once it is under 100; above
that it freezes and thaws as usual.

**To decide.**
- The chance (suggested 25%, perhaps rising with Frost Mastery).
- 400 is a fixed number while health grows with the level (1.04 a level) and the rank. A rank I skeleton knight has
  about 60 at level 1, 127 at 20, 187 at 30; at rank III, 449 at level 30; a black brood mother about 1,090 at 30. So
  the rule would bite only in the last two runs and on high ranks. Keep 400 and 100, or use shares (over three times a
  Frost Shard's damage, or under a quarter of its health)?
- Whether the shatter still doubles the blow that set it off.

### 4. Spirit: Harvest, more souls from spirit kills
`what_soul_drain_slays_leaves_a-1afe` (Spirit, the ability node)

Their words: *INCREASES THE VALUE OF SOULS ACQUIRED BY SPIRIT SPELL ELIMINATIONS, 22% PER POINT.*

> We need to retool this ability to offer a 22% increase in souls per kill, only when using Spirit Spells, up to 66%.
> Three points, incremental instead of all at once.

**Now.** A creature Soul Drain slays leaves one soul more, two from rank III. The node is bought whole for three points.

**The change.** Bought a point at a time, three points: +22% souls a point, +66% at most, for kills by spirit spells.

**Worth knowing before deciding.** A kill's mote holds half a soul at rank I, then 1, 1.5, 2 and 3 at ranks II to V
(`RANK_SOULS`). Today's Harvest takes those to 1.5, 2, 3.5, 4 and 5; at 66% they would be 0.8, 1.7, 2.5, 3.3 and 5. So the
new Harvest is weaker at every rank but the black, much weaker early. If that is the aim, good; if not, a larger share
(or a flat soul plus a share) keeps it level.

**To decide.** Which are spirit spells: Soul Drain, Soul Tether, Wisp Host's wisps, Spirit Mastery's share on other
spells? (Suggested: the first three.)

### 5. Spirit: Soul Tether, a heavier price
`hold_to_keep_the_beam_on-3a1d` (Spirit, path A's ultimate; renamed MAJOR SOUL TETHER)

Their words: *HOLD CAST TO CONJURE A MAJOR SOUL TETHER, SLOWING THE TARGETED WHILE DRAINING. JUMPS TO THE NEAREST ENEMY
WHEN DEFEATED. HIGH COST.*

> I'd like to review how quickly this ability consumes souls. It should be a heavy risk/reward ability with a little
> more thought required before using it endlessly.

**Now** (`tether_hold`). Held, every quarter second it spends a quarter of one Soul Drain cast's spirit (a whole cast's
worth a second) and deals 30% of a drain's damage (1.2 drains a second), slows the one it holds, and heals as the drain
does (the healing costs spirit too). When the one it holds dies it leaps to the nearest within four cells; it slips when
the souls run out.

**To decide**, the shape of the price (the owner asked for a review, so these are for choosing between):
- (a) a price to begin, say three souls, then the running cost as now;
- (b) a running cost that climbs the longer it is held, say a quarter more each second (recommended: short holds stay
  cheap, long ones grow expensive);
- (c) twice the running cost, the damage raised to match;
- (d) health, not souls, once the souls run out: the risk is literal.

### 6. Spirit: Wisp Host, wisps from the dead
`every_drain_looses_three_wisps_that-9f27` (Spirit, path B's ultimate)

Their words: *WHEN A DRAIN SPELL LEADS TO THE DEATH OF AN OPPONENT THREE WISPS ARE RELEASED FROM THE VICTIM, HUNTING THE
NEAREST ENEMY.*

> Wisps highlight enemies they strike using the same red glow from other abilities that show us enemies, for five
> seconds. Their color should be changed from red to a noticeable purple. The damage from these wisps should be a
> percentage of the damage done to the NPC that led to them being spawned, split evenly among the three. These wisps
> can only hit one target at a time. If no other targets extra wisps fizzle out.

**Now** (in `cast_spell`'s drain). Every Soul Drain that lands looses three wisps from your hand (`AT_WISPS`), each for
40% of the drain's damage, homing round corners for three seconds; with Spirit Mastery they can strike spectres.

**The change.** Only when a drain kills. The three wisps rise from the body, each going for a different creature, the
nearest first; each strikes once and is spent; with fewer targets than wisps, the rest fizzle. Their damage is a share
of the damage that killed, split evenly. A creature a wisp strikes glows through the walls for five seconds, in purple,
drawn as Commune draws its red shapes (`DR_COMMUNE`).

**To decide.** The share (suggested 60% of the killing blow, a fifth each)? "Drain spell": Soul Drain and the Soul
Tether's ticks? Commune itself stays red, and only the wisps' mark is purple (my reading)?

### 7. Physical: Impale, pinned to the wall
`the_first_a_spear_strikes_is-9d96` (Physical, the ability node)

Their words: *BONE SPEARS HAVE A CHANCE TO IMPALE THEIR TARGET, PINNING THEM TO THE WALL FACING THEIR BACK. HIGHER
SPIRIT INCREASES THIS CHANCE.*

> This ability also needs a rework. We need it to be a random chance, with a 100% hit rate when our spirit meter is over
> 90% full. It should stun the enemy once they are pinned, it should physically move the enemy to the nearest
> rear-facing wall. The stun should last .6 sec from impact to the wall, then the enemy should resume their attack. The
> moment an enemy is struck by an attack that would pin them it should interrupt wha they are casting or doing.

**Now.** The first creature each spear strikes is pinned where it stands for 0.6 s (no moving, no striking); a boss
staggers for 0.4 s. Every spear, every time.

**The change.** A chance, rising with spirit, certain when the souls held are over 90% of the vessel. The struck creature
is interrupted at once (whatever it was winding up, throwing or chanting starts again), carried back to the nearest wall
behind it, and stunned for 0.6 s from the moment it meets the wall; then it fights on.

**To decide.**
- The chance below 90% (suggested a straight line from 10% with an empty vessel to 100% at 90% full).
- How far it can be carried (suggested three cells at most; with no wall that near, it is knocked back three cells and
  stunned where it stops). Doors, props and other creatures in the way stop it.
- What can't be moved: bosses (staggered as now?), wall skulls, spiders in their webs, egg sacs, gargoyles on plinths.
- The Ballista already knocks back and stuns everything it passes; the two should feel different (Impale one creature,
  to the wall; the Ballista the line).

### 8. Physical: Ossified, armour lent to the spear
`spears_add_half_your_armour_to-0c00` (Physical, path A's second node)

Their words: *YOUR ARMOR AUGMENTS SPEARS, IMPROVING THEIR DAMAGE WHILE STRIPPING YOUR DEFENSE FOR 2S. IF THEY STRIKE YOUR
ARMOR IS BOOSTED 2S*

> This description also modifies this mechanic. We need Bone Spears to "take" some of our armor value, 25%, for two
> seconds. This value is calculated at attack, so our gear is considered as well. We take 25% of our armor, imparting
> that value as a percentage damage boost to the attack. If the attack connects we receive an armor boost of 50% for
> 2sec. If this eliminates an enemy it offers the armor boon for 5sec.

**Now.** A spear adds half your armour to its damage, and each spear that strikes gives a point of armour for six
seconds, ten at most (`G.oss_t`).

**The change.** Casting a spear takes a quarter of your armour (all of it counted: gear, Bone Plate, the class) for two
seconds and adds that number to the spear's damage as a percentage. If the spear strikes, your armour is half as much
again for two seconds; if it kills, for five.

**To decide.**
- "That value as a percentage": with 40 armour, 10 is taken and the spear does 10% more (my reading)?
- Several spears in two seconds: does the taking stack (suggested: no, it refreshes; armour never below three quarters)?
  Does the boon stack or refresh (suggested: refresh, the longer of the two)?
- The boon against Flamewalker's (half your armour again for five seconds): add, or take the larger?

## Part 2: implied by the new wording, no note

The owner rewrote 73 of the classes' and lines' strings. Where a rewrite describes something the game doesn't do, one of
the two has to move. Each needs a word from the owner: change the game, or change the sentence.

### The classes
- **Names.** THE WITCHFINDER and THE ANATOMIST became WITCHFINDER and ANATOMIST; the other four keep THE. The name
  fills "YOU WERE {1}", now "YOU PREVIOUSLY LIVED AS A {1}", which would read A ANATOMIST and A THE PLAGUE DOCTOR (and,
  with THE, 43 letters, past the caption's 40). Suggested: all six without THE, and the code choosing A or AN.
- **The Witchfinder's gift** reads *UNLOCK ACCESS TO FIRE SPELLS?* (question mark theirs). Every class has Fire open, so
  as written it gives nothing. The gift now: Fire costs a fifth less and hits a tenth harder, and three Alignment Points
  for Fire. What should it be?
- **The Sin-Eater's story** ends *ALLOWING YOUR ATTACKS TO SUSTAIN LIFE FORCE*: healing from attacks, which the class
  doesn't give (its gift: Spirit open, a fifth cheaper, the vessel twenty more). A new gift? The story also calls them
  SOUL RENDERS: a new name for the class?
- **The Headsman's story** stops at *STUN ENEMIES IN PLACE, PIN THEM TO A WALL, CALL* on its ninth line of ten:
  unfinished? It names three gifts (a stun, Impale's new pin, a call: Grave Call?) the class doesn't have; its gift now
  is Physical open, a fifth more spears that spend no bone, ten more bones, a tenth more armour.
- **The Witchfinder's story** ends with *...* on its tenth and last line: meant, or cut to fit?
- Her question now ends *PICK A NEW STARTING CLASS*; it is also asked after a death that costs and from her menu's vow.

### The lines
- **Fire is now FLAME**: the line and its three stats (FLAME POWER, EFFICIENCY, INSIGHT). The element is still FIRE
  elsewhere (its glyph, the readout, the Codex, Resist Burn, gear). All of it, or the line only?
- **Firestarter**: *FLAME SPELLS ENGULF THE FLOOR AS THEY PASS*. Now only the fireball's flight sets the floor alight.
- **Flamewalker**: *GENERATES A SWIRL OF FLAME AROUND YOU*: a sight to add (the armour is as now).
- **Firewall** and **Winter's Lance**: the words drop *HOLD TO*; both are still charged by holding the cast.
- **Permafrost**: *THE TIME ENEMIES REMAIN FROZEN*. Now it lengthens slowing too.
- **Winter's Lance**: *A CHANCE TO FREEZE*. Now it freezes everything it passes through for two seconds (a boss is
  slowed instead), every time.
- **Ionization**: *BRIEFLY CHARGES THE AIR IT PASSES THROUGH*. Now the trail hurts what crosses it for half a second;
  keep the harm?
- **Arc**: *WHEN CLOSE*: as now (a leap reaches 3.5 cells), each leap a quarter weaker.
- **Amperage**: *SLOWLY MOVES FORWARD THROUGH ALL ENEMIES, SHOCKING THEM*. Now it also arcs on to one more from each it
  passes; keep that? It already moves at under half a fireball's pace (3.5 cells a second against 8).
- **Seep, now VILE OOZE**: the pool *COVERING THE AREA*. Now what walks into it is poisoned again; keep that?
- **Plague Pit**: *EXPANDING THE SIZE OF THE POOL*. Now it also lasts six seconds and slows what wades in; keep those?
- **Contagion**: *POISONING THEM FOR THE SAME DURATION*. Now each within two cells gets a fresh dose at 60% for four
  seconds, not what was left of the dead one's.
- **Soul Drain**: *CONJURES A MINOR SOUL TETHER ... WHILE BRIEFLY ACTIVE*. Now it is one beam, at once: a short channel
  instead?
- **Communion**: *SPIRIT SPELLS RESTORE MORE HEALTH*. Only Soul Drain and the Tether heal now.
- **Spirit Mastery** no longer says *IT HARMS SPECTRES*. It is the only way to slay a spectre (for its souls); keep it?
- **Soul Tether** is renamed MAJOR SOUL TETHER (see Part 1, 5).
- **The Bone Spear**: *FIRES IN A STRAIGHT LINE*. Now it goes through everything in its line, costs a bone a spear and
  no spirit; the words drop both.
- **Physical Efficiency**: *A 5% CHANCE PER POINT TO SPEND NO RESOURCES*. Now it also carries five more bones; keep that?
- **Phalanx**: *HOLD TO CHARGE SPIRIT ENERGY*. Physical costs a bone, not spirit; does the Phalanx now cost spirit?
- **The Lightning bolt** is *A SPARK OF SPIRIT ENERGY*: in looks only, or should it cost or count as spirit?

## What was done with this

Nothing in the game has changed. The owner's words for these strings are in the editor's database; when the strings
move into the catalogue (docs/PLAN-words.md, *After the first pass*), the ones whose meaning waits on Part 1 or Part 2
should be held back until the mechanic and the sentence agree.
