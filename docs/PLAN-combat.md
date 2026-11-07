# The elements' teeth: statuses, elemental creatures, the classes' own, the Unholy Confessions and the spells' augments

**A draft, nothing built** (7 November 2026). Written on `claude/inspiring-archimedes-dbj5n6` from main at 1.0.0b18, from a
review of how harm is dealt today (section 1), the owner's answers to that review's first four questions, and the owner's
guide (kept as given in the appendix). The owner's answers so far: creatures cast through the kinds we have, "mixed up a
bit" as the guide sets out (Q1 b); their spells use the caster's six elements, so the resists, the Mirror and the wards all
answer them (Q2 yes); each class has its own starting pool of stats (Q3, close to a); the merchant leans to the class, a
little, and nothing Rare or better comes without something for a line the caster holds (Q4). The review's Q5 (three balance
fixes) and Q6 (which release) are still open; they are asked again in section 16 with the rest. Every number below not in
the guide is a default, marked as one, for the owner to keep or change.

*The brief, in a line: every element gets a status of its own, on the caster and on the dead; the dead get elements, weak to
one and strong against another, more with rank, and the commonest kinds learn new attacks; each class starts with its own
stats; the ascend screen becomes the Unholy Confessions, six groups weighted by class out of sight; and each spell line grows
augments, earned by using it.*

## 1. Where it stands today (from the review)

- **The caster's spells**: base x (1 + 0.07 a Level of Atonement) x the line's rank (+4% a rank to 11, +8% after) x (1 +
  Damage + the element's Power) (`spell_damage_at`, `src/crawl_gear.c`). Bases: Fire 10, Frost 7, Lightning 8, the flask
  10, Soul Drain 9, the spear 12. On the hit, `hit_mult` multiplies by the creature's weakness and adds the worn "ADDS %"
  stats and the Masteries' Additions. Nothing ever misses but by flying wide.
- **The dead**: health is the kind's base x 1.04 a level x the rank's (1.0 to 5.0); every blow is (4 + 0.8 x level) x the
  rank's (1.0 to 1.7) x a factor written inline for each attack in `update_mob` (and again in `mob_blow`). No creature rolls
  to hit: a blow lands if you are in reach as its wind-up ends; a missile if it reaches you. The caster's Dodge is the only
  roll.
- **Their resistances**: `KINDS[].weak[]`, a multiplier per element per kind (0 to 1.5), the same at every rank. Averaged
  over the 19 kinds: Fire 1.07, Frost 1.05, Lightning 1.16, **Poison 0.55**, Spirit 0.93, Physical 1.13. That, and wards
  picking the element a creature already shrugs off, is most of why the Plague Doctor is the weakest past.
- **Harm on the caster** (`hurt_ex`): Dodge, then the resist, then armour taken off flat (half against magic), then the Bone
  Ward. **`HURT_MAGIC` is always resisted by Resist Burn**, whatever its element: the Wraith's frost touch is cut by Resist
  Burn, the shade's spirit touch by Resist Burn and Resist Spirit both.
- **The classes** differ only in the gifts on their own line: everyone starts at 100 health, no armour, 2% Luck, a 2.5 s sprint.
- **The merchant** rolls pieces by kind, quality by run, stats by the piece's level; the only lean to the caster is a third of
  Power, Efficiency and Insight rolls going to the lines held, by rank.
- **Three faults on paper**: skeletons' throws grow 5% quicker a level with no floor (0.22 s between throws at level 40);
  flat armour grows as the blow does (one Rare robe with armour takes a rank-I blow at level 30 from 28 to 1); the attack
  numbers live in two places.

## 2. The shape of the work

Seven pieces, each resting on the ones before:

1. **One road for harm** (section 3): every attack in a table, every hurt on the caster through one function with its element,
   `HURT_MAGIC`'s fault mended. Nothing else changes; the dream's runs should be the same within the noise.
2. **Statuses** (sections 4-6): seven on the caster, their counterparts on the dead, and the chance each element carries to
   impart its own, from the caster's spells and from the dead's.
3. **The bestiary's elements** (section 7): one weakness and one strength a kind, another strength with rank, and elemental
   creatures past rank I.
4. **New attacks** (section 8): the skeleton's elements and the Bone Thrower, the shield-bearer's charge, the shambler's
   elements, the Drowned's spew, the hound's elements.
5. **The classes' own** (section 9): a starting sheet each.
6. **The Unholy Confessions** (section 10): the ascend screen replaced by six groups, weighted by class out of sight, with
   the Restoration spell, a killing blow survived, and cracks seen from further off.
7. **The spells' augments** (section 11) and **the merchant by class** (section 12).

Each is visual or felt at once, so each goes the way the art has gone: a sheet first where there is something new to see,
then frames from the game, then the dream's runs.

## 3. One road for harm (step 1)

- **An attack table.** `KINDS` gains its attacks as data, a row each: what it is (`AK_BLOW`, `AK_THROW`, `AK_LEAP`, `AK_SPEW`,
  `AK_CHARGE`, `AK_CAST`), its element (or "the creature's"), its factor on the blow, its wind-up, cooldown, reach, and
  whether it can be dodged. `update_mob`'s inline numbers and `mob_blow` both read it, so they cannot drift apart. The rows
  start as today's numbers exactly.
- **`foe_hit(m, attack, dmg)`** is the one way a creature's harm reaches the caster: it sets the blame, the element, whether
  Dodge applies, then calls `hurt_ex` with a new kind, `HURT_SPELL` (dodged, cut by its own element's resist, half armour
  unless physical), and then rolls the element's status (section 4). `hurt_by` becomes a thin wrapper.
- **`HURT_MAGIC` mended**: the wall skull's fireball and the imp's ember are Fire, the shade's touch Spirit, the Wraith's
  touch Frost, a spell the knight's Mirror turns is its own element, the Litany Spirit. Each is then resisted by its own
  resist only. This moves a few numbers (Resist Burn will matter less, Resist Slow and Resist Spirit more); the dream's
  runs say how much.
- **A balance sheet** (`make combat`, `tools/combat.c`, as `make items` is): for each level, kind and rank, the creature's
  health, each attack's harm and rate, the time a rank-I caster of each class needs to kill it with their line, and the time
  it needs to kill the caster through common armour. Printed before and after each step, so a change is seen on paper
  before the dream plays it.
- The review's three faults are asked in P1-P2; whichever the owner takes go in here.

## 4. Statuses on the caster (step 2)

`G.af[AF_*]` (time left) and `G.af_n` (stacks), cleared by `reset_caster_state`, never saved. Names in the code, `AF_*` for
*affliction*, since `CS_*` and `ST_*` are taken.

| Status | From | What it does | Defaults (to confirm) |
|---|---|---|---|
| **On Fire** (`AF_FIRE`) | a fire hit, by chance | the hit's damage again as a fire DoT over 4 s | a new ignite adds to what is left and restarts the 4 s (E2) |
| **Chilled** (`AF_CHILL`, stacks) | a frost hit, by chance | movement down a step a stack, sprint recovering far slower, a small aim penalty, the view bluer by stacks; four stacks become Frozen; they fall off 3 s after the last | -8% pace a stack; sprint recovery at 40%; aim spread 1.5 degrees a stack; all stacks go 3 s after the last chilling hit (S3, S4) |
| **Frozen** (`AF_FROZEN`) | four stacks of Chilled | 3 s unable to move; every harm on you +33%, a boss's and a hazard's too; the view iced; leaving it consumes the stacks | no casting or runes while frozen (S2) |
| **Stunned** (`AF_STUN`) | a lightning hit, by chance | 1.2 s: no spells or runes; you can walk but not sprint | |
| **Poisoned** (`AF_POISON`) | a poison hit, by chance; spiders' bites as now | spells cast slower while it lasts; each dose adds to its time, up to a minute | wind-up and cycle x1.5 for the caster; the harm a second stays as today's short poison, only the slowing lasts the stacked time (S5) |
| **Unbound** (`AF_UNBOUND`) | a spirit hit, by chance | 1.5 s: no spells, runes or E; you can walk | the map, Tab and the wheels stay open (S8) |
| **Knocked Down** (`AF_DOWN`) | a physical hit, by chance; the shield-bearer's charge | 1.5 s (the charge 1.2 s) rooted, no spells or runes; every other creature's harm halved, the one that knocked you down full through armour | `G.down_by`; section 4.2's view |

### 4.1 Rules for all of them

- **No locks.** Hard control (Stunned, Knocked Down, Frozen, Unbound) one after another could hold a caster for ever in a
  pack. Default (S1): when one ends, the same can't land for 3 s, and no hard control at all for 1 s.
- **Resisted** (the Confessions, section 10): Agility cuts the chance and length of Knocked Down, Stunned and Frozen;
  Spirituality cuts Unbound; Survival the rest. Whether an element's own resist also cuts its status's chance is S11.
- **What exists already becomes a status** (S9): the Wraith's touch and the frost hazards give Chilled stacks; spiders' bites
  Poisoned (their slowing as now); the shambler's grab stays a slow, but a physical shambler's becomes the grapple (section
  8.3); the void still silences as now (it is the void's, not Unbound).
- **Purify** cures Poisoned, Chilled and On Fire, as it cures poison and slowing now; Blink breaks Knocked Down and Frozen
  (a default; S2).
- **Second Wind and the killing blow** (section 10) still work through them; Frozen's +33% is counted before the killing blow
  is judged.

### 4.2 How they are shown

- **A row of glyphs** (S7): by the portrait on the status bar and after the readout when the bar is away, each with an edge
  that drains with its time and pips for stacks; cut in the stone kit (`ui_art.c`), so a sheet first.
- **The view**: Chilled, `tint`'s blue by stacks with frost creeping in at the corners; Frozen, the whole frame iced and a
  crack of sound as it breaks; On Fire, a flicker of orange at the bottom edge; Poisoned, today's green; Stunned, the view
  swimming a little and a high ring; Unbound, the hand greys and drops as the void's silence does.
- **Knocked Down** (S6): the view drops to the floor in a quarter second (`G.camz` to 0.12, as death's fall does, with the
  horizon tilted up a little), the hand gone, a thud, a heartbeat; getting up takes 0.4 s. A sheet of frames for the owner
  before it goes in.
- **Sounds**: one each, short; a person hears a status land without looking.

### 4.3 The pad

Rumble on a knock-down and on freezing; a stun's buzz short. Nothing else changes for the pad.

## 5. Statuses on the dead (step 2)

The same seven, as far as they mean anything to a creature (E3-E6):

| Status | On a creature |
|---|---|
| On Fire | the hit's damage again over 4 s, through `mob_burn` (stacking as E2 settles) |
| Chilled / Frozen | stacks slow it; four freeze it 3 s and it takes +33%; drawn bluer by stacks (`DR_ICE`) |
| Stunned | 1.2 s: no blow, throw or call; half pace; a ring of sparks over it |
| Poisoned | its wind-ups and cooldowns doubled while it lasts (the guide's "cast speed by 2x"), the poison's harm as now |
| Unbound | 1.5 s: no elemental attack, no Bone Thrower or charge, no call (the deacon's, the Sexton's bell, the horn, the wall skull's scream) |
| Knocked Down | the caster only, as the guide says |

What exists already: Frost Nova's freeze and Deep Chill's become Frozen (Deep Chill freezes at three stacks, not four);
Amperage's daze is a stun; Impale's pin stays its own. Spectres, the Wraith, sacs and bosses take none, or a boss only a
shortened one (E3).

## 6. The elements on the caster's spells (step 3)

Each of the caster's hits rolls its element's status. Default (E1): 10% at a line's rank 1, rising evenly to 25% by rank 20;
Spell Effect raises it; the augments (section 11) and a new stat (section 12) raise it further; a creature strong against the
element halves it, weak to it adds half. Physical imparts nothing on a creature (the guide's knock-down is the caster's
alone); the spear's stagger and Impale stay.

What already does something like it is reconciled, not doubled: Firestarter's burning floor gives On Fire as it does burning
now; the flask's poison is Poisoned; Frost's slow is a Chilled stack; Soul Drain's Unbound chance is the line's own.

## 7. The bestiary's elements (step 4)

### 7.1 The rule

The guide: every kind is weak to one element and strong against one; as it ranks up it gains another it is strong against.
Default (B1): weak x1.5, strong x0.5, every other x1.0, so the target's readout says plainly what to use. The old nuanced
table (0.75, 1.25) goes.

### 7.2 A draft table

Drawn to spread the elements evenly (each the weakness of three kinds and the strength of three), with the lore in mind. The
wall skull (one point of health) is left out.

| Kind | Weak | Strong | | Kind | Weak | Strong |
|---|---|---|---|---|---|---|
| Skeleton | Physical | Poison | | Knight | Lightning | Physical |
| Bat | Poison | Fire | | Brood mother | Poison | Frost |
| Shield-bearer | Lightning | Physical | | Shade | Spirit | Poison |
| Web spider | Fire | Frost | | Ember imp | Frost | Fire |
| Shambler | Fire | Frost | | Deacon | Fire | Spirit |
| Drowned | Lightning | Fire | | Gargoyle | Physical | Lightning |
| Bone hound | Frost | Physical | | Whipper-In | Spirit | Lightning |
| Hunting spider | Frost | Lightning | | Sexton | Poison | Spirit |
| Ghoul | Spirit | Poison | | Lightless | Physical | Spirit |

Weak: Fire 3, Frost 3, Lightning 3, Poison 3, Spirit 3, Physical 3. Strong: the same. Poison's average goes from 0.55 to
1.0. Lore lines that no longer fit are rewritten (B12): the gargoyle's "stone shrugs off fire and frost", the Drowned (now
weak to lightning, strong against fire).

### 7.3 With rank

Default (B3): a second strength at rank III, a third at rank V, chosen as wards are chosen now: among the lines the caster
holds, never the creature's weakness, never the only line held (B4). That makes the rank's strengths what wards were; B5 asks
whether wards go (default: yes, folded in; a rank's strength can still crack after a fifth of its health in that element, as a
ward does, so a person learns it can be broken). Packs share theirs, as wards are shared now.

### 7.4 Elemental creatures

Past rank I a creature may be born to an element (B7-B9): its attacks carry it some of the time, and its eyes and its missiles
show it. Default: rank II 15%, III 25%, IV 35%, V 45%; a pack shares one; any of the five that aren't Physical, weighted to the
run (I Fire and Spirit, II Frost and Poison, III Lightning and Poison, IV Fire and Spirit). Its own element becomes one it is
strong against (B6); its weakness stays.

**The eyes** (B10). Today the eyes' colour is the kind's at rank I and the rank's from II, and each rank has its own form (a
glow beside, a cold slit, a streak to the temple, a white flare, a gold cross). Default: an elemental creature's eyes take
its element's colour (`ELEM_RAMP`), and the rank keeps the form, so both still read. **Missiles** of an element shimmer in
its colour (motes trailing, as a wisp's); plain bone stays plain.

## 8. New attacks (step 5)

Every one through the attack table and `foe_hit`; every one has a tell a person can read and the dream can answer.

### 8.1 The skeleton

- **Its element** (M1): an elemental skeleton throws its element one throw in three (shimmering), plain bone otherwise.
- **The Bone Thrower**: three bones lobbed 0.5 s apart in a small arc (default: over 20 degrees, rank II and up, once in 12-18 s
  while it sees you; M3). Its body's edge flashes 0.6 s first (`DR_EDGE`, briefly). The ten-missile pool (`MAXBONES`) stays
  as it is: a full pool refuses the rest, as now.

### 8.2 The shield-bearer's charge

From 2 to 5 cells off, in a fight, with a clear lane: a stamp and the shield raised (0.5 s tell), then up to 5 cells at speed
along the lane (stopping at walls, doors and other creatures). A hit knocks the caster down 1.2 s. Cooldown 8-16 s at random.
**Mitigation** (M4), as the guide puts it: a dodge (the Dodge roll, or a step aside: Swift Dodge, Blink); a rune (the Bone Ward
holding all of it); or armour at least twice the charge's damage. Default damage 1.2 x the blow (M5). Unbound or Stunned, it
doesn't charge.

### 8.3 The shambler

Elemental by the colour of its eyes, and its grab carries the element at a much higher chance (default x2.5; M2): Frost a
Chilled stack (or two), Poison Poisoned, Fire On Fire, Spirit **souls taken** (2 + its rank, as the shade's touch; the
guide's "steal souls", in place of Unbound), Lightning Stunned, Physical **the grapple**: knocked down without the fall (the
view doesn't drop; the hand pushes at it), 1.2 s.

### 8.4 The Drowned

A spew of water up to two cells (default every 6-9 s from 1.2 to 2 cells, a 30-degree cone, a 0.5 s swell first): 80% of its
blow, and Poisoned. Never elemental (M7). Its burst on dying stays.

### 8.5 The hound

The skeleton's rules without the throw: past rank I, elemental bites, eyes in the element's colour.

### 8.6 The dream's counters

The charge's tell and the spew's swell go into `hz_dmap` as warnings, so `decide` steps off the lane; `auto_dodge` answers
the charge as it answers a missile. The Bone Thrower's flash is a missile warning. Knocked down or frozen, the dream waits; it
drinks under 40% as now.

## 9. The classes' own (step 6)

**The guide does not hold the starting pools** (K1); this draft follows the Confessions' order of each class (section 10), so
a class starts where it is best. Each is the caster's own (`gear_set_own` in `caster_sync`, under the same caps), nothing to
save, and the class's line gifts stay (K2).

| Class | Health | Starts with (a draft) |
|---|---|---|
| Witchfinder | 95 | Spell Haste +6%, Spirit Cost -6%, Spell Effect +8%, Resist Burn +10 |
| Anatomist | 105 | Armour +5, Resist Slow +15, Physic +15% |
| Galvanist | 90 | Dodge +6%, Movement +6%, Quiet +15%, Resist Lightning +15 |
| Plague Doctor | 100 | Resist Poison +25, every status's chance -10%, Luck +1%, Treasure +5%, Sprint +0.5 s |
| Sin-Eater | 95 | Restores 0.1 a second, Resist Spirit +15, souls from kills +10% (the vessel's twenty as now) |
| Headsman | 120 | Armour +4, Sprint +1 s (the bones and the tenth of armour as now) |

Flat, not growing with Atonement (K3); the Confessions are the growth.

## 10. The Unholy Confessions (step 7)

Her menu's *ASCEND* keeps its name for the act (Levels of Atonement claimed, ten points each to 50), but what the points buy
becomes **the Unholy Confessions**: six groups in place of today's sixteen lines (`LINES`, `UI_ASCEND`). Cut in the stone kit
as the other screens are (a sheet first).

### 10.1 The structure

Default (U1): five ranks a Confession (I-V), each raising everything in it together; ranks cost 10, 20, 30, 40, 50 points (150 a
Confession, 900 for all six against 500 points at Atonement 50: a caster confesses three or four deeply, or all six shallowly).
Past 50, an overlevel raises one Confession past V (VI at 55, VII at 60 ...; U6).

### 10.2 The six

The guide's numbers where it gives them; the rest defaults. Each at rank I / V; between them evenly.

| Confession | What it does | I | V |
|---|---|---|---|
| **Restoration** | the Restoration spell (10.3); Physic | 0.5 HP/s for 0.7 souls/s; Physic +10% | 2 HP/s for 0.1 souls/s; Physic +50% |
| **Agility** | Knocked Down, Stunned and Frozen rarer and shorter; Dodge, Movement, Quiet | -30%; +2% / +3% / +5% | -85%; +10% / +15% / +25% |
| **Spirituality** | souls from kills; Unbound rarer and shorter; Spell Haste, Spirit Cost, Spell Effect | +10%; -30%; +2% / -3% / +4% | +50%; -85%; +10% / -15% / +20% |
| **Constitution** | sprint recovery and length; health; a killing blow survived (10.4) | +10%, +0.5 s; +15; 5% | +50%, +2.5 s; +100; 25% |
| **Survival** | every other status rarer and shorter (Poisoned, Chilled, On Fire, grabs, webs, bile, Soul Sap); Treasure, Luck | -15%; +3%, +0.5% | -60%; +15%, +2.5% |
| **Toughness** | armour; cracks seen from further off (10.5) | +2 and +5% of worn; 4 cells | +14 and +25% of worn; 8 cells |

What becomes of today's lines (U5): Health into Constitution; Dodge and Movement into Agility; Spell Haste and Attunement into
Spirituality; Treasure and Luck into Survival; Sprint into Constitution; Physic into Restoration; the five resists into gear
alone (they cut their element's harm; the Confessions cut statuses). **Damage has no Confession**: a spell's strength is the
Alignment's and the gear's. Points spent in the old lines come back as points when a page is read (U5).

### 10.3 Restoration, the spell

A seventh spell (U8): key 7 and a place on the spells' wheel; held to channel after a breath's wind-up; walking at half pace
while it runs; a blow on you doesn't end it but a stun, a knock-down or Unbound does; souls spent by the second, healing by the
second. Drawn as a pale glow up the arm. U9 asks whether its numbers stay flat (2 HP a second is slow once health is 400) or
become a share of health (0.25% to 1% a second).

### 10.4 The killing blow survived

When a direct blow (not a DoT, not a hazard's seep) would kill, roll Constitution's chance: the blow lands, health stops at 1,
and nothing touches you for 3 s (as Second Wind's `G.wind_t`, drawn as it). Default (U10): Second Wind first while it is
unspent; the killing blow at most once a level.

### 10.5 Toughness's cracks

A shut secret's crack shows today within about two cells. With Toughness, motes rise from the crack (in the run's colour, as
the marked things' do), visible from 4 cells at rank I to 8 at V, thicker by rank, drawn through fog as the eyes are.

### 10.6 The class's weight, out of sight

Each class gets each Confession at a share by its place in the guide's lists: first 100%, second 90%, then 80%, 70%, 60%,
50%. It scales what a rank gives, never its cost (U4). The screen never names it (U3: default, the numbers shown are what the
caster actually gets, with no word of why). From the guide (Survival's "HA" read as the Headsman, U14):

| | Restoration | Agility | Spirituality | Constitution | Survival | Toughness | Sum of places |
|---|---|---|---|---|---|---|---|
| Witchfinder | 90% | 80% | 100% | 70% | 50% | 60% | 21 |
| Anatomist | 80% | 50% | 70% | 80% | 60% | 100% | 22 |
| Galvanist | 60% | 100% | 80% | 60% | 90% | 50% | 22 |
| Plague Doctor | 70% | 90% | 50% | 90% | 100% | 80% | 18 |
| Sin-Eater | 100% | 60% | 90% | 50% | 80% | 70% | 21 |
| Headsman | 50% | 70% | 60% | 100% | 70% | 90% | 22 |

The Plague Doctor's places sum lowest, so it gets the most from the Confessions overall; that suits the weakest past.

### 10.7 The dream

`auto_ascend` spends by its class's two best Confessions first, then evens out. It never confesses a person's run, as now.

## 11. The spells' augments (step 8)

### 11.1 The structure, as read from the guide

Each line has augments, three ranks each. Slots open at the line's three great steps: **its opening** (first learnt), **its
ability** (the three-point node: Firestarter, Deep Chill, Ionization, Seep, Harvest, Impale) and **its ultimate**. At each the
caster chooses one of two at the Lampstand; so a line ends with three augments chosen. Choosing is done with **Y (Space)** on
the line's augment slot at the Lampstand (`UI_ALIGN`), where Space today unbinds a line (that moves to Back; A6). The guide says
"five augments for each line" and also "one given at the start, two after the ability, two for the ultimate", while Fire's
example has a pair at each tier: A1 asks which.

**Credits.** Once chosen, an augment is fed by the line: a kill whose last blow was the line's spell, or a thing that wants the
spell struck (`mk_done`), counts; ranks come at once, in the crypt, when enough is counted (the guide: "unlock immediately ...
so a player who is active in the depths gets the benefit while in combat"). Default (A3): a kill 1, a thing 2, a boss 10;
ranks at 20, 50 and 100 credits, each augment in turn (A4: the newest chosen first, or the one the caster marks at the
Lampstand). The target (A2): all three chosen augments of the class's line at rank III by the end of run II (level 20), checked
with the dream's counts and tuned.

**They stack**: Burning Intensity with Multiply is a charged shot that splits, and costs both (A7: default, the increases add:
+30% and +30% is +60%).

### 11.2 Fire (the guide's)

- **Tier 1: Burning Intensity / Firecracker.** Burning Intensity: the bolt can be held; at once it is 20% weaker, held it grows,
  to +25% of its damage and 2.5 times its size at three seconds, costing up to +30%. Firecracker: a spark, 25% of the damage,
  near-certain to ignite, its burning three times as fierce (A11).
- **Tier 2: Multiply / Swift Flight.** Multiply: split in an arc, each half the cast's damage, +30% spirit. Swift Flight: the
  bolt flies far faster and further.
- **Tier 3: Volatility / (the guide's second is missing).** Volatility: what dies of your burning bursts, setting all within reach
  alight and dealing a third of the victim's health to them, +30% spirit. A12 asks for a cap on chains (default: each creature
  bursts once, a boss takes a tenth of what a burst would give).

Two clashes with what is built (A9, A10): **Firewall** is held too (Fire's path B ultimate), and the Fire line already has a node
called **Burning Intensity** (burning lasts longer). Defaults: with both, a hold grows the bolt and a hold past three seconds
calls the Firewall; the node is renamed *SLOW EMBERS*.

### 11.3 What three ranks mean

Default (A8): the guide's numbers are rank III; rank I gives half, rank II three quarters (Multiply: 0.3, 0.4, 0.5 of the
cast's damage a projectile; Volatility 17%, 25%, 33%).

### 11.4 The other five lines (drafts, for the owner to replace)

| Line | Tier 1 | Tier 2 | Tier 3 |
|---|---|---|---|
| Frost | **Hoarbite**: 20% weaker, every hit two Chilled stacks / **Splinterfrost**: the shard breaks into three behind its target at 30% | **Rime Trail**: the shard rimes the floor along its flight 2 s / **Glacial Weight**: slower, pierces one creature at 70% | **Cold Snap**: a creature frozen bursts a stack onto all within 1.5 cells / **Brittle**: the frozen take +25% more |
| Lightning | **Forked**: a second bolt to another within 3 cells at 50% / **Overcharge**: +30% damage, stun chance doubled, +25% spirit | **Static Build**: each hit on one creature within 3 s raises its stun chance, three times / **Grounding**: what stands in water or is poisoned passes it to one more | **Thunderclap**: a stunned creature struck stuns all within 1.5 cells / **Conductor**: Ionization's trail lasts three times as long and stuns |
| Poison | **Thick Brew**: Poisoned half as long again / **Straight Throw**: thrown flat and fast, bursting on what it meets | **Cluster**: the burst throws three small flasks at 40% / **Miasma**: the burst leaves a cloud 1.5 s that poisons what passes | **Pestilence**: the poisoned leave a pool when they die / **Rot**: the poisoned take +15% from every other element |
| Spirit | **Siphon**: heals 30% more, hurts 15% less / **Hollowing**: Unbound's chance doubled | **Lingering Hunger**: the beam holds 0.6 s after the cast / **Soul Echo**: a kill by the drain returns its spirit | **Exorcism**: the Unbound take +30% / **Shepherd**: the drain's kills' motes fly to you |
| Physical | **Barbed**: the struck bleed half the hit over 3 s / **Recovery**: a spear that kills leaves its bone half the time | **Twin Throw**: two spears side by side for one bone, 60% each / **Heavy Haft**: the first struck staggers 0.5 s | **Skewer**: each creature pierced adds 15% for the next / **Bone Rain**: a spear that kills splinters into three that seek the nearest |

### 11.5 Death and the dream

Augments, their credits and their choices go with the lines when a death costs, and come back as the lines are bought again
(A14). The dream chooses at random within the pair and feeds them as it plays (A15).

## 12. The merchant by class (step 9)

The owner: only a few pulls should carry stats no use to the class; nothing Rare or better without something for a spell the
caster holds. "No use" is an element's stat for a line not held (its Power, Efficiency, Insight, its "ADDS %", its status
chance); everything else (health, armour, Dodge, Movement, the resists, Haste, Spirit Cost, Spell Effect) is of use to everyone.

- **Off-line stats** (G1): one piece in six may carry one; never two. Otherwise an element's stat is rolled from the lines held,
  weighted by rank (`lean_elem` today does this a third of the time; it becomes always but for that sixth).
- **Rare and up** (G2): must carry at least one stat for a line held; rerolled until it does (a Rare hood with Insight for a held
  line, a Rare ring with that line's Power or status chance).
- **New stats** (G3): a status chance per element (*IGNITION*, *RIME*, *STATIC*, *BLIGHT*, *HUSH*: raise the chance of your
  element's status), on rings and bracers; *STEADINESS* (Knocked Down, Stunned and Frozen shorter and rarer, under Agility's cap),
  on boots and amulets. Appended to `ST_*`, so old pieces read as they were.
- **"ADDS %"** (G4) today rolls only fire, frost and lightning, at random; it rolls among the lines held, all six.
- **Class-specific abilities on gear** (G5): the owner's message says the guide offers them, but they weren't in it. When they
  come, they are new `AB_*` (a byte in the piece already; the Codex's bits have room).

## 13. Saves

All of it as ledger sections (a reader steps over what it doesn't know), so the run's version needn't change:

- `LG_CONF` (the Confessions' ranks and overlevels, a byte each); today's line steps (`LG_CASTER`) read once more and paid back
  as points, then written empty.
- `LG_AUG` (each line: the augment chosen at each tier, its rank, its credits, u16).
- The pieces' new stats are new `ST_*` numbers in the bytes they have.
- Statuses, elemental creatures and class sheets are never saved.

## 14. The dream

It must do all of it, for the screensaver and the harness: take its augments and feed them, confess by class, read the new tells
(8.6), wait out what holds it, and never stand still in a stun-lock (S1 makes that impossible). Debug for each step: `EVENT
af <status> <on|off> by <kind>`, `EVENT ignite`, `chill_stack`, `frozen`, `stun`, `unbound`, `down`, `bone_thrower`, `charge`,
`spew`, `aug <line> <tier> <rank>`, `confess <line> <rank>`; `CRAWL_AF=k` (a status on the caster every ten seconds),
`CRAWL_ELEM=e` (every creature that can be, of element e), `CRAWL_AUG=...`, `CRAWL_CONF=...`, `CRAWL_CHARGE=1`.

## 15. The order

Each step committed, pushed and logged at its end, as the art pass's log does, so the work can be handed on between sessions.

1. **One road for harm** (section 3): the attack table, `foe_hit`, `HURT_SPELL`, `HURT_MAGIC` mended, `make combat`; the review's
   fixes the owner takes (P1-P2). Sixteen seeds against 1.0.0b18 to show nothing else moved.
2. **Statuses** (sections 4-5): the sheet of glyphs and the knock-down's frames first; then the caster's and the dead's; the old
   effects folded in. Nothing imparts them yet but the old sources and `CRAWL_AF`.
3. **The elements on the caster's spells** (section 6).
4. **The bestiary's elements** (section 7): the table, the ranks' strengths in place of wards, the eyes' colours (a sheet of
   every kind's eyes in each element), elemental creatures; the bestiary's and the Codex's pages, the lore lines.
5. **New attacks** (section 8): a sheet of the flash, the charge, the spew; then each in the game with the dream's counters.
6. **The classes' own** (section 9).
7. **The Unholy Confessions** (section 10): the screen's sheet first; Restoration, the killing blow, the cracks; old pages paid
   back.
8. **The augments** (section 11): Fire first, played and tuned; then each line in turn.
9. **The merchant by class** (section 12).
10. **The checks**: sixteen seeds from each start for each class against 1.0.0b18, ASan, the README's and the page's words, the
    release notes for a player.

## 16. Questions

Defaults are marked (a\*). The owner's answers go after each, as in the other plans.

### Still open from the review

- **P1. The skeletons' throws.** (a\*) Never quicker than 0.6 s between throws (level 18's pace); (b) leave them.
- **P2. Flat armour.** (a\*) Armour takes a share: harm x 100 / (100 + armour x 100 / (4 + 0.8 x level)), so the same armour
  means the same share at any level against a rank-I blow, and Toughness, the Anatomist's and the Headsman's still tell;
  (b) keep it flat (the shield-bearer's "twice the armour" then means something early and nothing late); (c) leave it.
- **R1. Which release.** (a\*) Before the art pass's rest and the lower storey: this becomes 1.0.0b19, the art pass 1.0.0b20,
  the lower storey 1.0.0b21; (b) after them.
- **R2. One release or three.** (a\*) Three: statuses, elements and new attacks (steps 1-5); the classes and the Confessions
  (6-7); the augments and the merchant (8-9); (b) all at once.

### Statuses

- **S1. No locks.** (a\*) When a hard control ends, the same can't land for 3 s and none for 1 s; (b) each within ten seconds of the
  last half as long; (c) nothing.
- **S2. Frozen.** (a\*) No moving, casting or runes; Blink and Purify break it; (b) no moving, casting allowed.
- **S3. Chilled falling off.** (a\*) All stacks 3 s after the last chilling hit; (b) one stack every 3 s.
- **S4. Chilled's numbers.** (a\*) -8% pace a stack, sprint recovering at 40%, aim spread 1.5 degrees a stack (missiles only;
  lightning and the drain unaffected); (b) yours.
- **S5. Poisoned.** (a\*) Today's short harm a second; the cast slowing (x1.5 for the caster) lasts the stacked time, to a minute;
  (b) the harm lasts the stacked time too; (c) the caster's slowing x2 as the dead's.
- **S6. Knocked Down's view.** (a\*) The floor-height drop with a tilt, the hand gone, a heartbeat, 0.4 s to rise; (b) a caption and
  the hand only.
- **S7. A row of status glyphs** by the portrait and in the readout. (a\*) Yes; (b) captions only.
- **S8. What Unbound stops.** (a\*) Spells, runes and E; not the map, Tab, the wheels or walking; (b) everything but walking.
- **S9. The old effects as statuses** (the Wraith's touch and frost hazards Chilled; bites Poisoned; the void left alone). (a\*) Yes;
  (b) leave them all as they are.
- **S10. Do hazards impart statuses?** (a\*) Yes, each its element's, at the base chance; (b) no, only creatures.
- **S11. Element resists and status chances.** (a\*) An element's resist also cuts its status's chance by half its value (Resist Burn
  40% makes ignition 20% rarer), besides the Confessions; (b) only the Confessions and Steadiness.
- **S12. Bosses.** (a\*) Their own ways stay; their elements' statuses come later, a boss at a time; (b) now, with the rest.

### The elements' effects

- **E1. The caster's chances.** (a\*) 10% at rank 1 to 25% at rank 20, Spell Effect raising it, a creature's strength halving it, its
  weakness adding half; (b) flat 15%.
- **E2. Fire stacking.** (a\*) A new ignite adds its damage to what is left and starts the four seconds again; (b) the stronger
  replaces the weaker; (c) as burning now, never stacking.
- **E3. A stunned creature.** (a\*) No blow, throw or call; half pace; bosses stunned for a third as long; (b) stands still.
- **E4. Creatures frozen by stacks.** (a\*) Yes, four stacks, 3 s, +33%; Deep Chill freezes at three; (b) creatures only slow.
- **E5. An Unbound creature.** (a\*) No elemental attack, Bone Thrower, charge or call; (b) only its elemental attacks.
- **E6. A poisoned creature.** (a\*) Every wind-up and cooldown doubled; (b) only throws and casts.

### The bestiary's elements

- **B1. Flatten the table.** (a\*) Weak x1.5, strong x0.5, the rest x1; (b) keep today's shades beside the one weakness and strength.
- **B2. The table in 7.2.** (a\*) As drafted; (b) the owner's changes.
- **B3. Strengths by rank.** (a\*) A second at rank III, a third at V; (b) one more at each rank from II.
- **B4. Never against the only line held.** (a\*) Yes, as wards today; (b) any.
- **B5. Wards.** (a\*) Folded into the ranks' strengths, which can still crack; (b) both, a ward on top of the rank's strength.
- **B6. An elemental creature's element.** (a\*) It becomes a strength; its weakness stays; (b) and it becomes weak to its opposite
  (Fire and Frost, Lightning and Poison, Spirit and Physical); (c) it changes nothing.
- **B7. Which kinds are born elemental.** (a\*) Skeletons, shamblers and hounds, as the guide; the shield-bearer and the knight in a
  later round; (b) every kind with eyes and a blow.
- **B8. How often.** (a\*) Rank II 15%, III 25%, IV 35%, V 45%, a pack sharing one; (b) yours.
- **B9. Which elements.** (a\*) Any but Physical, weighted to the run (I Fire, Spirit; II Frost, Poison; III Lightning, Poison; IV
  Fire, Spirit); (b) evenly.
- **B10. The eyes.** (a\*) The element takes the colour, the rank keeps the form; (b) a ring of the element's colour round the rank's.
- **B11. Does the target's readout show the strengths a rank adds?** (a\*) Yes, once seen struck (as weaknesses are learnt now); (b) at
  once.
- **B12. The lore lines.** (a\*) Rewritten where they no longer fit; (b) left.

### New attacks

- **M1. An elemental skeleton's throws.** (a\*) One in three its element; (b) every throw.
- **M2. Chances on the caster.** (a\*) 12% at rank II, +4% a rank; a shambler x2.5; (b) yours.
- **M3. The Bone Thrower.** (a\*) Rank II and up, once in 12-18 s while it sees you, a 0.6 s flash, three bones over 20 degrees; (b)
  rank I too.
- **M4. The charge's mitigation.** (a\*) Not knocked down if it is dodged, if the Bone Ward takes all of it, or if armour is at least
  twice its damage (the harm still lands); (b) armour counts double against its harm, and a charge that does no harm doesn't knock down.
- **M5. The charge's harm.** (a\*) 1.2 x the blow; (b) the blow.
- **M6. The shambler's Spirit** takes souls (2 + its rank), as the guide lists, rather than Unbound. (a\*) Yes; (b) Unbound.
- **M7. The Drowned's spew.** (a\*) Every 6-9 s, 1.2 to 2 cells, a 30-degree cone, a 0.5 s swell, Poisoned; never elemental; (b) yours.
- **M8. The grapple's look.** (a\*) The view stays up, the hand pushes at the shambler, a pulse at the edges; (b) the knock-down's view
  without the fall.

### The classes' own

- **K1. Starting pools.** The guide doesn't hold them. (a) The owner sends them; (b\*) the draft in section 9 until then.
- **K2. The class gifts on its line** (a fifth off its cost, and the rest). (a\*) Kept beside the sheet; (b) folded into it.
- **K3. Does a sheet grow?** (a\*) No, the Confessions are the growth; (b) a little with each Level of Atonement.
- **K4. Class abilities on gear.** The owner's message says the guide offers them; they weren't in it. (a) The owner sends them; (b) one
  drafted for each class.

### The Unholy Confessions

- **U1. Ranks.** (a\*) Five a Confession, each raising all of it, at 10, 20, 30, 40, 50 points; (b) ranks with smaller steps between,
  bought a point at a time within each; (c) yours.
- **U2. Gates.** (a\*) None: points alone; (b) rank n needs Level of Atonement 10n - 5.
- **U3. The numbers shown.** (a\*) What the caster actually gets, the class's weight in it unnamed; (b) the nominal numbers, the weight
  hidden entirely.
- **U4. The class's weight.** (a\*) On what a rank gives, never its cost; (b) on both.
- **U5. Today's lines.** (a\*) Folded as section 10.2 says; Damage dropped; the five resists left to gear; points spent paid back on
  loading; (b) a seventh Confession for the resists.
- **U6. Past Atonement 50.** (a\*) An overlevel raises a Confession past V; (b) overlevels end.
- **U7. The cornerstones** (Second Wind, Premonition, Cartographer, the Lantern, Grave Sense). (a\*) Kept as they are; (b) folded into the
  Confessions.
- **U8. Restoration's key.** (a\*) Key 7 and the spells' wheel (the pad's down held), held to channel; (b) on the right mouse with the
  gifts.
- **U9. Restoration's amounts.** (a) Flat, as the guide (0.5 to 2 HP a second); (b\*) a share of health, 0.25% to 1% a second, the souls
  as the guide.
- **U10. The killing blow.** (a\*) Second Wind first; then Constitution's chance, at most once a level; (b) every time it rolls.
- **U11. The cracks.** (a\*) Motes from 4 cells at rank I to 8 at V; (b) yours.
- **U12. Survival's statuses.** (a\*) Poisoned, Chilled, On Fire, and the old grabs, webs, bile and Soul Sap; (b) only the guide's.
- **U13. A death that costs.** (a\*) The Confessions go with the Levels of Atonement, as the lines do now; (b) they stay.
- **U14.** Survival's list says "HA": read as the Headsman. (a\*) Yes.

### The augments

- **A1. How many a line.** (a) Five: a pair, a pair and one (Fire's example, its tier 3 single); (b) one given, then a pair and a pair;
  (c\*) six: a pair at each of the three steps, one chosen from each, three kept.
- **A2. "Level their way to 20".** (a\*) Crypt level 20, the end of run II; (b) Level of Atonement 20; (c) the line's rank 20.
- **A3. Credits.** (a\*) A kill by the line 1, a thing struck 2, a boss 10; ranks at 20, 50, 100; (b) yours.
- **A4. Which augment is fed.** (a\*) The newest not yet at III; (b) the one the caster marks at the Lampstand.
- **A5. Choices.** (a\*) Final, until the line is unbound; (b) changed at the Lampstand for two coins.
- **A6. Unbinding a line** moves from Space to Back, since Y (Space) becomes the augments'. (a\*) Yes.
- **A7. Stacked costs.** (a\*) Added (+30% and +30% is +60%); (b) multiplied.
- **A8. Ranks.** (a\*) The guide's numbers are rank III; I half, II three quarters; (b) they are rank I.
- **A9. Burning Intensity and the Firewall.** (a\*) A hold grows the bolt; past three seconds it calls the Firewall; (b) they can't be held
  together.
- **A10. The old node called Burning Intensity.** (a\*) Renamed SLOW EMBERS; (b) the augment renamed.
- **A11. Firecracker's "three times as intense".** (a\*) Its burning three times the spark's damage over the four seconds (75% of a full
  bolt); (b) three times a full bolt's.
- **A12. Volatility's chains.** (a\*) Each creature bursts once; a boss takes a tenth of a burst; (b) no cap.
- **A13. Fire's tier 3.** The guide has Volatility and a blank. (a) The owner names the second; (b\*) *Pyre*: what burns spreads it to one
  creature within a cell, once.
- **A14. Death.** (a\*) Augments go with the lines; (b) kept, as runes are.
- **A15. The other lines' augments** (11.4). (a) The owner's; (b\*) the drafts until then.

### The merchant

- **G1. Off-line stats.** (a\*) One piece in six may carry one, never two; (b) yours.
- **G2. Rare and up.** (a\*) Must carry a stat for a line held; (b) also the class's gear ability, when there is one.
- **G3. New stats** (a status chance per element, Steadiness). (a\*) Yes; (b) no.
- **G4. "ADDS %"** for all six elements, from the lines held. (a\*) Yes; (b) leave it at three.
- **G5. Class abilities on gear**: K4.

## Appendix: the owner's guide (7 November), as given

> Q1. B, but we mix things up a bit. I offered a guide I've spent a day working on. Use my responses to these questions and the
> guide I add at the bottom of this response to build this out. Q2. Yes Q3. This has been outlined in the guide below. Each class
> is distinct with their own initial stat pools, so very close to A. Q4. Yes, somewhat. I only want a small number of pulls to
> include stats that are not useful to the player regarding their class. Try to prevent rare and up items from generating for the
> player that do not contain spell effects that apply to a selected spell(s) they have earned. I offered class-specific abilities
> we can attach to gear in the guide. There is a lot, so review the information I offer you below along with my answers so we can
> build out an implementation plan. Offer plenty of questions to assist with clarification, I do believe we are touching a lot of
> the critical components that drive this experience.
>
> **Elemental effects:**
> Fire - chance to ignite player/NPCs per hit for the same damage value again, distributed as a Fire DoT over four seconds.
> Ice - Chance to slow the target a small amount by giving the player a stack of Chilled, increases as hits are made
> consecutively. NPCs and the players view acquire a blue-tint as the slow value increases while a player/NPC continues to connect
> attacks.
> Lightning - Chance to stun the player/NPC for 1.2sec - Imparts the Stunned state
> Poison - Reduces spell cast speed while poisoned by 2x - Imparts the Poisoned state
> Spirit - Has a chance to silence the player/NPC by applying Unbound, allowing them to move without casing spells or rune
> abilities for 1.5sec
> Physical - Carries a chance to impart Knock Down on the player only, knocking them on their feet for 1.5sec. We'll need a method
> to display this to the player when it happens to them.
>
> **Status effects -**
> Stunned - Cannot cast abilities, can move without sprint
> Knocked Down - Cannot cast abilities or move, roots player in place for defined time for the ability. Enemies other than the one
> that knocked down the player do 50% less damage to the player while they are knocked down, the initiator retains 100% damage that
> passes through the players armor rating.
> Poisoned - Reduces spell cast speed while poisoned. Poison can stack duration during the entirety of its application to the
> player, up to one minute.
> Chilled - After acquiring a slow from an Ice ability the player should be considered Chilled. Chilled players restore stamina at
> a far slower rate while suffering a growing movement penalty as stacks are acquired. Chilled players also suffer a small accuracy
> penalty, which only applies while Chilled. Chilled stacks. If the player acquires four stacks of Chilled they enter Frozen state
> for three seconds. Chilled stacks fall off after 3sec
> Frozen - All attacks and abilities against the player, including environmental and boss sources, have their damage amounts
> increased by 33% when the player is attacked while frozen. The player cannot move, their viewport taking on an icy blue tint.
> When the player leaves frozen state their Chilled stacks are consumed.
> On Fire - Enemies that throw single-shot fire spells have a chance based on their rank to ignite the player in flames, causing
> the same DoT mechanics we use for NPCs to apply to the projectiles and attacks made against the player.
> Unbound - Player is silenced for the duration. No spells, runes, or interactions allowed.
>
> **Elementals:** NPCs should be impacted by the elements we spawn them with. All enemies of a specific type should be weak to one
> element, strong with one, and as they rank up they acquire an additional element they are strong with.
>
> **New attacks:**
> Skeleton - chance for elemental spawns and attacks outside of physical after rank 1. Denote the use of elemental abilities with a
> colored shimmer on their projectiles and their eyes. - Physical damage most of the time with no shimmer, elemental at the base
> rate past rank 1. Skeletons can occasionally used the 'Bone Thrower' ability, lobbing three projectiles in a small arc toward the
> player in rapid succession, .5s apart. Their borders briefly flash before casting.
> Skeleton Shield-bearer- Quickly charges the player, up to 5 cells, starting from 2-5 cells away from the player. Only while in
> combat, offered with a random cooldown between 8-16 seconds. Knocks down the player for 1.2sec, needs an accompanying viewport
> adjustment for this. Can only be mitigated with 2x the armor value for the damage it would inflict, a dodge, or a rune.
> Shambler - Chance for elemental spawns and attacks outside of physical after rank 1. Denote elemental status via the color of
> their eyes. Elemental attacks from Shamblers have a much higher chance to impart their effects to the player. More likely to
> slow, poison, ignite, steal souls, stun, or "grapple" (knock down without the falling animation) the player with their spawned
> elemental alignment.
> Drowned - Can now expel water up to two units away from the player as an attack, hitting them for 80% of their usual damage while
> inflicting poison. Elements do not apply to this attack.
> Hound - Same rules as skeleton minus the ranged attack. Elements apply to their attacks when spawned above rank 1.
>
> **New player skill building mechanic: Per-spell line bonuses.** These apply to the use of a spell after it has been initially
> learned and after every major upgrade step in the Lampstand. The player needs to use the spell by eliminating enemies or
> interacting with an object that requires the spell a growing number of times in order to spend points toward an augment of the
> players choice. Five augments for each of the six spell lines, three ranks each. One given at the start, the player picks from
> two granted after the player activates their first three point class ability, and another set of two for the ultimate they
> unlock. While a player has an augment(s) selected any eliminations or activations with the spell line award credit toward a rank
> unlock for that spells augment(s). The augments unlock immediately when conditions are met, so a player who is active in the
> depths gets the benefit while in combat. The player interacts with the augments for each spell at the Lampstand, by tapping Y on
> the skill that carries their unlock after they unlock it. per-line bonuses should stack with one another when earned, so a Fire
> player who picks Burning Intensity and Multiplicity has both apply to their projectiles. (A charged shot with multiple
> projectiles and stacked cost increase). This should be tooled so a player can reliably select and level all three of their
> selected options in a spell line while they level their way to 20.
>
> Examples below:
>
> Fire - Rank 1 (Burning Intensity / Firecracker), Rank 2 (Multiply / Swift Flight) Rank 3 (Volatility /
>
> Burning Intensity - Modifies the projectile to one that can be fired instantly or channeled, reduces damage by 20% When held the
> fireball the player produces grows in size and effectiveness, increasing the damage so the final value is roughly +25% of the
> initial damage value if held for three seconds. This scales over the held time, with three seconds needed to reach +25% damage
> and the largest size. The size of this projectile is capped at 2.5x. Cost is also scaled, up to +30% at full casting power.
> Firecracker - Modifies the projectile from a fireball to a spark that has a near 100% chance to ignite the NPC struck, with only
> 25% of the initial damage being inflicted to the target. The DoT from this projectile is much stronger, burning 3x as intense as
> a normal fire DoT.
> Multiply - Splits the projectile into multiple projectiles when cast, in an arc in front of the player. Each projectile carries
> damage equal to half of the total damage of the initial cast, increases spirit cost by 30%.
> Swift Flight - Radically improves the speed and distance Flame Spells can be thrown.
> Volatility - Enemies eliminated by a flame DoT initiated by the player explode violently, lighting enemies within reach on fire
> while dealing 33% of the victims HP in damage to those it hits. Increases the spirit cost by 30%.
>
> **Secondary skill line: Unholy Confessions** - Like a Class skill line but for more general purpose skill building, giving the
> player the option to boost a specific resistance or mechanic using Ascension points without needing to be in a class to do it.
> This should functionally replace the Ascension Skill Point Menu with a far more developed set of skills we let the player modify
> in groups, so managing their point budgets is a little easier than currently offered. Each class will have the bonuses and
> effectiveness of these skill lines adjusted for their class, so a witchhunter with spell casting as a focus cannot dump a ton of
> points into agility expecting the same return as a Galvanist. Points spent in restoration or spirituality should offer better
> gains for this class. I'll offer suggestions for classes that should benefit and classes that should receive a smaller benefit
> with a list alongside each listed skill line noted by rank of impact. Class-specific influences should never be visible to the
> player on this screen or in its menus. R1 = 100% of the value, R2 90%, R3, 80%, R4 70% R5 60% R6 50%. Classes will be denoted with
> abbreviations: Witchfinder WF, Anatomist AN, Galvanist GA, Plague Doctor PG, Sin-Eater SE, Headsman HE
>
> We offer six lines:
>
> Restoration - Offers the player a spell called Restoration that gives the player the ability to channel their cast to restore
> their HP. We adjust the soul consumption and healing rates per rank, so a rank 1 version of Restoration would restore about
> .5HP/sec while consuming .7 souls/sec and the rank V would restore 2HP/sec with a cost of .1 soul/sec. While casting the player is
> slowed. Increases Physic Class rank effectiveness: SE, WF, AN, PD, GA, HE.
>
> Agility - Passively impacts how sturdy the player is, reducing the rates of Knock Down, Stunned, Frozen, and their related
> durations. Rank I would offer a 30% reduction, rank V would offer 85%. Increases dodge, movement speed, and quiet .Class rank
> effectiveness: GA, PD, WF, HE, SE, AN
>
> Spirituality - Increases the rate that souls are acquired from eliminations by the player, reduces the rate the player can be
> Unbound, per rank. Rank 1 with a 30% reduction and rank V being 85%. Impacts Spell Haste, Spirit Cost, and Spell Effects. Class
> rank effectiveness: WF, SE, GA, AN, HE, PD
>
> Constitution - How quickly you regenerate sprint, your maximum sprint, your HP, and your ability to resist a killing blow. The
> Killing Blow resist offers the player a chance to avoid death by bringing them back to 1HP and a brief period of invincibility
> when a direct attack, not DoT, would lead to the players death. The attack connects, the player drops to 1HP instead of 0, then
> makes them invincible for three seconds. Class rank effectiveness: HE, PD, AN, WF, GA, SE
>
> Survival - Improves the players resistance to status effects other than Knock Down, Stunned, Frozen. Improves their Treasure
> Gain modifier and Luck. Class rank effectiveness: PD, GA, SE, HA, AN, WF
>
> Toughness - Offers armor to the player, along with an ability that reveals secret entrances (cracks) to a player with a minor
> particle effect emanating from the cracks themselves, visible from twice the distance cracks normally are that grows in intensity
> and distance visible. AN, HE, PD, SE, WF, GA
