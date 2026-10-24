# 1.0.0b6: secrets that go deeper, Cursed Coins and the Rune Shrine

**Status: planned (24 October 2026) and settled; steps 1 to 4 are built (the hand fix, runes by
quality, the Rune Shrine, and the secret areas: secrets by set and in chains, bigger rooms, doorways,
the pockets, and the altar's wisp to one secret and back for a price), tested in the harness only. The
Backrooms (step 5) are built: the look, the tubes lit, failing and dead, things out of place, creatures
caught in a loop, the glitch, the silence, echo and hum, and the visportal's gift. Severance (step 6)
is built too: white halls and offices, the ward doors, the panels, the water cooler's drink, the
creatures lit brighter, the muzak. Step 7 is built: the coins, creatures in the pockets, the autopilot
in chains. The owner approved Blink's double, the shrine's art, the Backrooms and Severance as drawn;
step 8, the checks and the beta, is next.** The owner
asked for the things below after 1.0.0b5, answered nine rounds of questions, and left the rest to
defaults, marked *default* here so they can be changed. Nothing in this plan names the console's codes.

## What the owner asked for

1. **The hand.** Since the status bar now tucks away, the hand needn't jump up a second time when it
   appears to cast; it does need to while the bar is up, or the robes and bracer can't be seen.
2. **The Black Altar's wisp** goes from the altar to a secret and back to the altar, not from secret to
   secret. It waits at the secret's entrance while you are inside; once you are out you interact with
   it (not before, so that pressing E near the wall can't set it off by accident), and a further trip
   costs more: 1.5 times as much for each secret revealed, rolling within the level.
3. **Secrets, more of them and deeper.** More loot, more space, their own look. No enemies unless the
   secret is chained; the second step of a chain and beyond hold stronger enemies. Three to six secret
   rooms a level instead of one to three, to allow for chains, several chains, or one very long one.
   This is where challenge lives; the deeper into a set, the more a secret should feel like a level of
   its own.
4. **Two themes.** The Backrooms for the first two sets, Severance for the last two, each with its own
   wall textures, decorations, light and sound, and its own reward.
5. **Cursed Coins**, found deep in chains, and a reward for finding every secret on a level.
6. **The Rune Shrine** in the merchant's room, cursed awake with a coin, upgraded and pulled with coins:
   the only way to get runes from now on.

## Decided (24 October 2026)

- **The hand:** with the bar away, the hand rises once to its resting height and the spell leaves from
  there; with the bar up it rises clear of it as before. *Built: see below.*
- **Where secrets live: both.** A secret's first room is carved into the level, bigger than today's
  closet; the deeper links of a chain are pocket areas, separate places you go into and come back from.
- **The altar's wisp:** E at the wisp once you are out of the secret area sends it home to the altar;
  sending it to the next secret costs more each time (see the design).
- **Runes:** quality replaces tier. You hold at most one of each of the nine runes, the best you have,
  and every rune you hold is worn and fires on its own. Their strength is to be reconsidered so that an
  Exquisite rune, a long and costly road, is worth it: **at Exquisite each rune gains a second effect**,
  proposed below for approval.
- **How many secrets:** within each set, 3 secret rooms on its first level rising to 6 on its fifth,
  counting every link of a chain.
- **Cursed Coins:** one at the deepest reward of each chain (the visportal or the water cooler), and one
  more for finding every secret on the level. A lone secret with no chain gives none.
- **Pulls:** 1 to 3 coins a pull. The coins set how many runes show (1 coin 1-2, 2 coins 2-3, 3 coins
  3-5); the shrine's rank sets the best quality that can show, and lower qualities are likelier.
- **The nine slots** belong to the shrine and are kept between visits and in saves. Each pull earns one
  take: when you leave, you choose a rune to carry for each take that visit; the rest stay in the slots
  to combine later.
- **Attunement:** a kind of rune is attuned once you hold it at Rare or better (pulled so, or combined
  up). On a pull you may offer 1 to 3 more coins for one attuned kind; each coin makes it a third likelier
  than it was (three coins: about 2.4 times).
- **The autopilot plays the shrine too:** it goes into secret chains while it has time and health, takes
  the coins it finds, curses and upgrades the shrine, and pulls, keeping the best rune each time.
- **The water cooler:** one drink a level: full health if you are hurt, otherwise 50 souls.
- **New creatures for the deep links: later.** For now the deeper links hold the existing creatures a
  rank or two higher, with each theme's look and boosts. Creatures of their own come in a later beta,
  planned with the owner first.
- **Release:** the hand fix waits on the branch and goes out with the rest as 1.0.0b6.
- **Severance's sound:** bright FM muzak, slightly too calm, over a fluorescent hum.
- **The merchant's rune pedestal** becomes another piece to wear.
- **Severance (24 October 2026, ninth round):** the look goes in as drawn; narrow halls with an office
  in about one block of five; the creatures as shown, brighter; the muzak kept but quieter (it now sits
  about 9 dB under the crypt's march).
- **The Backrooms (24 October 2026, eighth round):** the look goes in as drawn (the sheet and the
  frames); the glitch and the things out of place as shown; the faint hum stays under the silence; at a
  chain's end the hoard stays beside the visportal, with the chain's coin to come.
- **The shrine (24 October 2026, sixth round):** it stands against the south wall, across from the
  wares; takes not used are lost when you leave the room, with a warning first; three alike combine
  past the shrine's rank, up to Exquisite; in a pull each quality is two thirds as likely as the one
  below it (at Rare: Common 47%, Uncommon 32%, Rare 21%).

## The design

### The hand (built)

`human_cast`: the cast's top is `lerp(rest, 1, G.bar)`, so with the bar away the cast stays at the
resting height (the hand has risen there once, over 0.08 s), and as the bar slides it follows. Reaching
for a door, a wall or a ware is unchanged. Harness frames show both cases.

### The altar's wisp

- Woken (10 spirit, as now), the violet wisp flies to the nearest secret still shut and hovers at its
  entrance (the cell in front of the cracked wall).
- It stays there while you are inside the secret area, the carved room and any pocket beyond it.
- Once you have stepped back out past the doorway, E facing the wisp sends it home to the altar. While
  you are inside, or until you have left, E does nothing to it, so a press meant for the wall or a chest
  can't send it off.
- At the altar, E offers *Send the wisp to the next secret*, at **10 souls and 5 more for each secret
  revealed on the level so far** (the owner's choice): 15 after one, 20 after two, 25 after three. Waking
  stays 10. The blessing and the anointing are unchanged.
- The altar burns gold once every secret on the level is found, every link of every chain included
  *(default)*: anointing is the prize for clearing a level's secrets.
- The autopilot sends it on only if the price leaves it 15 spirit for its spells *(default)*.

### Secret areas

- **How many:** a set's levels have 3, 4, 4, 5 and 6 secret rooms *(default)*, counting every link.
  They are split into chains at random: a level with 5 might have a chain of 3 and two singles, or one
  chain of 5. Chains of 3 and more grow likelier deeper into a set.
- **The first room** of every secret is carved into the level behind its cracked wall, as today, but
  bigger: up to two plan blocks (3 by 7 or 7 by 7 cells) where the level has the room, else a 3 by 3
  closet *(default)*. It holds more loot than today: two to four treasures and often a potion. No
  creatures, ever.
- **A chain's next link** is a door at the back of the first room: a doorway in the theme's look.
  Through it is a **pocket area**, its own map, built and lit in the theme. Pockets grow with the depth
  of the chain and the set: a few rooms (about 13 by 13 cells) early in the Crypts, up to the size of a
  whole level (45 by 45) at the end of a long chain in the Deep (the owner's choice). The level waits, as it is, while you are in a pocket; coming back through
  the doorway puts you in the first room again. A pocket may hold the door to the next pocket, so a
  chain of 4 is the carved room and three pockets, each a little bigger and harder than the last.
- **Enemies** are only in pockets: the level's own kinds, at one rank higher per link past the first
  *(default)*, capped at black, with the theme's boosts on top.
- **The deepest link of a chain** holds its reward: the Backrooms' visportal or Severance's water cooler,
  with the chain's Cursed Coin.
- **Tally and record:** every link counts as a secret in the tally, the record and the achievements.

### The Backrooms (secrets on levels 1 to 10)

- **Look.** The same yellow wallpaper on every wall, damp mono-yellow carpet, white ceiling tiles with
  inset fluorescent tubes. The tubes are the lights: most steady, some flickering, some dead, so some
  stretches are bright and others nearly dark. Empty, repetitive rooms and long corridors. All of it can
  be drawn from the palette's existing gold and bone ramps, recoloured inside the pocket.
- **Things out of place.** Props from the crypt hung upside down or floating; creatures caught in a
  loop on one spot (the level's own kinds, glitching, harmless and untouchable); a few stuck half into
  the walls, the floor or the ceiling.
- **Creatures that fight** look glitched (scanline tearing and colour slips on their sprites) and have
  25% more health and damage; a few more of them than a crypt room would hold.
- **Sound.** No music. A slight reverb on every sound the caster and the creatures make, and a faint
  mains hum from the tubes *(default)*.
- **The reward**, at the deepest link: an open doorway onto an endless run of doorways, a visportal
  effect. You can't walk through it; E at it restores your health in full (or gives 50 souls if you are
  already whole) and gives the chain's Cursed Coin.

### Severance (secrets on levels 11 to 20)

- **Look.** White walls, fluorescent panels across every ceiling, plain office carpet, half-height
  cubicle partitions, long empty corridors and corners, and doors more fit for a treatment facility than
  an office (the doorways between pockets). Drawn from the bone and glow ramps, recoloured.
- **Creatures** wear a brighter palette and have 30% more health and damage, and strike or throw a
  little more often (their waits between attacks a fifth shorter) *(default)*.
- **Sound.** Bright FM muzak over a fluorescent hum, with the same slight reverb.
- **The reward**, at the deepest link: one water cooler, the only one in the chain. One drink a level:
  full health if hurt, otherwise 50 souls, and the chain's Cursed Coin with the first drink.

### Cursed Coins

- One at the deepest reward of each chain, one more for finding every secret on the level.
- Shown on the status bar beside the keys, and on the caster's page; kept in saves.
- Spent only at the Rune Shrine.

### Runes by quality

- Each of the nine runes is held at one quality at most, the best you have; every rune held is worn.
- **Strength** *(default, to be tuned with the runs)*: Common about today's tier 2, Uncommon 5, Rare 8,
  Ultra-Rare 12, Exquisite about 18, with the cooldowns that go with those tiers.
- **At Exquisite, a second effect** (approved by the owner):
  - **Bone Ward:** when it breaks, it bursts into shards that hurt everything within two cells.
  - **Blink:** you leave a fading double where you stood, and creatures go for it for three seconds.
  - **Mirror Veil:** what it turns back flies home to whoever threw it, and hits twice as hard.
  - **Mend:** healing past full becomes a small ward.
  - **Purify:** while it lasts, you also mend a little each second.
  - **Frost Nova:** what it freezes takes double damage while frozen.
  - **Dread:** it reaches every creature, not only the dead.
  - **Time Slip:** what it slows also takes a quarter more damage.
  - **Soul Reaping:** each kill while it lasts also restores a little health.
- **Saves from 1.0.0b5 and before:** runes come across by tier: 1-3 Common, 4-6 Uncommon, 7-10 Rare,
  11 and up Ultra-Rare; none becomes Exquisite *(default)*. Save version 5.

### The Rune Shrine

- **Where:** in the merchant's room, against the left wall, with candles about it so it can be seen.
- **Sanctified** at first: dark, cold, unusable. E at it offers *Curse the shrine, one Cursed Coin*.
  Cursed, it lights, and a single grey rune circles it. The circling runes show its rank: their number
  and colour (grey, then green, blue, purple, gold) say what it is attuned to and the best quality it
  can give.
- **Its ranks** are the qualities: cursed it gives Common; 6 coins raise it to Uncommon, 12 to Rare, 24
  to Ultra-Rare, 32 to Exquisite.
- **Its own screen** (E at the shrine): the nine slots in a 3 by 3 grid, your coins, the shrine's rank,
  the runes you hold, and three actions: *Pull* (choose 1 to 3 coins, and optionally 1 to 3 more for an
  attuned kind), *Upgrade the shrine*, and *Leave*.
- **A pull** shows its runes appearing in empty slots. Any three of the same kind and quality in the
  slots combine into one of the next quality at once, with a flash, and that can chain upwards.
- **Before a pull** that could need more empty slots than there are, the shrine says so and offers to
  let go of runes from the slots first, chosen by you.
- **Leaving:** for each pull made this visit, you choose one rune from the slots to take. Taking one
  of a kind you already hold keeps the better of the two. The rest stay in the slots for next time.
- **The shop's rune pedestal** becomes another piece to wear.
- **Pockets (24 October 2026, seventh round):** you go through a chain's doorway by walking into it,
  as into the exit; dying in a pocket throws you back to the chain's first room, by its doorway, at full
  health, and the pocket behind you resets; the level's clock waits while you are in a pocket (so the
  Wraith can't come mid-chain), and pockets keep their own.
- **As built** (step 3): the shrine is a solid stele on two steps at (15.5, 17.5) with candles either
  side; sanctified it is pale with a gilded disc, cursed it is sooty with its broken disc and turned
  cross glowing in its rank's colour (white, green, blue, purple, gold), and the runes it is attuned to
  go round it (one plain mark before any). E facing it opens its screen: the slots on the left; PULL
  (A/D: 1-3 coins), FAVOUR (A/D: none, or an attuned kind with 1-3 coins), RAISE (Enter twice), TAKE A
  RUNE, LET GO OF ONE (Enter twice), LEAVE on the right, with a note under them; the runes you hold
  along the bottom. Taking a rune no better than yours asks for a second Enter. A pull that could bring
  more runes than there are empty slots asks first (let go of some, pull anyway, not now). A kind is
  attuned when a Rare or better comes into the slots or into your hands, or when you open the screen
  holding one.

### The autopilot

- **Secrets:** it presses every secret it passes while it has time, goes through a chain's doors while
  its health is above two thirds and there are more than 90 seconds before it means to leave *(default)*,
  and comes back out when either fails.
- **The altar's wisp:** it follows it as now, and calls it home and sends it on as the price allows.
- **The shrine:** after the wares it walks to the shrine if it has business there: it curses the shrine
  with its first coin, raises it whenever it can afford the next rank, and otherwise pulls with three
  coins while it holds fewer than three runes, at the top rank, or once it has half the next rank's
  price and three more put by (so that it still saves up); two pulls a visit at most. Before a pull it
  lets go of the runes it has least use for; after, it takes the one that does most for it *(default)*.

### Saves: version 5

Adds the coins, the runes by quality, the shrine's rank, its nine slots and its attunements.
Versions 1 to 4 still load.

## Still to settle with the owner

The Exquisite effects, the altar's price and the sizes are settled (24 October 2026). What remains is
the art: sheets of the new textures and sprites go to the owner before they go into the game, as the
creatures did:
Nothing: the Backrooms' art (the wallpaper, carpet, tiles and tubes, the doorway, the visportal, the
glitch), Severance's (the walls, carpet, panels, ward doors, desks and water cooler) and the shrine in its
two states are approved.

## Building it

1. **The hand** (built).
2. **Runes by quality** (built): the data, the strengths, the Exquisite effects, the old saves'
   runes, save version 5, the merchant's pedestal, the autopilot's sense of runes. Defaults taken
   while building it: Blink reaches two to six cells by quality; the double draws creatures within six
   cells of it for three seconds, and they don't strike while drawn; the Mirror Veil's Exquisite
   return is three times the missile (the veil's 1.5, doubled); Mend's ward holds up to a quarter of
   your health for 8 s; Purify mends 2% of your health a second; Soul Reaping heals a twentieth of it a
   kill; a burst ward's shards deal half the ward's strength to everything within two cells in sight.
3. **The Rune Shrine** (built): its art (approved), its place in the room, curse and ranks, its screen,
   pulls, combining, attunement and offerings, takes when leaving, the overflow question, and the
   autopilot at the shrine. Until the Cursed Coins come in step 7 there are none to find; `CRAWL_COINS`
   and `CRAWL_SHRINE` stand in for testing.
4. **Secret areas** (built): counts by set, the bigger first room, chains and the pocket machinery
   (going in and out, the level waiting), and the altar's new wisp and price. The autopilot calls the
   wisp home as it passes once the wall has moved, without going in first; it sends it on seldom, since
   it comes back past the altar in time only now and then (two runs in eight from level 7). It doesn't
   go through a chain's doors yet (step 7).
5. **The Backrooms** (built; art approved): textures and props, the tube lights (lit, failing, dead),
   the things out of place and the creatures caught in a loop, the glitch, the silence with its echo and
   hum, and the visportal (full health, or fifty souls if whole, once a chain).
6. **Severance** (built; art approved): textures and props, narrow white halls between a few
   offices of four desks, the ward doors, every panel lit, the water cooler (one drink: full health or
   fifty souls), creatures drawn brighter, the muzak (an electric piano, a bass, a vibraphone, eight
   bars of major sevenths) over the hum. The creatures' extra strength and haste come with the
   creatures in pockets, in step 7.
7. **Coins and challenge** (built): a coin at each chain's end and one for every secret found, each
   once a level whatever deaths come between; the creatures in pockets (the level's kinds, a rank up a
   link past the first, the theme's boost; a quarter more of them in the Backrooms, 16 at most); the
   autopilot in chains, by the rules above, within 150 s of chains a level *(default)*. In twelve
   15-minute autopilot runs (six from level 4, six from level 13 with `CRAWL_GEAR=3`) it went into a
   chain 12 times, came back out hurt most times, and reached the end twice, both in Severance; trying
   80% in and 50% out instead of two thirds changed little, so the plan's rule stays. The pockets'
   creatures are where the challenge is, for it as for a person. The desks stand in an office's corner,
   off the lanes the autopilot walks.
8. **Checks and the beta:** frames, eight-seed runs against 1.0.0b5, ASan, the README, the page,
   previews and release notes; 1.0.0b6 when the owner says.

Risks worth naming:

- **The autopilot may struggle.** Runes only from the shrine, and coins only deep in dangerous chains,
  could leave it with fewer runes than it buys today, so it may die more until it has them. The runs
  will show it, and the shrine's prices or the coins can be tuned.
- **A big change to the code.** The pocket areas mean the level waiting while a second map is played,
  which touches the generator, the renderer and the saves. It is built and tested on its own before the
  themes go on top.
