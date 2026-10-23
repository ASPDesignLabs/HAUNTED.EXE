# Items, abilities and ammunition: the merchant's new stock

**Status: agreed (23 October 2026); steps 1 and 2 built (items as data, the wardrobe, save version 3;
every worn stat felt in the game, footsteps and Quiet, the cuff and the totals page), tested in the
harness only; the owner checked the rolled wares (and asked for Exquisite a little more often); steps
3-7 to come.** A plan from the owner's guide, written after
reading the code as it stands at 1.0.0b3.

**Decided (23 October 2026):**

- I1 a: spirit is the souls you collect, spent on spells. I2 a: the starting fireball and any spell at
  tier-1 strength are free; above that, Fire Bolt costs too. I5 a: every spear needs a bone. I16 a:
  footsteps make noise, and Quiet boots soften it. I10 a: any item can roll an ability; hoods always do.
- **I19, in the owner's words:** no bag to begin with. Bags (two to five slots, with a fitting name) are
  bought from the merchant and hold items in reserve. Without a bag, buying an item for a slot already
  worn warns that the old item will be lost, and no gold comes back. With a bag that is full, the player
  is offered the chance to free a slot first, then buys.
- **I12, in the owner's words:** some items come Anointed. Black Altars also anoint, once every secret
  on their level has been found. Finding a Black Altar lights a wisp that leads to the nearest secret
  not yet found; it takes ten souls to wake. Black Altars stand deep in the maze, in the exit room
  where possible.
- **The follow-ups:** bags come now and then on a wearables pedestal, more often while you have none,
  sized by quality. Black Altars stand on every level from 6 (the Catacombs) on. Waking an altar's wisp
  spends ten souls. A finished altar anoints one of the worn items at random, never one already Anointed.
- Every other question takes its recommended answer (*). One default was set here, not by the owner:
  the autopilot leaves Black Altars alone (it seldom finds every secret anyway). Easily changed.

## 1. What the game does today

- **Wares.** Three pedestals: an attack spell, a defensive rune and something to wear. One ware a
  visit, the first visit free. A ware has a *tier* (about the level it is sold on, now and then one
  well ahead of its time) and a price set against the gold a level gives up (`gear_haul`).
- **What is worn.** A robe (padded, spider-silk, shadow, thorned or blessed), a hood (plain, warded,
  hunter's or Death's cowl) and up to four rings (Bone Charm, Soul Jar, Haste, Ember, Seeker's), each
  with one fixed effect that grows with its tier. Everything bought stays in the wardrobe (Tab).
- **Spells** cost nothing but time: each has a cycle (`spell_cycle`). Only the BFS9000 spends souls
  (100, fewer at higher tiers). Souls come one to six a kill, by rank; ten taken on a level bring the wisp.
- **Runes** fire on their own, each on its cooldown.
- **The record** of a run is saved with it (save version 2). `Gear` is copied raw into the save.

Measured in eight 50-minute autopilot runs at 1.0.0b3: the autopilot casts **30 to 54 spells a level**
(median) and nets **14 to 71 souls a level** (levels 1 to 8). Fire Bolt is a third of its casts, Bone
Spear a fifth. So if souls pay for spells, a soul a cast is about what a level can afford, and less on
the first levels.

## 2. The guide, as read

The owner's guide (23 October), restated, with readings marked *as read*.

**Quality.** Common (white), Uncommon (green), Rare (blue), Ultra-Rare (purple), Exquisite (gold).
Quality sets how strong each stat is, how many stats an item has (one at least, five at most, never
more than the item has slots, except that Exquisite gets one slot more), and its price.
**Anointed** can come on any item: it raises the item's rolled stats by 10% (Common) to 25%
(Exquisite) and costs nothing extra.

**Slots and their stats** (stat slots per item in brackets):

| Slot | Stats it can roll |
|---|---|
| Wrists, left and right (2) | Spell Haste, Spirit Cost Reduction, a little Armor, a little Dodge; a little Damage Multiplier from Rare up |
| Boots (2) | Movement, Quiet (creatures notice your steps less), Dodge |
| Neck (2) | HP, HP restored over time, Resist (Poison, Burn, Slow) |
| Hood (2) | Armor, Spell Effectiveness, HP; and an ability |
| Robe (4) | HP, HP restored over time, Resist (Poison, Burn, Slow), Armor, Spell Haste, Elemental Addition, Damage Multiplier |
| Ring, one only (1) | Damage Multiplier (to 2x), Elemental Addition (to +50%) |

Runes stay as they are.

**Abilities**, at most one an item *as read* (the name has room for one), named on the item and graded
by its quality. They stack across items, but only ever on the base value: the bonuses add up and are
applied once, so they never compound. The character sheet shows the sum.

| Ability | What it does | Graded by quality |
|---|---|---|
| Swiftness | more movement, up to +25% of the movement rolled | the percentage |
| Arcane Knowledge | more Spirit Cost Reduction, up to +50% of what was rolled; adds the stat if the item has none | the percentage |
| Treasure Hunter | treasure worth more (to +50%); an eerie sound near secret walls | the percentage |
| Keymaker | keys show while the wisp is lit; L cycles the wisp between the exit and each key (said on every equip and every purchase) | *as read*: how far away keys show |
| Commune | every 15 s, red silhouettes of every creature within a radius, for a moment (to 12 cells, 3 s) | radius and time |
| Thorns | hurts whatever strikes you in the flesh | damage |
| Mirrored | a chance to send a missile back, homing on whoever threw it | chance and damage |

**Names:** quality, ability, item, then "of" the strongest stat. *Uncommon Swift Boots of the Lightfoot*,
*Exquisite Treasure Hunter Robes of Rejuvenation*, *Anointed Common Ring of Spelltongue*.

**Ammunition.** The fireball, and every spell at its level-1 strength, is free. Above that a spell costs
Spirit Energy, by its damage: at least 0.1 a cast, at most 1. Seeking spells cost 1 for every
projectile. Spells that heal cost 25% to 50% of the health they restore. With too little spirit, a spell
is cast at its level-1 strength (and heals nothing). Bone Spear throws Bones, which skeletons and hounds
drop now and then; with none, no spears.

**The merchant.** Five pedestals, six from level 10, seven from 15. One purchase a visit to level 9,
two to level 14 (*as read*: the guide says "2 from 1-14"), three from 15. A person (not the autopilot)
can **hold** a ware for the next visit for 25% of its price, which isn't taken off the price; the hold
ends when the ware is bought or passed over on that visit.

## 3. The design, proposed

### Items

An item is a record: slot, quality, item level, anointed or not, up to five rolled stats (stat and
value), an ability (and its grade), and its price. The **item level** is the level the merchant is on,
and the numbers grow with it about as tiers do now, so a level-15 Common item still beats a level-3
Rare one at the same stat.

| Quality | Colour | Stats | Strength | Price | Ability | Anointed bonus |
|---|---|---|---|---|---|---|
| Common | white | 1 | x1.0 | x1.0 | never | +10% |
| Uncommon | green | 2 | x1.2 | x1.5 | 1 in 10 | +13% |
| Rare | blue | 3 | x1.45 | x2.2 | 1 in 4 | +16% |
| Ultra-Rare | purple | 4 | x1.75 | x3.2 | 1 in 2 | +20% |
| Exquisite | gold | all slots, and one more | x2.1 | x4.5 | always | +25% |

Stats are capped by the item's slots (boots and wrists two, robes four, rings one), so Exquisite boots
have three stats and an Exquisite robe five. The odds of each quality climb set by set:

| | Common | Uncommon | Rare | Ultra-Rare | Exquisite |
|---|---|---|---|---|---|
| The Crypts (1-5) | 59% | 30% | 9% | 1% | 1% |
| The Catacombs (6-10) | 39% | 35% | 18% | 6% | 2% |
| The Hollows (11-15) | 23% | 35% | 25% | 12% | 5% |
| The Deep (16-20) | 12% | 30% | 30% | 18% | 10% |

and one step further each time round. (Exquisite raised a little at the owner's word after the rolled
wares were checked, 23 October; the points came from Common.) About one item in ten comes Anointed; the Black Altars anoint
others (below).

**Totals are capped,** whatever is worn: Dodge 35%, each Resist 75%, Spell Haste +50%, Spirit Cost
Reduction 60%, Damage x2, Elemental +50%, Movement +40%, Spell Effectiveness +50% (and Quiet 80%). HP,
Armor, health restored and Thorns have no cap. No single piece rolls past a cap either, since the
excess could never count (a default taken while building step 1).

**What the stats do in the game:**

- *Armor*: points off every blow (as now). *Dodge*: a chance a blow misses (as the shadow robe).
- *HP*: more maximum health. *Restore*: health back each second.
- *Resist Poison / Burn / Slow*: poison (spiders, foul pools), burning (fireballs, embers, the burning
  floor) and slowing (grabs, frost) last shorter and hurt less.
- *Movement*: walking speed. *Quiet*: creatures notice your footsteps less (see question I16).
- *Spell Haste*: shorter time between casts. *Spirit Cost Reduction*: spells cost less spirit.
- *Damage Multiplier*: spell damage. *Elemental Addition*: rolls one element (fire, frost or lightning)
  and adds that much again of it to every spell, so it bites harder on what that element hurts.
- *Spell Effectiveness*: all a spell does besides damage: Soul Drain's healing, Frost's slow, Hellfire's
  burning floor, and the runes' strength.

As built in step 2: an element's share is weighed by the creature's weakness to Fire Bolt, Frost Shard
or Chain Lightning. Resisted poison lasts shorter by the full resist and stings less by half of it;
resisted slowing lasts shorter and drags less (60% pace at no resist, 75% at the cap). Spell
Effectiveness lengthens or strengthens each of those effects and every rune (the ward's bones, Mirror's
and Time Slip's time, Mend's healing, Purify's protection, Nova's freeze, Dread's fear, Reaping's time).
Footsteps, which no step named, came in with the boots in step 2: each step alerts creatures within
three steps along the halls (walls block it), for four seconds; Quiet shortens the reach, to the next
cell at its 80% cap.

**Names.** Each stat has its "of" name: Movement *of the Lightfoot*, Quiet *of Silence*, Dodge *of
Shadows*, Armor *of the Bulwark*, HP *of Vigour*, Restore *of Rejuvenation*, the resists *of Clean
Blood*, *of the Hearth* and *of the Unbound*, Spell Haste *of Quickening*, Spirit Cost Reduction *of the
Frugal Soul*, Damage *of Spelltongue*, Elemental *of Cinders*, *of Rime* or *of Storms*, Spell
Effectiveness *of the Adept*. The strongest stat is the one rolled closest to its best for that level.
Long names go on two lines on the merchant's cards and in the wardrobe (320 pixels hold 40 letters).

### Abilities

Graded by quality, Common to Exquisite; where two items carry the same ability their bonuses add up
(to the caps the guide sets) and are applied once to the summed stat.

- **Swiftness** +5, 10, 15, 20, 25% of the movement rolled; on an item with no Movement it adds Movement.
- **Arcane Knowledge** +10 to +50% of the Spirit Cost Reduction rolled; adds the stat if missing.
- **Treasure Hunter** +10 to +50% on treasure picked up; an eerie whisper within three cells of an
  unopened secret wall, louder as you near it.
- **Keymaker** keys glint through walls within 6 to 16 cells while the wisp is lit; **L** turns the wisp
  between the exit and each key still lying in the level, in colour order. "KEYMAKER: L CHOOSES WHAT
  THE WISP SEEKS" on every purchase and every time it is put on.
- **Commune** every 15 s, red silhouettes of creatures within 4 to 12 cells, for 1 to 3 s.
- **Thorns** hurts what strikes you hand to hand, by quality and level (the thorned robe's job now).
- **Mirrored** an 8 to 30% chance a missile goes back, homing on its thrower, with 50 to 100% of its
  damage. The Mirror Veil rune stays as it is.

### Spirit and bones

- **Spirit is the souls you already collect** (question I1), now spent, and shown with one decimal.
  The wisp still comes after ten souls *gained* on a level, whatever is spent.
- **What is free**: the fireball you start with, and any spell cast at its tier-1 strength.
- **What costs**: above tier 1, a cast costs its damage over a reference (about 0.1 for a tier-2 spell
  early on, rising to 1 for the strongest), less Spirit Cost Reduction. Seeker Wisps cost 1 a wisp.
  Soul Drain costs 50% of the health it restores at tier 1, down to 25% at the top. The BFS9000 keeps
  its own price.
- **Too little spirit**: cast at tier-1 strength, with no healing. A caption says so, once.
- **Bones**: every spear needs one. Skeletons, shield-bearers, knights and bone hounds drop one now and
  then (about 1 in 8, more at higher ranks); buying or improving the spear brings six. Carry up to 30.
- **The status bar** shows spirit beside the souls and, with the spear in hand, the bones.

### The merchant

- **The room** widens so that seven pedestals stand in an arc round the back wall.
- **Pedestals**: a spell, a rune and the rest things to wear: five to level 9, six to 14, seven from 15.
- **Purchases**: one a visit to 9, two to 14, three from 15. The first visit's first purchase is free.
- **Holding**: Q at a pedestal holds its ware for 25% of the price. It doesn't use up a purchase, and
  only one ware is held at a time. On the next visit it is back, exactly as it was and at the price it
  showed. Bought or passed over, the hold is gone. Holds are kept in saves and graves.
- **The autopilot** buys up to its allowance by value for gold, never holds, and keeps a little gold back.

### Bags

- **None at first.** A bag is a ware like the others. It turns up now and then on a wearables pedestal,
  more often while you have none, and buying one counts as a purchase. Its size goes by quality:
  Common 2, Uncommon 3, Rare 3, Ultra-Rare 4, Exquisite 5. Its name goes by size: a Burlap Sack, a
  Gravedigger's Satchel, a Pilgrim's Scrip, a Bone Reliquary. A bigger bag takes over a smaller one's
  contents.
- **Buying for a slot already worn**: the new item is worn at once. The old one goes into the bag if
  there is room. With no bag, the merchant's card warns first: "YOUR OLD BOOTS WILL BE LOST. NO GOLD
  COMES BACK." A second E buys. With a full bag, you are offered the bag's contents to let one go,
  then the purchase goes ahead; Esc thinks better of it.
- **The autopilot** buys bags too, but never asks: what it can't keep, it loses.

### Black Altars

- **Where**: one on every level from 6, deep in the maze: in the exit room where it can stand clear of
  the way, otherwise in the room farthest from the start. It is drawn apart from the black altars of
  the Deep's dressing: a violet flame burns on it when it wakes.
- **Waking it**: E at the altar spends ten souls. A wisp kindles there and leads to the nearest secret
  wall not yet found, then the next, until none is left. It goes out when the last one opens. While it
  is lit it takes the place of the ordinary wisp; L (Keymaker) still turns the ordinary one.
- **Anointing**: once every secret on the level is found, however, the altar's flame turns gold. E then
  anoints one of the worn items at random, never one already Anointed, and names it in a caption. One
  anointing an altar. With everything worn already Anointed, the altar says so and keeps its gift.
- **The autopilot** leaves altars alone (a default, not the owner's).

### The wardrobe and the character sheet (Tab)

- Seven worn slots (left wrist, right wrist, boots, neck, hood, robe, ring) and the bag, if you have one,
  below them. Items move between them with Enter.
- The selected item's card: name in its quality's colour, every rolled stat, the ability and its grade.
- The sheet: every total, with what abilities add shown beside it, for example `MOVE +18% (+3% SWIFT)`,
  green where it would get better and red where worse, as now.
- On the hand: the sleeve takes the robe's quality colour, a cuff at the wrist the bracer's, the ring's
  gem its colour; the hood frames the portrait. Boots and the amulet aren't seen.

### What goes

The robe, hood and ring kinds of today give way to rolled stats. The Thorned Robe's job passes to
Thorns, the Ember ring's to Damage and Spell Haste, the Bone Charm's to HP, the Haste ring's to
Movement. Four things have no successor: the Soul Jar and Death's Cowl (more souls), the Seeker's Ring
(gold sensed through walls) and the Hunter's Hood (picking out spiders in webs and wall skulls
sooner). With souls now spent as spirit, losing the first two makes spirit scarcer.

### Saves

`Gear` changes shape, so the save goes to version 3, carrying the worn items, the bag and its
contents, the bones and the held ware. Old saves keep their spells, runes, gold and souls; their worn
robe, hood and best ring become Common items of about the same strength, and they have no bag. The new
blob is about 950 bytes with the record; graves (`SLOT_MAX`) go from 1024 to 2048 bytes to leave room.

## 4. Building it

1. **Items as data**: `crawl_gear` rewritten around the item record: rolls, qualities, stats,
   abilities, names, prices, the totals and their caps. A harness page that rolls a hundred items a
   level, to check names and numbers before anything uses them. Save version 3 and the old saves'
   conversion.
2. **Worn and felt**: the seven slots in the game (armor, dodge, the resists, health, movement, haste,
   damage, elements, effectiveness), the wardrobe and character sheet, the hand and the portrait.
3. **Spirit and bones**: costs, the fallback, bone drops, the status bar, the autopilot's spending.
4. **Abilities**: Swiftness, Arcane Knowledge, Treasure Hunter (and its whisper), Keymaker (and L),
   Commune (silhouettes through walls), Thorns, Mirrored.
5. **The merchant**: the wider room, five to seven pedestals, more purchases, holding (Q), the cards in
   quality colours, bags and the warnings that go with them.
6. **Black Altars**: placing them, waking their wisp, the route to each secret, anointing.
7. **Balance**: eight-seed runs against 1.0.0b3, from level 1 and from 11; ASan; the README and plan
   kept true. Then a beta, once the owner has played it.

Risks worth naming:

- The autopilot will be weaker while spirit is scarce: it now pays for most of its spells. The runs
  will show how much.
- Quiet only means something if footsteps can be heard (question I16).

## 5. Questions

**Spirit and ammunition**

- **I1.** Spirit Energy is: a) the souls you collect, now spent on spells, shown with one decimal* b) a
  new meter beside souls that refills slowly c) a new meter filled by kills, apart from souls.
- **I2.** What casts free: a) the starting fireball and any spell at tier-1 strength; above tier 1, Fire
  Bolt costs too* b) Fire Bolt free at every tier, the others at tier 1.
- **I3.** A damage spell's cost: a) by its damage per cast, from 0.1 to 1, less Spirit Cost Reduction,
  the scale tuned in runs* b) a fixed table per spell and tier.
- **I4.** Soul Drain costs a) 50% of the health it restores at tier 1, down to 25% at the top* b) 25%
  rising to 50% c) a flat third.
- **I5.** Bones: a) every spear needs one, tier 1 too; the spear brings six when bought or improved;
  skeletons, shield-bearers, knights and hounds drop them (about 1 in 8); carry up to 30* b) tier-1
  spears free, like other spells.
- **I6.** Runes stay free, cooldowns only, Mend included: a) yes* b) healing runes cost spirit too.

**Items**

- **I7.** Quality odds: a) climbing set by set, as in the table* b) the same at every depth.
- **I8.** Stats by quality: a) 1, 2, 3, 4, and all slots plus one for Exquisite, never past an item's
  slots* b) Common 1, Uncommon 1-2, Rare 2, Ultra-Rare 3, Exquisite all slots plus one.
- **I9.** Abilities: a) at most one an item; never on Common, 1 in 10 Uncommon, 1 in 4 Rare, 1 in 2
  Ultra-Rare, always on Exquisite* b) other odds (say which).
- **I10.** "Hood ... + Ability": a) any item can roll an ability, and hoods always have one* b) any item,
  hoods no different c) hoods only.
- **I11.** An ability on an item without its stat (Swiftness on a robe) adds the stat, as the guide says
  for Arcane Knowledge: a) yes, for every ability with a stat* b) no, it does nothing there.
- **I12.** Anointed: a) a blessing by chance, about one item in ten, any quality* b) given at the black
  altars of the Deep, to an item you choose c) both. *Answered by the owner: by chance, and at Black
  Altars once a level's secrets are found (see Decided).*
- **I13.** Resist (Poison, Burn, Slow): a) three stats, each rolled on its own* b) one stat for all three.
- **I14.** Spell Effectiveness strengthens what spells do besides damage (healing, slowing, burning
  floors) and the runes; Damage Multiplier only damage: a) yes* b) another meaning (say which).
- **I15.** Elemental Addition: a) one element per roll, extra damage of it on every spell, by the
  target's weakness to it* b) the same, and frost also slows, fire also burns.
- **I16.** Quiet on boots: a) footsteps become a new way creatures notice you (a few cells, more when
  running), and boots quieten them* b) Quiet shortens how far creatures see you and hear spells.
- **I17.** Wrists: a) two separate pieces, left and right, each up to two stats* b) one pair.
- **I18.** Caps are on totals across everything worn (Dodge 35%, each Resist 75%, Spell Haste +50%,
  Spirit Cost Reduction 60%, Damage x2, Elemental +50%, Movement +40%, Effectiveness +50%): a) yes* b)
  other caps (say which).
- **I19.** *Answered by the owner: bags bought from the merchant, none at first (see Decided).* The bag:
  a) twelve unworn items kept; a full bag sells the weakest back for 25%* b) no bag: a
  new item replaces what was in its slot.
- **I20.** Let the old kinds go (Soul Jar, Death's Cowl, Seeker's Ring, Hunter's Hood): a) yes* b) keep a
  Soul Gain stat on rings and amulets, so spirit is less scarce.

**The merchant**

- **I21.** Purchases: a) one a visit to level 9, two from 10 to 14, three from 15* b) as written, "2 from
  1-14" (say what it means).
- **I22.** Pedestals: a) a spell, a rune and the rest things to wear* b) another mix (say which).
- **I23.** Holding: a) Q holds a ware for 25%; it doesn't use a purchase; one at a time; it comes back
  as it was, at the price shown; kept in saves* b) holding uses up a purchase.

**The rest**

- **I24.** Keymaker: a) keys glint through walls within 6 to 16 cells by quality while the wisp is lit;
  L turns the wisp between the exit and each key left, in colour order* b) other (say which).
- **I25.** On the hand: a) sleeve, cuff and ring gem in their quality's colours; the hood frames the
  portrait* b) nothing changes on the hand.
- **I26.** Old saves: a) spells, runes, gold and souls kept; robe, hood and best ring become Common
  items of like strength* b) the wardrobe starts empty.
