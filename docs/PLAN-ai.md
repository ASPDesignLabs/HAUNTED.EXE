# How the dead think: packs, hunters and ambushes

**Status: a draft for the owner, first round (1 November 2026). Nothing is built.** Written on
`claude/laughing-cori-a3jk29` from 1.0.0b13; brought onto its work branch, `claude/youthful-mayer-deaa11`, from main
once the art pass so far had been merged there (1 November). The art pass is visual only, and the numbers below were
checked again on main: none has moved. The owner asked for creatures
that are a little cleverer and more alive: quicker to answer what a person does, like kinds working in packs, some
that hunt, others that lead a person into an ambush laid by their own kind or another, rules for the generator to
place all this, and a difficulty curve that allows for the dead thinking harder. This plan sets out what the
creatures do today (from `scene_crawl.c` as it stands at 1.0.0b13, and from the harness), where that rubs, and
offers concepts that fit the game as it is: the hunt field, the ranks, the room types, the runs of ten, the dream.

Questions are asked where they arise, with letters and numbers as in the other plans; the recommended answer is
marked \*. They are gathered at the end so the owner can answer them in one go, as "all recommended but S2 (b),
P6 (c)" if that suits. Read it with `CLAUDE.md`, [`PLAN-bestiary.md`](PLAN-bestiary.md) (the kinds, the ranks and
the rules every creature keeps), [`PLAN-b10.md`](PLAN-b10.md) (room types), [`PLAN-b13.md`](PLAN-b13.md) (the
hazards; the lower storey, which this work has to agree with) and [`InitialArtPass.md`](InitialArtPass.md) (the
creatures redrawn, creature by creature; any art this plan asks for is drawn in its hand).

## What the owner asked for (1 November 2026)

- Smarter creatures, **more reactive to the player**, so the crypt feels alive as a person tries different
  approaches.
- **Like kinds in packs.**
- Some that **hunt**.
- Others that **lead the player into an ambush**, of like or unlike kinds.
- Since spawns are generated: **rules** for placing all this, and a **difficulty curve** that allows for creatures
  that respond more intelligently.
- A little more challenge, without the game turning unfair.

## What the dead do today

Every number here is from the code at 1.0.0b13.

**Seeing.** A creature sees the caster within nine cells if the line between them is clear (`sees` in
`update_mob`): all the way round, in light or dark alike. Nothing has a facing but the shield-bearer's shield and
the wall skull, which wakes only when the caster meets its eye.

**Hearing.** Three noises exist, all heard as hunt-field steps (`alert_near`): a footstep, three steps (less with
Quiet), for 4 s; a spell, eight steps, for 8 s; nothing else. A death, a door, a secret wall grinding open, a wall
skull's scream, a gargoyle shaking itself loose: none of these is heard by anything.

**Knowing.** Seen or hurt, a creature is *alert* for 7 s (a skeleton) or 12 s (the rest), counted down once it has
lost sight. While alert it follows the hunt field (`build_hunt`), which runs to the caster's cell as it is now:
round corners, through doors, wherever the caster has gone since. An alerted creature therefore knows exactly where
you are until its clock runs out, and then forgets entirely. There is no "last seen there", no searching, no going
back to where it was.

**Moving.** Cell to cell. Hunting, down the field; in sight, straight at you, stopping at its distance (melee
1.05 cells, throwers 2.2, imps and deacons 4, backing away under 2.8). Otherwise a random step at every cell, so the
wanderers drift like smoke; a resident keeps to its room (`Mob.home`) until it has seen you. In a one-cell hall they
queue behind each other (`occupied`).

**Together.** Never. Packs exist only as they are spawned (`spawn_wanderer_type`: hounds in pairs, a knight with two
hounds, a deacon with two or three skeletons, a brood mother with sacs); after that each walks alone and knows
only what it has seen. At most **three** hunt at once, the nearest by the field (`pick_hunters`, `MAX_HUNTERS`); a
fourth that is alert but not chosen wanders as if it weren't. Spiderlings and a boss's fight are exempt.

**Each kind's way.** Skeletons stand and throw, leading the caster a little. Shield-bearers turn the shield to you
and strike. Hounds dart in, bite and dart off (0.8 s of fear). The hunting spider, the ghoul and the gargoyle leap.
The buried, the web spider, the hunting spider on the ceiling, the wall skull and the gargoyle wait to be walked
into. The shade can't be seen in the dark. Imps lob embers from range. The deacon raises skeletons from remains. The
gilded run at half health. The Wraith walks the field, once a level after four minutes. Nothing else retreats,
takes cover, avoids a spell in flight or a hazard, or answers what spell, light or pace you use.

**How many.** A level opens with its wall skulls, web spiders, vault guards, the buried, imps and gargoyles; three
wanderers (four from level 4) placed 7-40 steps from the start out of sight; a newcomer on its first level;
residents in about 45% of the ossuaries and crypt rooms; a brood mother on six levels in ten from 14. A sample from
the harness (seed 5, the level's opening count): 9 creatures on level 3, 18 on 8, 19 on 13, 23 on 18, 21 on 23,
24 on 28, 22 on 33, 25 on 38, about half of them things that never move until woken. Then the **calm**: after 11 s
without a fight in the dream (35 s for a person) a wanderer is put 4-8 steps away, out of sight, while fewer than
eight mobile creatures live (`G.calm`). On a corrupted level two in five more wanderers.

**Strength.** Health 1.04x a level times the rank's; a blow (4 + 0.8 x level) times the rank's (a quarter more in the
Backrooms' pockets, three tenths more in Severance's), and 15% less for each death on the level (`G.retries`, to 45%); throws 5% quicker a
level; the rank mix (`pick_rank`) climbing over each run's ten levels. Every kind has met its debut by level 19, so
**runs III and IV bring no new kinds**: what is new there is ranks and the run's look.

**The halls.** The level's regions are joined mostly as a tree, with about one extra wall in twelve opened as a loop
(`gen_level`), and dead ends filled in (`mz_fill_dead_ends`). Doors shut by themselves 2.5 s after the last thing
passes, and bats and hounds can't open them.

## Where it rubs

1. **They know too much and remember too little.** While alert they track you through walls; a moment later they
   have forgotten you. Neither feels like a mind. A person can't hide, and can't be found.
2. **No pack is a pack.** The hounds of a pair may be a room apart within a minute. One that sees you tells no one.
   A person fights everything one at a time, at the door it comes through.
3. **The three-hunter cap is the main safety valve**, and it makes a level dole itself out. Any plan for packs has
   to replace it with something as safe for the dream.
4. **Wandering is noise.** Random steps don't read as patrols, and a person can't watch and time them.
5. **Nothing answers the player.** Fighting from a doorway, backing away and casting, running past, sneaking, using
   the dark, casting Hellfire into a crowd: the dead behave the same to all of them.
6. **The calm's wanderer appears from nowhere** four to eight steps away. It keeps the screensaver busy, but for a
   person it is a creature conjured behind a corner.
7. **The deep has no new faces.** Twenty levels of the descent introduce no creature; behaviour is the obvious thing
   to be new there.
8. **Few loops.** Flanking needs a second way round, and the halls seldom have one.
9. **The dream looks ahead.** Its `find_target` takes what is in front, what is attacking and a few kinds near it.
   Creatures that come from behind, or wait round a corner, will cost it more than they cost a person.
10. **`MAXMOBS` is 32**, and several things count creatures in a 32-bit mask (`chain_lightning`'s `hit`, a pocket's
    `PK.slain`). More creatures means widening those first; this plan assumes it stays at 32 and that packs and
    ambushes replace wanderers rather than add to them.

## Decide first

**A1. What this is.** *Recommended: (a).*
- (a\*) **A rework of how the dead think, plus set pieces for the generator**: senses and memory for everything,
  packs, roles (posts, patrols, callers, scavengers, hunters, lures, ambushers), cunning that comes with rank, and an
  encounter budget in place of today's wanderers and the calm's respawn. No new kinds.
- (b) The same and a few new kinds made for the roles (a hound-master, a bell-ringer, a stalker of the deep), new
  art for each.
- (c) Lighter: senses and memory, packs that share what they see, the hunt; no lures or ambushes.

**A2. Where it goes.** Since this draft was written, 1.0.0b14 became the first art pass (merged into main unreleased,
to be taken up again) and the lower storey 1.0.0b15 (settled, not begun). *Recommended: (a).*
- (a\*) **This is 1.0.0b15 and the lower storey moves to 1.0.0b16**, as 1.0.0b12's plan moved on a number before.
  The minds touch every level and the dream's balance; the storey adds stairs to the fields, "a third more
  creatures" on the lower storey (its L3), and ambushes from below are a gift to it. Building the storey on minds
  that already know packs and budgets is easier than teaching both at once.
- (b) The lower storey stays 1.0.0b15; this follows as 1.0.0b16 and has to make every new field run between storeys
  from the start.
- (c) One release with both.

**A3. The same minds for the dream and for a person?** *Recommended: (a).*
- (a\*) **The same minds**, with the dream given counters (Part G) and the caps tuned so its deaths stay within a
  margin (section 21). A few things already differ by who plays (the calm waits 35 s for a person, 11 for the dream)
  and may go on differing.
- (b) Full cunning for a person's run, a gentler set for the dream (no lures, flanking from rank V only).
- (c) A setting, *THE DEAD: DULL / WARY / CUNNING*, in the settings dialog and Esc's question, saved; the dream
  always WARY.

**A4. Keep the old minds behind a switch?** *Recommended: (a).* (a\*) Yes, `CRAWL_MIND=0` in the debug build, so the
balance runs compare the same build on the same seeds. (b) No; compare against a worktree of 1.0.0b13 as before.

**A5. The art pass and this.** Main carries the art pass so far, so whatever is released next takes it with it unless
1.0.0b14 goes out on its own first. *Recommended: (a).* (a\*) The art pass goes on as it was handed on (runs III and
IV's props, then the Wraith Unbound), in its own steps and commits, between or alongside these; it moves nothing in the
simulation, so the balance runs here aren't disturbed by it. (b) 1.0.0b14 is released as it stands before this
begins. (c) The art pass waits until this is done.

# Part A: senses and memory

## 1. Sight

Today's sight is nine cells all round. Two changes would give a person something to play with.

- **A facing.** Every creature looks where it walks (`m->face`, which shield-bearers and wall skulls already have),
  and turns its head now and then while it stands. It sees nine cells within about 120 degrees of its facing, and
  only two and a half behind and to the sides ("felt", the way you know someone is in the room). Every kind has a
  front view and a side view (`Kind.side`, `side2`), but only the bestiary draws the side; in the crypt every creature
  faces you whichever way it looks. For a person to tell where one is looking, the crypt has to draw the side view
  (mirrored for the other side) when it faces across your line, and something when it faces away: there is no back
  view. Part H's art grows by that: a back view for each kind that can turn its back, in the art pass's hand, or
  the side view standing in for it.
- **The dark.** Out of the light (the lightmap at the caster under about a third), it sees you at five cells instead
  of nine. The caster's own faint glow (`light_dynamic(G.px, G.py, 0.62, 2.8)`) is what it sees; a cast flares it
  for a second, so casting in the dark is seen at nine again, and the Lantern of the Dead, a wisp or a burning floor
  near you light you up.

**S1. A facing.** *Recommended: (a).* (a\*) As above, the crypt drawing side views and new back views (no eyes from
behind: the one exception to the art pass's rule). (b) A facing only for posts and patrols; hunting creatures see
all round, as now. (c) No facing.

**S2. The dark.** *Recommended: (a).* (a\*) As above, for everything but the kinds that live by other senses: bats
and hounds (they hear and smell; see section 11), and the shade and the Wraith (the dark is theirs: they see you at
nine in the dark and five in light, the other way about). (b) The dark shortens sight for everything alike. (c) No
change.

## 2. Noise with a place

Every noise becomes `noise(x, y, loud, what)`: a place, a reach in hunt-field steps, and what made it. What hears it
knows that something happened **there**, not where you are now. A first table of what is heard, and how far:

| Noise | Steps | Notes |
|---|---|---|
| A footstep | 3 | Quiet shortens it as now; running (Shift, the stick click) makes it 5 |
| A splash (run II's water, a flooded floor) | 5 | `on_water` already plays it |
| A spell cast | 8 | the spear 5, Hellfire and the ultimates 10, the BFS9000 the whole level |
| A spell landing, an explosion | 6 | at the place it lands, not the caster's |
| A door opening | 4 | doors opened by creatures too, so a hunting pack is heard |
| A secret wall grinding | 8 | |
| A creature's death | 4 | heard by its own kind at 8: the pack hears its own fall |
| A wall skull's scream, a gargoyle's shriek, a deacon's call | 10 | the callers (section 10) |
| A fight (blows exchanged, a bite) | 5 | |

**S3. Noise with a place.** *Recommended: (a).* (a\*) As above. (b) Keep today's three noises, but give them a
place. (c) Keep hearing as it is.

## 3. Awareness, memory and searching

Four states for each creature (`Mob.aware`), each with a tell, so a person can read them:

1. **Unaware.** At its post, on its patrol, or wandering. No sound but its own.
2. **Wary.** It heard something, or glimpsed you at the edge of sight. It stops, turns to the place, and after a
   moment goes to look (half pace, three to eight cells at most). Its tell is a sound of its kind: a rattle, a growl,
   a hiss, a click of chitin. Seen fully while wary, it is hunting at once; nothing more, and after about 8 s it goes
   back.
3. **Hunting.** It has you. It comes for you as now, and while it can see you, or one of its pack can (section 5),
   it knows where you are. Out of everyone's sight it goes to **where you were last seen, facing the way you were
   going** (`lkx, lky`, `lkh`), a few cells on along the hunt field in that direction, and then searches.
4. **Searching.** It looks into the rooms and halls within six to ten steps of the last place, for 15-25 s
   (longer by rank), and gives up: back to its post or its patrol, wary for a while after, so it hears more.

What this changes: a person who breaks sight and moves on can lose the dead; a person who stands still behind a
door is found. Hunters (section 11) don't search: they follow your trail.

**S4. Memory.** *Recommended: (a).* (a\*) As above, for every kind but the hunters, the spiderlings (a swarm, all at
once, as now) and the boss's fights (all at once, as now). (b) As above but the hunting state keeps today's live
field for the first 3 s after sight is lost, so a person can't break off by stepping round one corner. (c) Keep
today's alert clock.

## 4. Caught unaware

A creature that is unaware or wary, struck from where it isn't looking, takes **half again** the blow (the ghoul
caught feeding already takes double), staggers 0.6 s if it is one that staggers, and its tell is cut off. It rewards
sneaking and the dark without a stealth meter on the screen.

**S5. Caught unaware.** *Recommended: (a).* (a\*) Half again, unaware or wary, from outside its sight. (b) Double,
unaware only. (c) None.

# Part B: packs

## 5. What a pack is

A `Pack` in `G.packs` (eight at most a level): its template (section 16), its members (a 32-bit mask, as
`MAXMOBS` allows), a leader, a state shared by all of it (resting, patrolling, wary, hunting, searching, laid in
ambush, broken), what it knows (where you were last seen and when), its post or route, and its morale.

- **What one sees, all know.** When a member sees you it gives its kind's call (the tell); a moment later (0.4 s)
  the pack is hunting, from wherever its members are. A person who kills the one that saw them, before the call
  finishes, keeps the rest unaware. (The wall skull's scream is the same idea for a single watcher.)
- **Moving as one.** On a patrol the leader walks the route and the rest follow its trail a cell or two apart, so a
  pack comes down a hall as a file and spreads in a room. Hunting, each takes the field, but the second and third
  take a **different neighbour from the one in front where the scores tie**, so they come round a pillar or a
  pew on both sides rather than in a queue.
- **A pack's turn.** Members of a melee pack don't all strike at once: one engages while the others hold just out
  of reach and circle (the hounds' bite and dart, made into a rhythm). That keeps the damage of a pack near what
  today's three hunters deal while looking and feeling worse.
- **Morale.** When the leader falls or half the pack is down, the pack breaks, by its kind: the Order's dead (the
  skeleton family) hold their drill and fight on; the living (bats, spiders, imps) scatter and come back later as
  wanderers; hounds run to the nearest other pack or hunter and bring it (a mixed band, made in play); the buried
  don't notice. A broken pack's tell is a sound of its own, so a person learns that killing the leader matters.

**P1. Shared sight.** *Recommended: (a).* (a\*) With the call and its 0.4 s, so it can be cut short. (b) Shared at
once, no call. (c) Packs share nothing; they only move together.

**P2. Taking turns.** *Recommended: (a).* (a\*) One engages, the rest circle and hold, changing every few seconds or
when the one engaged is hurt. (b) Two engage at once from rank III. (c) All engage, as now.

**P3. Morale.** *Recommended: (a).* (a\*) By kind, as above. (b) Every pack breaks at half. (c) No morale.

## 6. Packs of a kind

Each kind gets a pack it lives in, drawn from what it already does and what its bestiary line says of it.

| Kind | The pack | What it does together | Its tell |
|---|---|---|---|
| Skeletons and shield-bearers | **A file of the Order** (2-5): shields in front, throwers behind | Advances as a line in halls, the shields first; throwers throw past them; it holds a room's far side and makes you come to it. "A skeleton that remembers its drill." | Bones clacked in time, like a drum |
| Bats | **A roost** (3-6) on the ceiling of a dark room | Asleep upside down (`DR_FLIPV`); light, a spell or a noise in the room wakes all of it, and the cloud goes for the face; a broken roost scatters to other rooms | A rustle and a single squeak before it wakes |
| Shamblers, the Drowned | **A wake** (a graveyard, a flooded nave) | When one rises the rest of the yard or the water stirs over the next seconds, nearest first, a slow tide | Earth shifting, water lapping where nothing moves |
| Bone hounds | **A pack** (2-4) | Hunts by scent (section 11); circles at four cells and takes turns to dart in; one holds you at bay while the rest close; can't open doors | A howl when the hunt starts, yelps as it closes |
| Web spiders, hunting spiders, brood mother | **A nest** (the spinners' gallery, a stope) | The web spiders are its tripwires: one dropping wakes the hunters overhead, who come along the ceiling; the mother lays faster while the nest fights | The webs tremble before the hunters come |
| Ghouls | **Scavengers** (1-3) | Come to the sound of a death, late, and feed on what you left; a fight draws them (section 10) | Chewing, somewhere close |
| Imps | **A brood of the magma** (2-3) | Split to either side and lob from both; fall back into the vent's heat when you come close | Hissing, a pop of embers |
| Gargoyles | **Sentinels** (2-3 in a hall) | One wakes and shrieks, and the others on the hall wake with it | Grit falling from a statue that shouldn't move |
| Shades | **Alone**, always | A stalker (section 11) | Footsteps with no one there, as now |
| Deacons | **A congregation** (a deacon and his dead) | He keeps behind them, raises the fallen, and calls the level's dead when he first sees you (section 10) | A whispered chant |

**P4. Packs of a kind.** *Recommended: (a).* (a\*) All of them as drafted. (b) The skeletons' file, the hounds and
the roost first, the rest later. (c) The owner's pick from the table.

## 7. Mixed bands

Unlike kinds working together, each pairing chosen so that one covers the other's weakness.

| Band | From | How it works |
|---|---|---|
| **The knight and his hounds** | 13 | The hounds find you by scent and hold you at bay without biting until the knight is within four cells, then everything goes in. Kill the knight and the hounds break. (Today's knight with two hounds, made to act as one.) |
| **The deacon's congregation** | 18 | As above: skeletons and shield-bearers in front, the deacon behind raising the fallen. |
| **The watch and the dead** | 3 | A wall skull over a hall and a file of skeletons in the room beyond: meet its eye and its scream brings them. |
| **The roost and the shade** | 16 | Bats roosting over a dark room a shade keeps: the cloud at your face while the shade's touch comes unseen. Light the room and both show. |
| **Bait and teeth** | 5 | A lure and an ambush of another kind (section 12). |
| **The brood's guard** | 14 | A brood mother with two hunting spiders that never stray far from her and come back if she is hurt. |
| **The Wraith's chill** | 9 | When the Wraith comes, packs that hear its wail stop hunting and fall back to their posts: the dead fear it too. A small mercy, and a tell that it is near. |

**P5. Mixed bands.** *Recommended: (a).* (a\*) All seven, from the levels shown. (b) The knight's, the deacon's and
the watch only. (c) None; packs of a kind only.

## 8. How many come at once

Today's cap (three hunters, the nearest) is the dream's main protection and the reason fights come one at a time.
With packs it has to count differently.

**P6. The cap.** *Recommended: (a).*
- (a\*) **Engaged, not alert.** Any number may hunt; at most so many may be *engaged* (within striking or throwing
  reach and in sight) at once: 3 in run I, 4 in run II, 5 in run III, 6 in run IV, +1 for a person. The rest hold at
  the edge of a fight (circling, waiting round a corner, keeping a throw back). A pack counts its members.
- (b) Keep three hunters, but a pack counts as one hunter.
- (c) Keep three hunters, the nearest; packs move together but only three of them come.

# Part C: roles

Every creature placed has a role as well as a kind; the generator gives it (Part E). A kind has roles it is good at.

## 9. Posts and patrols

- **A post** (today's residents and vault guards): a cell in a room, facing a door. It turns its head now and then.
  Wary, it goes to the door; searching done, it comes back to the post.
- **A patrol**: a loop of three to five waypoints (rooms' middles, hall junctions) that the generator picks along
  lanes and ties into a cycle, or walks there and back where there is no loop. It walks at its pace and pauses at
  each waypoint. In place of today's random steps.
- **A wanderer** stays for the living that wander (bats, hunting spiders in run III), but goes from room to room
  with a purpose (to a room it hasn't been in for a while), not a random step.

**R1. Posts and patrols.** *Recommended: (a).* (a\*) As above, patrols in place of random wandering. (b) Patrols
for packs only; lone wanderers wander as now. (c) No change.

## 10. Callers and scavengers

- **Callers** make a noise that brings others: the wall skull's scream when it wakes (ten steps), the gargoyle's
  shriek, the deacon's call when he first sees you, a bat roost's waking cry. Everything that hears it is wary of
  that place; a pack hearing it goes there hunting. A caller killed before it calls doesn't call. The wall skull
  becomes an alarm a person can avoid by not meeting its eye, as its bestiary line says.
- **Scavengers** (the ghoul) go towards the sound of a death and feed on fresh remains (it already eats bones). A
  person who fights for long in one place draws them; one who moves on finds them feeding there later, open to a
  blow.

**R2. Callers and scavengers.** *Recommended: (a).* (a\*) Both. (b) Callers only. (c) Neither.

## 11. Hunters

The owner's "some should hunt". A hunter doesn't wait to be found and doesn't search; it tracks.

- **The trail.** The caster leaves a scent in each cell it walks (`G.scent`, the time it was there; it fades after
  about two minutes). A hunter steps to the neighbour with the freshest scent, so it comes **the way you came**, up
  behind you, quicker than you walked. When the scent is fresher than a few seconds it is close enough to hunt you by
  sight and the field as anything else does. Water breaks the trail (run II's canals and flooded floors: a person can
  shake a hunt there); a shut door it can't open stops it until something opens it.
- **The hunt.** On some levels a hunt is set loose: a hunting band starts at the far end of the level at a time
  (between 60 and 150 s in, sooner deeper in the run), picks up the trail where you were a minute ago, and follows.
  It is announced by a far-off sound of its kind (a howl, a skitter overhead, a whisper), and once it has your fresh
  scent by a second, nearer. It is the calm's replacement: instead of a creature put round the corner, a pack that
  was always on the level comes from where it was.
- **Who hunts.** Hounds (from 8) by scent; hunting spiders (from 11) across the ceiling, by the hunt field's
  ceiling (it ignores what stands on the floor); the shade (from 16), alone, as a stalker: it follows unseen in the
  dark four to six cells behind and strikes when you are fighting something else or stand still, and fades back if
  you turn and light it; the knight's band (section 7).
- **Run I** has no hunters before the hounds; its hunts are the Order's watch: a file of skeletons that comes to the
  last noise you made, not your trail.

**H1. The trail.** *Recommended: (a).* (a\*) As above. (b) Hunters use today's live hunt field, always knowing where
you are, but are released late. (c) No hunters.

**H2. The hunt in place of the calm.** *Recommended: (a).* (a\*) A hunt replaces the calm's respawn for a person;
the dream keeps the calm (it needs the action to keep the screen alive) but its wanderer comes from beyond sixteen
steps and walks in, instead of appearing at four to eight. (b) A hunt on top of the calm, as it is. (c) Keep the calm
alone.

**H3. How often.** *Recommended: (a).* (a\*) A hunt on one level in three from level 8, two in three in runs III and
IV, never on a boss's level. (b) Every level from 8. (c) Only on corrupted levels and replays.

## 12. Lures and ambushes

The owner's "lead the player into an ambush". A set piece in three parts: the **kill room**, the **teeth** and the
**bait**.

- **The kill room.** A room with one or two doors, off the lanes the start and the exit lie on, at least twelve steps
  from the start, the exit, the merchant's doorway, a font or a puzzle's wing. The teeth wait in it **beside the
  inside of the doorway**, where the line from the door doesn't reach, or wherever their kind hides: on the ceiling
  (hunting spiders), in the earth (the buried), in the water (the Drowned), on plinths (gargoyles), in alcoves (knights
  in a knights' vault), in the dark (shades).
- **The bait.** A creature that shows itself in a hall six to twelve steps from the kill room. When it sees you it
  gives its tell and waits to be seen; when you come within six cells or hit it, it backs away down the way to the
  kill room, through it and past the teeth to the far side, and turns. Its retreat **looks wrong**: it keeps
  looking back, it goes at your pace, not its own. The teeth spring when you are two cells into the room, or see
  one of them, or when the bait is killed in the room.
- **Every answer has an outcome.** Follow: the trap springs, as it was meant to. Refuse: the bait comes back to
  tease twice more, then the whole set piece gives up its trap and comes hunting as a pack, on ground you chose. Kill
  the bait at range: the teeth, having heard it die, come out to search, and you know where they were. Find the room
  first another way (the map, Commune, Grave Sense, a careful look): walk in from the other door, or burn it out.
- **Baits.** A straggler of the Order (run I: a lone skeleton that throws once and backs off); a bone hound that
  yelps and runs (run II); **a gilded creature**, any run from level 7: it already runs when hurt, and running with
  its treasure into an ambush is the greed trap; a bat that flutters at the edge of your light and goes to its
  roost; an imp that falls back across a cinder field with a hazard in it (run IV).
- **Teeth.** Shield-bearers behind the door (run I), the buried in a burial vault (run I), the Drowned in a cistern
  or canal (run II), knights in the knights' vault (once they are met, from 13), hunting spiders overhead in a timbered gallery or
  stope (run III), ghouls in the charnel pit (run III), gargoyles in a glass hall (run IV), shades in an unlit room
  (run IV).
- **Tells.** No ambush without a sign a person could have seen (the bestiary's rule 4). By run: a torch snuffed by
  the kill room's door and bones raked aside on the floor (run I); still water rippling where nothing walks (run
  II); fresh scree at a doorway, claw marks low on the timber (run III); cinders still warm, and two points of light
  in the dark (run IV). The spectres, which only want you to know they are there, drift towards set pieces and hang
  about their doors: harmless, and a warning for anyone who has learnt to read them.
- **First of each.** The first time each set piece is sprung on a run, a caption names it once, as the omens are:
  *AN AMBUSH*, *THE HOUNDS HAVE YOUR SCENT*, *THE ORDER HOLDS THE ROOM*.

**L1. Lures and ambushes.** *Recommended: (a).* (a\*) As drafted: kill rooms, teeth, baits, the three answers.
(b) Ambushes only (the teeth waiting by a door), no baits. (c) Baits and ambushes only from run II.

**L2. The gilded as bait.** *Recommended: (a).* (a\*) Yes, one gilded in two from level 7 runs to a kill room if
one is near; its treasure is still there to be had. (b) No; the gilded stay a prize.

**L3. The spectres as a tell.** *Recommended: (a).* (a\*) They drift to set pieces now and then (not always). (b)
No; spectres stay as they are.

**L4. How many.** *Recommended: (a).* (a\*) At most one ambush a level from level 5, two from run III; never on a
boss's level; none in a pocket. (b) One a level from level 3. (c) One level in two.

# Part D: answering the player

## 13. Cunning by rank

Colour already means strength, the same way everywhere. It could mean cunning too, so that the curve of
intelligence comes from the rank mix the game already climbs, and is legible: *the red ones dodge*. Each rank keeps
what the ranks below it do.

| Rank | What it adds |
|---|---|
| I (white) | The senses and memory of Part A; its pack's sharing and turns |
| II (green) | Falls back to its pack when alone and hurt (a quarter of its health); a thrower steps back behind a corner between throws if one is a step away |
| III (blue) | **Sees a slow missile coming** (fireball, Hellfire, a flask, a spear thrown from beyond five cells) and steps aside, once in 4 s; keeps out of hazards' warnings and fixed hazards (`hz_dmap`); won't walk into a doorway you are holding with a spell ready: it waits beside it |
| IV (red) | **Flanks**: given a second way to you within six steps more, the second and later of a pack take it; spreads out against splash (Hellfire, the flask, Shatter, Arc): no two within a cell and a half when you have such a spell in hand |
| V (black) | Feints (starts a blow or throw and holds it, to draw a dodge or a cast); reads the charge: keeps out of reach of a charged ultimate while you hold it, and comes in when you let go |
| Gilded | Bait (L2), and runs as now |

Flanking needs loops, which the halls seldom have (one opening in twelve, and dead ends filled). Part E's rules ask
the generator for a second door into the rooms that hold packs, and a few more loops in the stretches between them.

**D1. Cunning by rank.** *Recommended: (a).* (a\*) As in the table. (b) Cunning by the run's step, the same for
every rank. (c) Cunning by kind only (each kind its own tricks, no ladder).

**D2. Flanking's loops.** *Recommended: (a).* (a\*) A pack's room gets a second door where the plan allows, and the
generator's extra loops go from one in twelve to about one in six near those rooms only. (b) More loops everywhere.
(c) No change to the halls; flanking only where a loop happens to be.

## 14. How the dead answer how a person plays

What the owner called different approaches. Each has an answer and a counter to the answer, so no way of playing
wins outright.

| A person who... | Today | What answers it | And its counter |
|---|---|---|---|
| Holds a doorway and fights what comes through | Wins, one at a time | Rank III waits beside the door; throwers keep back out of sight; a pack sends one through while the rest come round (rank IV) | Go through first; a wide spell through the door |
| Backs away and casts | Wins against everything but leapers | Hounds and leapers close; packs take turns; throwers lead | Fight in rooms, not halls; frost |
| Runs past everything | Mostly works | Running is loud (S3); hunters follow the trail; what you passed is hunting you at the exit | Run where it's dark, through water, shut doors behind you |
| Sneaks, in the dark | Means nothing | Works (S1-S2, S5): blows from the dark land harder; callers and the wake are the danger | Light (a cast, the Lantern) gives you away |
| Clears every room | Works, slowly | Scavengers come to the fights; the hunt comes on a long level | Move on before the scavengers |
| Leaves things alive | Nothing follows | Broken packs fetch others; survivors join into bands; the reckoning counts them | Kill the leader; let the pack break |
| Throws Hellfire into a crowd | Wins | Red and black spread out | Single-target spells, Arc's leaps |
| Charges an ultimate | Wins | Black keeps out of its reach while it is held | Release early; hold it round a corner |
| Chases what runs | Nothing runs but the gilded | The bait (section 12) | Read the tells; refuse |

## 15. Hazards and the dead

Today hazards hurt only the caster and the dead ignore them.

**D3. Hazards and the dead.** *Recommended: (a).*
- (a\*) **Hazards hurt creatures too** (at half the share), ranks I and II blunder into them, rank III and up walk
  round them. A person can draw a pack through Hellrent or into bile; a red bait can lead a person into it (run
  IV's imp).
- (b) Creatures avoid hazards; hazards still hurt only the caster.
- (c) No change.

# Part E: generation

## 16. An encounter budget and set pieces

In place of "three wanderers, four from level 4, and residents in about half the right rooms", each level gets a
budget of threat points and spends it on set pieces. Today's fixed placements (wall skulls, web spiders, the buried,
imps, gargoyles, vault guards, the brood mother) become set pieces too, so that one budget governs the lot.

| Set piece | Costs | From | Where | What |
|---|---|---|---|---|
| A wanderer | 1 | 1 | halls | one creature, roaming with a purpose |
| A post | 1 a head | 1 | a room of its kind (section 18) | today's residents, facing a door |
| A patrol | 2-3 | 2 | a loop of the lanes | two or three of a kind |
| A watch | 2 | 3 | a hall and a room | a wall skull and what its scream calls |
| A file of the Order | 4-6 | 4 | a room or a hall | shields and throwers in drill |
| A roost | 3 | 5 | a dark room | bats on the ceiling |
| A wake | 3-4 | 2 | a graveyard, a flooded nave | the buried, the Drowned |
| A nest | 5 | 6 | a spinners' gallery, a stope | web spiders and hunters |
| An ambush | 6-8 | 5 | a kill room and a hall | teeth and a bait (section 12) |
| A hunt | 5 | 8 | the far end, set loose later | hounds, hunting spiders, a shade |
| A band | 6-8 | 13 | a room | the knight's, the deacon's, the brood's guard |
| A guard | 2 | 1 | a vault | today's guardians |

The budget: about 10 on level 1, rising through each run to about 24 on its ninth level, a run's first level a
little above the last run's first, and the cunning of section 13 paid for by fewer heads (a rank's points cost more
as it climbs, so a run of red packs has fewer of them). These numbers are first guesses for the balance runs to
settle; the rule that counts is that the creatures on a level end up near today's numbers (section 21).

**G1. A budget.** *Recommended: (a).* (a\*) One budget spent on set pieces, today's placements among them. (b) Keep
today's placements and add packs, patrols and ambushes on top, with fewer wanderers. (c) Today's placements, with
the wanderers made into packs and nothing else.

## 17. Rules

1. **One new thing a level.** The bestiary's rule, widened: a level brings at most one new creature, one new rank
   or **one new set piece**. Its first showing is easy to read: lit, alone, near a lane, its tell plain.
2. **Nothing hunts the opening.** No hunt is set loose before 60 s; no pack posted within ten steps of the start;
   nothing set within six steps of the merchant's doorway, a font, the Black Altar, a puzzle's wing door or a trial's
   door (they have their own).
3. **A way out.** A kill room never has the exit, a key or the level's only way on behind it; the door you came in
   by is never shut on you.
4. **Within the cap.** No set piece needs more engaged at once than the run allows (P6).
5. **Room types choose.** A room's type (`RTYPES`, section 18) says which set pieces may live there.
6. **Signs ahead.** Every set piece carries its tell (section 12); a run's first ambush carries two.
7. **Thirty-two.** The level's creatures stay within `MAXMOBS`, with room kept for what is raised, hatched and
   called (eight places at least).
8. **The dream walks lanes.** Every set piece must be one the dream can come through on its lanes; one that can't
   is put elsewhere (the dream's counters are in Part G).

**G2. The rules.** *Recommended: (a).* (a\*) All eight. (b) The owner's changes to them.

## 18. Where they live, by run and room type

| Run | Room type | What lives there |
|---|---|---|
| I, the Crypts | Ossuary | a post or a file of the Order |
| | Burial vault | a wake of the buried; an ambush's teeth |
| | Chapter house | a file of the Order holding the room; a watch over its door |
| | Knights' vault | shield-bearers at their posts (knights once the descent comes round to run I again); an ambush's teeth |
| | Undercroft | a roost; a patrol's waypoint |
| II, the Catacombs | Saints' catacomb | a watch; posts |
| | Cistern, canal | the Drowned in the water (a wake or teeth); the hunt's trail broken |
| | Silted nave | a wake |
| | Spinners' gallery | a nest |
| III, the Hollows | Timbered gallery, stope | hunting spiders overhead; a nest |
| | Charnel pit | ghouls (scavengers' den, teeth) |
| | Fossil hall | a file of the Order; a band |
| | The chapel | the deacon's congregation |
| IV, the Deep | Geode | a roost (ember bats) |
| | Glass hall | gargoyles (sentinels, teeth) |
| | Cinder field, magma vent | imps; a bait over a hazard |
| | The throne | a band |
| Any | Halls | patrols, wanderers, baits, hunts |

**G3. Room types choose.** *Recommended: (a).* (a\*) As in the table. (b) Room types choose only posts, as today;
packs and ambushes go anywhere that fits.

## 19. Special places

- **Pockets** (the Backrooms, Severance): their creatures come as today (`gen_pocket`), but in packs of a kind, with
  memory and cunning; no hunts, no ambushes. Severance's halls are narrow: a file of the Order there is a corridor
  fight a person can't avoid, which suits it.
- **Puzzle wings and trials** keep their own creatures and rules (`Mob.tri`); these minds apply to them, but no set
  piece is placed inside.
- **Boss levels**: outside the arena, about half a level's budget, no hunt; the arena as now (the boss's adds come
  all at once and are not capped).
- **Corrupted levels** (the Dead Bell): the budget two in five larger, in place of two in five more wanderers; a hunt
  always.
- **Replays and challenges**: as their level would be.
- **After a death on the level** (`G.retries`): today the blow softens 15% a death. Proposed: the same, and the set
  pieces that killed you come back a head short, so a person isn't put through the same trap word for word.

**G4. Special places.** *Recommended: (a).* (a\*) As above. (b) Packs and cunning in pockets too, with ambushes. (c)
The new minds on the level only; pockets, wings and trials as today.

# Part F: the curve

## 20. Three dials, and the rollout

Difficulty has three dials: **numbers** (the budget), **strength** (health, blow, rank: as today), and **cunning**
(section 13, carried by rank). The proposal is that cunning rises as numbers fall a little, so that a level feels
harder because of what the dead do, not because more of them come. Within a run the rank mix climbs as it does now,
so cunning climbs with it; from run to run the set pieces widen.

| Levels | What is new to the dead |
|---|---|
| 1 | Senses and memory; a facing; the dark |
| 2 | Patrols; the wake (with the shambler) |
| 3 | The watch (with the wall skull) |
| 4 | The file of the Order (with the shield-bearer) |
| 5 | The first ambush, plainly signed; roosts |
| 6 | The nest (with the web spider) |
| 7 | The gilded bait |
| 8 | Hounds' packs, the first hunt |
| 9 | The Wraith's chill (packs fall back from it) |
| 11 | Hunting spiders hunt overhead |
| 12 | Scavengers (with the ghoul) |
| 13 | The knight and his hounds |
| 14 | The brood's guard |
| 16 | The shade stalks; the roost and the shade |
| 17 | Imps' baits over the vents |
| 18 | The deacon's congregation, and his call |
| 19 | Sentinels (with the gargoyle) |
| 21-30 | Run III: two ambushes a level, hunts on two levels in three, mixed bands of every kind met; rank IV's flanking comes in with the run's reds |
| 31-40 | Run IV: everything, black's feints with the run's blacks; bands of three kinds |
| 41 on | As run IV; the loops of the descent come round with a rung more of cunning, as the ranks do |

Since runs III and IV bring no new kinds, their novelty becomes how the old kinds work together: a hunting spider
pack that drops round you as a deacon's dead come through the door.

**D4. The rollout.** *Recommended: (a).* (a\*) As in the table. (b) Everything from run II on, run I as today but for
the senses. (c) The owner's changes.

## 21. Keeping it fair, and the numbers to hold

- **Every danger tells.** A call before a pack hunts; a howl before a hunt; a sign before an ambush; a pose before a
  blow, as now.
- **The dream's margin.** At 1.0.0b13, eight 100-minute dream runs from level 1 (seeds 1-15 odd) cleared a median 9
  levels and died 22 times (19 on level 10); from 11, 21 and 31, no deaths. Proposed target: from level 1, deaths no
  more than a quarter above 1.0.0b13's on the same seeds, and the knight beaten as often; from 11, 21 and 31, at most
  two deaths in eight runs each; level time within 30 s of today's median. If the minds cost more, the budget and the
  cap give way before the minds do.
- **A person's run** should be harder, and we can't measure that in the harness. The scripted player
  (`HOST_INPUT`) can at least walk into an ambush and a hunt and show that it springs and that the tells come first.
  How hard it feels is the owner's to say, by hand.
- **No death loop.** A set piece that kills the dweller twice on a level comes back two heads short the third time;
  a hunt that killed you isn't set loose again on that level.

**D5. The dream's margin.** *Recommended: (a).* (a\*) As above. (b) Tighter: no more deaths than 1.0.0b13. (c) Looser:
the dream may die a third more, for a harder game.

# Part G: the dream

The autopilot walks lanes, looks ahead and fights what comes. The new minds hit it hardest from behind and round
corners. What it needs:

- **A glance back.** When it hears a hunt's nearer sound, a pack's call behind it, or a scavenger's chewing, it
  turns about once at the next lane cell (`decide` already turns quickly) and fights what is there.
- **Reading tells.** It treats an ambush's sign as a soft hazard (`hz_soft`): it would rather go another way, and if
  it must go in, it goes in casting (a spell at the door first, as it fires at the Wraith).
- **Never chasing bait.** It doesn't chase now; it keeps on to its goal. A bait that stands in its way is a target
  as anything else.
- **Leaders first.** `find_target` already puts the deacon first; it does the same for a pack's leader and a
  caller about to call (as it does the wall skull gathering fire).
- **Corners.** With a pack hunting it and health under half, it backs to the nearest lane corner it has walked and
  fights from there (a corner halves the ways in).
- **Its own cap.** P6's engaged cap is the dream's main safety; it is one less while the dream plays (A3).

**Y1. The dream's counters.** *Recommended: (a).* (a\*) All six. (b) The glance back and leaders first only, the rest
as the runs show a need.

# Part H: showing it

- **Sounds** (`snd.c`, new `SFX_*` at the end): each pack's call (the Order's clack, the hound's howl and yelps, the
  roost's rustle, the nest's tremble, the deacon's call), a wary creature's tell by family, a broken pack's sound,
  the scavenger's chewing. All positional through `sfx_at`, so a person can tell where. About a dozen new sounds.
- **Captions**: the first of each set piece named once a run, as the omens are (`RUN.omens` grows a field, saved).
- **The bestiary**: each kind's page gains a line on its pack and its ways, written in its voice ("THEY HUNT IN
  PAIRS, BITE, AND RUN" already is one).
- **The Codex**: a part, *THE WAYS OF THE DEAD*, a page for each set piece once met: what it does, its tell, how it
  was answered.
- **The record and achievements** (bits from the spare after `ACH_DEED0`'s, of the 512): ambushes sprung and survived (1, 5, 20),
  hunts shaken off (lost the trail in water or behind a door), packs broken by their leader's fall, a caller silenced
  before it called, a file of the Order beaten without being touched.
- **Art**, sheets first for the owner's eye: a roosting bat (folded, upside down), the ambushes' signs for each run
  (raked bones, a snuffed torch, scree, warm cinders, eyes in the dark), a wary pose for the skeleton family (a head
  turned) if the side view doesn't serve, and for S1 the side views drawn in the crypt and a back view for each kind
  that can turn its back (section 1), in the art pass's hand. The art pass keeps every creature's eyes visible from
  anywhere; a back view would hide them, which is what tells a person it is looking away, so S1 (a) asks the owner to
  allow that one exception.

**V1. Showing it.** *Recommended: (a).* (a\*) All of it. (b) Sounds and captions now; the bestiary lines, the Codex
part and achievements later.

# Building it

In steps, sheets before art in the game, each step checked on the eight seeds against `CRAWL_MIND=0` before the next.

1. **Senses and memory** (Part A): facing, the dark, noise with a place, the four states, searching, caught unaware.
   The old minds behind `CRAWL_MIND=0`.
2. **Posts and patrols** (section 9): the generator's routes; residents as posts.
3. **Packs** (sections 5-6): `Pack`, shared sight, the file, turns, morale; the Order's file, the hounds, the roost,
   the wake. The engaged cap (P6) in place of `pick_hunters`' three.
4. **Callers and scavengers** (section 10).
5. **Hunters** (section 11): the scent trail, the hunt, the shade's stalking; the calm's change (H2).
6. **Lures and ambushes** (section 12): kill rooms, teeth, baits, tells; the spectres' drift.
7. **Cunning by rank** (sections 13-15): dodging, cover, doorways, flanking and the loops it needs, spreading, the
   charge, hazards and the dead.
8. **The budget** (Part E): set pieces, the rules, room types, the special places; the mixed bands (section 7).
9. **The dream's counters** (Part G).
10. **Showing it** (Part H): sounds, captions, the bestiary, the Codex, the record, achievements; the art.
11. **The checks**: the balance runs (D5), ASan on long dream and scripted sessions, `bench` (the new fields must not
    show: a few breadth-first searches a second over 2,025 cells, and one scent stamp a step).

Debug switches, as the others: `CRAWL_MIND=0` (today's minds), `CRAWL_PACK=k` (set piece k a few cells ahead five
seconds in), `CRAWL_AMBUSH=1` (an ambush on every level, `CRAWL_TPAMBUSH=1` standing you where its bait can see
you), `CRAWL_HUNT=s` (a hunt set loose at s seconds), `CRAWL_CUNNING=r` (every creature thinks as rank r, whatever its
colour), `CRAWL_BUDGET=f` (the budget times f). Logging: `EVENT pack_call`, `pack_hunt`, `pack_broken`, `wary`,
`search`, `lost_you`, `caught_unaware`, `called`, `scavenge`, `hunt_begin`, `hunt_scent`, `hunt_lost`, `bait_seen`,
`bait_run`, `ambush_sprung`, `ambush_refused`, `dodged`, `flank`, `spread`; the debug state line gains counts of the
wary, hunting and searching, and `SETS level n: ...` lists the set pieces placed, as `SECRETS` and `ROOMS` do.

What it touches: `update_mob` (most of all), `spawn_wanderer_type`, `start_level`'s placements, `pick_hunters`,
`alert_near` and `footstep`, `damage_mob` and `kill_mob` (noise, caught unaware, morale), `render_sprites` (the side
and back views by facing), `gen_level` (loops near
packs' rooms), `update_hazards` (D3), `find_target` and `decide` (the dream), `snd.c` (sounds), the record and the
Codex. `Mob` grows a few fields; nothing about a creature is saved, so the save version moves only for the record's
new counts and the omens' new field, as a ledger section of its own.

## The questions, gathered

The recommended answer for each is (a). In short:

- **Decide first.** A1 what this is; A2 1.0.0b15, the lower storey to 1.0.0b16; A3 the same minds for the dream; A4
  `CRAWL_MIND=0` kept; A5 the art pass goes on alongside.
- **Senses.** S1 a facing; S2 the dark (the shade and the Wraith the other way about); S3 noise with a place; S4
  memory and searching; S5 caught unaware, half again.
- **Packs.** P1 shared sight after a call; P2 turns; P3 morale by kind; P4 every pack of a kind; P5 every mixed band;
  P6 an engaged cap of 3, 4, 5, 6 by run, +1 for a person.
- **Roles.** R1 posts and patrols; R2 callers and scavengers.
- **Hunters.** H1 the trail; H2 the hunt in place of the calm for a person, the dream's calm walking in from afar; H3
  one level in three from 8, two in three from run III.
- **Ambushes.** L1 as drafted; L2 the gilded as bait; L3 the spectres as a tell; L4 one a level from 5, two from run
  III.
- **The curve.** D1 cunning by rank; D2 loops near packs' rooms; D3 hazards hurt creatures, the low ranks blunder; D4
  the rollout; D5 deaths at most a quarter above 1.0.0b13's.
- **Generation.** G1 one budget; G2 the eight rules; G3 room types choose; G4 the special places.
- **The dream.** Y1 all six counters.
- **Showing it.** V1 all of it.

And three that are only the owner's:

- **O1.** Of everything here, what would you most like to meet first by hand? The plan's order puts senses and packs
  first and ambushes sixth; if ambushes are the heart of it, they can come third, after packs.
- **O2.** Is there a fight from another game you'd like the dead to remember (a Wolfenstein guard turning at a
  door, a Thief guard's search, a pack of something that frightened you once)? A sample or two would set the tone.
- **O3.** How hard should a person's first run feel after this: as hard as it is now, with the dead more alive; a
  little harder (the plan's aim); or a good deal harder, with the dream given more help to keep up?
