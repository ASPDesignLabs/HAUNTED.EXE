# Stat caps: what they are, where they bite, and caps by class

A brief for the owner (8 November 2026), written beside 1.0.0b20's Confessions. Nothing here is built; section 4 asks what
you'd like, and section 3 drafts class-specific caps for you to look into.

## 1. How a cap works today

- **One table.** Every capped stat has its cap in `STAT_CAP` (`src/crawl_gear.c`). A stat's total is what is worn (with
  the abilities that lift it: Swiftness on Movement, Arcane Knowledge on Spirit Cost, Treasure Hunter on Treasure) plus the
  caster's own, and the whole is held to the cap. The caster's own is the class's sheet (D5, the innate stats a run starts
  with) and the Unholy Confessions (C5), all summed before the cap.
- **No piece rolls past its stat's cap**, so a single piece can never be wasted outright; it is the sum that meets the cap.
- **What raises a cap.** Only overlevels, past Level of Atonement 50: a Confession taken past V raises the cap of each stat
  it feeds by what the overlevel adds (the old lines did the same, five a time). Nothing else moves a cap today.
- **The elements share one cap.** Fire, Frost and Lightning (the share of every spell's damage) have one cap of 50 between
  the three, not 50 each.
- **Uncapped:** Armour (it keeps off a share that already flattens: armour / (armour + 4 + 0.8 x level)), Health and
  Restores.
- **Other caps outside the table:** the abilities' own, summed across what is worn (Swiftness 25, Arcane Knowledge 50, Treasure
  Hunter 50, Keymaker 16, Commune 12 and 30 for its two numbers, Mirrored 50), and the Confessions' own: a status cut never passes 90%, the chance to
  live a killing blow never 50%.

## 2. The caps, and what reaches for them

The Confession's column is its rank V at a class's full share (100%); a class at 50% gets half. The class sheet is D5's
draft as built.

| Stat | Cap | Class sheets that add to it | Confession at V (100%) |
|---|---|---|---|
| Dodge | 35% | Galvanist +6 | Agility +10 |
| Movement | 40% | Galvanist +6 | Agility +15 |
| Quiet | 80% | Galvanist +15 | Agility +25 |
| Spell Haste | 50% | Witchfinder +6 | Spirituality +10 |
| Spirit Cost (less) | 60% | Witchfinder +6 | Spirituality +15 |
| Spell Effect | 50% | Witchfinder +8 | Spirituality +20 |
| Damage | 100% | | |
| Fire, Frost, Lightning (shared) | 50% | | |
| Each element's Power / Efficiency / Insight | 50 / 40 / 30 | | |
| Resist Poison | 75% | Plague Doctor +25 | |
| Resist Burn | 75% | Witchfinder +10 | |
| Resist Slow | 75% | Anatomist +15 | |
| Resist Spirit | 75% | Sin-Eater +15 | |
| Resist Lightning | 75% | Galvanist +15 | |
| Treasure | 100% | Plague Doctor +5 | Survival +15 |
| Luck | 25% (2% everyone's) | Plague Doctor +1 | Survival +2.5 |
| Vessel (spirit carried) | 100 | | |
| Sprint | 7.5 s over the 2.5 s base | Headsman +1, Plague Doctor +0.5 | Constitution +2.5 |
| Physic | 400% | Anatomist +15 | Restoration +50 |
| Armour, Health, Restores | none | Anatomist +5 and Headsman +4 armour; health 90-120 | Toughness +14 and +25% of worn; Constitution +100 |

**Where they bite.** With every slot worn at the debug build's gear 8 (generous: an Uncommon piece of level 16 in every
slot), the frames for the new screen showed Quiet at 80%, Spirit Cost at 60% and Spell Effect at 50%, and Movement reaching
40% at Agility V. A run's real gear is thinner, and how often the dream's runs meet a cap hasn't been measured; a debug line
counting it is a small job if you want numbers before deciding.

**What the screen now shows** (built today): each stat as you have it in all (worn, innate and confessed) and as the next
rank would leave it; a pale blue lid over any number at its cap, and over a Confession's numeral in the list when anything of
it is capped; *AT ITS CAP: ...* at the foot of the pane; and when everything a Confession gives is already capped, D asks
twice before confessing it.

## 3. Caps by class: options

- **(a) A few caps in each class's sheet.** Each class lifts the caps of the two or three stats its sheet already favours,
  by about a fifth, through the same road overlevels use. Small, readable on the class question's screen, and it moves no
  other class. It needs one change to the shared contract (D5's `class_own` would hand back a cap lift as well as the
  stats). *Recommended.*
- **(b) Caps by the class's Confession shares.** Every stat a Confession feeds takes a cap scaled by the class's share of it
  (say cap x (0.9 + 0.2 x share)): the Galvanist's Agility stats rise, the Anatomist's fall a little. Thorough, but it
  touches every class and lowers some caps from today.
- **(c) Caps raised by rank.** A Confession whole at V lifts its stats' caps by a fifth of what it gives, and each overlevel
  as now. Earned, not given; class-flavoured only through the shares. Could sit beside (a).
- **(d) Leave caps common, and give classes more of what is uncapped** (health, armour, restores, the cuts). No caps move;
  the classes differ only below them.

A draft for (a), to start from (every number is yours to change):

| Class | Caps lifted |
|---|---|
| Witchfinder | Spell Haste 50 to 60, Spell Effect 50 to 60, Resist Burn 75 to 85 |
| Anatomist | Resist Slow 75 to 85, Physic 400 to 480, Resist Poison 75 to 80 |
| Galvanist | Dodge 35 to 42, Movement 40 to 48, Resist Lightning 75 to 85 |
| Plague Doctor | Resist Poison 75 to 85, Luck 25 to 30, Treasure 100 to 120 |
| Sin-Eater | Resist Spirit 75 to 85, Vessel 100 to 120, Spirit Cost 60 to 65 |
| Headsman | Sprint 7.5 to 9 s over the base, Damage 100 to 110 (armour and health have no cap to lift) |

## 4. Questions

- **CP1.** Which: (a), (b), (c), (d), or (a) with (c)?
- **CP2.** If (a): the draft above, or your own numbers?
- **CP3.** Should the class question's screen show a class's caps beside its sheet (*DODGE TO 42%*)?
- **CP4.** The Confessions' own caps (a status cut 90%, the killing blow 50%): common to all, or by class too?
- **CP5.** The five resists sit at 75% for everyone: should a class's own element's resist go higher (the Witchfinder's
  Burn, the Galvanist's Lightning, and so on), as the draft has it?

## 5. Who would build it

The caps live in `src/crawl_gear.c` (shared by both tracks); the class sheets are track D's (D5); the Confessions' screen
and its cap marks are track C's. Option (a) or (b) would be one contract change (17.3), then a few lines on each side. It
could go into 1.0.0b20 with D5, or wait for 1.0.0b21. Tested in the harness only, as everything in 1.0.0b20 so far.

## 6. Settled with the owner (8 November), for track D to build

The owner's answers, and what was worked out from them. **Track D builds it** (the class sheets are D's), on the hooks track C
added (`class_cap`, below); C has done its two pieces (the hook, and the killing blow's cap).

- **Every resist has a cap of its own**: Fire (Resist Burn), Frost (Resist Slow, which is the frost resist), Lightning, Spirit,
  Poison, and now **Armour**, so a class can be more or less resistant over a long run. A class is most resistant, by default,
  to its own element.
- **CP1, CP2: option (a), with every lift at +30% of the cap, not +20%**, so the class is felt in the middle and late game.
- **CP3: yes.** The class question's screen shows each class's caps beside its sheet.
- **CP4: the killing blow survived is capped at 30%** (built by C: Constitution's chance never passes it).
- **CP5: yes, and a yin for every yang.** Each class starts with its own element's resist raised (that element hurts it less)
  and one opposite element's lowered (it hurts it more), both shown in the class's stats.

**The pairs** (the owner gave the Galvanist's; the other five worked out here, for the owner's word):

| Class | Strength (its own) | Weakness | Why |
|---|---|---|---|
| Witchfinder | Fire | Frost | the pyre's servant; the cold numbs the hand that lights it |
| Anatomist | Frost | Fire | cold flesh kept in spirits of wine: it burns |
| Galvanist | Lightning | Frost | the owner's: ice stops the current |
| Plague Doctor | Poison | Spirit | tends the body, never the soul |
| Sin-Eater | Spirit | Poison | eats the bread laid on the dead, and whatever is in it |
| Headsman | Physical (armour) | Lightning | plate carries it to the man inside |

Frost is two classes' weakness and Physical is nobody's: most harm is physical, so a class weak to it would be weak to almost
everything, and armour can't sensibly go below nothing. The owner took these pairs as they stand (8 November).

**The numbers** (a draft for D to start from):

- Starting resists: the strength **+15%** (today's sheets give 10-25; evened out), the weakness **-15%** (that element harms
  it 15% more). A resist may now go below nothing.
- Caps lifted by +30% of the base, for the strength's resist and the draft's other two:

| Class | Caps lifted (+30%) |
|---|---|
| Witchfinder | Resist Fire 75 to 97.5, Spell Haste 50 to 65, Spell Effect 50 to 65 |
| Anatomist | Resist Frost 75 to 97.5, Physic 400 to 520 (its draft's Resist Poison dropped: one strength each now) |
| Galvanist | Resist Lightning 75 to 97.5, Dodge 35 to 45.5, Movement 40 to 52 |
| Plague Doctor | Resist Poison 75 to 97.5, Luck 25 to 32.5, Treasure 100 to 130 |
| Sin-Eater | Resist Spirit 75 to 97.5, Vessel 100 to 130, Spirit Cost 60 to 78 |
| Headsman | Armour 75% to 97.5% of a blow kept off, Sprint 7.5 to 9.75 s over the base, Damage 100 to 130 |

- **Armour's cap**: armour keeps off a share of a blow (`armour_keep`); its cap is on that share, 75% for everyone, as the
  resists', and lifted for the Headsman.
- **The owner's word (8 November): the pairs above as they stand, and a resist's lift stops at 90%** (armour's share too: 75% to
  90%), Spirit Cost's at 70% (60 to 70); every other cap takes the full +30%. So in the table above, read 90 for each 97.5, and
  the Sin-Eater's Spirit Cost 60 to 70.

**Who does what.** D: the class sheets' strengths and weaknesses (`class_own`), the caps (`class_cap`: the lift of stat st's
cap in its units, which `caster_sync` adds to the caps), armour's cap in `armour_keep` (given to D for this, 17.2 b), and the
class question's screen and the Codex's class pages showing them. C: done (the hook in `caster_sync`, and the killing blow's
cap in `conf_at`). A resist below nothing already works in `el_resist` and `hurt_ex` (the harm grows by it); the Confessions'
screen shows the caps as they are.
