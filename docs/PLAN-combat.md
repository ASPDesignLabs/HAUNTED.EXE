# The elements' teeth: statuses, elemental creatures, the classes' own, the Unholy Confessions and the spells' augments

**Settled** (7 November 2026). **1.0.0b19 is built and released** (8 November, harness only; with 1.0.0b18.1 merged in); 1.0.0b20's C5 and D5 are built and went out early as **1.0.0b19.1** (9 November, harness only, before J20); J20 is done and **1.0.0b20 is released** (9 November, harness only). 1.0.0b21 (C6, D6, D7) is built (9 November, harness only; C6's and D7's sheets approved), J21 is done (the owner kept the augments as they are) and **1.0.0b21 is released** (9 November, harness only): the plan is done. Written on `claude/inspiring-archimedes-dbj5n6` from main at 1.0.0b18, from a
review of how harm is dealt today (section 1), the owner's answers to that review's first four questions, and the owner's
guide (kept as given in the appendix). The owner's answers to the review: creatures cast through the kinds we have, "mixed
up a bit" as the guide sets out (Q1 b); their spells use the caster's six elements, so the resists, the Mirror and the wards
all answer them (Q2 yes); each class has its own starting pool of stats (Q3, close to a); the merchant leans to the class, a
little, and nothing Rare or better comes without something for a line the caster holds (Q4). **Section 16 was answered the
same day**: every default, but B7 (b), U1 (b), U8 (Restoration chosen as an active spell, no key of its own), A2 (b) and
K4 (b); the body below is written to the answers. It comes **before the art pass's rest and the lower storey, in three
releases** (R1 a, R2 a): **1.0.0b19** the statuses, the elements and the new attacks; **1.0.0b20** the classes' own and the
Unholy Confessions; **1.0.0b21** the augments and the merchant by class. The rest of the art pass becomes **1.0.0b22**, the
lower storey **1.0.0b23**. The owner asked for the work to be split between **two agents on this one branch**: section 17
is how (the tracks, who owns what, the markers in the shared lists, the rules for pulling and pushing), and sections 18 and 19
are the tracks' logs. Every number below not in the guide is a default, marked as one, for the owner to change by playing.

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

## 3. One road for harm (C1)

- **An attack table.** `ATTACKS[]`, an array of its own beside `KINDS` (which the dead's track edits: section 17), a row an
  attack: its kind, what it is (`AK_BLOW`, `AK_THROW`, `AK_LEAP`, `AK_SPEW`, `AK_CHARGE`, `AK_CAST`), its element (or "the
  creature's"), its factor on the blow, its wind-up, cooldown, reach, and whether it can be dodged. `update_mob`'s inline numbers and `mob_blow` both read it, so they cannot drift apart. The rows
  start as today's numbers exactly.
- **`foe_hit(m, attack, dmg)`** is the one way a creature's harm reaches the caster: it sets the blame, the element, whether
  Dodge applies, then calls `hurt_ex` with a new kind, `HURT_SPELL` (dodged, cut by its own element's resist, half armour
  unless physical), and then rolls the element's status (section 4). `hurt_by` becomes a thin wrapper.
- **`HURT_MAGIC` mended**: the wall skull's fireball and the imp's ember are Fire, the shade's touch Spirit, the Wraith's
  touch Frost, a spell the knight's Mirror turns is its own element, the Litany Spirit. Each is then resisted by its own
  resist only. This moves a few numbers (Resist Burn will matter less, Resist Slow and Resist Spirit more); the dream's
  runs say how much.
- **A balance sheet** (`make combat`: `build/host_debug combat`, `crawl_combat_sheet`, so it reads the formulas that play): for each level, kind and rank, the creature's
  health, each attack's harm and rate, the time a rank-I caster of each class needs to kill it with their line, and the time
  it needs to kill the caster through common armour. Printed before and after each step, so a change is seen on paper
  before the dream plays it.
- The review's faults, as the owner took them: the skeletons' throws never quicker than 0.6 s apart (P1); armour takes a share,
  harm x 100 / (100 + armour x 100 / (4 + 0.8 x level)), so the same armour keeps off the same share of a rank-I blow at any
  level (P2); poison's weakness mended by section 7's table.

## 4. Statuses on the caster (C2)

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
  Spirituality cuts Unbound; Survival the rest. An element's own resist also cuts its status's chance by half its value (S11).
- **Hazards** impart their element's status too, at the base chance (S10); bosses keep their own ways for now (S12).
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

## 5. Statuses on the dead (D3)

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

## 6. The elements on the caster's spells (D4)

Each of the caster's hits rolls its element's status. Default (E1): 10% at a line's rank 1, rising evenly to 25% by rank 20;
Spell Effect raises it; the augments (section 11) and a new stat (section 12) raise it further; a creature strong against the
element halves it, weak to it adds half. Physical imparts nothing on a creature (the guide's knock-down is the caster's
alone); the spear's stagger and Impale stay.

What already does something like it is reconciled, not doubled: Firestarter's burning floor gives On Fire as it does burning
now; the flask's poison is Poisoned; Frost's slow is a Chilled stack; Soul Drain's Unbound chance is the line's own.

## 7. The bestiary's elements (D1, D2)

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

A second strength at rank III, a third at rank V (B3), chosen as wards are chosen now: among the lines the caster
holds, never the creature's weakness, never the only line held (B4). That makes the rank's strengths what wards were; B5 asks
whether wards go (default: yes, folded in; a rank's strength can still crack after a fifth of its health in that element, as a
ward does, so a person learns it can be broken). Packs share theirs, as wards are shared now. The target's readout shows a
rank's strengths once one has been seen struck (B11).

### 7.4 Elemental creatures

Past rank I a creature may be born to an element (B7-B9): its attacks carry it some of the time, and its eyes and its missiles
show it. **Every kind with eyes and a blow** (B7 b): the skeleton, the bat, the shield-bearer, the web spider, the shambler, the
Drowned (its grab; never its spew), the bone hound, the hunting spider, the ghoul, the knight, the brood mother (her sacs and
spiderlings take hers), the gargoyle and the Whipper-In (his lash). The ember imp and the wall skull are Fire by nature and are
never born to another. Not the spectre, the Wraith, the shade or the Lightless (no eyes), the deacon or the Sexton (no blow),
a boss or its adds (S12), nor an ally. Rank II 15%, III 25%, IV 35%, V 45%; a pack shares one; any of the five that aren't
Physical, weighted to the run (I Fire and Spirit, II Frost and Poison, III Lightning and Poison, IV Fire and Spirit). Its own
element becomes one it is strong against (B6); its weakness stays. `Mob.elem` (1 + the element) is where it is kept.

**The eyes** (B10). Today the eyes' colour is the kind's at rank I and the rank's from II, and each rank has its own form (a
glow beside, a cold slit, a streak to the temple, a white flare, a gold cross). Default: an elemental creature's eyes take
its element's colour (`ELEM_RAMP`), and the rank keeps the form, so both still read. **Missiles** of an element shimmer in
its colour (motes trailing, as a wisp's); plain bone stays plain.

## 8. New attacks (C3, C4)

Every one through the attack table and `foe_hit`; every one has a tell a person can read and the dream can answer.

**Any elemental creature** (7.4) carries its element on one attack in three (M1), whatever its kind: a missile shimmers in the
element's colour (motes trailing, as a wisp's), a blow is shown by the eyes flaring in that colour as it strikes (they flare
white today). The chance of its status on the caster is M2's (12% at rank II, +4% a rank), the shambler's two and a half times.
What follows is what the guide adds kind by kind.

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

## 9. The classes' own (D5)

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

## 10. The Unholy Confessions (C5; Restoration, the killing blow and the cracks D5)

Her menu's *ASCEND* keeps its name for the act (Levels of Atonement claimed, ten points each to 50), but what the points buy
becomes **the Unholy Confessions**: six groups in place of today's sixteen lines (`LINES`, `UI_ASCEND`). Cut in the stone kit
as the other screens are (a sheet first).

### 10.1 The structure

Five ranks a Confession (I-V), **bought a point at a time** (U1 b): rank I is 10 points, II 20 more, III 30, IV 40, V 50 (150 a
Confession, 900 for all six against 500 points at Atonement 50: a caster confesses three or four deeply, or all six
shallowly). Every point moves all of the Confession's numbers its share of the way to the next rank at once (a point in rank
II's twenty is a twentieth of the step from I to II); what a rank opens comes when the rank is whole (Restoration's spell and
the crack sight at rank I, the killing blow's chance from rank I). The screen shows the rank, the points into the next, and
what the next point gives. Past 50, an overlevel raises one Confession past V (VI at 55, VII at 60 ...; U6).

### 10.2 The six

The guide's numbers where it gives them; the rest defaults. Each at rank I / V; between them evenly.

| Confession | What it does | I | V |
|---|---|---|---|
| **Restoration** | the Restoration spell (10.3); Physic | 0.25% of health a second for 0.7 souls/s; Physic +10% | 1% a second for 0.1 souls/s; Physic +50% |
| **Agility** | Knocked Down, Stunned and Frozen rarer and shorter; Dodge, Movement, Quiet | -30%; +2% / +3% / +5% | -85%; +10% / +15% / +25% |
| **Spirituality** | souls from kills; Unbound rarer and shorter; Spell Haste, Spirit Cost, Spell Effect | +10%; -30%; +2% / -3% / +4% | +50%; -85%; +10% / -15% / +20% |
| **Constitution** | sprint recovery and length; health; a killing blow survived (10.4) | +10%, +0.5 s; +15; 5% | +50%, +2.5 s; +100; 25% |
| **Survival** | every other status rarer and shorter (Poisoned, Chilled, On Fire, grabs, webs, bile, Soul Sap); Treasure, Luck | -15%; +3%, +0.5% | -60%; +15%, +2.5% |
| **Toughness** | armour; cracks seen from further off (10.5) | +2 and +5% of worn; 4 cells | +14 and +25% of worn; 8 cells |

The cornerstones stay as they are (U7); a death that costs takes the Confessions with the Levels of Atonement (U13). What
becomes of today's lines (U5): Health into Constitution; Dodge and Movement into Agility; Spell Haste and Attunement into
Spirituality; Treasure and Luck into Survival; Sprint into Constitution; Physic into Restoration; the five resists into gear
alone (they cut their element's harm; the Confessions cut statuses). **Damage has no Confession**: a spell's strength is the
Alignment's and the gear's. Points spent in the old lines come back as points when a page is read (U5).

### 10.3 Restoration, the spell

A spell chosen as the others are (U8, the owner: no key of its own): once Restoration's rank I is confessed it joins the spells
the caster turns between (the mouse wheel, the spells' wheel, the pad's), and the cast button held channels it after a breath's
wind-up; walking at half pace while it runs; a blow on you doesn't end it but a stun, a knock-down or Unbound does; souls spent
by the second, healing by the second. Drawn as a pale glow up the arm. The dream chooses it as it chooses a spell: out of a
fight, hurt, with souls to spare. U9 (taken as its default, b): its healing is a share of health, 0.25% a second at rank I to
1% at V, the souls as the guide.

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

## 11. The spells' augments (C6, D7)

### 11.1 The structure, as read from the guide

Each line has **six augments, a pair at each of its three great steps** (A1 c): **its opening** (first learnt), **its ability**
(the three-point node: Firestarter, Deep Chill, Ionization, Seep, Harvest, Impale) and **its ultimate** (whichever path's comes
first). At each the caster chooses one of the pair at the Lampstand, so a line ends with three augments, three ranks each.
Choosing is done with **Y (Space)** on the line's augment slot at the Lampstand (`UI_ALIGN`), where Space today unbinds a line
(that moves to Back; A6). A choice is final until the line is unbound (A5).

**Credits.** A line's credits are counted from its opening: a kill whose last blow was the line's spell, or a thing that wants
the spell struck (`mk_done`). A kill 1, a thing 2, a boss 10 (A3). They are banked by the line and spent at once, in the crypt,
on the newest augment chosen that is not yet at rank III, then the next newest (A4); what can't be spent yet (every chosen one
at III, the next not chosen) waits in the bank, so a pair chosen late can rank up the moment it is chosen. An augment's ranks
come at 20, 50 and 100 credits of its own (A3; 300 for a line's nine). **The target** (A2 b): a caster who plays the class's
line reaches all nine ranks by **Level of Atonement 20**. The thresholds are checked against the dream's counts (credits banked
by Atonement 20, by class) and, if the target can't hold at A3's numbers, scaled down together until it does; the target
governs. Where a line's ultimate comes after Atonement 20, its pair ranks up from the bank as soon as it is chosen.

**They stack**: Burning Intensity with Multiply is a charged shot that splits, and costs both (A7: default, the increases add:
+30% and +30% is +60%).

### 11.2 Fire (the guide's)

- **Tier 1: Burning Intensity / Firecracker.** Burning Intensity: the bolt can be held; at once it is 20% weaker, held it grows,
  to +25% of its damage and 2.5 times its size at three seconds, costing up to +30%. Firecracker: a spark, 25% of the damage,
  near-certain to ignite, its burning three times as fierce (A11).
- **Tier 2: Multiply / Swift Flight.** Multiply: split in an arc, each half the cast's damage, +30% spirit. Swift Flight: the
  bolt flies far faster and further.
- **Tier 3: Volatility / Pyre.** Volatility: what dies of your burning bursts, setting all within reach alight and dealing a
  third of the victim's health to them, +30% spirit; each creature bursts once, a boss takes a tenth of what a burst would give
  (A12). Pyre (A13 b, ours): what burns spreads its burning to one creature within a cell, once.

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

## 12. The merchant by class (D6)

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
- **Class-specific abilities on gear** (G5, K4 b: drafted here). New `AB_*` (a byte in the piece already; the Codex's bits have
  room), one a class, its numbers by quality as the others' are (shown as the Common to Exquisite range):

  | Class | Ability | What it does |
  |---|---|---|
  | Witchfinder | **PYREBRAND** | your ignitions 5-15% more often, and they burn a second longer |
  | Anatomist | **THE SLAB** | the Chilled stacks you lay last a second longer; the frozen take 5-15% more from you |
  | Galvanist | **LIVE WIRE** | your stuns 5-15% more often; a stunned creature you strike passes a fifth of the hit to the nearest within two cells |
  | Plague Doctor | **FEVER** | the creatures you poison take 5-15% more from every element |
  | Sin-Eater | **SWALLOWED SIN** | Unbound from your spells 5-15% more often; what dies Unbound leaves a soul more |
  | Headsman | **THE BLOCK** | Knocked Down 15-40% shorter (with Agility, under its cap); a spear that kills a staggered or pinned creature leaves its bone |

  A piece of Uncommon or better that rolls an ability rolls the caster's class's one time in three. One of another class's (from
  the Compendium, the Repository, or a vow changed) does nothing, and its card says whose it is (*FOR THE WITCHFINDER*).

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
af <status> on by <what/type/rank>` and `EVENT af <status> off` (C2 built it so, one line for every status), `bone_thrower`,
`charge`, `spew`, `aug <line> <tier> <rank>`, `confess <line> <rank>`; `CRAWL_AF=0` none of them, `CRAWL_AF=<status>` or `all`
(that one on the caster every ten seconds: the names `fire`, `chill`, `frozen`, `stun`, `poison`, `unbound`, `down`),
`CRAWL_ELEM=e` (every creature that can be, of element e), `CRAWL_AUG=...`, `CRAWL_CONF=...`, `CRAWL_CHARGE=1`.

## 15. The order: three releases, two tracks

Two agents build it at once on this branch (section 17), one a track: **the caster's track (C)**, harm that reaches the caster,
and **the dead's track (D)**, harm that reaches the dead. Each release's steps are split between them; a release is cut only when
both tracks' steps for it are built and its joint checks are done. Each step committed, pushed and logged at its end in the
track's own log (sections 18 and 19), so the work can be handed on between sessions.

### 1.0.0b19: the statuses, the elements, the new attacks

| Step | Track C (the caster's) | Track D (the dead's) |
|---|---|---|
| 1 | **C1. One road for harm** (section 3): `ATTACKS[]`, `foe_hit`, `HURT_SPELL`, `HURT_MAGIC` mended, `make combat`; P1's floor under the throws and P2's armour as a share. Sixteen seeds against 1.0.0b18. | **D1. The bestiary's elements** (7.1-7.3): the flat table, the ranks' strengths in place of wards (cracking kept), the readout, the bestiary's and the Codex's words, the lore lines. |
| 2 | **C2. The caster's statuses** (section 4): the sheet of glyphs and the knock-down's frames first; then the seven, their rules, the old effects folded in, the view, sounds and rumble, the dream waiting them out. | **D2. Elemental creatures** (7.4): `Mob.elem` at birth, packs sharing it, the eyes in the element's colour and flaring in it while `Mob.elem_on` (a sheet of every kind's eyes in each element first). |
| 3 | **C3. Their elements on you** (section 8's opening): one attack in three carrying `Mob.elem` (`Mob.elem_on` while it does), M2's chances, missiles shimmering. | **D3. The dead's statuses** (section 5): `mob_stunned`, `mob_unbound` filled in, one gate at the head of `update_mob`, Frozen by stacks, drawn. |
| 4 | **C4. New attacks** (8.1-8.6): a sheet of the flash, the charge and the spew first; then each, with the dream's counters. | **D4. The elements on your spells** (section 6): the chances in `damage_mob` and the hits, Firestarter, Deep Chill and Amperage reconciled. |
| J | **J19, both:** sixteen seeds from each start for each class against 1.0.0b18 (each track runs half), ASan, then the README, the page and the notes (track C writes them, D sends its numbers in its log). | |

### 1.0.0b20: the classes' own, the Unholy Confessions

| Step | Track C | Track D |
|---|---|---|
| 5 | **C5. The Confessions** (section 10): the screen (its sheet first), points a step at a time (U1 b), the class's weight unseen, Agility, Spirituality and Survival wired into C2's statuses, today's lines folded and paid back (`LG_CONF`), overlevels, the dream's spending. Its first commit is the release's contract (17.3): `conf_rank`, `conf_value`. | **D5. The classes' own** (section 9), **Restoration** the spell (10.3), **the killing blow** (10.4) and **Toughness's cracks** (10.5), each reading C5's `conf_value`. |
| J | **J20, both**, as J19, against 1.0.0b19. | |

### 1.0.0b21: the augments, the merchant by class

| Step | Track C | Track D |
|---|---|---|
| 6 | **C6. The augments' frame** (11.1, 11.3, 11.5): the slots, the Lampstand's Y, the bank and the ranks (`LG_AUG`), the dream's choices; then **Fire's six** (11.2), played and tuned. Its first commit is the release's contract: the augment hooks the lines' spells call. | **D6. The merchant by class** (section 12): the off-line share, the Rare-and-up rule, the new stats, "ADDS %" for six, the class abilities on gear. |
| 7 | | **D7. The other five lines' augments** (11.4's drafts, or the owner's), on C6's frame once its contract is in. |
| J | **J21, both**, as J19, against 1.0.0b20. | |

After J21: the rest of the art pass (InitialArtPass's P7, P8, the Wraith Unbound) is **1.0.0b22**, the lower storey **1.0.0b23**.

## 16. Questions

Defaults are marked (a\*). **Answered 7 November**: the owner took every default but B7 (b), U1 (b), U8 (Restoration chosen as an
active spell) and A2 (b), and chose (b) for K4, which had none; the answers that settle something are noted after their
question. The body above is written to them.

### Still open from the review

- **P1. The skeletons' throws.** (a\*) Never quicker than 0.6 s between throws (level 18's pace); (b) leave them.
- **P2. Flat armour.** (a\*) Armour takes a share: harm x 100 / (100 + armour x 100 / (4 + 0.8 x level)), so the same armour
  means the same share at any level against a rank-I blow, and Toughness, the Anatomist's and the Headsman's still tell;
  (b) keep it flat (the shield-bearer's "twice the armour" then means something early and nothing late); (c) leave it.
- **R1. Which release.** (a\*) Before the art pass's rest and the lower storey: this becomes 1.0.0b19, the art pass 1.0.0b20,
  the lower storey 1.0.0b21; (b) after them.
  **The owner: (a).** With R2 (a), the three releases are 1.0.0b19, b20 and b21; the art pass 1.0.0b22, the lower storey 1.0.0b23.
- **R2. One release or three.** (a\*) Three: statuses, elements and new attacks (steps 1-5); the classes and the Confessions
  (6-7); the augments and the merchant (8-9); (b) all at once.
  **The owner: (a).**

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
  **The owner: (b).** Section 7.4 lists them.
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
  **The owner: (b),** the draft in section 9 for now.
- **K2. The class gifts on its line** (a fifth off its cost, and the rest). (a\*) Kept beside the sheet; (b) folded into it.
- **K3. Does a sheet grow?** (a\*) No, the Confessions are the growth; (b) a little with each Level of Atonement.
- **K4. Class abilities on gear.** The owner's message says the guide offers them; they weren't in it. (a) The owner sends them; (b) one
  drafted for each class.
  **The owner: (b).** Drafted in section 12.

### The Unholy Confessions

- **U1. Ranks.** (a\*) Five a Confession, each raising all of it, at 10, 20, 30, 40, 50 points; (b) ranks with smaller steps between,
  bought a point at a time within each; (c) yours.
  **The owner: (b).** Section 10.1.
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
  **The owner: neither: Restoration is chosen as an active spell, as the others are, so it needs no key of its own.** Section 10.3.
- **U9. Restoration's amounts.** (a) Flat, as the guide (0.5 to 2 HP a second); (b\*) a share of health, 0.25% to 1% a second, the souls
  as the guide.
  **The default, (b).**
- **U10. The killing blow.** (a\*) Second Wind first; then Constitution's chance, at most once a level; (b) every time it rolls.
- **U11. The cracks.** (a\*) Motes from 4 cells at rank I to 8 at V; (b) yours.
- **U12. Survival's statuses.** (a\*) Poisoned, Chilled, On Fire, and the old grabs, webs, bile and Soul Sap; (b) only the guide's.
- **U13. A death that costs.** (a\*) The Confessions go with the Levels of Atonement, as the lines do now; (b) they stay.
- **U14.** Survival's list says "HA": read as the Headsman. (a\*) Yes.

### The augments

- **A1. How many a line.** (a) Five: a pair, a pair and one (Fire's example, its tier 3 single); (b) one given, then a pair and a pair;
  (c\*) six: a pair at each of the three steps, one chosen from each, three kept.
  **The owner: (c).**
- **A2. "Level their way to 20".** (a\*) Crypt level 20, the end of run II; (b) Level of Atonement 20; (c) the line's rank 20.
  **The owner: (b), Level of Atonement 20.** Section 11.1: the line's credits are banked from its opening.
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
  **The owner: (b), Pyre.**
- **A14. Death.** (a\*) Augments go with the lines; (b) kept, as runes are.
- **A15. The other lines' augments** (11.4). (a) The owner's; (b\*) the drafts until then.
  **The owner: (b),** the drafts until the owner's come.

### The merchant

- **G1. Off-line stats.** (a\*) One piece in six may carry one, never two; (b) yours.
- **G2. Rare and up.** (a\*) Must carry a stat for a line held; (b) also the class's gear ability, when there is one.
- **G3. New stats** (a status chance per element, Steadiness). (a\*) Yes; (b) no.
- **G4. "ADDS %"** for all six elements, from the lines held. (a\*) Yes; (b) leave it at three.
- **G5. Class abilities on gear**: K4.

## 17. Two agents, one branch

The owner (7 November): the work is to be split with another agent working in the same repository and on the same branch.
Both agents work on **`claude/inspiring-archimedes-dbj5n6`**, each in its own checkout (its own session or machine; never a
shared working tree). The agent that wrote this plan takes **track C**; the other takes **track D**, unless the owner says
otherwise. Everything here exists so that two people editing one 28,000-line file never trip on each other.

### 17.1 The tracks

- **Track C, the caster's**: harm that reaches the caster. In 1.0.0b19: C1 to C4 (section 15); it also cuts the releases.
- **Track D, the dead's**: harm that reaches the dead. In 1.0.0b19: D1 to D4.

### 17.2 What each owns (1.0.0b19)

Only the owning track changes these. Redrawn by track C's contract commit at the start of 1.0.0b20 and 1.0.0b21 (section 15
sketches who does what in each).

| Track C only | Track D only |
|---|---|
| `hurt_ex`, `hurt_by`, the new `foe_hit` and `HURT_SPELL`, `harm_of`, `windup_blow`, `windup_break`, `slow_factor`, `reset_caster_state` | `KINDS` (its weaknesses and lore lines), `weak_el`, `weak_to`, `hit_mult`, the wards (`ward_*`, `Pack.ward`) |
| the `Bone` struct, `throw_to`, `throw_missile`, `bone_land`, `update_bones`, `bite`, `update_leap` | `pick_rank`, `set_rank`, `spawn_mob` (an element at birth) |
| `mob_blow` and the new `ATTACKS[]`; the per-kind attack branches of `update_mob` (from the skeleton's throw to the bat's bite) | one gate block at the head of `update_mob`, before the per-kind branches (nothing inside them); `mob_stunned`, `mob_unbound` |
| the touches in `update_wraith` and `wraith_boss`; `latch_begin`, `update_latch` | `damage_mob`, `mob_afflict`, `mob_burn`, `mob_poison`, `deep_chill`, `daze`, Frost Nova's freeze |
| `update_human`, `human_walk`, `human_cast`, `human_use`, `cast_spell`'s aim (Chilled's spread only) | the hit cases of `update_shots`, `chain_lightning`, `flask_burst`, `tether_hold`, `update_ultimates` |
| `sprint_tick`, `sprint_recovery`, `tint`, `render_statusbar`, `render_readout` | `render_eyes` and its tables, `render_warded`, `render_target`, the bestiary's and the Codex's element pages |
| the dream's side of harm: `auto_dodge`, `decide`, the tells in `hz_dmap` | the dream's aim by weakness: `choose_spell`, `find_target` |
| the missiles' part of `render_sprites`; `crawl_combat_sheet` and `make combat`; the status glyphs in `ui_art.c` | the creatures' part of `render_sprites` |

**Shared** (either may make a small change; pull first, keep both): `update_mob` outside the two parts above, `render_sprites`
outside its two, `start_level`, `begin_shop`, the debug state lines, the Codex's lists, the Makefile, README and the page (track C
at a release).

### 17.2 b. What each owns (1.0.0b20)

Redrawn by C5's contract commit (8 November). Everything 1.0.0b19's table gave a track stays its own; these are added:

| Track C only | Track D only |
|---|---|
| the Confessions: `CONF`, `CONF_W`, `CONF_CUM`, `conf_*`, `RUN.conf`, `RUN.conf_over`, `LG_CONF`; `LINES` and the old lines paid back (`LG_CASTER`'s steps) | the bodies of `class_own`, `class_extra` and `blow_survived` (17.3), in their marked block before `xp_rooms`, and whatever they need |
| `ascend`, `line_buy`, `over_due`, `auto_ascend`, `update_ascend`, `render_ascend` and the screen's sheet; `caster_sync` (but its one line calling `class_own`) | the classes' words (`CLASSES`), her question (`update_class`, `render_class`), the Codex's class pages |
| Agility's, Spirituality's and Survival's cuts in C2's statuses (`af_rolls`, `af_put`, `af_hold`); the grabs', webs', bile's and Soul Sap's lengths; `sprint_max`, `sprint_recovery`; the souls from kills (the share on `spawn_mote` in `kill_mob`) | Restoration the spell (10.3), wherever it is chosen, cast, channelled and drawn: its `AT_` number, the spells' turning and wheel, its channel in `human_cast` (C keeps the rest of `human_cast`), the dream's choosing of it |
| Tab's caster page, the Codex's `CX_CASTER` | the killing blow (10.4) as `blow_survived` gives it, and how it is shown; Toughness's cracks (10.5): the motes at a shut secret's crack |
| | **Caps by class** (the owner, 8 November: docs/BRIEF-caps.md section 6, given to D): the body of `class_cap`, the sheets' strengths and weaknesses, `armour_keep` (for armour's cap), and showing them on her question's screen and in the Codex |

### 17.2 c. What each owns (1.0.0b21): D6's part

Drawn by track D's contract commit for D6 (9 November), since D6 began first; C6's contract commit adds C's own rows. Everything
the tables above gave a track stays its own; these are added:

| Track D only |
|---|
| the pieces' rolls and words in `crawl_gear.c`: `piece_roll`, `lean_elem`, `pick_ability`, `KIND_STATS`, the stats' and abilities' tables (`STAT_NAME`, `STAT_OF`, `STAT_SCALE`, `STAT_CAP`, `ABIL_NAME`, `ABIL_WORD`, `CAP0`, `piece_grade`), `piece_stat_text`, `piece_ability_text`, `piece_worth`, the new `st_adds`, `gear_set_class` |
| the new stats' and the classes' abilities' effects where they are felt on the dead (`hit_mult`, `el_chance`, `el_status`, `mob_ignite`, `mob_chill`, `mob_poison`'s dose, `damage_mob`), the bone and the soul they leave in `kill_mob` (one line each, beside C's share), the bodies of `gear_cut` and `abil_short` |
| the Codex's words for stats and abilities (`STAT_DOES`, `stat_does`, `ABIL_DOES`), Tab's fifth page (`render_elements`), the cards' *FOR THE ...* |

C6's contract commit (9 November) adds these:

| Track C only | Track D only |
|---|---|
| the augments' frame: `RUN`'s choices, credits, bank and order, `LG_AUG`, `aug_of`, `aug_rank`, `aug_k`, the credits (kills, things, bosses), the dispatchers `aug_*` (17.3) and their call sites | the bodies of `augd_*` (17.3), in their marked block after D5's, and whatever they need for Frost, Lightning, Poison, Spirit and Physical (11.4); those five lines' rows of the augments' names and words |
| the Lampstand's augment slots and choosing (`update_align`'s and `render_align`'s augment parts; Space to the augments, Back to unbind, A6), the Codex's augment pages | |
| the dream's choosing and feeding (A15) | |
| Fire's six (11.2): their handlers, the held bolt in `human_cast` and its clash with the Firewall (A9), *SLOW EMBERS* (A10), `Shot.scale` and how a shot is drawn by it | |

`cast_spell` is track C's now (its cost, its damage and its shots' augments); its hits through `chain_lightning`, `flask_burst` and
`damage_mob` stay track D's.

### 17.3 What the two tracks share (the contract)

In `src/scene_crawl.c`, after `Mob`, committed with this section:

- `Mob.elem`: 1 + the element a creature was born to, 0 none. Set by D (D2), read by C's attacks (C3).
- `Mob.elem_on`: the attack under way carries the element. Set by C (C3), read by D's eyes (D2), which flare in its colour.
- `AF_*` (`AF_FIRE`, `AF_CHILL`, `AF_FROZEN`, `AF_STUN`, `AF_POISON`, `AF_UNBOUND`, `AF_DOWN`, `NAF`) and `af_of(el)`, the status
  each element imparts: the same names on the caster (C) and on the dead (D).
- `mob_stunned(m)`, `mob_unbound(m)`: stubs that say no; D fills them in (D3); C's attacks call them (C3, C4) and don't start
  what they forbid.
- `Mob.c_flags` (added for C4, 7 November): draw flags (`DR_*`) track C asks a creature to be drawn with while one of its attacks
  is under way (the Bone Thrower's flash, `DR_EDGE`). Set by C, cleared by C; the creatures' part of `render_sprites` ORs it into
  the creature's flags, one line, and does nothing else with it. Anything else C draws for its attacks (a charge's after-images,
  the dust of its stamp, the Drowned's spew) C adds in the missiles' part.
- 1.0.0b20 (C5's contract, 8 November): `CF_*` (the six Confessions, `NCONF`) and each one's numbers (`CV_*`: Restoration's
  `CV_HEAL`, `CV_HEAL_SOULS`, `CV_PHYSIC`; Agility's `CV_HARD`, `CV_DODGE`, `CV_MOVE`, `CV_QUIET`; Spirituality's `CV_SOULS`,
  `CV_UNBOUND`, `CV_HASTE`, `CV_THRIFT`, `CV_EFFECT`; Constitution's `CV_RECOVER`, `CV_SPRINT`, `CV_HEALTH`, `CV_LASTBLOW`;
  Survival's `CV_SOFT`, `CV_TREASURE`, `CV_LUCK`; Toughness's `CV_ARMOUR`, `CV_ARMOUR_WORN`, `CV_CRACKS`). `conf_rank(cf)`: the
  whole ranks, 0-5, past 5 by overlevels. `conf_value(cf, which)`: the number as the caster has it, the class's weight in it,
  moved a point at a time; shares as fractions, the rest in their stat's units; 0 with nothing confessed, and Restoration's two,
  the killing blow's chance and the cracks' reach 0 till rank I is whole (the cracks' reach is today's two cells and the weighted
  gain past them). Set by C (C5); read by D (D5). `CRAWL_CONF=dddddd` in the debug build sets each Confession's rank (6-9: V and
  overlevels). D's, called by C: `class_own(own)` adds the class's sheet to `caster_sync`'s own totals (per `ST_*`, in its units,
  before the caps); `class_extra(KX_AF_CUT)` is a share off every status's chance on the caster (`af_rolls` reads it),
  `class_extra(KX_SOULS)` a share more souls from kills (C5 reads it beside Spirituality's); `blow_survived(kind)` is asked in
  `hurt_ex` when a blow that isn't a seep would kill, after Second Wind, and returns 1 if the caster lives (D sets the health and
  the breath). All three are stubs that do nothing till D5 fills them.
- 1.0.0b20, added 8 November (the owner's caps by class, docs/BRIEF-caps.md section 6): `class_cap(st)`, D's, the class's lift of
  stat st's cap in its units; `caster_sync` adds it to the caps (`gear_set_own`'s `cap_up`) for every stat. A stub returning 0
  till D fills it.
- 1.0.0b21 (D6's contract, 9 November): the new stats at the end of `ST_*` (`ST_ADD_POISON`, `ST_ADD_SPIRIT`, `ST_ADD_PHYS`:
  "ADDS %" beside `ST_FIRE`, `ST_FROST`, `ST_SHOCK`, which `st_adds(el)` names; `ST_CHANCE` + `EL_*` for the five status chances;
  `ST_STEADY`) and the classes' abilities at the end of `AB_*` (`AB_CLASS` + the class's element: `AB_PYRE`, `AB_SLAB`, `AB_WIRE`,
  `AB_FEVER`, `AB_SIN`, `AB_BLOCK`); old pieces read as they were. `gear_cut(k)`, D's: the share what is worn takes off status k's
  chance and length on the caster; C's `conf_cut` adds it to Agility's for the three hard ones, under the same 0.9 (one line,
  written by D in this commit, and it counts with the old lines too). `abil_short(ab, a, buf, n)`, D's: a class ability's words
  on Tab's caster page, whose switch now names Mirrored and leaves the rest to it. Stubs that do nothing till D6 fills them.
- 1.0.0b21 (C6's contract, 9 November): the augments (section 11). `AUG_OPEN`, `AUG_ABIL`, `AUG_ULT` (`NAUGT`), a line's three steps.
  `aug_of(e, t)`: which of the pair at step t of line e is chosen (0 none, 1 the first, 2 the second); `aug_rank(e, t)`: its rank,
  0-3; `aug_k(e, t, w)`: 0 unless w is the one chosen, else 0.5, 0.75 or 1 by its rank (A8). Set by C (C6), read by both. **The
  hooks**, declared after `Shot`, each handing a line's spell to its own (Fire's C's; the other five's `augd_*`, D's, with the
  line `e` first), and changing nothing while no augment is chosen: `aug_cost(sp)` (a share added to the spell's spirit, A7),
  `aug_dmg(sp)` (a multiplier on its damage as cast), `aug_cast(sp, s, t)` (a shot just made by the line's spell, NULL for
  lightning and the drain once they have struck; it may change the shot and make more), `aug_hit(m, atk, flags, &dmg)` (in
  `damage_mob`, after its weaknesses, before its health), `aug_af(m, atk, q)` (in `el_status`: `AUGQ_CHANCE` a multiplier on the
  status's chance, `AUGQ_HARM` on its harm), `aug_kill(m, atk, flags)` (in `damage_mob`, before `kill_mob`; `DM_TICK` a burn's or
  poison's tick; C banks the line's credit there too), `aug_tick(m, dt)` (each creature, from `mob_afflict`), `aug_fly(s, dt)`
  (each shot in flight, from `update_shots`); `augd_built(e)`: D's, which of its lines' augments are built, so the Lampstand
  offers only those. `Shot.scale`: a shot's size, 0 as drawn. The ultimates (`cast_charged`) take no augments. The call sites in
  track D's functions are one line each, put there by this commit: `damage_mob`'s hit and kill, `el_status`'s roll and harm,
  `mob_afflict`'s tick; and `update_shots`' flight (its shared part).
- 1.0.0b21 (D7's contract, 9 November): `augd_name(e, t, w)`, the name of augment w (1 or 2) at step t of line e, one of the
  dead's track's five ("" for Fire); `augd_does(e, t, w, k, buf, n)`, what it does at a rank's share k (0.5, 0.75, 1; 0 as at rank
  III), with its numbers. D's, for C's Lampstand and Codex to read beside Fire's own; filled now (11.4's drafts, the owner's A15).

**Changing the contract**: write the change here and in the code's declarations in one commit of its own, push it at once, and
put a line starting **CONTRACT** in your log. Each agent reads the other's log at every pull.

### 17.4 The marks in shared lists

Lists both tracks add to have a pair of marks: add only just above your own track's end mark, so the two never touch the same
lines and git merges them by itself.

- `src/snd.h`: the `SFX_*` enum, before `SFX_COUNT`. `src/snd.c`: the `SFXDEF` table's end, and the synth's switch before its
  `default`.
- `src/crawl.h`: the `SP_*` enum, before `NSPR`. `src/crawl_art.c`: the end of the sprite list in `crawl_art_init`.
- `src/scene_crawl.c`: the end of `Mob`, the end of `G` (what the caster carries into a pocket goes in `carry_caster`'s spans
  instead: C's by `poison_t`), the debug counters before `hurt_by`.
- `CLAUDE.md`: each track has one bullet of its own under *How CATACOMB works* (C's after *The lull's watch*, D's after *Warded
  creatures*) and writes its notes there; track C keeps the paragraph at the top.

A list that isn't marked and that both need (the Codex's `CX_*` and `CX_MAX`, a one-line enum): whoever needs it first puts it
on separate lines with a pair of marks, in a commit of its own, pushed at once.

### 17.5 Pulling and pushing

1. **At the start of every session**: `git fetch origin claude/inspiring-archimedes-dbj5n6`, check the branch out, `git pull
   --rebase origin claude/inspiring-archimedes-dbj5n6`; read the other track's log since you last looked.
2. **Commit small**: a piece that works, an hour's work at most. **Before every push**: `git pull --rebase` as above, then `make
   host` and `make build/host_debug` (no new warnings in either) and `make smoke`. Push with `git push origin
   claude/inspiring-archimedes-dbj5n6`. Never force-push; never rewrite a commit already pushed. Refused because the other pushed
   first: pull with rebase again, build again, push.
3. **A conflict**: in the other track's lines, keep theirs exactly and put yours back round them; in a shared function, keep both
   and build. If what the other meant isn't clear, push nothing, write it in your log and ask the owner.
4. **Off switches**: each new behaviour has a debug switch that takes it out (`CRAWL_AF=0` for C's statuses, `CRAWL_ELEM=0` for
   D's elements and strengths, and so on), so either track's balance runs can set the other's aside; a run's notes say which were set.
5. **Sheets** go to the owner as before; while one waits, carry on with what doesn't hang on it.
6. **A step ends with its log entry**: what was built, how it was tested (harness only, ASan, which seeds), its commits, what is left.
7. **Nothing else in a commit**: no logs, build output or saves; never the console's codes.
8. **The release**, only at the owner's word: track C cuts it (the README, the page, the notes, `VERSION`, the squash onto main as
   one commit) once both logs mark the release's steps built and its J checks are done. Track D sends its numbers in its log. The
   branch goes on after a release as it did before.

### 17.6 Starting the second agent

Paste this into a new session on `ASPDesignLabs/HAUNTED.EXE`:

> You are the dead's track (D) of docs/PLAN-combat.md in ASPDesignLabs/HAUNTED.EXE. Work on the existing branch
> claude/inspiring-archimedes-dbj5n6: you have the owner's permission to commit and push there, and to no other branch. Another
> agent is building the caster's track (C) on the same branch at the same time. Read CLAUDE.md, then docs/PLAN-combat.md in
> full, then its section 17 again and both tracks' logs (sections 18 and 19). Follow section 17 exactly: change only what 17.2
> gives D, add to shared lists only above D's end marks, pull with rebase and build before every push, never force-push. Build
> D's steps for the current release in order (1.0.0b19: D1, D2, D3, D4), sheets to the owner first where the plan says, each
> step committed, pushed and logged in section 19. Don't cut a release; tell the owner when your steps are built.

For track C in a fresh session, the same with C, C1 to C4 and section 18.

## 18. Track C's log (the caster's)

*Append only, newest at the bottom.*

- **7 November.** The plan settled with the owner. The marks (17.4) and the shared declarations (17.3) committed: `make host`,
  `make build/host_debug` (no warnings) and `make smoke` pass; nothing in the game changes. Next: C1.
- **7 November. C1 built** (harness only), in three parts, `0ef4e55`, `99f391f`, `a788b4d` (and `bff69d5`, the notes). (1)
  `ATTACKS[]` (`FoeAtk`, `FA_*`) holds every attack the dead make on the caster; `update_mob`'s branches, `update_leap`,
  `update_spider`, `update_wskull`, the Wraith's touches and `mob_blow` read it; `foe_hit` and `bone_hurt` are the one road,
  `Bone.el1` a missile's element. Three five-minute dreams from levels 1, 21 and 31 gave the old events line for line (275, 443,
  301). `mob_blow` (webs and allies only) moved a little: the shield-bearer's blow there is its blow plus one, not a tenth more, and
  the shade, the Whipper-In and the Lightless strike at their own rate. (2) `HURT_SPELL` (once `HURT_MAGIC`, an alias kept) by its
  element's resist alone (`el_resist`, which `windup_blow` reads too); armour as a share (`armour_keep`, P2); the throws' floor
  (P1); `CRAWL_OLDHARM=1` gives back 1.0.0b18's events line for line on the same three dreams. (3) `make combat`. Not changed: a
  hazard's fire, a point at a time, can still be dodged, as before. ASan clean on four sessions: a dream from level 1 for fifteen
  minutes, from 31 for ten, the Ill-Fated Knight's fight (his Mirror turned a spell, which struck), and the sheet.
  **The checks**, C1 alone (built before D1 came in), against 1.0.0b18's play (built at `2444cd2`), sixteen seeds (1-31 odd,
  `CRAWL_RUNID` the seed, so the two builds meet the same runs): from level 1 for 100 minutes, 148 levels cleared (155), 62 deaths
  (59), 43 on level 10 (39) and 19 elsewhere (20), the knight felled twice (5 times); of the deaths 11 to the knight (9) and 4 to his
  Mirror (2), where a turned frost or lightning spell is now resisted by its own resist, not Resist Burn. From 11, 21 and 31 for 50
  minutes (gear 3, 5, 8; `CRAWL_XPBANK` 9450, 35322, 77658): no deaths either way, 100, 100 and 95 levels cleared (101, 98, 98), a
  level's median time 355, 368 and 366 s (356, 371, 365). Inside the noise everywhere, then; the knight's level is the noisiest
  number there is (28 to 48 deaths between builds that never touched him). No contract changed. Next: C2, its sheet first (the
  status glyphs and the knock-down's frames), for the owner.
- **7 November. C2's sheet, for the owner** (`media/sheets/combat/c2-statuses.png`). The look of the seven, built and shown, nothing that
  plays: `G.af[NAF]`, `af0`, `af_n`, `af_k`, `af_shake`, `af_by` in `carry_caster`'s span by `poison_t`; `af_tick` (beside
  `update_hazards`) runs their clocks and how far a knock-down has the view down; nothing sets one yet but `CRAWL_AFSHOW` (the
  debug build: `k` an `AF_*`, `1:n` Chilled with n stacks, `7` the grapple, `all` four at once; the knock-down every 3 s). The
  marks are `UI_AFMARK` (ui_art.c, nine square), each on a stone of the kit's, `ui_status` (crawl_ui.c: the mark inlaid, its
  colour round the edge for the time left, pips for stacks); the row is `render_af_row`, centred over the portrait on the bar's top
  edge, or on a band at the foot while the bar is away (lifted over the readout). The view: `render_af_view` after the hand (a
  shadow at the edges for Unbound, a throb for the grapple, the dark for a knock-down; rime from the corners by Chilled's stacks
  and all round Frozen; flames at the foot; sparks wheeling while Stunned), `af_roll` (knocked down: the crypt leans, each column
  slid by its distance from the middle), the horizon and the eye in `crawl_render` (down 40 px and 0.40 lower on the floor; the
  view swimming while Stunned), the hand in `render_hand` (grey and dropped Unbound, gone down, shoving grappled) and `tint` (bluer
  by the stack, icy Frozen, warm On Fire, a little violet Unbound, greyer Stunned). `build/uisheet status` draws the marks at three
  times size. The dream's events can't change (nothing sets a status); ASan clean on five sessions of the look. Waiting on the
  owner's word; the rules (4.1), the old effects folded in, sounds and rumble come after it.
- **7 November. C2 built** (harness only), on the owner's word on its sheet, in `0b34b47` and `911418b`. The seven land and hold
  with the owner's defaults (4.1, S1-S12, E2): `af_can` (S1: no hard control within 3 s of the same ending, none within 1 s of any;
  Purify running keeps fire, cold and poison off), `af_rolls` (S11: the element's own resist halves the chance), `af_hold`,
  `af_ignite`, `af_chill`, `af_poison`; the gates `no_cast` and `rune_barred` in place of the void's at every spell, gift, Swift
  Dodge, rune and the dream's aim, and E refused unbound, frozen or down. Frozen x1.33 on all harm; down, every creature's harm but
  the knocker's halved. Where they come from: a creature's elemental hit by its rank (`af_struck`: 8% at rank I, M2's 12% at II, 4%
  more a rank; never a boss, its fight, a turned spell or the Wraith); a hazard's stroke (`af_stroke`: frost a stack, the rest 12%,
  a physical one knocking down); the Wraith's touches two stacks in place of their slow; bites, the Drowned's pool, bile and
  `hz_poison` Poisoned by what their poison rose; the cold a stack every 1.5 s in place of `hz_chill`. Purify cures fire, cold and
  poison and breaks a freeze, Blink a freeze or a knock-down (`af_purify`, `af_break`, both in their `def_trigger`s); the dream waits
  out a freeze or a knock-down at the head of `update_player`. Six sounds (`SFX_AF_*`, at the loudness of the old ones), rumble on a
  freeze and a knock-down, a short buzz for a stun. Defaults where section 4 was silent: a rolled Poisoned adds 6 s; Frozen's third
  more counts what seeps too; the knock-down from a hazard has no knocker, so every creature's harm is halved; Purify and Blink are
  the two runes a freeze lets fire, Blink alone on the floor; the dream spends Purify on a freeze out of a fight too. Physical
  blows from creatures knock nobody down (the charge and the grapple, C4, are the creatures' ways to). `CRAWL_AF=0` gives back the
  previous build's events line for line from levels 1, 21 and 31; `CRAWL_AF=<status>|all` puts one on the caster every ten
  seconds. ASan clean on five sessions (a person under each in turn; dreams from 11 and 21 with every rune, Blink and Purify
  breaking holds; a person into a pocket, out and struck down). **The checks**, statuses on against `CRAWL_AF=0` on the same build
  (D1-D4 in both), sixteen seeds (1-31 odd, `CRAWL_RUNID` the seed): from level 1 (100 minutes) 165 levels cleared and 59 deaths
  (167 and 48), 33 of them on level 10 either way, the knight felled 6 times (5); from 11, 21 and 31 (50 minutes, gear 3, 5, 8 and
  `CRAWL_XPBANK` as C1's) no deaths either way, 99, 98 and 96 levels cleared (101, 99, 100), a level's median time 364, 365 and 368
  s (361, 369, 364). Landed on the dream, from 1, 11, 21 and 31: Poisoned 65, 187, 122 and 42 times; Chilled 20, 21, 3 and 12;
  Knocked Down 19, 2, 1 and 2; On Fire 1, 0, 0 and 5; Unbound once; Stunned and Frozen never. The eleven deaths more from level 1
  were all off the knight's level (26 against 15), but only one came within 12 s of a status landing: the runs part ways at the
  first roll, and level 1's deaths off the knight have run 9 to 27 between builds that changed nothing there. The dream walks
  into few statuses: its creatures' elements wait for C3, and it sees most hazards coming. A person who stands in the bile, the
  cold or a censer's swing will meet far more of them; untried. No contract changed. Next: C3, then C4 (its sheet first).
- **7 November. C3 built** (harness only), in `29b6591`. An elemental creature carries its element on one attack in three (M1),
  decided as its wind-up begins (`elem_arm` beside each `feint_roll`, in `start_leap`, at a web spider's drop and as a bat
  settles on you; `Mob.elem_on` while it holds, a field of C's, `Mob.elem_t`, for how long, run down by `elem_tick` in `update_mob`'s
  shared part), or as a quick bite with no wind-up lands (the hound's, the brood mother's, a spiderling's). `elem_take` spends it:
  `foe_hit` makes a physical blow that element, `throw_to` the bone (`Bone.el1`), `bite` the web spider's; never a boss, an add or
  an ally, nothing from an Unbound one (E5). Its status by rank as any elemental hit (M2), the shambler's grip at two and a half
  times. A bone carrying one shimmers in its colour with three motes trailing at its height (`DR_ELEMX`, the caster's bit 24; fire
  kept low on its ramp, orange, apart from lightning's gold); D2's eyes flare while `elem_on`. Defaults where section 8 was
  silent: an attack's element held through its wind-up and 0.35 s after, then forgotten; the web spider's bite carries it as a
  status only (the bite does no harm of its own); the shambler's two and a half times is in now, its grapple and Spirit's souls wait
  for C4; ordinary physical blows knock nobody down. `CRAWL_AF=0` takes C3 out with the statuses (C2's events line for line from
  levels 1, 21 and 31); `CRAWL_ELEMON=1` makes every attack carry it, for frames; `EVENT elem_hits`, `elem_throws`. ASan clean on
  two sessions (a dream from 21 with every creature of Lightning and every attack carrying it; a person gripped by three Frost
  shamblers). **The checks**, the same sixteen seeds as C2's, statuses on: from level 1, 161 levels cleared and 54 deaths, 40 on
  level 10 and 14 elsewhere, the knight felled 4 times (C2 on: 165, 59, 33 and 26, 6; off: 167, 48, 33 and 15, 5); from 11, 21 and
  31, 101, 94 and 95 levels cleared and one death each from 11 and 31 (C2 on: 99, 98, 96, none; off: 101, 99, 100, none), a level's
  median time 364, 372 and 368 s (from 1, 354). The two deaths, a rank-I hound on level 13 and a rank-III Lightless on level 32,
  had no status or element near them. Elements carried, from 1, 11, 21 and 31: 65, 11, 19 and 27 blows, 75, 17, 48 and 85 bones
  (of 391, 364, 874 and 1,377 creatures born to one). Landed, from 1: Poisoned 77, Knocked Down 23, Chilled 17, Unbound 9 (run I's
  Spirit), On Fire 5, Stunned 1; from 11: Poisoned 179, Chilled 28, Stunned 3, On Fire 1, Frozen 1; from 21: Poisoned 112, Chilled 5,
  On Fire 3, Stunned, Frozen and Knocked Down once each; from 31: Poisoned 24, Chilled 11, Stunned and On Fire 4 each, Knocked Down 2.
  Inside the noise, then: the dream answers most of it at range, so a carried element reaches it about once a level. No contract
  changed: `Mob.elem_on` is set, so D2's flare shows in play. Next: C4, its sheet first (the Bone Thrower's flash, the charge, the
  spew), for the owner.
- **7 November. CONTRACT: `Mob.c_flags`** (17.3): draw flags track C asks a creature drawn with while one of its attacks is under
  way (first the Bone Thrower's flash, `DR_EDGE`); set and cleared by C, ORed in by one line in the creatures' part of
  `render_sprites` (D's), which does nothing else with it. Nothing sets it yet, so nothing is drawn differently.
- **7 November. C4's sheet, for the owner** (`media/sheets/combat/c4-attacks.png`). The new attacks built behind `CRAWL_C4=1` (the debug
  build) and off in play, so the frames are the game's own: the Bone Thrower (rank II up, a 0.6 s flash through `Mob.c_flags`,
  then three bones lobbed half a second apart over twenty degrees), the shield-bearer's charge (the stamp with the shield up and
  dust, then the rush up to five cells along a clear lane; struck, knocked down 1.2 s, M4's mitigation), the Drowned's spew
  (swelling, then a gout in a thirty-degree cone, Poisoned) and the shambler's grapple (held 1.2 s with no fall, the hand shoving
  at it; M8). New sprites by rank on `make sheets`' `c4`, `c4drown` and `c4fx`: `SP_SHIELD_RAISE`, `SP_SHIELD_RUSH`,
  `SP_DROWN_SWELL`, `SP_DROWN_SPEW`, `SP_SPEW_A/B`, `SP_DUST_A/B`. Defaults where section 8 was silent: the Bone Thrower aims each
  bone where you stand as it throws, not where you will be; the charge runs seven cells a second, then stands 0.6 s; the rush's
  after-images hide behind it head on, so dust is kicked up along its way; the spew's gout is Poison, resisted by Resist Poison,
  and its Poisoned lasts 6 s less that resist; a shambler grapples only on a grip with no element in it, at 2.5 times M2's chance
  by its rank (rank I 20%), and a Spirit shambler's grip takes 2 + its rank in souls (M6) instead of Unbound. With `CRAWL_C4` unset
  nothing is drawn from the dice: three five-minute dreams from levels 1, 21 and 31 give the previous commit's events line for
  line. The dream's counters (8.6) and the balance runs wait for the word on the sheet.
- **7 November. C4 built** (harness only), on the owner's word on its sheet, in `234dc2f` (and `5559111`, the notes): the four
  attacks in play (`c4_on`; `CRAWL_C4=0` or `CRAWL_AF=0` takes them out, and then the previous commit's events come line for line
  from levels 1, 21 and 31), and the dream's counters (8.6): a Bone Thrower's flash or a stamp in its sight counts as a missile on
  its way (`c4_tells` in `incoming`), so its runes and Swift Dodge answer it; the charge's lane and the spew's cone are warnings on
  `hz_dmap` (`c4_dmarks`), so `decide` steps off them and `auto_abilities` dodges out; knocked down or frozen it waits (C2). The
  hound's rules (8.5) came with C3 and D2. ASan clean on a dream from level 13 with the attacks every few seconds and Swift Dodge
  given (it dodged eight times; the one charge missed), and on the two before the switch. **The checks**, on against `CRAWL_C4=0`
  on the same build, sixteen seeds (1-31 odd, `CRAWL_RUNID` the seed; the off side is C3's batch again, line for line): from level 1,
  167 levels cleared and 50 deaths, 31 on level 10 and 19 elsewhere, the knight felled 6 times (off: 161, 54, 40 and 14, 4); from
  11, 21 and 31, 97, 98 and 96 levels cleared and no deaths (101, 94 and 95, and one each from 11 and 31), a level's median time
  365, 362 and 362 s (364, 372, 368). From level 1 the dream met 298 charges, was struck by 170 and knocked down by 147 (the rest
  out-armoured), 327 volleys, 11 spews (4 struck) and 30 grapples, and lost souls to a Spirit shambler once; from 11, 114 charges
  (50 struck, 19 down), 37 spews (7), 208 volleys, 2 grapples; from 21 and 31, 89 and 102 charges (39 and 35 struck, 9 and 10 down)
  and 317 and 395 volleys, no spews (the Drowned are run II's). Deaths to shield-bearers from level 1 went from 10 to 6, to
  skeletons from 18 to 23. Inside the noise, then: the dream steps off most lanes it sees and keeps its distance from the Drowned,
  and run I's armour seldom turns a charge; a person fighting close will meet more of all of it, untried. No contract changed
  (`Mob.c_flags` was C4's, earlier). **C1 to C4 are built**: track C's steps for 1.0.0b19 are done, as are track D's; J19, the
  joint checks, is next.
- **8 November. J19 begun: how the two tracks share it.** The runs are the dream from each start for each class, this branch
  against 1.0.0b18 (main at `0201a5f`), on the same seeds with `CRAWL_RUNID` the seed so the two builds meet the same runs.
  Each track runs **half the seeds**, so that each half covers every start and class: **track C seeds 1-15 odd, track D seeds
  17-31 odd**. The classes are the five a fresh run can be given, `CRAWL_CLASS` 1-5 (the Headsman's line is shut till the first
  gift, as 1.0.0b16's checks had it). The starts as C1's: from level 1 for 6,000 s with nothing else set; from 11, 21 and 31 for
  3,000 s at `CRAWL_GEAR` 3, 5, 8 and `CRAWL_XPBANK` 9450, 35322, 77658 with `CRAWL_LEVEL` the start. Each run's log is its
  `EVENT` and `STALL` lines. For each start and class: levels cleared, deaths (on a boss's level and elsewhere), the knight
  felled, a level's median time (from `cleared`), and what landed (`EVENT af`, the dead's `maf`, `elem_hits`, `c4_*`). Track
  D: your half's numbers in your log in the same shape, and I fold both into one table here and into the README. Then ASan on a
  session a class, and the README, the page and the notes (track C, 17.5.8).
- **8 November. J19, track C's half, cut down; the halves folded** (harness only). Run by track D's agent at the owner's word,
  who asked for it inside an hour, so it is not the whole of what the entry above set out: seeds 1-15 odd, the five classes, this
  branch at `98013d1` against 1.0.0b18 (`0201a5f`), `CRAWL_RUNID` the seed, **from level 1 only, for 3,000 s, not 6,000** (about
  levels 1 to 8, where track D's half found the classes moving, and short of the knight's level). The starts from 11, 21 and 31
  were not run on these seeds (track D's half: two deaths in 120 runs of this branch, none of 1.0.0b18's). A run of 3,000 s is
  exactly the first 3,000 s of one of 6,000 (track D's seed 17 Galvanist, run again, gave the same 2,559 lines), so track D's runs
  cut there join these as sixteen seeds: the table below. **Track C's half alone**: 250 levels cleared (248), 47 deaths (50),
  a level's median 356 s (355); by class the Witchfinder 6 (2), the Anatomist 11 (6), the Galvanist 8 (9), the Plague Doctor 18
  (28), the Sin-Eater 4 (5). **Both halves**: 500 levels cleared (495), 92 deaths (94), a level's median 357 s (355); the deaths
  on levels 1 to 8 by level 11, 8, 19, 24, 10, 12, 7 and 1 (13, 7, 26, 23, 8, 11, 5, 1); to rank I skeletons 50 (37), the rest
  spread. So, as far as the first eight levels go: neutral in all, and the classes move. The Plague Doctor's gain holds on both
  halves (18 against 28, 14 against 24), as D1's table meant; the Anatomist's loss holds on both (11 against 6, 11 against 8), most
  likely Frost's slow become a Chilled stack (track D's entry); the Galvanist's doesn't (8 against 9 here, 17 against 7 there): one
  half's noise, mostly. Landed on the dream in track C's half: Knocked Down 237 (141 by a charge), Poisoned 46, Chilled 19, On Fire
  4, Stunned 1; on the dead: Chilled 2,541, Frozen 216, On Fire 321, Stunned 214, Unbound 196; 213 born to an element, carrying it
  in 30 blows and 71 bones; 338 Bone Thrower volleys, 258 charges (158 struck), 84 grapples. Stalls: seven runs of the forty here
  (two of 1.0.0b18's), all of 40 s, none to 120; four on level 3, where the Knight's Wager is a duel of 40 s. ASan: track D's five
  sessions, a class each (section 19), stand for J19's. CI: the Build workflow run by hand on the branch at `98013d1`, the Windows
  build and its smoke test and the browser's build and its smoke test in Chromium all passing. The README's *What has been
  verified* and the page's *Beta status* now carry 1.0.0b19 (and the page's account of the betas moves on one, and the
  sixteenth's sentence is the sixteenth's again). **J19 is done as far as an hour allowed**: the knight's level and the deeper
  starts on seeds 1-15 were not run, about two hours more on four cores, if the owner wants J19 whole before the release. Nothing
  tuned; no contract changed.

  | Class | Cleared | Deaths | Deaths, seeds 1-15 (C) | Deaths, seeds 17-31 (D) | A level's median, s |
  |---|---|---|---|---|---|
  | Witchfinder | 105 (107) | 8 (4) | 6 (2) | 2 (2) | 355 (343) |
  | Anatomist | 101 (104) | 22 (14) | 11 (6) | 11 (8) | 361 (355) |
  | Galvanist | 99 (101) | 25 (16) | 8 (9) | 17 (7) | 350 (350) |
  | Plague Doctor | 89 (82) | 32 (52) | 18 (28) | 14 (24) | 370 (365) |
  | Sin-Eater | 106 (101) | 5 (8) | 4 (5) | 1 (3) | 354 (362) |
  | **all five** | 500 (495) | 92 (94) | 47 (50) | 45 (44) | 357 (355) |

  *From level 1, the first 3,000 s of each run, sixteen seeds a class, this branch against 1.0.0b18 (in brackets) on the same
  runs. Track D's longer runs and deeper starts are in its own table, section 19.*

- **8 November. 1.0.0b20 begun: C5's contract.** **CONTRACT.** At the owner's word the agent that was track D through 1.0.0b19
  takes track C for 1.0.0b20 (C5), another agent track D (D5); each reads the other's log at every pull, as before. Released
  1.0.0b19 is merged into the branch (`7d100ca`). The contract (17.3's 1.0.0b20 line, and 17.2 b for who owns what):
  `conf_rank`, `conf_value` and their `CF_*`/`CV_*`, with the Confessions' table, the classes' shares and `CRAWL_CONF` behind
  them, working now, though nothing in play reads them yet; `RUN.conf`, `RUN.conf_over` (not yet saved); D's three stubs,
  `class_own` (called at the end of `caster_sync`), `class_extra` (`af_rolls` takes `KX_AF_CUT` off every status's chance) and
  `blow_survived` (asked in `hurt_ex` after Second Wind). With the stubs nothing in play changes. Defaults taken here: a run with
  no past takes each Confession at 75%, the places' mean; past V an overlevel adds a quarter of I-to-V again; a status cut never
  passes 90%, the killing blow's chance 50%. Next: the screen's sheet for the owner, and what doesn't hang on it.
- **8 November. C5's sheet, for the owner** (`media/sheets/combat/c5-confessions.png`). The ascend screen as the Confessions would have it,
  drawn by the game (`render_confess`, shown in place of today's only with `CRAWL_CONFUI=1` in the debug build; play is as it was):
  the six down the left, each its rank's numeral beside a groove that fills to V with the ranks notched in it; the chosen one on
  the right, its rank and the points into the next, its numbers as you have them and as the next rank has them (VI and on when an
  overlevel is due on one at V), what it does under them. Agility's first number and Spirituality's second are the statuses'
  marks, not words (knocked down, stunned, frozen; Unbound); Survival's are poison, chill and fire and a +. `--` where nothing is
  confessed, and for Restoration's spell, the killing blow and the cracks till rank I is whole. `CRAWL_CONF` takes six points too
  (`12,45,0,30,10,150`). Defaults taken: the same marks the old lines had are not used (the names say enough at this size); the
  class's share shows only in the numbers. Waiting on the owner's word; the rest of C5 goes on meanwhile.
- **8 November. C5 built, behind its switch till the sheet is approved** (harness only). Everything section 10 gives C5 is in
  the game but reached only with `CRAWL_CONFUI=1` in the debug build (`conf_on`); once the owner has seen the sheet, it becomes
  the game's and `CRAWL_LINES=1` puts the old lines back for balance runs. **The points**: `update_confess` (D a point, held ten
  a second after a breath; A takes back what this opening gave; Space an overlevel on one whole at V, asked twice), more health
  felt at once as the old line's was, `EVENT confess <name> <rank>` as each rank is made whole. **The numbers**: `conf_own` from
  `caster_sync` (before D5's `class_own`): Physic, Dodge, Movement, Quiet, Spell Haste, Spirit Cost, Spell Effect, Sprint, Health,
  Treasure, Luck and armour (with Toughness's share of what is worn) into the caster's own under the same caps; past V the caps
  rise with what an overlevel adds. **The statuses**: `conf_cut` takes Agility's share off Knocked Down, Stunned and Frozen,
  Spirituality's off Unbound and Survival's off On Fire, Chilled and Poisoned, both their chance (`af_rolls`) and their length
  (`af_hold`; `af_ignite` its harm with its length; `af_chill` the fall; `af_poison`); `soft_pace` runs the old clocks down
  sooner (grabs', webs' and bile's `slow_t`, a web's `snare_t`, the old `poison_t`, the bat's latch, which also needs longer
  against you to take hold). A forced knock-down (`CRAWL_AF=down`) lasted 1.5 s, 0.21 s with a Galvanist's Agility at V; six
  seconds of poison 2.4 s with a Plague Doctor's Survival at V. **Souls**: `soul_share` on the motes from kills and spectres
  (Spirituality, and D5's `KX_SOULS`). **The sprint**: Constitution's recovery divides `sprint_recovery`'s wait and fill.
  **Overlevels**: `over_due` counts the Confessions' too; an overlevel gives its Alignment Point as before, unless an old line's
  overlevel paid back already gave it (`RUN.ap_overs`). **Saves**: `LG_CONF` (each Confession's points and overlevels, a byte
  each, and `ap_overs`); an older page's lines are paid back as it is read (`lines_payback`: every step as the points it cost,
  every overlevel as one to choose again; `EVENT lines_payback`), and a person is told at her greeting (*THE OLD LINES ARE UNDONE*).
  Checked by signing a page on the old build (490 points in lines, four overlevels) and opening it on this one: 490 points and four
  overlevels to choose, and the Confessions saved and read again. A death that costs takes them (U13). **The dream**:
  `auto_confess`, its class's two best to V and then the lowest, overlevels to the best whole (the Witchfinder Spirituality and
  Restoration, the Plague Doctor Survival and Agility, the Headsman Constitution and Toughness); with no past, Constitution and
  Agility. **The Codex**: Luck's page and the overlevels' page in the Confessions' words. With the switch off, a dream of 400 s
  plays the commit before's 392 events line for line. ASan clean on three sessions with the switch on: a Witchfinder's
  dream from level 21 for 600 s (eight ranks confessed at its greeting), a Plague Doctor's from level 1 for 600 s, and a person
  opening the old page above (paid back), confessing held and tapped, taking back and trying an overlevel. Not done: the Codex has no page of the Confessions
  themselves (the screen says what each does); a number at its cap gives no warning, as the old lines' screen did; balance runs
  wait for the switch to be the game's (J20).
- **8 November. The sheet approved; C5 in play.** The owner approved the Confessions' screen as drawn. `conf_on` is now the
  game's: the Confessions stand in place of the ascend screen's lines for a person and the dream, and an older page's lines are
  paid back as it is read. `CRAWL_CONFUI` is gone; `CRAWL_LINES=1` in the debug build puts the old lines back (no payback, the old
  screen, the old dream's buying), for balance runs. **C5 is built.** Track C's part of 1.0.0b20 is done; J20 waits on D5.
- **8 November. Totals and caps on the Confessions' screen, and a brief on caps** (at the owner's word; harness only). The pane
  now shows each stat as the caster has it in all (worn, the class's own and every Confession: `conf_total`, from
  `gear_total_free`, a total before its cap) and as the next rank would leave it; the rest (the cuts, the souls, the spell's)
  as they stand. A number at its cap is pale blue under a lid (`cap_lid`), as is the list's numeral of a Confession with
  anything capped; *AT ITS CAP: ...* stands at the pane's foot; D asks twice before confessing one whose every number is
  capped. Spirituality's words shortened to fit. No overflow for any class on any Confession, all at V with caps reached and an
  overlevel due. `docs/BRIEF-caps.md`: every cap, what reaches for it, and options for caps by class (CP1-CP5), for the owner;
  nothing of it built. Any of it would be one contract change (`class_own` handing back a cap lift) and a few lines a side.
- **8 November. The owner's caps by class, given to D.** **CONTRACT.** The owner answered the brief (docs/BRIEF-caps.md section
  6): every resist with a cap of its own, armour too; option (a) with every lift at +30%; caps shown on her question's screen;
  the killing blow capped at 30%; a strength and a weakness for each class (the Galvanist's Lightning and Frost the owner's; the
  other five worked out in the brief, for the owner's word). The owner gave the work to track D, the class sheets being D's.
  C's part, done: `class_cap(st)` (17.3; a stub in D's block, added to the caps in `caster_sync`) and the killing blow's cap at
  30% in `conf_at` (and on the Confessions' screen). 17.2 b gives D `armour_keep` for armour's cap. With the stub, nothing in play
  changes but the killing blow's cap (25% at Constitution V was already under it; only overlevels reach past). Track D: section 6
  of the brief is yours, with two things still to the owner's word (the pairs, and whether a resist's lift stops at 90%).
- **8 November. The owner settled the last two** (docs/BRIEF-caps.md section 6): the pairs as worked out (Witchfinder Fire and
  Frost, Anatomist Frost and Fire, Galvanist Lightning and Frost, Plague Doctor Poison and Spirit, Sin-Eater Spirit and Poison,
  Headsman Physical and Lightning; starting resists +15 and -15), and a resist's lift (armour's share too) stops at 90%, Spirit
  Cost's at 70%; every other cap takes the full +30%. Nothing left to ask: track D, it is yours to build.
- **9 November. Restoration on Tab's caster page** (at the owner's word; harness only). D's log left it undone, the page being
  C's: once Restoration is confessed, the spells' row ends with *8*, an ankh in ghost-light and its rank, gold while it is in
  hand; the spell left in `G.spell` is no longer lit gold while Restoration is (`render_caster`). Frames with it in hand, out
  of hand and unconfessed (absent); no overflow.
- **9 November. Released as 1.0.0b19.1; J20 begun.** At the owner's word the branch went out as it stands (C5 and D5 whole,
  both of D's sheets approved, the cracks in play), squashed onto main as one commit. The owner asked for 1.0.0b19 again; CI
  publishes only a version that has no release, so it is a point release, and **1.0.0b20 goes out after J20**. The README,
  the page and the notes say it is not yet weighed against 1.0.0b19. **J20**, as J19 and against 1.0.0b19 (main at `41fe10c`,
  built in a worktree): track C's half is seeds 1-15 odd, the five classes a fresh run can be given, from level 1 for the first
  3,000 s (`CRAWL_RUNID` the seed), both builds run here, so neither half leans on J19's numbers; track D's half, seeds 17-31
  odd and the start from 31 on its eight seeds, as J19's. Under way.
- **9 November. J20, track C's half, and the two folded: the Confessions cost the dream** (harness only). Seeds 1-15 odd, the five
  classes, from level 1 for the first 3,000 s, this branch at `a3efa7a` (as track D's half: the Galvanist's Frost at -10, the
  buried shambler's mend) against 1.0.0b19 at `41fe10c`, both run here with `CRAWL_RUNID` the seed. 1.0.0b19's two halves, 30
  and 48 deaths, make J19's 78 again, line for line. **Both halves** (track D's from its log; 1.0.0b19 in brackets):

  | Class | Cleared | Deaths |
  |---|---|---|
  | Witchfinder | 102 (106) | 19 (7) |
  | Anatomist | 103 (99) | 8 (18) |
  | Galvanist | 96 (102) | 45 (18) |
  | Plague Doctor | 100 (99) | 32 (21) |
  | Sin-Eater | 103 (105) | 13 (14) |
  | **all five** | 504 (511) | **117 (78)** |

  *Sixteen seeds (1-31 odd) a class, the first 3,000 s from level 1; from level 31 track D's half alone, 119 (114) cleared and one
  death (none).* Half again as many deaths, past D5's quarter: the halves part (track C's 69 against 30, track D's 48 against 48),
  but sixteen seeds together say it. On track C's half, the release build (`4aec226`, before `a3efa7a`) died 68 times, so the two
  mends moved nothing there. The deaths are on levels 3 to 6, a third of them to rank I skeletons (25 against 11). **Why**, on
  track C's half: the same build with the old lines back (`CRAWL_LINES=1`, the classes' sheets kept) died **47** times; with the
  Confessions and no sheets (track D's earlier runs) **71**; so the Confessions are the most of it and the sheets some. On the
  Sin-Eater's seed 13 the dream began level 6 with 120 health under the Confessions and 246 under the lines (the shop's wares part
  ways too, so not all of that is the points). The lines gave the dream, by about 60 points (Level of Atonement 7), some +55
  health, +15% to every resist, +4% damage and spell haste and +3% dodge, by the old order; the Confessions give health only through
  Constitution (15 at I to 100 at V, at the class's share) and no resist at all, and the dream confesses its class's two best, for
  most classes not Constitution. A scratch build confessing Constitution first for every class (not kept) died **64** times: at its
  share, rank I or II, that is too little health this early to matter. The classes that fare well confess Toughness (the
  Anatomist, armour) or Restoration and Spirituality (the Sin-Eater); those that fare worst Agility (the Galvanist, the Plague
  Doctor). Restoration was channelled 19 times, Constitution's chance lived no killing blow (none of the five confesses it first),
  4 runs of 40 stood 40-41 s, none longer. **For the owner**: whether the early caster should have more of its own (health with
  each Level of Atonement, say, as the cheap Health line was), the Confessions' early numbers raised, or the harder start kept.
  J20's runs are otherwise done.
- **9 November. The owner's word: keep it harder.** 1.0.0b20 goes out with the Confessions as they are and J20's numbers in the
  README; main's README and page keep their *not yet weighed* till then, and 1.0.0b20's words carry the finding. **J20 is done.**
- **9 November. C6's contract: the augments' hooks, and 17.2 c** (1.0.0b20 released by track D at the owner's word; C6 begun).
  **CONTRACT.** 17.3's 1.0.0b21 bullet: `AUG_OPEN`, `AUG_ABIL`, `AUG_ULT`; `aug_of`, `aug_rank`, `aug_k` (C's, saying none till
  C6's frame); the hooks `aug_cost`, `aug_dmg`, `aug_cast`, `aug_hit`, `aug_af`, `aug_kill`, `aug_tick`, `aug_fly`, declared after
  `Shot`, which hand Fire to C and the other five lines to D's `augd_*` (stubs in a new marked block after D5's, with
  `augd_built`); `Shot.scale`. Their call sites: `cast_spell` (the cost, the damage, each shot; lightning and the drain after they
  strike), and one line each in track D's `damage_mob` (the hit, after its weaknesses; the kill, before `kill_mob`), `el_status`
  (the chance and the harm), `mob_afflict` (the tick) and `update_shots` (the flight). The ultimates take none. 17.2 c (C's rows
  added under D6's, whose contract came first, 978a9a4; the two met in the contract block and the plan, kept both): the frame, the
  Lampstand's slots, the dream's choosing and Fire's six are C's; `augd_*` and the five lines' names and words are D's;
  `cast_spell` is C's now. Checked: the dream from level 1 for 600 s on seed 5 as the Witchfinder (661 events) and the Plague
  Doctor (603) plays the build before line for line. Next: the frame (the slots at the Lampstand, credits, ranks, `LG_AUG`, the
  dream's choices) and Fire's six, behind a debug switch till their sheet is approved. Track D: D7 can begin on `augd_*` now.
- **9 November. C6's frame and Fire's six, and their sheet, for the owner** (`media/sheets/combat/c6-augments.png`; harness only). All of
  it behind `CRAWL_AUG=1` in the debug build (`aug_on`, 0 in the release build), so play is the build before's: with it unset four
  600 s dreams (seeds 1 and 3, the Witchfinder and the Sin-Eater) give the previous commit's events line for line. *The frame*
  (`a47c96a`): `RUN.aug` (the choice at each step, 1 or 2), `aug_ord` (the order chosen), `aug_cr` (each one's credits),
  `aug_bank`; `LG_AUG` after `LG_CONF` (`AUG_LINE_BYTES` a line; an older page reads as none chosen); `aug_credit` (a kill by the
  line 1, the BFS9000 none; a thing struck 2, in `mk_finish`; a boss 10 to the line of its last blow, in `boss_fall`), `aug_feed`
  (the newest chosen not at III, then the next; a rank's caption and `SFX_RUNE`), `aug_choose`, `aug_forget` (unbinding,
  `align_unbind`; a death that costs, all six, A14), `auto_augment` (the dream, at random, never a person's run or a kept page,
  A15). The Lampstand: each step's node carries a slot beside it (its mark, `UI_AUGMARK`, Fire's six drawn and a plain rune for the
  rest, and three pips); Space on the opening, the ability or an ultimate held opens the pair's tablet (`ui_augpair`): A/D the one,
  Enter twice takes it (*AGAIN, AND IT IS YOURS FOR GOOD.*), its credits and the line's bank at its foot; Back unbinds (A6), the
  pad's X being Back in a menu while the augments are on. Unchosen, a card gives rank III's numbers; chosen, the rank it is at or
  comes to next. *Fire's six* (11.2, A7-A13): Burning Intensity holds the bolt (`spell_holds`; `human_cast`'s release: a tap
  under 0.25 s is the ordinary cast with its wind-up, at 0.8; held, `cast_held` with `g_fire_hold` the share of three seconds,
  0.8 + (0.2 + 0.25k) of it, up to 30% more spirit, `Shot.scale` 1 + 1.5k of it; past three seconds with the Firewall, the Firewall,
  A9; `render_charge` fills over three seconds); Firecracker a spark (0.25 the damage, scale 0.55, its ignite 95%, 60% on what
  shrugs fire off, its burning 1 + 2k times the spark's, A11); Multiply three bolts 0.25 rad apart, each `MULTIPLY_K` of the cast,
  30% more spirit; Swift Flight (1 + k) times as fast and (1 + k/2) as far, so its life is shortened and Firestarter's floor still
  catches from the hand; Volatility (`fire_kill`: a kill by a burn's tick bursts, `explode_big`, setting all within 1.6 cells in
  sight alight for `VOLATILE_K` of its health, a boss a tenth, A12; 30% more spirit); Pyre (`fire_tick`, from `aug_tick`: what burns
  passes k of what is left of its burning to the nearest not alight within 1.25 cells, once, `Mob.pyre`). The node that was
  BURNING INTENSITY is SLOW EMBERS (A10). Shots are drawn by `Shot.scale`. The dream holds the bolt when its mark is beyond four
  cells and nothing within 2.5, (d - 4) x 0.6 s, half a second at least, 2.75 at most; with the Firewall it doesn't, and its
  Firewall takes three seconds. The controls card says HOLD THE FIRE BOLT AND IT GROWS when only it holds. Not done: a grown bolt
  strikes by its middle as any bolt does (the hits are track D's `update_shots`), and its burst is the ordinary one. With the
  switch on, a dream from level 13 at `CRAWL_GEAR=3` for 600 s (seed 3) held 10 bolts, split 73 and burst 9 with Burning
  Intensity, Multiply and Volatility at III; with Firecracker, Swift Flight and Pyre 53 of its marks went alight (none with no
  augment) and three passed it on (before Pyre's reach was widened from a cell). ASan clean on four sessions: two such dreams, a
  fresh run from level 1, and a person holding for 1 s (a grown bolt) and 3.5 s with the Firewall (the Firewall). The sheet: the
  Lampstand's slots and pair, and each augment but Pyre in play (Pyre shows only as a second creature alight). Next, on the
  owner's word: the augments on in the release build (`CRAWL_AUG=0` to take them out), then the pacing to A2 b's target and the
  balance runs.
- **9 November. C6's sheet approved; the augments in play; J21 begun.** At the owner's word ("Approved"): `aug_on` is 1 in every
  build (`CRAWL_AUG=0` in the debug build takes the frame and Fire's six out; then four 600 s dreams, seeds 1 and 3, the
  Witchfinder and the Sin-Eater, play the build before line for line), so with D7's switch all six lines are in play. The Codex has
  a part for the augments chosen (`CX_AUG`: what each does now and at III, its rank, its credits and the line's bank). D's note
  that the missiles' part doesn't draw `Shot.scale`: it does since `72fbf9a`, for every shot, so Splinterfrost's, Glacial Weight's
  and Cluster's sizes show. **The pacing** (A2 b), a first look: the Witchfinder from level 1 (seed 1, 7,200 s) had 324 credits on
  Fire by Level of Atonement 13 on level 10, past the 300 that nine ranks take, Burning Intensity at III by level 3; so A3's
  thresholds hold and stay (11.1 scales them only down). The dream seldom passes the knight, so Atonement 20 is far for most runs
  from level 1; the batch below counts each class's line by its last ascend. **J21**, as J20 and against 1.0.0b20 (main at
  `e982abf`, built in a worktree): track C's half is seeds 1-15 odd, the five classes a fresh run can be given, from level 1 for
  the first 3,000 s (`CRAWL_RUNID` the seed), both builds run here; track D's half, seeds 17-31 odd and the start from 31 on its
  eight seeds, as J20's. This branch at `8fe7e56` (both tracks' augments on, D6 in). Under way.
- **9 November. J21, track C's half** (harness only). Seeds 1-15 odd, the five classes, from level 1 for the first 3,000 s, this
  branch at `8fe7e56` against 1.0.0b20 at `e982abf`, both run here with `CRAWL_RUNID` the seed (1.0.0b20 in brackets):

  | Class | Cleared | Deaths | Augments chosen, ranks gained |
  |---|---|---|---|
  | Witchfinder | 51 (51) | 17 (9) | 24, 47 |
  | Anatomist | 49 (52) | 10 (4) | 24, 47 |
  | Galvanist | 51 (46) | 13 (27) | 24, 46 |
  | Plague Doctor | 50 (48) | 24 (20) | 23, 43 |
  | Sin-Eater | 48 (51) | 19 (9) | 24, 44 |
  | **all five** | 249 (248) | **83 (69)** | 119, 227 |

  A fifth more deaths, none on a boss's level (3,000 s doesn't reach 10), 8 on level 1 (4); a level's median 349 s (356). Fire's
  augments left 986 events, D's 2,183. Stalls: two runs stood 40 s (four in 1.0.0b20's), none to 120 s. **Where**: by the opening
  augment the dream drew at random (A15), the Witchfinder died 11 times on the four seeds it took **Firecracker** (1.0.0b20: none on
  those seeds) and 6 on the four it took Burning Intensity (9); the Sin-Eater 15 on the four it took Siphon (4) and 4 on Hollowing's
  (5). Both trade a hit's damage now for something later: the spark is a quarter of the bolt and its burning, at rank I twice the
  spark's and at III three times (A11: 75% of a bolt over four seconds), never quite a plain bolt's hit and the 10-25% chance of
  its own burning, and slower, so the early dead live longer; Siphon hurts 15% less for its healing. Four seeds a side, so a lean,
  not a verdict; track D's half will say more. The Galvanist's 13 against 27 is the noise J20 saw in it. **The pacing** (A2 b),
  four seeds a class to 7,200 s with the augments on: every run that reached Level of Atonement 18 or more had 300 credits or more
  on its class's line by then (the Galvanist's four 433-501 at 19-20, the Sin-Eater's three 428-521, the Plague Doctor's one 387,
  the Anatomist's one 398 at 18), and 300 came at Atonement 13 to 19; the rest stood at the knight with Atonement 10 to 14 and 182
  to 347, some 20 to 27 a level. So A3's thresholds hold and stay. `LG_AUG` checked: augments chosen and their credits, signed onto
  a page and read back in a fresh session, show as they were on the Codex's pages.
- **9 November. J21, the two halves folded** (harness only; the owner's word, (a), keep it, is in track D's log). This branch at
  `8fe7e56` (both tracks' augments on, D6 in) against 1.0.0b20 at `e982abf`; 1.0.0b20's two halves, 69 and 48 deaths, make J20's
  117 again, line for line, class by class (1.0.0b20 in brackets):

  | Class | Cleared | Deaths |
  |---|---|---|
  | Witchfinder | 95 (102) | 42 (19) |
  | Anatomist | 96 (103) | 18 (8) |
  | Galvanist | 102 (96) | 27 (45) |
  | Plague Doctor | 97 (100) | 39 (32) |
  | Sin-Eater | 95 (103) | 32 (13) |
  | **all five** | 485 (504) | **158 (117)** |

  *Sixteen seeds (1-31 odd) a class, the first 3,000 s from level 1; from level 31 track D's half alone, 117 (119) cleared and one
  death (one).* A third more deaths, past D5's quarter, none on a boss's level (3,000 s doesn't reach one), most on levels 3 to 7 to
  rank I skeletons; a level's median 349-357 s (355-356). Track D's variants put it on the augments (with them out, 64 deaths on its
  half against 75; D6 inside the noise). By the opening the dream drew at random on its own line, against 1.0.0b20 on the same seeds:
  Firecracker 27 (4) and Siphon 23 (4), the openings that take from the one spell it has early; Burning Intensity 15 (15), Hollowing
  9 (9) even; the Galvanist, whose openings add to the hit, died less (27 against 45). From level 31 nothing moved. The pacing holds
  (A2 b): every run that reached Level of Atonement 18 had its class's 300 credits. Stalls: nine runs of eighty from level 1 stood 40 s
  within a cell of a place (nine in 1.0.0b20), none past 41 s. ASan clean on track D's seven dreams and track C's four (C6's
  entry). **J21 is done**; 1.0.0b21 goes out with these numbers as they stand, at the owner's word, when the owner says. The
  README, the page, the notes and CLAUDE.md are written for it on the branch (the page's twenty-first beta goes out with the
  squash). Found while writing them: 1.0.0b20's `piece_unpack` reads a piece carrying one of D6's new stats (a mark's chance,
  Steadiness) or a class's ability as nothing, so going back to it loses such pieces; the notes, the README and the page say so
  and advise a copy first. Nothing to change (a reader can't know a stat it hasn't got); track D, for your note.

## 19. Track D's log (the dead's)

*Append only, newest at the bottom.*

- **7 November.** Track D started from `2444cd2` (the marks and the shared declarations), with nothing of track C's pushed
  since. Marks put on the Codex's one-line enum (`CX_*`, `NCX`) and on `CX_MAX` (17.4: whoever needs them first), in a commit
  of their own, since D1 gives the elements' part a second page; nothing else in it. Next: D1.
- **7 November. D1 built** (harness only): 7.2's table flat in `KINDS` (weak x1.5, strong x0.5, the rest x1; every element the
  weakness of three kinds and the strength of three, each averaging 1.0), a rank's strengths in place of wards (III and IV one,
  V two, x0.5, from the lines held, packs sharing, cracking at a fifth of its health), shown only once seen struck (B11: the rim,
  the barred glyph, the readout, a caption), the bestiary's rows and its line for the rank shown, a second page in the Codex's
  elements part (*THE DEAD'S ELEMENTS*), the gargoyle's and the Drowned's lore. Defaults I took where the plan was silent:
  spiderlings and sacs take the brood mother's pair (the sac's Spirit stays at nothing); the gilded take one strength; strengths
  no longer wait for level 5 (the lines gate them: never fewer than two held); the rim and the glyph over the head wait for the
  ring too, not only the readout; the Drowned's lore got its hint though it didn't contradict anything. `CRAWL_ELEM=0` puts back
  1.0.0b18's table and wards, but not for the hazards' harm to the dead (`hz_hurt_mob`, `hz_harm` read `KINDS` directly and are
  neither track's in 17.2), so a balance run with it keeps the new table there, from level 31. Tested: frames of the Codex's
  page, the bestiary at ranks I, III and V, and a person's frost on a rank III skeleton (nothing shown, a ring, then all of it
  shown; a crack); ASan clean on that and on a dream of fifteen minutes from level 21. Eight seeds (1-15 odd, `CRAWL_RUNID` the
  seed) from level 1 for 50 minutes against `CRAWL_ELEM=0` on the same build: 52 levels cleared and 6 deaths (49 and 10), none
  reached a boss; 14 strengths given against 97 wards, since run I has few of rank III. The same from level 21 is running; its
  numbers come in my next entry. Next: D2's sheet of eyes.
- **7 November.** D1 from level 21 (eight seeds, 50 minutes, gear 5, `CRAWL_XPBANK` 35322) against `CRAWL_ELEM=0`: 47 levels
  cleared and no deaths (49, none), a level's median time 378 s (376); 923 strengths given and 187 cracked (606 wards, 82
  cracked). Neutral. D2's sheet of eyes (every kind that can be born to an element, at rank III and by rank, calm, striking and
  in motion, from the game's own renderer) went to the owner; D2 waits on it, kept aside, and D3 goes on meanwhile (17.5.5).
  Marks put on the Draw flags (`DR_*`), in a commit of their own: the caster's track takes bits from 24 upwards, the dead's
  from 30 downwards. Next: D3.
- **7 November. D3 built** (harness only): the dead's statuses (section 5), on `Mob` fields of D's (`stun_t`, `unb_t`, `chl_n`,
  `chl_t`, `stun_cd`, `frz_cd`) beside what was there (`burn_t` On Fire, `pois_t` Poisoned, `frozen` Frozen); the contract's
  `mob_stunned` and `mob_unbound` filled in. One gate at the head of `update_mob` (`mob_gate`) runs them and returns the dt the
  rest of the creature's step runs on, so nothing inside the kinds' branches changed: chill and a stun slow the clock; a stun
  holds `cool`; a stun or Unbound holds the calls (the deacon's and the horn's `call_t`, the Sexton's raised bell, a pack's call
  when it is the caller) by adding back what the branches take off; Poisoned doubles wind-ups and waits the same way. Drawn: frost
  by the stack (`DR_CHILLA`, `DR_CHILLB`), sparks over the stunned, grey eyes while Unbound. Defaults where section 5 was silent:
  18% off the clock a Chilled stack; S1's "no locks" for the dead too (a stun or a freeze can't land again within 3 s of its end,
  and neither is lengthened while it lasts); a boss is slowed where it would freeze, as Deep Chill has it. In play nothing gives
  them yet but what was there: Frozen's third more on what Frost Nova and Winter's Lance freeze, and Poisoned's slowing on what
  the flask poisons; D4 gives the hits their chances. `CRAWL_MAF=k` puts status k on what is in sight. Tested: staged frames of
  each, ASan clean on six dreams of four minutes from level 11 (one a status, 421 landed), and eight seeds (1-15 odd) of 50
  minutes against `CRAWL_ELEM=0` on the same build: from level 1, 52 levels cleared and 7 deaths (49 and 10); from level 21, 46
  cleared and none (49, none), a level's median time 367 s (368). Seed 15 from 21 stalls on level 23 casting at a Wraith it has
  driven back, as it did with D1 alone: the known stall, not D3's. Commits: the D3 commit before this entry's push. Next: D4.
- **7 November. D4 built** (harness only): each of the caster's hits that leaves a creature standing (not a tick) rolls its
  element's status (`el_status`, `el_chance` in `damage_mob`): 10% at the line's rank 1 to 25% by 20, times Spell Effect, halved
  on what is strong against it, half again on what is weak to it; Fire ignites (the hit again over four seconds, E2), Lightning
  stuns 1.2 s, Spirit unbinds 1.5 s. Reconciled, not doubled: Frost's slow is a Chilled stack on every hit (no longer
  `m->slowed`), and the flask's poison Poisoned, so neither rolls; Physical gives the dead nothing. Deep Chill's count of shards
  gives way to freezing at three stacks (`chill_need`); Permafrost lengthens the freeze as it did the slowing; Amperage's daze is
  a 1.5 s stun. Firestarter's burning was already On Fire (`burn_t`); left as it was. The Codex's first elements page says what
  each leaves on them. Defaults where section 6 was silent: Frost and Poison don't roll on top of what they always do; the roll
  comes after the kill (a hit that kills gives nothing). Tested: ASan clean on six dreams of five minutes from level 11, a class
  each; eight seeds (1-15 odd) of 50 minutes against `CRAWL_ELEM=0` on the same build: from level 1, 50 levels cleared and 7
  deaths (49 and 10); from 11 (gear 3), 48 and none (49, none), a level's median time 362 s (350); from 21 (gear 5), 47 and none
  (49, none), 365 s (368). Landed, from 1, 11 and 21: chill 1,191, 454 and 188; frozen 122, 32 and 6; fire 103, 46 and 115; stun
  4, 54 and 25; Unbound 1, 147 and 190 (the dream's classes by seed: from 1, five Witchfinders and three Anatomists). Frost is the
  one to watch at J19: three Anatomists from level 1 froze things 122 times in 50 minutes, each freeze a third more damage. Seeds
  1 and 3 from 11 stall on level 12 by a Wraith driven back, in both modes: the known stall again. **D1, D3 and D4 are built;
  D2 waits on the owner's word on its sheet of eyes** (its code kept aside: elemental creatures at birth, packs sharing, the
  element a strength, the eyes in its colour and flaring while `Mob.elem_on`). No contract changed.
- **7 November. D2 built** (harness only), on the owner's word on its sheet (approved as drawn; `tools/eyesheet.py` draws it
  from the game's renderer: every kind at rank III in each element, a skeleton by rank calm and striking, the motes in motion).
  `Mob.elem` given with the strengths the first time a creature moves (`elem_give`): every kind with eyes and a blow (B7 b), rank
  II 15%, III 25%, IV 35%, V 45%, never the gilded, any element but Physical, the run's two thrice as likely, never its kind's
  weakness; a pack's rolled once for its leader (`Pack.elem`) and taken by members past rank I whose kind allows it; sacs and
  spiderlings the nearest mother's. Born to an element it shrugs it off (x0.5, shown at once in the readout since the eyes tell
  it), its weakness kept. The eyes: the element's colour (in tones apart from the ranks' own: Frost white in cyan, not rank III's
  blue; Poison bile over green, not rank II's), the rank's form kept, a flicker, and a mote that moves each element's way; while
  `Mob.elem_on` they flare in the colour (C sets it, C3); Unbound wins, grey. Defaults I took: the gilded are never born to one;
  the weights 3 to 1; the readout shows a born strength at once. Two weak spots told the owner with the sheet: at rank V Frost's
  pale cross is near a plain rank V's in a still (the falling mote parts them), and a waiting web spider's two points look alike
  for Frost and Lightning in a still. Tested: ASan clean with each element forced from level 11 (15-25 born a run) and with a
  person and the flare; eight seeds (1-15 odd) of 50 minutes against `CRAWL_ELEM=0` on the same build: from level 1, 51 levels
  cleared and 5 deaths (49 and 10), 61 born; from 21, 49 and none (49, none), a level's median time 366 s (368), 507 born (Fire 88,
  Frost 62, Lightning 142, Poison 162, Spirit 53: run III's two lead). A long stall line on level 3 of seed 7 is THE KNIGHT'S
  WAGER, a trial's duel of 40 s, not a stall. Bench on this session's new machine: 2.19-2.31 ms a frame against D4's 2.09-2.19
  on the same machine (the earlier 1.5 was another machine). **D1 to D4 are built**: track D's steps for 1.0.0b19 are done, but for
  J19. **CONTRACT** unchanged: `Mob.elem` is set, and its colour and flare wait only on C3 setting `Mob.elem_on`.
- **7 November.** A slip of mine: D2's push went up after its rebase onto C2's sheet (`edfbf83`) without building the rebased tree
  first, against 17.5.2. Built at once after: no warnings, smoke clean. The chain I push with now builds after the pull.
- **7 November. The dream's stall by a driven-back Wraith, mended** (the owner asked for it in this session; outside D's steps,
  harness only). Found by the `STALL` lines (seeds 1 and 3 from 11, 15 from 21; about one run in four from 11) and reproduced to the
  line on seed 1: the dream at a corridor's end with the exit beyond a vault's locked door, the Wraith driven back into the cell
  before that door. Driven back it retreats up the hunt field, and the only cell further from the dream was the locked door: it
  stood "opening" it for ever. The dream, blocked by it (`update_player`'s blocker, which never asked whether it was a driven-back
  Wraith; `find_target` had been mended for that in 1.0.0b11), drove it back every 1.8 s till its souls ran out. Mended in the
  Wraith's movement (`update_wraith`, its retreat; not one of 17.2's lists, so neither track's): driven back with nowhere further to
  go, or only a door it can't open (a key's lock, or sealed, winged, trialed, gated), a second later it goes into the stone and
  comes again from the far end of the level, as it first came (`wraith_cornered`, `Mob.corner_t` in D's part of `Mob`; `EVENT
  wraith_cornered`). A person sees it too: a Wraith driven into a corner now goes, where it stood there to come on again at
  arm's length. The dream's blocker is left as it was. Tested: the three seeds run clean (6, 6 and 5 levels against 5, 5 and 4;
  cornered 1, 2 and 6 times); ASan clean on three dreams from 11 with the Wraith at 30 s (cornered four times); and eight seeds
  (1-15 odd) of 50 minutes from 11 and from 21, both modes (32 runs): no `STALL` lines at all (the same seeds before it: 7 from 11 and
  2 from 21), the Wraith cornered 82 times, no deaths, levels cleared 49 and 49 from 11, 51 and 50 from 21.
- **8 November. J19, track D's half** (harness only), as track C's entry set it out: the dream from each start for each of the
  five classes (`CRAWL_CLASS` 1-5), seeds 17-31 odd with `CRAWL_RUNID` the seed, this branch at `4a1508c` against 1.0.0b18 (main
  at `0201a5f`); from level 1 for 6,000 s, from 11, 21 and 31 for 3,000 s at gear 3, 5, 8 and `CRAWL_XPBANK` 9450, 35322, 77658.
  160 runs a build, each its full length, nothing switched off. The table is below this entry, 1.0.0b18's numbers in brackets.
  **From level 1**: 411 levels cleared (408), 136 deaths (122), 82 on level 10 (64) and 54 elsewhere (58), the knight felled 15
  times (19), a level's median 360 s (361). **From 11, 21 and 31**: 252, 247 and 233 levels cleared (250, 245, 246); one death
  from 11 (the Plague Doctor, to a rank III skeleton on level 12) and one from 31 (the Plague Doctor, to a rank V Lightless on
  level 36), none in 1.0.0b18; a level's median 362, 369 and 370 s (369, 369, 368).
  **By class, from level 1**, D1's table moves strength between the pasts as it was meant to, and for two of them further than
  meant. The Plague Doctor died 40 times (51), 19 of them off the knight's level (34). The Galvanist died 28 times (15), 18 off
  the knight's level (9), 11 of those to rank I skeletons (4), six on level 1 itself, in five seeds of the eight (1.0.0b18: none
  on level 1). The Anatomist died 40 times (26), 26 on level 10 (18) and 14 elsewhere (8), 8 of those to rank I skeletons (5),
  and was knocked down 140 times, the most of the five. The Witchfinder (19, against 20) and the Sin-Eater (9, against 10) are where
  they were. What the runs point to: run I's commonest kinds took Lightning and Frost harder in 1.0.0b18's table (the skeleton
  and the bat 1.25 to Lightning; the bat and the web spider 1.5 to Frost, the shambler 0.75) and take them at 1.0 now, or 0.5
  (the web spider and the shambler against Frost); in the seed looked at, a rank I skeleton that took two bolts in 1.0.0b18 took
  three. Frost on the skeleton went up (0.75 to 1.0), so the Anatomist's loss is more D4's: a Frost Shard put a creature at half
  pace for 2.5-3 s in 1.0.0b18, and a hit is now one Chilled stack, 18% off its clock, half only from the third (the Anatomist's
  stacks still froze things 610 times). Eight seeds a class is small (a class's deaths have moved by half between builds that changed nothing
  for it), so track C's half will say whether this holds. If it does, the levers are 7.2's table for run I's kinds (each element
  is now the weakness of three kinds and the strength of three, so moving one moves that evenness) and the slow of a stack.
  Nothing tuned: that is for the owner. **From 31**, 13 levels fewer (233, against 246): the rank's strengths are about three
  times as many as the wards were there (8,287 given and 1,232 cracked, against 2,841 wards and 169 cracked; from 1, 11 and 21:
  656, 1,235 and 4,363 given, against 1,520, 1,687 and 3,025 wards), and rank V's two slow every fight a little.
  **What landed**, from 1, 11, 21 and 31, the five classes together. On the dream (`EVENT af`): Knocked Down 536, 74, 35 and 23
  (by a charge 344, 60, 29 and 14; the rest falling rock), Poisoned 207, 451, 292 and 121, Chilled 64, 45, 12 and 15, On Fire 14,
  4, 6 and 20, Stunned 5, 5, 4 and 5, Unbound 6, 1, 0 and 1, Frozen never. On the dead (`maf`): Chilled 5,186, 2,194, 2,200 and
  3,081, Frozen 610, 225, 171 and 306, On Fire 678, 191, 327 and 708, Unbound 654, 650, 1,081 and 1,654, Stunned 437, 245, 398 and
  955, each class's own the most (from 1 the Anatomist's chills 5,128, the Galvanist's stuns 412, the Sin-Eater's Unbound 647;
  Poisoned isn't counted, since the flask's poison goes in through `mob_poison`, which notes nothing). Born to an element 855,
  922, 1,986 and 3,322, carrying it in 123, 35, 78 and 61 blows and 184, 80, 175 and 305 bones. Bone Thrower volleys 842, 433, 777
  and 915; charges 670, 326, 262 and 217, striking 398, 161, 119 and 77; spews 29 and 106 from 1 and 11 (6 and 18 struck), none
  deeper; grapples 140, 10, 5 and 6, and a Spirit shambler's souls taken 7 times from 1.
  **Stalls** (a `STALL` line, 40 s within a cell): 7, 0, 1 and 4 runs of each start's forty (1.0.0b18: 10, 1, 0 and 11), three
  of them to 120 s (1.0.0b18: one). The longest, the Galvanist's seed 29 from 1 on level 6, run again to the line: the dream
  before a portcullis that had come down again. It works a winch only once (`m->times` in `dream_looks`, since 1.0.0b16), and
  its way out went back through the gate, so it stood about five minutes till the level gave up on it. Older than this branch
  and neither track's: for the owner.
  **The text**: the README and the page said three Chilled stacks freeze the dead. It is four, three with Deep Chill
  (`chill_need`; the Codex has *IT CHILLS; FOUR FREEZE*); mended in both, a few words, in a commit of its own (both are shared,
  17.2). The release notes say no number, and are right.
  ASan clean on five dreams of 900 s, a class each across the starts (the Witchfinder from 1, the Anatomist and the Sin-Eater
  from 11, the Galvanist from 21, the Plague Doctor from 31; seeds 17-25): 109 statuses on the dead, 37 born to an element, 75
  strengths given and 18 cracked, 35 of the new attacks.
  Track C: the numbers are for your table; the logs aren't in the repo, and any run comes again from its seed and class with the
  settings above. No contract changed. **Track D's part of J19 is done.**

  | From | Class | Cleared | Deaths (level 10 / elsewhere) [1.0.0b18] | Knight felled | A level's median, s |
  |---|---|---|---|---|---|
  | 1 | Witchfinder | 85 (78) | 19 (17 / 2) [20 (18 / 2)] | 3 (5) | 364 (344) |
  | 1 | Anatomist | 72 (75) | 40 (26 / 14) [26 (18 / 8)] | 0 (1) | 361 (355) |
  | 1 | Galvanist | 92 (91) | 28 (10 / 18) [15 (6 / 9)] | 5 (5) | 348 (366) |
  | 1 | Plague Doctor | 71 (72) | 40 (21 / 19) [51 (17 / 34)] | 0 (1) | 367 (368) |
  | 1 | Sin-Eater | 91 (92) | 9 (8 / 1) [10 (5 / 5)] | 7 (7) | 360 (363) |
  | 1 | **all five** | 411 (408) | 136 (82 / 54) [122 (64 / 58)] | 15 (19) | 360 (361) |
  | 11 | Witchfinder | 53 (50) | 0 [0] | 0 (0) | 362 (372) |
  | 11 | Anatomist | 51 (49) | 0 [0] | 0 (0) | 362 (371) |
  | 11 | Galvanist | 49 (48) | 0 [0] | 0 (0) | 362 (374) |
  | 11 | Plague Doctor | 50 (51) | 1 (0 / 1) [0] | 0 (0) | 362 (362) |
  | 11 | Sin-Eater | 49 (52) | 0 [0] | 0 (0) | 360 (371) |
  | 11 | **all five** | 252 (250) | 1 (0 / 1) [0] | 0 (0) | 362 (369) |
  | 21 | Witchfinder | 50 (49) | 0 [0] | 0 (0) | 368 (373) |
  | 21 | Anatomist | 47 (49) | 0 [0] | 0 (0) | 370 (364) |
  | 21 | Galvanist | 51 (50) | 0 [0] | 0 (0) | 366 (366) |
  | 21 | Plague Doctor | 49 (50) | 0 [0] | 0 (0) | 367 (377) |
  | 21 | Sin-Eater | 50 (47) | 0 [0] | 0 (0) | 370 (372) |
  | 21 | **all five** | 247 (245) | 0 [0] | 0 (0) | 369 (369) |
  | 31 | Witchfinder | 48 (48) | 0 [0] | 0 (0) | 367 (370) |
  | 31 | Anatomist | 45 (48) | 0 [0] | 0 (0) | 372 (369) |
  | 31 | Galvanist | 49 (53) | 0 [0] | 0 (0) | 369 (353) |
  | 31 | Plague Doctor | 45 (49) | 1 (0 / 1) [0] | 0 (0) | 376 (369) |
  | 31 | Sin-Eater | 46 (48) | 0 [0] | 0 (0) | 370 (372) |
  | 31 | **all five** | 233 (246) | 1 (0 / 1) [0] | 0 (0) | 370 (368) |

- **8 November. 1.0.0b18.1 merged in, and J19 run again over both** (at the owner's word, who asked that its work go out
  with 1.0.0b19; by track D's agent). `claude/funny-lamport-onhkrp` (steps 1-8 of docs/PLAN-b18-1.md, harness only) merged with
  `--no-ff` into this branch without a conflict (`e8169e0`), as its trial merges had said; one line its section 14 asked for
  added after (`4808d0f`): `wraith_cornered` never lands the Wraith inside a lit soul-lamp's ward. None of 17.2's lists is
  touched by it. **The runs**, this branch's side only, the 1.0.0b18 runs above standing as the baseline (the same binary,
  seeds and settings; a run cut short is the same as the first part of a longer one): from level 1, sixteen seeds (1-31 odd) for
  each of the five classes, 3,000 s; from level 31, track D's eight seeds a class, 1,500 s. **From level 1**: 511 levels cleared
  and 78 deaths, against 1.0.0b18's 495 and 94 and the combat work alone's 500 and 92; a level's median 353 s (355). By class
  (the combat alone, then 1.0.0b18): the Witchfinder 7 (8, 4), the Anatomist 18 (22, 14), the Galvanist 18 (25, 16), the Plague
  Doctor 21 (32, 52), the Sin-Eater 14 (5, 8). So with both, the Anatomist's and the Galvanist's losses are inside the noise, the
  Plague Doctor's gain holds, and the Sin-Eater's two halves moved the other way (one more sign that eight or sixteen seeds a
  class carry a third or more either way). Deaths on level 1 fell to 3 (11 and 13). Of 298 trials begun the dream won 220, lost 22 and
  died in 49 (216, 19 and 54 for 1.0.0b18), took every reward and drank 176 draughts. **From level 31**: 114 levels cleared
  (110, 114), no deaths in any; the dream offered souls at a lamp 116 times, lit 80, and the Wraith was held at an edge 10 times
  and driven out 5. **A fault found there, of 1.0.0b18.1's**: one lamp lit while the Wraith was already stepping in let it finish
  the step and stand inside, where nothing drove it out (`EVENT ward_breach`, seed 23, the Plague Doctor, level 32). Replayed to
  the line and mended (`c183123`): a step whose cell a lamp bars turns back to where it came from; the replay holds at the edge,
  and six runs with lamps lit from the start and the Wraith at 20 s (`CRAWL_WARDLIT=5 CRAWL_WRAITH=20`, one under ASan) show no
  breach. The runs from level 1 end before the Wraith's levels, and from 31 only that run could have gone otherwise. Stalls: four
  runs of the eighty from level 1 (thirteen and seven), four of the forty from 31 (three and seven), none past 41 s. **ASan** on the
  merged build: 1.0.0b18.1's three sessions (a trial peeked at, which began nothing, and walked into; the ward with the Wraith at
  12 s; a scripted pad taking over and opening the look) and a dream from level 21 for 600 s: clean. The heads and treasure on a
  level weren't counted again. CI (the Build workflow, by hand) passed at `98013d1`, before the merge; the release's own run will
  show the merged build's. **J19 is done** over 1.0.0b19 as it goes out.

  | Class | Cleared | Deaths | A level's median, s |
  |---|---|---|---|
  | Witchfinder | 106 (105) [107] | 7 (8) [4] | 347 (355) [343] |
  | Anatomist | 99 (101) [104] | 18 (22) [14] | 358 (361) [355] |
  | Galvanist | 102 (99) [101] | 18 (25) [16] | 350 (350) [350] |
  | Plague Doctor | 99 (89) [82] | 21 (32) [52] | 361 (370) [365] |
  | Sin-Eater | 105 (106) [101] | 14 (5) [8] | 350 (354) [362] |
  | **all five** | 511 (500) [495] | 78 (92) [94] | 353 (357) [355] |

  *From level 1, the first 3,000 s, sixteen seeds a class: with 1.0.0b18.1 merged in; the combat work alone in brackets; 1.0.0b18 in
  square brackets. From level 31, 1,500 s on eight seeds a class: 114 (110) [114] levels cleared, no deaths.*

- **8 November. D5 begun: the classes' own sheets** (harness only; `14b56b5`, before C5 was in play). Section 9's draft, which
  the owner took for now (K1 b), in D's marked block: `CLASS_OWN` (each class's health against the hundred everyone had, and its
  stats in their own units), `class_own` (into `caster_sync`'s own, under the same caps), `class_extra` (`KX_AF_CUT`: the Plague
  Doctor's tenth off every mark's chance, which `af_rolls` takes; `KX_SOULS`: the Sin-Eater's tenth more souls from kills, which
  C5's `soul_share` reads). Her question has a second page (Space turns between *THEIR PAST* and *WHAT THEY BRING*: the sheet as
  rows), the Codex's class pages a sentence of it under the story. A class taken (or traded) feels its health at once, as more
  health from a line was; a debug start with `CRAWL_CLASS` starts whole. `CRAWL_SHEET=0` leaves the sheets out: a dream on it
  plays released 1.0.0b19's line for line, so J19's merged runs stand as the baseline. **The runs**: from level 1, the first
  3,000 s on sixteen seeds (1-31 odd) for each of the five classes: 503 levels cleared and 85 deaths (511 and 78), a level's
  median 356 s (353), deaths on level 1 6 (3). By class, cleared and deaths: the Witchfinder 108 and 5 (106, 7), the Anatomist
  101 and 11 (99, 18), the Galvanist 99 and 25 (102, 18), the Plague Doctor 91 and 33 (99, 21), the Sin-Eater 104 and 11 (105,
  14). The Plague Doctor's sheet can do it no harm (a quarter off poison, a tenth off every mark, Luck, Treasure, half a second's
  sprint), so its twelve more deaths are the noise's size; the Galvanist's seven could be its ninety health, so both were run on
  sixteen more seeds (33-63 odd), the sheet against `CRAWL_SHEET=0`: on them the Galvanist died 27 times
  against 13 (52 against 31 over all thirty-two seeds; seventeen more of them to rank I skeletons on levels 1-4), and the Plague
  Doctor 33 against 29 (66 against 50). The Plague Doctor's is the noise again; the Galvanist's looks real against the dream, and
  a batch with its health at 100 (the rest of its sheet kept) was run on all thirty-two: 45 deaths (52 at 90, 31 with no sheet),
  198 levels cleared (202, 208). So its health is a little of it; the rest is near the noise, or in its Movement or Dodge, and
  wasn't chased, since the owner's caps by class (below, in C's log) reshape the sheets. The table is the owner's draft (K1 b). From level 31, 1,500 s on eight seeds a class
  (17-31 odd) against `CRAWL_SHEET=0`: 118 levels cleared (114), no deaths in either, a level's median 366 s (366). What the
  marks did: knocked down 510 times (468), poisoned 80 (70). ASan clean on a Plague Doctor's dream from level 11 for 400 s, a
  person turning her question's pages, and one taking the Anatomist's vow over the Witchfinder.
  **A stall found there, older than this branch and neither track's: for the owner.** The one run past 41 s (the Plague
  Doctor's seed 9, level 5, 121 s by the line and about four minutes in all) was replayed to the line: hurt under two fifths, the
  dream wants its place to mend (`rest_goal`); one cell on, a shambler still under the earth comes into its sight within seven
  cells, and `enemies_within` (so `in_combat`) counts the buried (`buried(m)`, which `targetable` refuses), so it doesn't; one
  cell back the shambler is out of sight and it wants the place again. It went back and forth between (10,14) and (11,14) till it
  gave the level up at 270 s. `enemies_within` skipping the buried would mend it, but it is called from the runes' triggers and
  the dream's fights in many places, so it would move both tracks' runs: left for the owner's word.
- **8 November. The killing blow survived** (10.4; harness only). `blow_survived`: asked by C's `hurt_ex` after Second Wind, for
  a blow that isn't a seep; D refuses as well what is stood in (`lastblow_can`: fire, bile or a void hazard ticking, a burning
  floor, a pool, poison, as `windup_blow` has it), a rock, an arc, a stroke and every creature's blow count. Constitution's chance
  (`conf_value(CF_CONST, CV_LASTBLOW)`, the class's weight in it and C's cap of a half), once a level (`G.lb_spent`, in D's part
  of `G`, cleared in `start_level`, which is shared; a pocket starts afresh, as a place of its own): health stops at 1, and
  `G.wind_t` 3 s keeps everything off as a Second Wind's does, with its flash and *UNBROKEN* in place of its name (`render_wind`,
  two lines, while `lastblow_shown`); the hurt, a heartbeat and a deep chime. What is on the caster stays (the owner's words: the
  blow connects, one health, three seconds). With nothing confessed nothing is rolled, so a run without Constitution plays as
  before. Tested: struck down by `CRAWL_KILLAT` with `CRAWL_CONF=000500` on sixteen seeds for three classes, the chances as their
  weights give them (0.25 the Headsman, 0.22 the Plague Doctor, 0.15 the Galvanist) and 15 of 48 lived; forced (`CRAWL_LASTBLOW=1`)
  over a Plague Doctor's dream of 2,400 s, once on each of levels 2, 4 and 5, never twice on a level, a retry after a death
  allowed again; frames of the breath; ASan clean on that dream (1,200 s) and on a person struck down on level 12.
- **8 November. Toughness's cracks: the sheet, for the owner** (`media/sheets/combat/d5-cracks.png`; 10.5). `render_cracks` (from the
  end of `render_warded`): motes off each shut secret's crack face, rising and drifting off the stone over two or three seconds,
  in the run's colour (the Order's candle gold, the drowned's ghost-light, the mine's amber, the black glass's violet), a bright
  core and a dim cross about it, at full brightness through dark and fog, hidden by walls; seen within Toughness's reach
  (`conf_value(CF_TOUGH, CV_CRACKS)`: 4 cells at rank I to 8 at V for the Anatomist, 3 to 5 for a class at half weight), thinning
  over its last cell, six motes at rank I to eighteen at V. The cracks' own texture still shows only within two cells, as today.
  The sheet: each run at rank V close to, rank I at 3.2 cells, III at 5, V at 7, from the game (`CRAWL_TPAT`). Behind
  `CRAWL_CRACKS=1` (debug) till it is approved; `CRAWL_CRACKS=2` adds a `CRACKS` line a second (the face, the distance, the reach).
  Next: Restoration the spell, its glow up the arm on a sheet first.
- **8 November. Restoration's glow: the sheet, for the owner** (`media/sheets/combat/d5-restoration.png`; 10.3). `render_restore` (from
  `render_hand`, one line after `render_charge`): while it is channelled, three bands of pale ghost-light go down the arm from the
  fingers into the sleeve every 1.7 s, dithered over the hand's own shading so the skin shows through, a faint wash at the fingers,
  and motes rising off them. The sheet shows the hand at rest and the claw, in run I's light and run IV's dark, untouched and at
  four moments. Nothing in play draws it yet (`restore_glow` is 0 but for `CRAWL_RESTSHOW=1`, debug); the spell itself (its
  number, the spells' turning and wheel, the channel, the dream's choosing) comes next, built while the sheets wait.

- **8 November. Restoration the spell** (10.3; harness only). Once Restoration's rank I is confessed (`restore_open`) it is a
  spell among the others: 8 takes it up, the mouse wheel turns to it after the seven, and the spells' wheel (the pad's D-pad down
  held) has it last, an ankh (`RESTORE_MARK`). It is in hand as `G.rst_sel` (D's part of `G`), with `G.spell` left as it was for
  when another is taken; its id among the spells is `RS_ID` (100, past every `AT_` and `SHOT_`, so nothing indexed by a spell
  sees it), not a new `AT_` number, which would have changed the saves' spellbook bytes for nothing kept. Held, the cast button
  channels it after 0.4 s (`restore_cast`, one line at the head of C's `human_cast`): souls by the second and health by the
  second as Restoration's numbers give them (`restore_tick`; nothing spent while whole), half pace (one factor in `slow_factor`),
  the hand up (`hand_raise`) and the glow down the arm (`render_restore`, the sheet's). A blow doesn't end it; letting go, a menu,
  a stun, a knock-down, Unbound, the void or the souls running out do (`EVENT restore_begin`, `restore_end <why> healed n souls
  x`). The status bar shows the ankh and *RESTORE* with its rank (`restore_hud`, one line in `render_statusbar`). The dream
  (`dream_restore`, one line in `update_player`): out of a fight and not casting, under three fifths of its health, with souls to
  mend a fifth of it and six over, it channels as it walks, to nine tenths or till a fight; at rank I that is 56 souls, so the dream
  seldom does (once in 25 minutes from level 1 at rank V, as the Sin-Eater). With the owner's numbers (U9 b) it is slow at rank I:
  a hundred-health caster mends a point every four seconds for 0.7 souls a second; at V a point a second for a tenth. Tested: a
  person at rank V (the Sin-Eater, at full weight) mended 10 health in 10.6 s for 1.1 souls, walking at half pace, the wheel and
  the pad's wheel took it up and put it down; frames of the bar and the glow; ASan clean on that and on a dream of 1,200 s that
  channelled. Not done: Tab's spellbook page doesn't list it (C's page), and balance runs wait for J20.

- **8 November. The owner's caps by class** (docs/BRIEF-caps.md section 6, given to D; harness only). **The sheets**: each class's
  own element's resist +15 and its weakness's -15 (`CLASS_OWN` now has the pair, `strong` and `weak`): the Witchfinder Resist Burn
  +15 (it was +10) and Resist Slow (frost) -15, the Anatomist Resist Burn -15 (its Resist Slow +15 as it was), the Galvanist Resist
  Slow -15, the Plague Doctor Resist Poison +15 (it was +25) and Resist Spirit -15, the Sin-Eater Resist Poison -15, the Headsman
  (its strength its armour, +4 as it was) Resist Lightning -15. A total may go below nothing, as the brief says `el_resist` and
  `hurt_ex` already take. **The caps** (`class_cap`, filled: the brief's table in each stat's units, read by C's `caster_sync`):
  the Witchfinder Resist Burn to 90, Spell Haste and Spell Effect to 65; the Anatomist Resist Slow to 90, Physic to 520; the
  Galvanist Resist Lightning to 90, Dodge to 45.5, Movement to 52; the Plague Doctor Resist Poison to 90, Luck to 32.5, Treasure
  to 130; the Sin-Eater Resist Spirit to 90, Vessel to 130, Spirit Cost to 70; the Headsman Sprint to 9.75 s over the base,
  Damage to 130. **Armour's cap** (`armour_keep`, given to D by 17.2 b): the share of a blow kept off never past 75%
  (`ARMOUR_CAP`), the Headsman's 90% (`armour_cap`). **Shown**: her question's second page has each lifted cap beside its row in
  pale blue (*MAX 90%*), the lifted caps the sheet has no row for after (the Sin-Eater's Vessel and Spirit Cost, the Headsman's
  Damage and *ARMOUR KEEPS OFF*), and the pair in the class's colour (*FIRE HURTS LESS, FROST MORE*); the Codex's class has a
  second page, *WHAT THEY BRING*, laid out the same with a line on what the caps and a resist below nothing mean (the sentence
  that had been on the first page had run over the Plague Doctor's long story; `CX_MAX` one more, in D's place). `CRAWL_SHEET=0`
  leaves the sheets, the lifts and armour's cap out: a dream on it plays the commit before line for line (checked, 500 s). Tested:
  frames of her question's second page for all six and of both Codex pages for all six, no overflow; ASan clean on her question
  turned through all six and the Codex's pages, and a Headsman's dream from level 21 for 600 s. **The runs** (on this build, C5 in
  play, against `CRAWL_SHEET=0` on the same seeds): from level 1, the first 3,000 s on eight seeds (1-15 odd) for each of the five
  classes, 253 levels cleared and 68 deaths (242 and 71), a level's median 353 s (356); the Witchfinder 4 deaths (2), the
  Anatomist 6 (16), the Galvanist 21 (13), the Plague Doctor 22 (27), the Sin-Eater 15 (13). From level 31, 1,500 s on four seeds a
  class (17-23 odd): 60 levels each way, one death each way. The dream channelled Restoration 7 times (13) and lived no killing blow
  (it confesses Constitution for none of these five). The Galvanist is the one that keeps doing worse with its sheet, in every
  batch (52 against 31 over thirty-two seeds before the caps, 21 against 13 here); with the caps it also takes Frost 15% harder.
  Whether that is its ninety health, Frost, or its Dodge and Movement changing how the dream walks is not known; a person may fare
  otherwise. For the owner.

- **9 November. Both sheets approved; the cracks in play.** The owner approved `media/sheets/combat/d5-cracks.png` and
  `media/sheets/combat/d5-restoration.png` as drawn. Toughness's motes are now the game's (`cracks_on`; `CRAWL_CRACKS=0` in the debug build
  leaves them out, `CRAWL_CRACKS=2` still traces); Restoration's glow was already in play with the spell. Checked: a frame of run I's
  crack at 1.5 cells with Toughness at V shows the motes and one with nothing confessed shows none, so a run without Toughness looks
  as it did. Nothing of D5 waits on a sheet now. Still the owner's: the Galvanist's sheet, and the stall by a buried shambler.

- **9 November. The owner's word on the two left open.** The Galvanist's weakness is Frost at -10, not -15 (`CLASS_OWN`; her question
  and the Codex read it from there). The stall by a buried shambler is mended at the owner's word: `enemies_within` (neither
  track's) no longer counts the buried (`buried(m)`, as `targetable` refuses them), so `in_combat` doesn't flicker as one under the
  earth comes into sight and goes, and the dream wants its place to mend whichever side of it it stands. It also leaves the buried
  out of the runes' triggers and the dream's fights that count enemies near, which is as `targetable` has always had it. Checked:
  the Galvanist's page reads *RESIST SLOW -10%*; the Plague Doctor's seed 9 from level 1 for 3,000 s on this build (C5 in play, so
  not the run that showed it) stood no longer than 40 s anywhere. Both go into J20.

- **9 November. J20, track D's half** (harness only), as track C's entry set it out: this branch at `a3efa7a` (with the Galvanist's
  Frost at -10 and the buried shambler's mend, which track C's half, run before them, hasn't) against 1.0.0b19 (`41fe10c`, built in
  a worktree), both run here on the same seeds with `CRAWL_RUNID` the seed: seeds 17-31 odd for each of the five classes a fresh
  run can be given, from level 1 for the first 3,000 s, and from level 31 for 1,500 s at `CRAWL_GEAR` 8 and `CRAWL_XPBANK` 77658.
  **From level 1**: 256 levels cleared and 48 deaths (251 and 48), a level's median 355 s (352), none on a boss's level (the runs
  end before it). By class: the Witchfinder 10 deaths (5), the Anatomist 4 (12), the Galvanist 18 (9), the Plague Doctor 12 (12),
  the Sin-Eater 4 (10). **From level 31**: 119 levels cleared (114), one death (none: a Sin-Eater on level 31, to a rank IV ember imp), a level's median
  364 s (366). What landed on the caster from level 1: knocked down 231 times (237), poisoned 43 (32), chilled 16 (13), on fire 0
  (6); from 31, on fire 15 (8), stunned 4 (none), the rest alike. The dream channelled Restoration 12 times from level 1 and 200
  from 31, confessed 160 and 720 ranks, lived no killing blow (none of the five confesses Constitution first); creatures born to
  an element 223 and 1,525 (224 and 1,506). Stalls: five runs of the forty from level 1 stood 40 s within a cell of a place
  (two), none past 41 s; from 31 none (four). **The Galvanist** again: 18 deaths against 9, eight of them to rank I skeletons on
  levels 1-4 (four); every batch since its sheet went in has it so (52 against 31 over thirty-two seeds, then 21 against 13, now
  18 against 9), with its Frost at -15 or -10. Its ninety health is a little of it (at 100 it had died 45 times where it died 52). What the rest is, its
  Dodge and Movement changing how the dream walks, or the eight seeds, isn't known; a person may fare otherwise. For the owner.
  **ASan** clean on five dreams of 600 s on this build, a class each across the starts (the Witchfinder
  from 1, the Anatomist from 11, the Galvanist from 21, the Plague Doctor and the Sin-Eater from 31; Restoration channelled and
  ranks confessed in them). Track C: the numbers are for your table; no contract changed. **Track D's half of J20 is done.**

  | From | Class | Cleared | Deaths | A level's median, s |
  |---|---|---|---|---|
  | 1 | Witchfinder | 51 (52) | 10 (5) | 353 (342) |
  | 1 | Anatomist | 51 (49) | 4 (12) | 363 (348) |
  | 1 | Galvanist | 50 (50) | 18 (9) | 359 (350) |
  | 1 | Plague Doctor | 52 (49) | 12 (12) | 352 (361) |
  | 1 | Sin-Eater | 52 (51) | 4 (10) | 348 (354) |
  | 1 | **all five** | 256 (251) | 48 (48) | 355 (352) |
  | 31 | Witchfinder | 24 (22) | 0 (0) | 380 (360) |
  | 31 | Anatomist | 24 (22) | 0 (0) | 362 (366) |
  | 31 | Galvanist | 24 (23) | 0 (0) | 362 (355) |
  | 31 | Plague Doctor | 24 (23) | 0 (0) | 348 (378) |
  | 31 | Sin-Eater | 23 (24) | 1 (0) | 386 (375) |
  | 31 | **all five** | 119 (114) | 1 (0) | 364 (366) |

  *Seeds 17-31 odd; from level 1 the first 3,000 s, from level 31 1,500 s; 1.0.0b19 in brackets.*

- **9 November. Released as 1.0.0b20** at the owner's word ("do what you need to, then release b20"), by track D's agent: J20
  done (track C folded both halves; the owner kept the harder start), the README's *New in 1.0.0b20* with J20's numbers, the page
  (the twentieth beta, the status row weighed, the upgrading note), the notes rewritten for it (the Galvanist's frost, the stall
  mended, the harder start said plainly), CLAUDE.md, `VERSION` 1.0.0b20; squashed onto main as one commit, as the betas before it.
  Next for track D: **D6**, the merchant by class (section 12), for 1.0.0b21; D7 waits on C6's contract.
- **9 November. D6 begun: its contract.** **CONTRACT.** At the owner's word ("go ahead and start D6"). Section 17.2 c gives D the
  pieces' rolls and the new stats' and abilities' effects (C6's contract will add C's rows). In 17.3: the new `ST_*` and `AB_*` at
  the ends of their lists (nothing rolls them yet), `st_adds`, `gear_cut(k)` (a stub; track C, I put the one line into your
  `conf_cut` that adds it to Agility's cut for Knocked Down, Stunned and Frozen, under the same 0.9, and it now counts with
  `CRAWL_LINES=1` too), and `abil_short` (a stub; your `render_caster`'s switch names Mirrored now, so a class ability on the page
  isn't drawn as a mirror). "ADDS %" shares its one cap among six. Nothing in play changes.
- **9 November. D6 is built: the merchant by class** (section 12; harness only). Commits `83c37b3` (the rolls), `ddea244` (felt),
  `87710b1` (her wares rolled after a class or a gift is taken in her room). **The rolls** (`crawl_gear.c`): an element's stat
  comes from the lines held by rank (`pick_elem`, G1); a piece in six may carry one for a line not held, never two; Rare and up
  are rolled again till they carry one for a held line (`on_line`, G2; boots and amulets roll no element's stat, so carry none
  and are left alone); the status chances on bracers and rings, Steadiness on boots and amulets (G3); "ADDS %" among all six,
  sharing one cap (G4); an ability on a piece of Uncommon or better is the class's one time in three (G5). Checked with `make
  items` (it takes `ITEMS_LINES`, `ITEMS_CLASS` and `ITEMS_OLD` now): 300 wares a level, levels 1-30, as a Witchfinder with Fire
  at 8 and Lightning at 3: every Rare-and-up bracer, hood, robe and ring carries one of the two; 5% of all pieces carry an
  off-line stat (13% of those with any element's stat), none two; Pyrebrand is a third of the abilities at Uncommon and up. A
  Headsman holding Physical alone rolls no status chance (there is none for Physical). **Felt**: `st_odds` (Ignition, Static
  and Hush a share more on their status's chance in `el_chance`; **my reading for the two that roll no chance today**: Rime lays
  a second Chilled stack that share of the time, Blight makes that share of your doses half again as long and as strong; for
  the owner to change), `cls_ab` (a class's ability felt by its class alone: Pyrebrand, a share more ignitions and a second
  more of burning at the same pace; the Slab, a share more on the frozen and the stacks a second longer; Live Wire, a share more
  stuns and `wire_pass`, a fifth of a hit on the stunned to the nearest within two cells, a spark that rolls nothing; Fever, the
  poisoned a share more from everything; Swallowed Sin, a share more Unbound and a soul from what dies Unbound; the Block, a
  share off Knocked Down and the spear's bone where a staggered or pinned one fell). **The Block cuts Knocked Down as Agility
  does, its chance with its length**, since it joins Agility's cut; the plan says shorter only. Steadiness and the Block go
  through `gear_cut` (CONTRACT, above), under Agility's 0.9. The cards say *FOR THE WITCHFINDER* on another's (and it does
  nothing); Tab's elements page has ADDS and ODDS; the Codex has words for each new stat and ability (frames of the wardrobe,
  her card, the elements page and the Codex's pages: nothing overflows). `CRAWL_D6=0` plays the build before line for line
  (two seeds, 900 s), and with it `make items` gives the old wares byte for byte.

  **Runs**, this build against `CRAWL_D6=0` on the same seeds (`CRAWL_RUNID` the seed), the five classes a fresh run can be given:

  | From | Class | Cleared | Deaths | A level's median, s |
  |---|---|---|---|---|
  | 1 | Witchfinder | 53 (51) | 9 (9) | 349 (343) |
  | 1 | Anatomist | 52 (52) | 5 (4) | 342 (362) |
  | 1 | Galvanist | 51 (46) | 19 (27) | 340 (356) |
  | 1 | Plague Doctor | 42 (48) | 29 (20) | 370 (357) |
  | 1 | Sin-Eater | 50 (51) | 8 (9) | 348 (345) |
  | 1 | **all five** | 248 (248) | 70 (69) | 349 (356) |
  | 31 | **all five** | 117 (118) | 0 (0) | 370 (374) |

  *Seeds 1-15 odd; from level 1 the first 3,000 s, from level 31 1,500 s at gear 8; `CRAWL_D6=0` in brackets.* Even in all. The
  Plague Doctor and the Galvanist were run on seeds 17-31 too: the Galvanist 27 deaths (18) there, 46 (45) over sixteen seeds;
  the Plague Doctor 14 (12) there, **43 (32) over sixteen**, most of it on levels 1-3 of seeds 1 and 7 (17 deaths on levels 1-3
  against 5). The two builds part from the first ware (a fresh run's free piece is rolled by the new rules), so the early levels
  differ at once; whether the class's wares, now leaning to Poison (Blight, Poison's Power, "ADDS %" Poison), leave the dream's
  early Plague Doctor worse off than random ones did isn't known; the Galvanist swung as far each way between the two halves.
  From level 1 the dream bought 18 pieces with its class's ability in 40 runs (3 from 31); Swallowed Sin left 31 souls from
  level 1, Live Wire arced 6 times from 31. No run stood still past 41 s. **ASan** clean on four dreams of 600 s (the Plague
  Doctor from 1; from 11, 21 and 31 the Galvanist, the Headsman and the Sin-Eater with `CRAWL_ABIL` their class's ability on every
  worn piece: Live Wire, the Block and Swallowed Sin fired). **Track C**: Tab's caster page (yours) has no row for Steadiness or
  the three new "ADDS %"; the right column has room for one more, and Steadiness would sit there. **D6 is built**; next for track
  D, D7 (the other five lines' augments) on C6's frame.
- **9 November. D7 begun: its contract.** **CONTRACT.** At the owner's word ("go ahead and start", expecting to approve the look).
  17.3's D7 bullet: `augd_name` and `augd_does`, the five lines' names and words for the Lampstand and the Codex (track C: read them
  for every line but Fire; `__attribute__((unused))` till you do). The bodies come next, behind `augd_built` returning 0 till the
  owner approves their sheet, and a debug switch of D's that forces a choice for testing before C6's frame chooses.
- **9 November. D7 is built: the other five lines' augments** (11.4's drafts, A15; harness only). Commits `6fbb937` and `1525dc0`.
  The thirty, each at the rank's share k (C's `aug_k`: half, three quarters, whole, the drafts' numbers being rank III, A8),
  through `augd_*` and D's own functions (CLAUDE.md's D bullet has where each sits): **Frost** Hoarbite (a fifth weaker; a second
  chill k of the time), Splinterfrost (three shards on behind what it strikes, 30% x k each), Rime Trail (the floor rimed 2 s x k
  along its flight), Glacial Weight (seven tenths the pace, through the first it strikes at 70% x k after), Cold Snap (what freezes
  bursts k of the time, a chill on all within 1.5 cells; a snap sets off no other), Brittle (the frozen take 25% x k more from
  everything); **Lightning** Forked (a second bolt within 3 cells at 50% x k), Overcharge (30% x k harder, its stun 1 + k as likely,
  a quarter more spirit), Static Build (each strike on one creature within 3 s raises its stun half x k a time, three times),
  Grounding (what stands in run II's water or is poisoned passes the bolt on at 70% x k), Thunderclap (the stunned struck, k of
  the time all within 1.5 cells stunned), Conductor (Ionization's trail 1 + 2k as long, stunning what crosses it k of the time);
  **Poison** Thick Brew (your poison 50% x k longer, as much more harm), Straight Throw (flat, 60% x k quicker, bursting on what it
  meets), Cluster (three small flasks at 40% x k), Miasma (a cloud 1.5 s, poisoning at k of the burst's dose), Pestilence (the
  poisoned leave a pool of k of what was left in them), Rot (the poisoned 15% x k more from every other element); **Spirit**
  Siphon (the drain heals 30% x k more, for the spirit healing costs, and hurts 15% less), Hollowing (Unbound 1 + k as likely),
  Lingering Hunger (the beam held 0.6 s x k, draining half the hit again x k; it doesn't heal), Soul Echo (a kill by the drain gives
  back k of its spirit), Exorcism (the Unbound 30% x k more from everything), Shepherd (the drain's kills' souls fly to you at 5 x k
  cells a second); **Physical** Barbed (half the hit x k bled over 3 s), Recovery (the bone left half x k of the time), Twin Throw
  (two spears for one bone, **45%, 52% and 60% by rank**: halving the guide's 60% would have made rank I weaker than one spear),
  Heavy Haft (the first struck staggers 0.5 s x k), Skewer (15% x k more for each pierced), Bone Rain (a kill bursts into three
  splinters at the nearest, 35% x k each; a splinter's own hits bleed, leave and rain nothing). The fixed costs (Hoarbite's fifth,
  Siphon's 15%, Overcharge's quarter) don't move with rank. Brittle, Rot and Exorcism count whatever strikes; the rest are the
  line's own spell. **The sheet** (`media/sheets/combat/d7-augments.png`, sent to the owner): what is new to the eye is three things,
  Barbed's bleed (`DR_BLEED`), Static Build's gold bolts and Miasma's green vapour; Thunderclap and Cold Snap use the orb's flash and
  the frost's burst; the rest what is drawn already. Till it is approved `augd_built` says none (`CRAWL_D7=1` offers them, with C's
  `CRAWL_AUG=1`). **Checked**: with nothing chosen, the dream plays the build before line for line (two seeds, 600 and 900 s);
  forced by `CRAWL_AUGD` on all five lines for each pair of the five classes from level 21, every augment that leaves an event left
  it; ASan clean on four dreams of 600 s with them forced (two pairs, ranks I and III, from levels 1, 21 and 31). With C's frame
  (`CRAWL_AUG=1 CRAWL_D7=1`) the dream chooses D's at the Lampstand and they rank up from its credits (the newest first, A4).
  **Runs**, both arms with C's frame on (Fire's augments in both), D7 on against off, five classes, seeds 1-7 odd, from level 21
  at gear 5 for 1,200 s: 44 levels cleared (42), no deaths either way, a level's median 364 s (371); 59 ranks gained on D's lines;
  Pestilence pooled 87 times, Twin Throw threw 60, Thunderclap 15, Lingering Hunger 5, Cold Snap 4, Conductor 2. One run stood 120 s
  on level 21, a Galvanist in the arm without D7. Too few runs to weigh (J21 will). **Track C**: Splinterfrost's shards (0.5),
  Glacial Weight's (1.35) and Cluster's flasks (0.6) set `Shot.scale`, which your missiles' part doesn't draw yet; they show at
  their usual size till it does. **D7 is built**; track D's part of 1.0.0b21 waits on the owner's word on the sheet, then J21.
- **9 November. D7's sheet approved; D's augments switched on.** At the owner's word ("I approve the sheet, switch them on"):
  `d7_on` is on in every build (`CRAWL_D7=0` in the debug build takes them out, for the balance runs), so `augd_built` says the five
  lines are built. They come into play wherever C6's frame is: with `aug_on` still off in the release build (C6's own sheet,
  `media/sheets/combat/c6-augments.png`, with the owner), nothing is chosen yet and play is the build before line for line (checked, 900 s);
  with `CRAWL_AUG=1` alone the dream now chooses D's at the Lampstand beside Fire's. **Track C**: once the owner approves your sheet,
  turning `aug_on` on brings all six lines in at once; nothing more is needed on D's side.
- **9 November. J21, track D's half** (harness only), as track C's entry set it out: this branch at `14adf0d` (code as `8fe7e56`:
  all six lines' augments in play, D6 in) against 1.0.0b20 (`e982abf`, built in a worktree), both run here with `CRAWL_RUNID` the
  seed: seeds 17-31 odd for each of the five classes, from level 1 for the first 3,000 s, and from level 31 for 1,500 s at
  `CRAWL_GEAR` 8 and `CRAWL_XPBANK` 77658. 1.0.0b20's runs here are J20's own for that build, line for line, as they should be.
  **From level 1**: 236 levels cleared and 75 deaths (256 and 48), none on a boss's level, a level's median 357 s (355). Deaths by
  level 1-7: 5, 6, 19, 11, 11, 17, 6; to rank I skeletons 34, rank II 8, shamblers 10, bats 5. **From level 31**: 117 cleared (119),
  one death (one: a Witchfinder on 32 to a gilded Lightless; 1.0.0b20's a Sin-Eater on 31 to a rank IV imp), a level's median
  376 s (364). With track C's half: **from level 1, 485 cleared and 158 deaths (504 and 117)**.

  | From | Class | Cleared | Deaths | A level's median, s |
  |---|---|---|---|---|
  | 1 | Witchfinder | 44 (51) | 25 (10) | 346 (353) |
  | 1 | Anatomist | 47 (51) | 8 (4) | 362 (363) |
  | 1 | Galvanist | 51 (50) | 14 (18) | 350 (359) |
  | 1 | Plague Doctor | 47 (52) | 15 (12) | 369 (352) |
  | 1 | Sin-Eater | 47 (52) | 13 (4) | 359 (348) |
  | 1 | **all five** | 236 (256) | **75 (48)** | 357 (355) |
  | 31 | all five | 117 (119) | 1 (1) | 376 (364) |

  **Which part.** The same forty starts from level 1 again, with the augments out (`CRAWL_AUG=0`, D6 in): 244 cleared and 64
  deaths; with D6 out (`CRAWL_D6=0`, the augments in): 243 and 80; with both out it is 1.0.0b20 line for line (checked to 900 s
  on two seeds). So the augments are most of it (+11 with D6, +32 without), and D6 is inside the noise (-5 with the augments, +16
  without; its own sixteen seeds came out even). The classes swing by ten between these variants (the Galvanist 27, 14, 10, 18),
  so only the totals and the pattern below are worth reading. **Where**: by the opening the dream drew at random for its own line
  (A15), each run against its own variant without the augments (ten or six runs a side): Siphon 21 deaths (5), Firecracker 22
  (7), Burning Intensity 20 (11), Hoarbite 13 (7), Thick Brew 26 (16), Straight Throw 16 (10), Splinterfrost 7 (4), Hollowing 6
  (7), Forked 13 (22), Overcharge 11 (23). With track C's half, against 1.0.0b20 alone (eight seeds a side): **Firecracker 27
  (4), Siphon 23 (4)**, Burning Intensity 15 (15), Hollowing 9 (9). What costs the dream is an opening that takes from the one
  spell it has on levels 1-7, against rank I skeletons that a quicker kill keeps off it: Firecracker's spark (a quarter now, the
  rest burning over four seconds), Siphon's 15% less (for healing that is small at rank I and paid in spirit), Hoarbite's fifth
  less (which costs less: its second chill holds them). Burning Intensity's 0.8 on a bolt not held comes out even over both halves: the
  dream holds it at range. What adds to the hit (Overcharge, Forked) helps it: the Galvanist died less with the augments in either
  variant. Thick Brew only lengthens a dose, so its 26 against 16 is the Plague Doctor's usual noise. **For the owner**, three ways:
  (a) keep it (a person chooses, and the dream's random choice is A15's); (b) the dream's first choice on its own line weighs the
  pair (the side that keeps its hit: Burning Intensity, Hollowing, Splinterfrost), which touches only the dream (`auto_augment`,
  track C's); (c) ease the two trades, Firecracker's spark (C's) and Siphon's 15% (D's: to 5%, or its healing free of spirit at
  rank I), which a person would feel too. I'd take (b), and (c) for Siphon only if a person finds it a poor trade. Nothing changed
  yet; the owner's word first. **What else**: from level 1 the dream channelled Restoration 9 times (12) and from 31 107 (200; not looked
  into: the spirit Multiply and Overcharge take may be what it lacks), lived no killing blow, and 197 creatures were born to
  an element from level 1 (223), 1,619 from 31 (1,525); the merchant by class: 30 souls from Swallowed Sin, one
  Live Wire's arc, no bone from the Block. The augments' events from level 1: Multiply's split bolts 1,303, Splinterfrost 1,295,
  Forked 301, Burning Intensity's held bolts 275, Cluster 266, Lingering Hunger 220, Soul Echo 133, Miasma 22; from 31, Twin Throw
  151, Miasma 114, Multiply 71, Cluster and Volatility 59 each, Thunderclap and Cold Snap 54 each, Bone Rain 36, Pestilence 27,
  Pyre 24, Conductor 9. 119 augments chosen from level 1 and 227 ranks gained (312 and 149 from 31, the deep start choosing at its
  first visit). Stalls: seven runs of the forty from level 1 stood 40 s within a cell of a place (five), two from 31 (none), none
  past 41 s. **ASan** clean on seven dreams of 600 s on this build: a class each across the starts (the Witchfinder from 1,
  the Anatomist from 11, the Galvanist from 21, the Plague Doctor and the Sin-Eater from 31), and two with every line open and
  every augment at III by `CRAWL_AUGSET`, the first of each pair from 31 and the second from 21 (Cold Snap, Forked, Thunderclap,
  Multiply, Burning Intensity's held bolt, Soul Echo, Miasma and Splinterfrost among what fired). Track C: the numbers are for your
  table; no contract changed. **Track D's half of J21 is done.**
- **9 November. The owner's word on J21: (a), keep it.** The augments stay as they are: the dream still draws each pair's side at
  random (A15), Firecracker's spark and Siphon's 15% unchanged; a person chooses their own. So 1.0.0b21 goes out with J21's numbers
  as they stand (from level 1, 158 deaths against 117 over both halves), as 1.0.0b20 did with J20's. Nothing changed in the code.
  Track C: both halves are in; the table, the README, the page and the notes are yours to fold, as J20's were. The release waits on
  the owner's word.
- **9 November. Released as 1.0.0b21** at the owner's word ("check the status of the work and if done, release"), by track D's
  agent: C6, D6 and D7 built and their sheets approved, J21's halves folded by track C (`09bbd35`, with the README, the page, the
  notes and CLAUDE.md) and the owner's word on them (keep it). Mended in those words before it went out, all D's side or the
  numbers: an Uncommon-or-better piece's ability is the class's one time in three (not one piece in three); Swallowed Sin's soul
  comes from what dies Unbound (not every kill); the build plays 1.0.0b20 line for line only with the augments and D6 both out
  (with the augments alone out it doesn't: D6 is in); the deaths ran from level 1 (track C's half had 8 there), and in track D's
  half rank I skeletons took 34 of 75 (not most). Added: the Alignment strip's caption says it shows the Lampstand before its
  slots, CLAUDE.md's current release and D7 note, the plan's status, `VERSION` 1.0.0b21; squashed onto main as one commit, as the
  betas before it. Track C: your log's fold says "most on levels 3 to 7 to rank I skeletons"; left as written (append only), the
  correction is here. **PLAN-combat is done.** Next, at the owner's word: the art pass's rest (1.0.0b22), the lower storey (1.0.0b23).

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

**The owner's answers to section 16 (7 November), as given:**

> R1 a, R2 a, take every other default for now aside for: B7. B; K1. B; K4. B; U1. B; U8. The player needs to select
> restoration as an active spell, so no need to keybind; A1. C; A2. B; A13. B; A15. B. I'd like the ability to split this work
> with another agent interacting in the same repo and branch.
