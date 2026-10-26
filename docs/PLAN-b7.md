# 1.0.0b7 and after: death that costs, runs of ten, bosses, and the merchant's hall

**Status: planned (26 October 2026); questions open; nothing built.** The owner offered the revamp below
after 1.0.0b6. This plan restates it, says how each part has been read, fills the gaps with defaults
(marked *default* so they can be changed), proposes the bosses and abilities the owner asked for ideas
on, and ends with the building steps and the questions. Questions marked **(needs an answer)** change
the shape of the work; the rest have a default that will be taken unless the owner says otherwise.

A mock-up of the ability socket is in [`plan-b7/ability_socket.png`](plan-b7/ability_socket.png).

## What the owner asked for

1. **Death costs.** Dying takes every item, the gold, the spells, the Cursed Coins carried and the
   souls, and sends you back to the merchant's before level 1. Kept, with the save: what the merchant's
   room has been upgraded with, what the Corpus Compendium has indexed, the runes, the Hexed Totem's
   abilities and the gold in the Wretched Repository. Each death raises a gravestone in a new grave plot
   by the merchant's: the last ten, each with the date and time, what killed you, the level reached and
   what you carried.
2. **Saving only with the merchant.** Loading works anywhere, as now. No save scumming: if a load is used
   to cheat death, she takes up to half your gold and one thing you have equipped (a spell drops a rank
   instead; never a rune or a boss ability), signs the save, and tells you that you were caught trying
   to cheat the dead of their bounty, so they took their claim, whatever you meant by it.
3. **Runs of ten.** A run is ten levels, not twenty, and ends with a boss on its tenth. The four sets'
   looks stretch across a whole run each; the decay creeps across the run.
4. **Bosses.** A much stronger version of one of the run's creatures, or of one still to come. Far more
   health; at each quarter of it lost it turns untouchable and calls a wave of adds, and moves again once
   they are dead; a wave also comes at 10% health even if the blow would have killed it, so that every
   fight has its three waves. Each boss sets its own adds. A planned arena for each boss, the same every
   time: open, with cover that breaks sight and some ways round. AdLib-style boss music, eerie, that
   changes while the adds are out. A few healing points, placed with care. Bosses drop only their own
   Named Odious Scrip.
   - **Level 10: the Ill-Fated Skeleton Knight.** Bigger, faster, a little shiny. A Rare Mirror Rune that
     turns back some of your missiles, with a sign you can see and hear. **Swift Dodge**: half a second
     untouchable while it darts two to five cells, with three cooldowns: to close on you (shortest), to
     dodge a hit (longest), to flee (between). Adds: skeleton kinds, one ranged or caster a wave and two in
     the last, at the highest rank you have met.
   - **Level 20: the Wandering Hunting Spider.** Its size, a little faster, often hopping between floor
     and ceiling. A Rare Mend Rune. **Tangled Web** roots you until you have done a third of your health in
     damage to it; one web at a time; its cooldown starts when you are free. Adds: smaller hunting spiders
     without the web, and one more every fifteen seconds that comes straight for you.
5. **The merchant's room grows.** More pedestals, two to seven (500 gold for the third, each next 2.5
   times the last), bought from her. The **Corpus Compendium**: every piece worn and spell learned, by
   slot and quality, to buy again at the shop's price x1.2, 5% less for each run completed, down to half.
   The **Wretched Repository**: pieces (10% tax), Cursed Coins (free in, 2,500 gold each out) and gold (25%
   tax), kept through death; two places free, one of them untaxed; more from 2,500 gold, x1.5 each, to
   twelve. The **Accursed Portal**: once a run is completed, start there again after a death (levels 1,
   11, 21, 31), and fight beaten bosses again for scrip. The **Hexed Totem**: each boss's ability, unlocked
   with one of its scrip and raised with more, each to its own cap; only known once that boss has fallen.
6. **Boss abilities for the caster.** **Swift Dodge** (three ranks): tap a direction twice and dart two
   to five cells, not a teleport; cooldown 15, 7 and 3 s. **Tangled Web** (three ranks): webbing at what
   is in front of you; it holds the creature, halves its attacks and makes it claw at the web instead of
   you, does no damage, and holds a third, two thirds or all of your health in damage. A rank holds that
   many webs at once; a kill or a break frees a place; a miss costs only 2 s of cooldown; your own spells
   don't touch your webs.
7. **The Codex of Affixations** on C: what every ability does, from gear, runes and bosses; only the ones
   known; kept through death. **A bosses section in the bestiary.** **A place on screen for abilities
   on the right mouse button**, with a suggestion where.
8. **Ideas wanted** for more bosses and their abilities.

## How it has been read (please correct anything wrong)

- **Runs and sets.** Run I is levels 1-10 in the Crypts, run II 11-20 in the Catacombs, run III 21-30
  in the Hollows, run IV 31-40 in the Deep; each set's look and decay are spread over its run's ten
  levels instead of five. (Q1)
- **Completing a run** is beating its boss. You then go on, everything still yours, to the merchant's
  before the next run's first level. A run completed counts towards the Compendium's discount and opens
  the next run's start in the Accursed Portal. (Q24, Q27)
- **"Shop slots"** are the merchant's pedestals: two at first, up to seven, bought from her menu.
- **"Equipped"**, for her claim, means the seven worn pieces and the spells known; the bag, the runes and
  the Totem's abilities are never taken.
- **The three waves** are read as 75%, 50% and 25%, each mark holding however hard the blow that reaches
  it. The brief also says "again at 10%"; Q12 asks which it is.
- **The Repository's free places**: two from the start, one of which charges no tax on what goes in.
  Twelve places in all, ten of them to buy. (Q30)
- **Scrip** is a currency of its own per boss, kept through death. (Q7)

## 1. Death

### What goes and what stays (*default*, where the brief is silent)

| Lost on death | Kept with the save |
|---|---|
| Gold carried | The Repository: its pieces, gold and Cursed Coins, and its places bought |
| The seven worn pieces, the bag and what is in it | The pedestals bought |
| Every spell but the Fire Bolt, which goes back to the fireball | The Rune Shrine: its rank, its nine slots, its attunements |
| Souls (spirit) and bones | Every rune held, at its quality |
| Cursed Coins carried | The Corpus Compendium's index |
| The ware held at the merchant's (Q17) | The Hexed Totem: abilities, ranks and unspent scrip |
| | The Accursed Portal's runs opened; the runs completed |
| | The record: the bestiary, the achievements (Q7) |
| | The Codex's entries; the grave plot; the L menu's choice |

- You wake at full health in the merchant's room before level 1, or choose a later run's start at the
  Accursed Portal once it is open. Level 1's free first ware is there as always.
- The fall and the red haze stay; then a card: *YOU DIED ON LEVEL 7, TAKEN BY A COLD SHIELD-BEARER*, what
  was lost in a line, and *A NEW STONE IN THE PLOT*.
- The death is written to the save the moment you fall, not when the haze clears, so closing the window
  during the fall does not undo it.
- The easing after a death on a level (15% softer blows a death, up to 45%) has nothing left to do and
  goes, except in the screensaver's dream (section 3).
- **In a pocket** a death throws you back whole to the chain's room today. Q6 asks whether that mercy
  stays. Recommended: keep it. Chains are side trips whose prize is a coin and a drink; losing a whole
  run to one would leave nobody going in.

### The grave plot

- A walled yard south of the merchant's room (the map in section 6): ten stones in two rows, the newest
  nearest the arch. An eleventh death takes the oldest stone away.
- **E at a stone** opens its page, laid out like the bestiary's: the epitaph (*HERE LIES A CASTER OF THE
  THIRD RUN, TAKEN BY A BLOODY SHIELD-BEARER ON LEVEL 24, THE BROKEN SEAM*), the date and time (*26
  OCTOBER 2026, 21:14*), and what was carried: the worn pieces (W/S over them shows each card), the bag,
  the spells and their tiers, the runes, the gold, souls and coins. A/D goes to the next stone.
- **What killed you** is kept as it lands: the creature by its caption name and rank (*A COLD
  SKELETON*), a boss by its title, the Wraith, or the thing it left behind (*A SPIDER'S POISON*, *AN EMBER
  IMP'S FIRE*, *A DROWNED ONE'S POOL*, *THE KNIGHT'S MIRROR*: your own bolt, turned back).
- The stones belong to the save, so each save has its own plot.

## 2. Saving with the merchant, and her claim

### Signing and opening

- **The save slots need a new name**, since gravestones now mean deaths. Proposed: **the merchant's
  ledger**, three pages, I to III (Q8). *SIGN THE LEDGER* saves; *OPEN THE LEDGER* loads.
- **E at the merchant** opens her menu *(default: explicit rows, so you always know when and where it
  saved; Q9)*:
  - **SIGN THE LEDGER**: choose a page; the page you came from is picked first; signing over another
    run's page asks twice, as carving over a grave does now. Each page shows its level, gold and when it
    was signed.
  - **ANOTHER PEDESTAL**: the next one and its price (section 6).
  - **PAT HER**: as now. The cat stays exactly where she is.
  - **LEAVE**.
- **The Esc menu**: *FLEE*, *STAY*, *SLEEPWALK*, *OPEN THE LEDGER*. Carving goes; the merchant's
  ledger is the only place to sign.
- **Fleeing** keeps nothing since the last signature *(default)*: the next start opens your last page.
  Fleeing in danger brings the claim, below.
- **Three things write a page**: signing it, a death, and the merchant's claim. Nothing else.
- With a console code used, nothing is written, as today: no signing, no deaths, no claims.

### Her claim: what counts as cheating death

The rule has to work the same whatever a person meant, so it is mechanical *(default; Q13-Q15)*:

- **A page is marked while you are in danger** in the run it holds, and the mark clears 8 s after the
  danger has passed. Danger is any of:
  - a creature hunting you within eight cells, or a missile coming at you (the game's own `in_combat`);
  - health under a quarter;
  - poisoned, burning, held in a web or grabbed;
  - inside a boss arena, from the doors shutting until the boss falls.
- **The mark is written the moment danger begins**, as a small value beside the pages, and cleared when
  it ends. So a load from the Esc menu, a flee, a closed window, Alt-F4, a crash or a power cut while
  marked all count. The brief says "regardless of intention", and a crash is the hard case (Q14).
- **The next time a marked page is opened** (by a load, or the next start), she takes her claim before
  you can move:
  - **gold:** a quarter to a half of what you carry, rolled *(default; Q15)*;
  - **one thing equipped, at random:** a worn piece is taken; a spell drops one tier (one at tier 1 is
    lost; the Fire Bolt never goes below the fireball). Each worn piece and each spell has the same
    chance. Never a rune, the bag, a Totem ability, or anything in the Repository or Compendium.
  - She signs the page herself, and says so, on a card at her room *(wording to approve; Q16)*:

    > **CAUGHT CHEATING THE DEAD**
    >
    > YOU TRIED TO CHEAT THE DEAD OF THEIR BOUNTY. WHATEVER YOU MEANT BY IT, SHE HAS TAKEN THEIR CLAIM:
    > 1,240 GOLD, AND THE RARE BOOTS OF THE LIGHTFOOT. THE LEDGER IS SIGNED.

- **Fair warning** *(default; Q13)*: while a page is marked, a small red eye shows beside the keys on the
  status bar and in the readout, and *OPEN THE LEDGER* and *FLEE* in the Esc menu carry the line *THE
  DEAD ARE WATCHING*. Nobody should trip the claim without knowing the rule exists; the Codex explains it
  too.
- Loading outside danger costs nothing; going back on a bad purchase is allowed.
- Someone with the registry editor can always copy a page; the claim guards the game's own doors.

## 3. The screensaver and your ledger (needs an answer: Q5)

Today the screensaver plays on from your last saved progress and keeps nothing unless a person takes
over and flees. With death this costly, the autopilot must not be able to lose a person's run while they
are away. Three ways:

- **(a) The dream (recommended).** The screensaver plays on from your last page, and nothing it does is
  kept: its deaths use today's mercy (the level starts again, with the easing), and it never signs or
  dies on your page. **Holding H takes over where it stands**, and from that moment until the program
  closes the run is yours: the new rules apply, deaths are written, the mark watches. Whatever the dream
  gained is yours only if you sign the ledger. Sleepwalking after taking over leaves the run under the
  new rules (otherwise handing over mid-fight would dodge a death).
- **(b) Wake at the merchant's.** Holding H puts you at the merchant's from your last page; the dream is
  let go. The simplest and most predictable, but you can't drop into the action from the screensaver any
  more.
- **(c) One run, one rule.** The autopilot plays your page under the same rules, signs at the merchant's
  as you would, and its deaths cost you.

## 4. Runs of ten

- `crawl_set(level) = ((level - 1) / 10) % 4`, `crawl_step` 0 to 9, decay `step / 9`: slowly across the
  whole run, at its worst on the boss level. Depth climbs run by run and stays at its deepest from run IV.
  Each set's two tints spread over ten levels.
- **After level 40** the four runs come round again (41 to 80), deeper, with the loop moods the descent
  plan proposed (not yet built) and the bosses back stronger; the numerals on the names go on as now
  *(default; Q2)*.
- **Creatures keep their first levels** (2 to 19) *(default; Q3)*, so every kind is met in runs I and II;
  runs III and IV bring no new kinds until they are planned. The Knight at level 10 is "a future
  encounter", as the brief allows: ordinary knights first come at 13.
- **The rank and quality mixes follow the sets** *(default; Q4)*, so they now climb over ten levels:
  levels 6 to 10 get the Crypts' gentle mix where they had the Catacombs', and the Deep's comes at 31 to
  40. Levels 6 to 20 grow gentler; the bosses become the step up. The alternative is to keep the mixes
  on the old five-level steps.
- **Secrets**: levels 1 to 9 of a run hold 3, 3, 4, 4, 4, 5, 5, 5 and 6 links; the boss level none.
  Pockets are the Backrooms in runs I and II and Severance in III and IV, as the two themes followed the
  first two sets and the last two. Black Altars from level 6 and the Wraith from level 9 as now, but
  neither on a boss level.
- **The record and achievements**: each of levels 1 to 40 cleared (from 1 to 20); *RUN I COMPLETE* and
  on, in place of the descents at 40, 60 ...; and a slayer's line for each boss.
- **Level names**: forty, ten a run. Drafts: the twenty we have stay where they fit; the boss levels
  take their boss's name (Q11).
  - **I, the Crypts:** The Ossuary, Hall of Torches, Crypt of the Nameless, The Bone Gallery, The
    Warden's Hall, The Chapter House, The Pilgrims' Stair, The Reliquary, The Knights' Vault, *The
    Ill-Fated Lists*.
  - **II, the Catacombs:** The Drowned Vault, Catacomb of Saints, The Weeping Stair, Cistern of Bones,
    The Abbess Below, The Silted Nave, The Black Canal, The Spinners' Gallery, The Hanging Dark, *The
    Wanderer's Web*.
  - **III, the Hollows:** The Charnel Mine, Warrens of the Dead, The Fossil Road, The Broken Seam, The
    Colossus Deep, The Timbered Dark, The Gnawing Caves, The Lampless Shaft, The Flooded Chapel, *The
    Deacon's Pulpit* (if Q22 picks him).
  - **IV, the Deep:** The Glittering Dark, Veins of the Earth, The Ember Root, The Black Glass, Throne of
    the Hollow King, The Singing Geode, The Cinder Fields, The Molten Stair, The Last Light, *The Bottom
    of Everything* (if Q22 picks the Wraith).

## 5. Bosses

### The boss level (*default*; Q10 asks whether it is the arena alone)

- The merchant's before the boss level as always: the last chance to sign.
- Her east door opens on a short antechamber, then an arch into the arena. Crossing it, the doors grind
  shut behind you, the boss's name card shows with a line of lore, the boss music starts, and its health
  bar appears along the top of the view, with the wave marks as notches.
- Inside: no exit, no secrets, no altar, no Wraith; L casts nothing (*THE WAY IS SHUT*) until it falls.
- When it falls: its scrip drops where it fell (walk over it), the exit opens in the far wall, the tally
  gains a BOSS row with the time, the run is counted complete, and on you go.
- Dying in the arena is a death like any other: the run ends. That is the risk at the end of a run.

### Waves

- At each mark (Q12: 75, 50 and 25 by default), any damage past the mark is lost; the boss stops there.
- It turns untouchable at once: an animation of its own (the knight kneels on its sword, the spider
  cocoons itself on the ceiling), a pale shell over it, and spells pass through with a dull ring.
- It calls, and two seconds later (dust falling, a horn, so you can see where they will come from) the
  adds come in at set places: gates in the walls, burrows, the floor.
- It stays still and untouchable while any add of the wave lives, then rises with a sound, and the music
  goes back to the fight.
- The bar greys while it is untouchable and shows the adds left: *3 REMAIN*.
- Adds give souls and bones, never potions or treasure, so the arena's healing stays as placed.

### Health, healing, music, scrip

- **Health**: set by runs in the harness so that a fight with gear right for the level takes three to five
  minutes, waves included. A starting point: about twenty times a rank-I skeleton knight of the level.
- **Bosses are slowed but never frozen or frightened** (a frozen boss would end the fight's shape); Time
  Slip slows them by half as much as it slows a creature.
- **Healing** *(default)*: three fonts set into the arena's walls behind cover, away from where the adds
  come in. Each holds one draught of a third of your health. One is full when the fight begins; one more
  fills as each of the first two waves is cleared. At most three draughts a fight. The autopilot drinks
  under 40%.
- **Music** *(default; Q20)*: a theme for each boss, OPL2 voices like the rest: a slow off-kilter waltz in
  a minor key over an ostinato, a tritone in the tune, a bell on the first beat. While the adds are out,
  the same theme turned up: drums come in, the bass doubles, the key rises a semitone, the pace lifts a
  little. When the last add falls it settles back. The knight's in D minor with a brassy lead; the
  spider's in B-flat minor, plucked, with a skittering hi-hat.
- **Scrip**: one Named Odious Scrip a kill, e.g. *THE ILL-FATED KNIGHT'S ODIOUS SCRIP*, a curl of
  parchment with a black seal. It goes to the Totem's account the moment it is taken, so it is kept
  through death *(default; Q7)*.

### The Ill-Fated Skeleton Knight (level 10)

- **Look**: the skeleton knight half again as big, quicker on its feet, its plate polished bright with a
  slow glint sliding across it. It sheds plate at each wave, as knights do by health now.
- **Lore** (draft): *HE RODE DOWN TO FETCH THE ORDER'S DEAD, AND NEVER RODE BACK UP.*
- **Blows**: the knight's wind-up and heavy strike, at the boss's strength.
- **The Rare Mirror Rune**: about one missile in seven that strikes it is turned back at you *(default)*,
  never twice within two seconds. The instant it happens its plate flashes silver in a ring and a struck
  bell rings, a sound of its own, unlike your Mirror Veil. The turned shot comes back at half its damage
  and can be dodged. Only missiles: Fire Bolt, Frost Shard, Bone Spear, Hellfire, Seeker Wisps and
  Rapid Sparks. Chain Lightning, Soul Drain and the BFS9000 are not missiles and are never turned.
- **Swift Dodge**: half a second untouchable while it darts two to five cells in a straight line, as far
  as the floor is clear, leaving two or three fading after-images and a whoosh with a clank of plate.
  Three reasons, three cooldowns *(default)*:
  - **closing in**, every **5 s** at most: you are four cells off or more and in its sight; it dashes to
    a cell short of you, ready to strike;
  - **fleeing**, every **9 s**: you have taken a tenth of its health within three seconds, or it stands
    in burning floor; it dashes away from you;
  - **dodging**, every **16 s**: a shot of yours would hit it; it dashes aside.
  - No dodging while a wave holds it.
- **Adds**: skeleton kinds only *(default)*:
  - **wave 1**: a skeleton (the thrower), two shield-bearers and a bone hound;
  - **wave 2**: a skeleton, two shield-bearers and two hounds;
  - **wave 3**: two skeletons, two shield-bearers, a hound and an ordinary skeleton knight.
  - They come through two gates in the north wall and two in the south. Their rank: Q18.
- **The arena, THE LISTS** (draft; frames to the owner before it goes in):

  ```
  ###########G###G###########     @  the way in (shut behind you)   E  the exit, shut till it falls
  #B.......................B#     O  great pillars: they break sight
  #...OO.......=.......OO...#     =  a low tilt rail: walked round, seen and cast over
  F...OO.......=.......OO...F     G  gates the adds come through
  #.........................#     F  fonts, set into the walls
  @.....===....K....===.....E     B  braziers
  #.........................#     K  where the knight stands at first
  #...OO.......=.......OO...#
  #...OO.......=.......OO...#
  #B.......................B#
  ###########G#F#G###########
  ```

### The Wandering Hunting Spider (level 20)

- **Look**: a hunting spider of the usual size, a little quicker, with a pale skull marked on its back
  so it can be told from its brood at a glance.
- **Floor and ceiling**: it hops between them every four to seven seconds, along the ceiling as hunters
  do and down again in a leap.
- **The Rare Mend Rune**: when it has lost a fifth of its health since it last mended, it mends an eighth
  over four seconds, at most every twenty, never above the last wave mark it has passed; a green glow and
  the Mend sound, so it can be seen and burst through *(default)*.
- **Tangled Web**: it rears up for 0.6 s with a hiss, then spits a glob of web, slower than a bone, that
  can be sidestepped. If it lands you are held: you can turn and cast but not walk, and the view's edges
  fill with webbing that tears as it weakens. To get free you must do a third of your health in damage
  to the web. Q19 asks how; the default is that every spell you cast while held strikes the web, and
  fire strikes it twice as hard. One web out at a time; its next comes **12 s** after you are free
  *(default)*.
- **Adds**: smaller hunting spiders, without the web: four, five and six in the three waves, out of
  burrows in the walls. **Every fifteen seconds**, wave or no wave, one more comes out of a burrow
  already hunting you, with at most four of those alive at once *(default)*, so there is always something
  coming.
- **The arena, THE WEB** (draft): a round cistern hall with a high ceiling.

  ```
        ####h#####h####           h  burrows the small spiders come from
      ###......w......###         O  pillars wrapped in web: they break sight
     ##...O.........O...##        w  sheets of web hung from the ceiling (to look at, not to stop sight)
    ##.....w.......w.....##       :  the cistern's well, low: walked round, seen and cast over
    h.....................h       S  where it hangs at first, over the well
    #.....O....:....O.....#       F  fonts
    @.........:S:.........E
    #.....O....:....O.....#
    h.....................h
    ##.....w.......w.....##
     ##...O.........O...##
      ###...F.....F...###
        ####h##F##h####
  ```

### Proposed bosses for levels 30 and 40 (the owner asked for ideas; Q22)

Each takes one of its run's creatures (or one still to come), carries one Rare rune used against you,
and has one ability that becomes the caster's at the Totem. Recommended: **B for 30 and D for 40**,
because with Swift Dodge (moving) and Tangled Web (holding) they make a Totem of four different kinds:
moving, holding, an ally, and a way out.

**Level 30, the Hollows**

- **B. The Black Deacon (recommended).** The deacons' master, twice their size, in a mine chapel ringed
  with bone heaps; black robes, a censer of green smoke, a crozier of bone.
  - **Rare Bone Ward**: a ring of shards turns round him and takes the first blows; it grows back on
    the rune's cooldown, so it has to be broken before he can be hurt.
  - **Grave Call** (his): he chants over a heap and a skeleton gets up from it. A tenth of his health
    in damage mid-chant breaks it off.
  - **Adds**: risen skeletons, shield-bearers and a knight, with one deacon a wave and two in the last.
  - **Arena**: timber pews for low cover, the heaps along the walls, a pulpit at the far end, mine lamps.
  - **Grave Call for the caster** (three ranks): right mouse at the remains of something you slew,
    within four cells: it rises and fights for you for 12, 16 or 20 s, one, two or three at once.
    Creatures turn on it as they do on Blink's double; it strikes whatever is near.
- **A. The Gorging Ghoul.** A ghoul swollen on the mine's dead; it leaps across the arena and feeds on the
  spoil heaps when it loses you. **Rare Soul Reaping**: each add that falls near it gives it health. Adds:
  ghouls and shamblers. **Ravenous Leap for the caster**: right mouse at a creature within 4, 5 or 6
  cells; you leap onto it (an arc, not a teleport) and your next spell within a second strikes double;
  cooldown 14, 10 or 7 s. Close to Swift Dodge in feel.
- **C. The Stone Matriarch.** A cathedral gargoyle in a cavern church, waking the gargoyles on the arena's
  plinths; it turns to stone, untouchable, when struck hard. **Stoneskin for the caster**: 1.5, 2 or 2.5 s
  as stone: nothing hurts you, and you can neither move nor cast; cooldown 30, 24 or 18 s.

**Level 40, the Deep**

- **D. The Wraith Unbound (recommended).** The Wraith that has walked the halls since level 9, waiting at
  the bottom of everything, and here at last it can be hurt, but only in light. Braziers ring the arena
  and it gutters them as it passes; relight one with E, or bring your wisp close; in the dark spells go
  through it.
  - **Rare Time Slip**: your missiles slow to a crawl near it for a moment, a grey ripple and a deep
    chime, and it steps out of their way.
  - **Wraithwalk** (its): through walls and creatures, gone from sight, out of a wall near you.
  - **Adds**: shades (seen only in light) and the Drowned, with two shades in the last wave.
  - **Wraithwalk for the caster** (three ranks): for 1.5, 2 or 2.5 s you pass through creatures and their
    missiles, and they lose you and must find you again; cooldown 25, 18 or 12 s.
- **E. The Cinder Tyrant.** An ember imp grown into a furnace, in a hall cut by magma channels, lobbing
  embers that burn the floor in lines. **Rare Purify**: it shakes off your frost twice as fast. Adds: imps
  from the vents. **Ember Rain for the caster**: right mouse: three embers at the floor up to six cells
  ahead, burning it for 4, 5 or 6 s; cooldown 16, 12 or 9 s.
- **F. The Hollow King.** The descent plan's lich in a geode heart, shielded by crystal pillars you must
  break, blinking between them and raising shades. The biggest art job of the six.

## 6. The merchant's hall

### The room, grown (draft; Q26)

The shop stays as it is, the cat where she lies. A west arch, beside her, opens on a back room; a south
arch, past the shrine, on the grave plot.

```
#############S########      M  the merchant on her chest     p  pedestals (four of seven shown)
##T....#...........###      S  her sign                      $  the Rune Shrine
#R.....A..p.p.p.p..###      D  the door on, into the level   A  arches
#R..L..#...........###      L  the Corpus Compendium, on a lectern
#R.....#M..........D>>      R  the Wretched Repository: iron doors in the wall's niches
#R.....#...........###      T  the Hexed Totem
##.....#.....$.....###      +  gravestones, ten
#################A####      P  the Accursed Portal, in the plot's far wall
#################.####
#########...........##
#########...........##
#########.+.+.+.+.+.##
#########...........##
#########.+.+.+.+.+.##
#########...........##
#########...........##
##############P#######
```

New art, as sheets for the owner first: the lectern and its tome; the Repository's iron niche doors (a
wall texture); the Totem (a pole of skulls hung with the bosses' scrip, a fresh one for each kind
unlocked); the Portal (an arch filled with a slow red-violet swirl, animated like the visportal); the
plot (stones from the graveyard sprites we have, a new one for the freshest, turned earth, a lantern).

### Pedestals

- **Two at first**, the first a spell and the second a thing to wear. More from her menu: the third
  **500** gold, then **1,250**, **3,125**, **7,815**, **19,530** for the seventh. Kept through death.
- The wares-a-visit limit (one to level 9, two to 14, three after) becomes one in run I, two in run II,
  three after *(default; Q28)*.
- Old saves keep the pedestals their level gave them (five to seven), so nobody loses wares by updating
  *(default; Q31)*.

### The Corpus Compendium

- **E at the lectern.** A page for each slot (left and right wrist together, boots, neck, hood, robes,
  ring, bag) and one for the spells; A/D turns the pages, W/S runs down the list, sorted by quality,
  Common to Exquisite, each quality in its colour. The card is the shop's card, with its price.
- **What it keeps** *(default; Q29)*: each piece exactly as it was when you first put it on (its level,
  stats and ability; never anointed: anointing stays the altar's gift). Copies alike in kind, quality,
  ability and stats are kept once. Up to twelve on each slot-and-quality page; when a thirteenth comes,
  the weakest goes. Spells at the highest tier you have learnt each.
- **Price**: what the shop would ask for it, x1.2, less 0.05 for each run completed, never under x0.5
  (14 runs). Buying puts it on as a pedestal's ware does, with the same warnings for what it replaces.
  It doesn't count against the merchant's wares a visit.
- An old save's worn pieces and spells go into it as it loads.

### The Wretched Repository

- **E at the niches.** Its places down one side, your gold and coins on the other.

| | Going in | Coming out |
|---|---|---|
| A piece (worn or from the bag; an empty bag too) | 10% of its shop price in gold; nothing in the untaxed place | free *(default)* |
| Gold | a quarter is taken: put in 1,000, 750 is kept | free *(default)* |
| Cursed Coins | free | 2,500 gold each |

- **Places**: two from the start, one of them untaxed. Ten more to buy, to twelve in all: **2,500**,
  3,750, 5,625, 8,440, 12,655, 18,985, 28,475, 42,715, 64,070 and **96,110** (283,325 for all ten).
- No limit on gold or coins. Nothing in it is lost to death or the claim. Spells don't go in; the
  Compendium has them.

### The Accursed Portal

- **E at the portal** opens its menu:
  - **The runs**: run I (level 1) always; run II (level 11) once run I's boss has fallen; and so on.
    Going through puts you in the merchant's room before that level, free. Offered only in the room before
    level 1, where a death or a fresh start leaves you, so it never takes you out of a run in progress
    *(default; Q24)*.
  - **The trials**: each boss you have beaten, to fight again for its scrip, from any visit. The arena
    alone, as you are; a win gives one scrip and brings you back to the room you left; a death is a death
    *(default; Q25)*. A trial won is not a run completed (Q27).

### The Hexed Totem

- **E at the totem.** A row for each boss beaten; bosses not yet beaten don't show at all. Each row: the
  ability, its rank and cap, the scrip of that boss you hold, and *RAISE: 1 SCRIP*. A card under it
  describes the rank you would get.
- One scrip unlocks rank I; each rank after costs one more *(default for Tangled Web; the brief gives it
  for Swift Dodge)*. Swift Dodge and Tangled Web stop at III; later abilities may stop elsewhere.

## 7. The caster's boss abilities

### Swift Dodge

- **Tap W, A, S or D twice** within a quarter of a second, the second a fresh press: you dart that way as
  far as the floor is clear up to five cells, stopping before walls, doors, props and creatures, over
  about a quarter of a second; you pass through the cells between and can be seen doing it. With less
  than two cells clear nothing happens and no cooldown is spent.
- **Cooldown** 15, 7 and 3 s by rank.
- **Untouchable while it moves**, as the knight's is *(default; Q32)*, so a dash well timed goes through
  a bone.
- A double tap can go off by accident while walking. If it does in play, a setting can turn it to a
  one-press form instead (Q33).

### Tangled Web

- **Right mouse** spits a glob of web straight ahead, at the creature your spells would aim at, eight
  cells a second, eight cells' reach.
- **On a creature**: it is held where it stands and must claw at the web. Its blows land on the web at
  half its usual rate and at its usual strength; the web holds a third of your health (rank I), two
  thirds (II) or all of it (III). Held, it cannot strike or throw at you. Your spells pass through your
  own webs and hit the creature. Once free it hunts you at once.
- **What can be held** *(default)*: anything that walks or flies and is solid; not spectres, shades,
  wall skulls or egg sacs; **not bosses** (it falls away from them with a tearing sound) (Q34).
- **Webs at once**: one, two or three by rank. A web holding nothing frees its place.
- **Cooldown** *(default)*: 12, 9 or 6 s by rank after a hit; 2 s after a miss.

### The ability socket (Q35)

![Where the ability socket could go](plan-b7/ability_socket.png)

From the left: **A** with the bar up, **A** with the bar away, **B** in the top corner.

- **A (recommended): bottom left.** With the status bar up, the sockets sit on its top edge over LEVEL
  and GOLD; when the bar slides away they go down to the corner, and the readout pops up beside them.
  The hand comes up on the right, the toasts on the upper right and the captions in the middle, so the
  lower left is clear, and the eye is near it already for the health.
- **B: top left.** Always in the same place and never moving, but far from where the eye is in a fight.
- **Each socket** shows its ability's mark, a line along its foot that fills as it recovers and brightens
  when ready (with a soft tick *(default)*), and, for Tangled Web, a pip for each web it may hold, lit while
  one holds something. Swift Dodge has a socket too, for its cooldown, though it has no button.
- **Only abilities unlocked** have a socket; none show before the first boss falls.
- **More than one right-mouse ability** (from level 30): the right mouse uses the readied one, outlined;
  a tap of Q (free in the crypt) readies the next *(default)*.

## 8. The Codex of Affixations (C)

- A book like the bestiary, opened with **C** in the crypt and at the merchant's; the crypt waits while
  it is open.
- **Pages**: the gear's abilities (Swiftness, Arcane Knowledge, Treasure Hunter, Keymaker, Commune,
  Thorns, Mirrored); the runes, and each one's Exquisite effect; the bosses' abilities; and the sixteen
  stats a piece can roll, with their caps *(default; Q36)*; and a page on the merchant's claim.
- **Each entry**: what it does in plain words, its numbers at each quality or rank, where it comes from
  (*ON HOODS*, *FROM THE RUNE SHRINE*, *THE HEXED TOTEM, FOR THE ILL-FATED KNIGHT'S SCRIP*), and what you
  have of it now.
- **Known** once you have worn, held or unlocked it: an ability once you have worn a piece with it; a
  rune once held, and its Exquisite effect once held at Exquisite; a boss ability once its boss has
  fallen. Unknown entries don't show at all. Kept through death, with the save.
- C is a new key for the game, the Windows shell and the harness.

## 9. The bestiary's bosses

After the creatures and before the achievements, a page for each boss met: its name and lore, its
pictures (from the front, and from the side, turned about as the creatures are), its rune and ability,
what its waves bring, times slain, tries, its best time, scrip won. Bosses not yet met don't show.

## 10. Saves: version 6

- A page grows from about 1 KB to about 12 KB (`SLOT_MAX` from 2,048): the run as now, the pedestals,
  the Compendium, the Repository, the Totem and scrip, the runs opened and completed, the Codex, the ten
  stones, and a count of signatures for the mark.
- Beside the pages: `Mark` (the page and the signature it belongs to) and `Progress`, which now names the
  page to start from.
- Versions 1 to 5 still load. 1.0.0b6's graves become ledger pages as they are; its last fled progress
  stays as the unsigned run the game starts from, until it is signed onto a page of your choice. An old
  save's hall comes empty but for its pedestals and what it wears and knows, which go into the
  Compendium.

## 11. The autopilot

- **Bosses**: it keeps three to five cells off, moves between the pillars, turns on the adds while the
  boss is untouchable, drinks at a font under 40%, sidesteps the web's glob and burns the web off with
  fire, and against the knight prefers Chain Lightning when it has it, since that is never turned back.
  It can't win every time; in the dream the easing after deaths still applies, so it gets through in the
  end.
- **Its abilities**: if the page has them, Swift Dodge out of a missile's path, Tangled Web on the
  nearest creature that comes to blows.
- **The hall**: it buys pedestals when it can afford one and keep a level's haul after; it buys from the
  Compendium when a page there beats the pedestals; it leaves the Repository, the Portal and the Totem to
  people *(default)*.

## Building it (the tasks)

Each step is tested in the harness before the next; art goes to the owner as sheets first.

1. **Runs of ten**: the sets, steps, decay and tints over ten levels; forty names; secrets by step; the
   rank and quality mixes; the themes by run; no Wraith, altar or secrets on boss levels; the record's
   levels and runs.
2. **Boss machinery**: arenas from planned maps; the boss level (antechamber, doors shut, name card,
   top bar); waves (marks, untouchable, adds at set places, rising again); fonts; scrip; the run
   completed; the tally's BOSS row; the bestiary's bosses.
3. **Boss music**: the knight's theme and the spider's, each with its wave section.
4. **The Ill-Fated Skeleton Knight**: art sheet; the Mirror Rune's turned missiles, flash and bell; Swift
   Dodge's three reasons; its adds; THE LISTS; the autopilot's tactics.
5. **The Wandering Hunting Spider**: art sheet; floor and ceiling; the web on the caster and getting
   free; the Mend Rune; the small spiders and the fifteen-second spiders; THE WEB; the autopilot.
6. **The Hexed Totem and the caster's abilities**: the totem; Swift Dodge; Tangled Web; the sockets.
7. **The Accursed Portal**: run starts and trials.
8. **Saves version 6 and the ledger**: bigger pages, signing with the merchant, opening from Esc,
   fleeing, the screensaver's dream (Q5), old saves.
9. **Death's cost**: what goes and stays, written at the fall; the death card; the grave plot, its
   stones and what killed you.
10. **The claim**: danger and the mark, written at once; what she takes; her card; the warning eye.
11. **The hall**: the room's new shape and art; her menu; pedestals two to seven.
12. **The Corpus Compendium.**
13. **The Wretched Repository.**
14. **The Codex of Affixations.**
15. **Checks**: frames of everything new; ASan; autopilot runs (boss wins and times, how far the dream
    gets); the README's "What has been verified", the page and release notes at the release.
16. **Levels 30 and 40**, once chosen (Q22): a later beta.

Proposed releases (Q37): **1.0.0b7** with steps 1 to 7 (runs of ten, the two bosses, the Totem and its
abilities, the Portal), while death still only restarts the level, so the fights can be tuned without
sending anyone back to level 1 for a fight that is still being tuned; then **1.0.0b8** with steps 8 to 14
(the ledger, death's cost, the claim, the hall, the Compendium, the Repository, the Codex); then the
bosses of 30 and 40.

## Risks worth naming

- **The autopilot against bosses.** If it can't win, the screensaver would sit on level 10. The dream's
  easing after deaths keeps it moving; the runs will show how often it wins first time.
- **Death is now expensive for a person** while the Repository, Compendium and Totem are what soften
  it; the order of the betas keeps the fights tunable before the cost comes in.
- **Gentler levels 6 to 20**, from the stretched mixes, until the bosses; the runs will show it.
- **The claim catches crashes too**, by the brief's rule. If that proves unkind on real Windows, a crash
  can be told from a deliberate close only roughly (the window closing cleanly or not).
- **Writing the mark** a few times a minute: one small registry value, written from the main loop like
  the pages, so a frame never waits on it.
- **The biggest change since the items**: saves, the merchant's room, death, the level flow and a new kind
  of fight. It goes in steps, each tested on its own.

## Questions for the owner

**The shape of it (needs an answer)**

- **Q1.** Runs and sets: (a) each run is one set, stretched over its ten levels (I the Crypts 1-10, II the
  Catacombs 11-20, III the Hollows 21-30, IV the Deep 31-40), as read here; (b) each run passes through
  all four sets in its ten levels.
- **Q5.** The screensaver and your ledger: (a) the dream (recommended); (b) wake at the merchant's; (c) one
  run, one rule. See section 3.
- **Q6.** A death in a pocket: (a) keep the throw-back to the chain's room, whole (recommended); (b) a death
  like any other.
- **Q10.** The boss level: (a) the arena alone, after a short antechamber from the merchant's
  (recommended); (b) an ordinary level with the arena at its end.
- **Q12.** The waves: (a) at 75%, 50% and 25%, each mark holding (recommended, three waves); (b) at 75%,
  50% and 10%; (c) four, at 75, 50, 25 and 10.
- **Q19.** Getting free of the spider's web: (a) every spell cast while held strikes the web, fire twice as
  hard (recommended); (b) E tears at it; (c) either.
- **Q22.** Bosses for 30 and 40: B (the Black Deacon, Grave Call) and D (the Wraith Unbound, Wraithwalk)
  are recommended; or A, C, E or F, or your own.
- **Q24.** The Portal's run starts: (a) offered only in the room before level 1 (recommended); (b) at any
  visit to the merchant.
- **Q30.** The Repository: two free places, one of them untaxed, and twelve places in all, as read here?
- **Q37.** Releases: (a) bosses first (1.0.0b7), then death's cost and the hall (1.0.0b8) (recommended);
  (b) the other way round; (c) all at once.

**Defaults taken unless you say otherwise**

- **Q2.** After level 40, round again deeper with the bosses stronger; or end the descent at 40.
- **Q3.** Creatures keep their first levels (2 to 19), or spread over forty.
- **Q4.** Rank and quality mixes stretch with the runs, or stay on five-level steps.
- **Q7.** The record (bestiary, achievements) and unspent scrip are kept through death.
- **Q8.** The save slots become the merchant's ledger, pages I to III.
- **Q9.** Signing is an explicit row in her menu, not every word with her.
- **Q11.** The forty level names (section 4).
- **Q13.** Danger as listed in section 2, clearing 8 s after; with a red eye and a line in the Esc menu
  while the dead are watching.
- **Q14.** A crash or power cut while marked counts, as the brief's "regardless of intention" says.
- **Q15.** She takes a quarter to a half of the gold at random (or always half; or more the nearer you were
  to death).
- **Q16.** The wording of her card.
- **Q17.** The ware held at the merchant's is lost on death.
- **Q18.** The adds' rank: the highest of each kind met since your last death or Portal start (or the
  highest ever met, which could bring black skeletons to level 10 after a deep run).
- **Q20.** A theme for each boss, rather than one for all.
- **Q21.** The numbers: the knight's cooldowns 5, 9 and 16 s, one missile in seven turned back at half
  damage; the spider's web every 12 s after you are free, at most four fifteen-second spiders; three
  fonts of a third of your health; Tangled Web's cooldowns 12, 9 and 6 s. All to be tuned in the runs.
- **Q23.** Bosses are slowed but never frozen or frightened.
- **Q25.** A trial is fought as you are, and a death there is a death.
- **Q26.** The hall's layout as drawn.
- **Q27.** Only beating a run's boss on the way down counts as a run completed; trials don't.
- **Q28.** Wares a visit: one in run I, two in run II, three after (or buy every ware you can afford).
- **Q29.** The Compendium keeps each piece exactly as first worn, twelve to a page (or every piece ever
  worn, or only the kind, quality and ability, rolled fresh at the current level when bought).
- **Q31.** Old saves keep their five to seven pedestals.
- **Q32.** Swift Dodge is untouchable while it moves.
- **Q33.** A double tap only, unless accidental dashes show up in play.
- **Q34.** Tangled Web can't hold a boss.
- **Q35.** The socket at the bottom left (A).
- **Q36.** The Codex includes the sixteen stats as well as the abilities, and a page on the claim.
