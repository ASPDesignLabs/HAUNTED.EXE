# HAUNTED.EXE 1.0.0b14: everything since 1.0.0b13

**The full record**, kept for whoever works on this next. It sets down every change on main and on the work
branches since the thirteenth beta went out on 31 October: the first art pass, how the dead think, the caster's new
legs, and the pass over the screens, with the faults found and mended on the way, what was tested and how, and what is
still to come. The owner read it on 3 November and chose the number (1.0.0b14), a pre-release as before with nothing
pinned as Latest, and release notes cut to what a player needs to know (`.github/release-notes.md`); the page and the
README took their share in the release commit.

Nothing in it has been played by hand, run on Windows or under Wine, or seen on a real screen. Everything was tested in
the harness, which draws exactly what the game draws; the Windows build was compiled and smoke-tested by GitHub Actions
on each branch. The numbers below say what the screensaver's own caster (the dream) made of it.

---

## At a glance

- **The first art pass.** Every creature drawn again by hand, face first: skulls and faces pixelled one at a time in a
  gothic hand, eyes that show from anywhere in any light and fog, rank told by tatters, frost, blood, char and gilt as
  well as colour, and a spirit's vapour off the higher ranks. Three of the four bosses redrawn (the knight, the spider,
  the Deacon), the caster's hand moving, and every prop and pickup of the first two runs remade.
- **How the dead think.** The dead see and hear, remember and forget; they keep posts, walk rounds, roam; they come in
  packs that call, take turns, flank and break; callers raise the alarm, ghouls come to a death, hunters follow your
  trail, kill rooms wait behind a bait; the higher ranks dodge, take cover, feint, wait by a door you hold and read a
  charge. One budget a level spends all of it on set pieces. Three new kinds: the Whipper-In, the Sexton and the
  Lightless.
- **The caster.** A sprint on Shift, with a meter; Physic, a stat and a line that multiply what a potion restores; the
  Lantern of the Dead shuttered on a long press of L; places in the walls to mend at.
- **The pass over the screens.** Every menu, card and the tally cut in weathered marble, in faces of our own at today's
  sizes; the Alignment a true skill tree with an icon to each node and a pane beside it; the wardrobe as sockets on a
  carved figure; headstones for the cards and Esc's question; a memorial tablet for the tally; the HUD in the run's own
  stone. The same words and the same keys.
- **On the page:** *Then and now*, single frames from the first beta, the thirteenth and this one, side by side, each
  opening large; the README's previews of menus taken again.

---

## Before you upgrade

**Coming from the thirteenth beta:** your saves carry over. Going back is another matter, as it was last time: the
thirteenth can't read a piece that has rolled one of this beta's two new stats (Sprint and Physic) and drops it, and it
forgets the steps bought in their two new lines of points. If you might go back, keep a copy first, from a command
prompt:

```
reg export HKCU\Software\HalloweenVGASaver haunted-backup.reg
```

Double-clicking `haunted-backup.reg` puts it back. Your settings carry over either way; the controller's sprint button
is a new setting of its own (`PadSprint`), which older betas ignore.

**Coming from the twelfth or earlier:** as the thirteenth's notes say. The eleventh changed saves one way, and this one
keeps that change.

Nothing in the art or the screens touches a save. The minds touch none either: what a creature thinks lasts only while
its level does.

---

## Part 1. The first art pass

The owner's word on 31 October: many of the creatures looked basic, the skulls and the wall skulls, the imps' and the
gargoyles' heads "like a child's drawing", the Drowned goofy. The pass was planned the same day (`docs/InitialArtPass.md`)
with the owner's answers to seventeen questions, and built creature by creature, each one committed, pushed and logged,
so that it could be handed on between sessions. It is visual only: nothing in the simulation, the saves or the dream's
numbers moved, and each unit was checked by running the dream with the same seed before and after and comparing every
event it logged.

### Across every creature

- **One skull, drawn by hand.** A master skull, fifteen pixels wide, drawn pixel by pixel in the bone's own ramp: a
  domed brow, deep sockets, the nasal notch, the cheekbones' arch, teeth. Every skull in the game is that skull: the
  skeleton's, the shield-bearer's, the wall skull's (the same skull three times over by Scale3x, then given grain and
  cracks at the finer size), the knight's under his helm, the deacon's in his hood, the skulls on the Wandering Spider's
  back and in the props. Faces are kept clear of the level's grime, which had been darkening two bone pixels in five and
  wiping out the shading by hand.
- **Eyes for everyone.** Every creature that has eyes has them bright and visible from anywhere, in any light and the
  thickest fog: drawn a second time over the body after the fog, so a pixel lost to distance, frost or a glitch comes
  back; far off a point (a pair never closer than two pixels), nearer a small square with its rank's form round it;
  their colour by kind (a skeleton's pale, the wall skull's embers, the Whipper-In's amber). The one exception is the shade,
  which shows only in light, eyes and all, as it always did.
- **Rank beyond colour.** A rank still recolours the body, but no longer drowns it: bone stays bone in its lights, and
  the rank sits in its hollows (rotten, cold, bloody, charred, gilt). Over it a layer of dressing by rank, drawn for each
  kind in its own material: at II rags and mould, at III frost and a chip or a notch, at IV old blood, an arrow, a cut,
  at V char, an iron crown, a shackle and chain; the gilded a death-mask and gold leaf.
- **Spirit vapour.** From rank III a thread of pale blue from the eyes; at IV a red-orange plume from the eyes and chest
  with falling embers; at V a violet-black cloak standing behind it with a pale rim; the gilded, gold motes circling.
  It is drawn, not lit, and kept within a budget so a crowded hall stays readable. (Gathering into the soul's mote at a
  death is still to do.)
- **Proportions.** Every creature re-proportioned to the owner's word (A4): a head a quarter of the body or less (the
  first skeleton, at one to three and a half, read as a cartoon).
- **Side views** for walking across your line, mirrored for the other way.

### The creatures, one by one

- **The skeleton** (with the skull master): a neck, the shoulders two pixels lower and the right one lower still; fallen,
  it lies in profile. II rags and mould; III frosted rags and a notch out of the crown; IV bloodied rags, an arrow in
  the ribs and a cut brow; V charred, a broken iron crown, a shackle and chain, a torn cloak behind; gilded a
  death-mask, gold leaf, a clean shroud and a gold chain. **The shield-bearer** shares it, its blade, guard and shield
  moved down with the arm.
- **The wall skull:** the master skull three times over, with fire filling the sockets, then the mouth, the nose and the
  cracks as it wakes; asleep, ember eyes that show at thirteen cells in the dark; broken, split with its right half
  dropped. II moss, III frost, IV tears of old blood, V char and an iron crown, gilded gold and a mask.
- **The ember imp:** no pot belly any more: a narrow chest, wiry arms to the knee with talons, reverse-jointed legs,
  ragged bat wings, a tail with an ember; a horned, torn-eared, long-muzzled face with slit eyes and a lit grin of
  needles, the right horn broken. V black crust veined with fire under an iron collar.
- **The gargoyle:** slab shoulders, a hunched chest, wings folded high round it on its plinth like a cloak and spread and
  torn in flight; perched, its eyes are blank stone and nothing shows in the dark; rain stains from the eyes and mouth,
  lichen in the hollows, the cracks alight once awake. V black basalt veined with cold light.
- **The Drowned,** drawn again from nothing: a gaunt, too-tall frame round a belly swollen tight with veins and a
  splitting seam; a long face, hair plastered down it, small cold eyes, a slack black mouth; the Order's shroud sodden
  down its side, a rope round the ankle to a stone, weed, barnacles, drips. Its skin is its own ramp now, never the
  walls' stone.
- **The shambler:** the buried: sunken sockets with a sulphur light, the nose rotted to holes, the jaw dropped on a few
  yellowed teeth; its shirt now burial linen wound in bands, grave-dirt clotted on it, roots out of the tear in its chest.
- **The ghoul:** starved, the spine beaded and the shoulder blades jutting, long arms with knobbed elbows and claws twice
  as long; close-set eyes deep in their sockets, slit nostrils, lips peeled back from fangs; feeding, its mouth bloodied.
- **The bone hound:** its bones kept ("pretty good"), its skull drawn by hand with a long muzzle, a deep orbit and a
  canine hanging, a broken chain from its studded collar; late in a run its eye goes cold.
- **The skeleton knight:** a great helm by hand, a ridged crest and a plume's stump, a cross-shaped slit with a crimson
  light in it; the master skull under it once it is hacked off; the Order's tabard under the plate, torn at half plate,
  gone when stripped.
- **The deacon:** a peaked hood in the lancet's shape, a small skull dim in its mouth with two lights for eyes, the staff's
  skull lit, a rosary of knuckle-bones.
- **The spectre:** a face of its own at last, so it no longer reads as a small Wraith: long and drawn, the brow knitted,
  the mouth dragged open; hands of mist; its shriek flings its arms up and flares the shroud.
- **The Wraith:** its hood fallen to the shoulders, its mouth a lancet of nothing with the two lights in it, its body
  wound in cerements that come loose below the waist into strips and wisps; thinner grey hands with frost on the nails.
- **The bat:** ears taller than its head, a leaf of a nose, two fine fangs, eyes like coals, a furred barrel of a body,
  boned and scalloped wings; dead, its wings fold about it. The latched bat wears its rank's marks too.
- **The web spider:** seen from a standing height, eight eyes like coals, pale fangs with venom on each, its abdomen a
  pitted shell with an hourglass in blood.
- **The hunting spider:** a wolf spider's face, two great eyes shining, a furred body with a dark stripe and chevrons.
- **The brood mother:** the web spider's kind at the size of a dog, her abdomen stretched over sixteen pale eggs with
  veins of blood between them; the sac wound in silk; spiderlings.
- **The shade:** checked and left as it was: the exception holds.
- **The three new kinds** (Part 3) were drawn in this pass's hand before anything of them went into the game, and
  approved by the owner as drawn.

### The bosses

- **The Ill-Fated Knight:** the owner's fallen paladin: a crown of iron spikes about the great helm, a torn crimson plume,
  the Order's cross inlaid in bone on the breastplate, its gilt worn, chains wound across the plate, a ragged crimson
  cloak and surcoat, great spiked pauldrons, a greatsword with a ruby in its pommel. Its plate comes off by waves as it
  did, its left side bared at half plate, and with the helm gone a crowned skull with a light in its sockets.
- **The Wandering Spider:** the owner's old matriarch: grizzled fur gone grey-white at the tips, two great burning eyes
  and six milky blind ones, the master skull on her back in raised bone, old claw marks across her abdomen, spurs at the
  knees, a hind leg snapped. Her two great eyes keep their fire in the fight, at the owner's word.
- **The Black Deacon:** the owner's profane bishop: a black chasuble lined in crimson and worn gilt, an inverted cross in
  bone on the orphrey, a tall black mitre, the master skull for a face lit green from below, a crozier of black iron with
  a small skull in its crook, a censer swinging its smoke. He is drawn in his own sprites at twice a deacon's size.
- **The Wraith Unbound** waits: it is the pass's next boss when it is taken up again.

### The caster's hand

Asked for on its own: the traced hand now moves. An idle of three frames while it is up between casts, and a cast of
four (drawn in as it rises, a claw while a spell gathers or a charge is held, flung open as it goes, easing after). The
bitmap traced from the owner's photograph is not redrawn: each frame bends it at the knuckles, so the sleeve, the ring,
the bracer and the tattoos come along; the spell forms at the frame's fingertips.

### The props and pickups of runs I and II

At the owner's answers on 1 November, every thing that stands, hangs or lies in the Crypts and the Catacombs, in the
same hand:

- **Bones and skulls:** litter in three looks, heaps in two, skull piles in three (an ossuary stack with a cross in red
  ochre, a loose pyramid, a pile under a burnt-out candle), bone totems with candles burning inside the skulls.
- **Lights:** church candles in their own pooled wax (one guttering, a stub gone out), candles on a skull's crown, an iron
  pricket; run I's twisted iron candle stand; an iron corona chandelier on four chains with a skull hung at its middle;
  fire pits of dressed stone with charred logs glowing; braziers; horn lanterns.
- **Graves:** headstones with a winged skull in relief or an hourglass cut in, ringed and trefoil crosses, obelisks and
  draped urns, broken stumps; carved work raised and lit, cut work dark with a lit lip; lichen, moss and rain streaks.
- **What hangs:** forged chains from ring bolts with a manacle open and one shut on a hand's bones; a chain slung with
  leg irons; butcher's hooks on an adzed beam with dried meat on one; a gibbet of riveted straps; a bronze bell; roots
  through the ceiling with the soil falling; cobwebs with a dead spider in one.
- **Stone and floor:** clustered gothic pillars, one with a prisoner's tally scratched in it, one broken to its rubble
  core; flowstone; an altar stone; cinerary urns; broken statues; a spiked collar; pools.
- **Pickups:** the Order's coin, worn and clipped; a chalice with its wine dried black; gems torn from a reliquary with
  its gilt mount; a crown; a chest; a phial; a soul urn; a ring on its cushion; the spear's bone.

Not yet: runs III and IV's own props (the Hollows' mine lamps, carts, timbers and fungus; the Deep's), and the setting
that would turn creature effects down (A14).

---

## Part 2. The caster

- **The sprint.** Hold Shift (or, on a controller, press the left stick, which toggles it; any other button can be
  chosen) and you run forward at half again your pace for two and a half seconds, more with the new **Sprint** stat on
  boots and its line in the points. A slim meter shows over the status bar while it isn't full. Run dry and you can't
  sprint again until you let go; it waits, then fills the whole meter, sooner the more health you have past 200 (the
  owner's choice). A dry breath when it runs out. Sprinting is louder than walking (Part 3).
- **Physic.** A new stat on robes and amulets, and a line of its own in the points: what a potion restores, multiplied,
  up to five times.
- **The Lantern of the Dead,** with that cornerstone, is shuttered or lit by holding L for a little over half a second;
  a tap opens the wisps' list as before (with *L always casts this* on, a tap casts and a double tap opens the list),
  and the list has the lantern's row. A small lantern on the portrait's corner says which.
- **Places to mend.** One a level (two from the Hollows), set in a room's wall facing its way in and drawn in each run's
  stone. Press E facing one and stand still: your health comes back, to three quarters of it (all of it with Physic at
  its cap), while you keep still and face it; moving, turning away, casting, a blow or E again ends it. It is heard far off, so the dead may come to see.
- **The controller's sprint button** is chosen from Esc's question (a sixth row while a pad is in: press A, then the
  button); the settings dialog shows it too.

---

## Part 3. How the dead think

Planned on 1 November (`docs/PLAN-ai.md`), settled with the owner over two rounds, and built in ten steps, each checked
against the old minds on the same build and the same seeds (the old ones are kept behind a debug switch). The same minds
think for the dream and for a person.

### What they sense

- **Sight** has a facing now: about nine cells within a third of a circle ahead, two and a half behind; a hound smells
  four cells round it; bats see all round. In the dark (when you stand in little light and have cast nothing for a
  moment) the living see five cells, and the shade the other way about. A creature caught unaware, struck from outside
  its view while calm or wary, takes half again and staggers.
- **Hearing:** a footstep three steps (five sprinting or in water, less with Quiet), a cast eight (the spear five,
  Hellfire and the ultimates ten, the BFS9000 the whole level), a blast where it lands six, a door four, a secret wall
  eight, a death four (eight to its own kind), a blow on you five, a place to mend sixteen; all measured along the halls,
  not through the walls.
- **States.** Calm; wary (it stops, turns, goes to look at half pace, and calms after eight seconds); hunting; tracking
  (to where you were, and a little on the way you went); searching (places round where it lost you, rooms first, fifteen
  to twenty-five seconds, then calm but edgy for half a minute, hearing farther). So the dead forget you now, and can be
  lost.

### What they do when calm

- Residents and a vault's guards **keep a post**, facing the room's way in, their heads turning, and walk back after a
  search. Bats and hunting spiders **roam** room to room. The rest **walk a round** of three or four places and pause at
  each; some walk it in company.

### Packs

- A pack has a rank, its leader's. One that sees you (or is struck) **calls**, with its kind's tell; the call heard, all
  of it hunts. Kill the caller first and the call dies with it. What one sees, all know.
- **Turns:** its fighters engage one or two at a time; the rest circle at three cells, a hound at four.
- **Broken** (its leader down, or half of it): the living scatter, hounds run to the nearest other creature and bring it
  to where you were, the dead keep their drill.
- **The cap:** only so many fight you at once (two to five by run for the dream, four to seven for a person); the rest
  hold off and throw nothing.
- **The packs placed:** the file of the Order (from level 5: shields leading, throwers a cell behind, a drum), hounds in
  twos to fours (a howl), the roost (from 11: bats asleep upside down on a dim room's ceiling, woken by a noise, a step,
  your light or a blow), the wake (from 13: three or four buried under one room's graves or puddles, rising one after
  another).

### Callers and scavengers

- The wall skull **screams** as it wakes (kill it first and nothing hears), the gargoyle **shrieks** as it comes loose,
  the deacon **calls** the first time he sees you, the roost **cries** as it drops: whatever hears goes to look.
- **Ghouls** hear a death three times as far, go to it and feed (mending, and taking double while they do), in packs of
  one to three past their first level.

### Hunters

- You leave a **trail**, cell by cell, that goes cold in two minutes and is lost in water.
- **The hunt:** on some levels (a quarter of the Crypts' from level 5 up to two thirds of the Deep's; every corrupted
  level; never a boss's), a band waits at the far end and is let loose after a couple of minutes, with a far-off tell:
  the Order's watch goes to your last loud noise and the next; hounds and hunting spiders (along the ceiling) take up
  your trail where you were a minute before, a tenth quicker on it, and hunt you as anything does once they are close.
- **The shade's stalk** (from 16): it holds five cells off, behind you, and strikes only while you stand still, fight
  something else nearby or have a bat on your face; lit and before you, it fades back.

### Lures and ambushes

- From level 7 (on half of the first two runs' levels, two thirds of the last two's; never a boss's) a **kill room**:
  its teeth waiting along the wall either side of its way in, doing nothing until it springs (you two cells in, a tooth
  seen, a tooth struck, the bait slain inside). The teeth by run and room: the Order's shields or the buried in the
  Crypts, the Drowned in a cistern or a knight and his shield in the Catacombs, hunting spiders overhead or ghouls in the
  Hollows, gargoyles on plinths or shades in the Deep. A stinger and *AN AMBUSH* once a run.
- **The bait** (two times in three): a straggler that throws once, a hound that yelps, a bat, an imp, which shows itself,
  backs away before you towards the room, and turns there. Lose it and it comes back to tease; the third time the whole
  set piece comes hunting. Else a glint: a chest or a potion just inside.
- **The signs:** bones, a puddle or rubble by the door outside; a run's first ambush has a torch by its door snuffed. A
  spectre may drift to an unsprung ambush's door and hang there.

### Cunning by rank

- **Green:** alone and hurt, it falls back to its pack; throwers step behind a corner after a throw (three times running
  at most).
- **Blue:** it sees a slow spell coming and steps aside; keeps out of what warns or harms; won't step into a doorway you
  hold with a spell ready, but waits beside it out of sight, eight seconds at most.
- **Red:** the second and later of a hunting pack take another way round to you (from the Hollows' later levels there are
  more loops near the lairs); with a splash in your hand, none comes within a cell and a half of another.
- **Black:** it feints one wind-up in three; it reads a charge and holds off your line.
- **Hazards hurt the dead** now (from the Deep on), at half your share. A thrower lobs over a corner to where it last saw
  you; in a hound pack's turns, one from behind you goes in first. A person doing well, untouched for a while, may find
  one more come in.

### One budget a level

- Every level now has a budget fitted to what the old placements held, so the heads on a level stay within about a tenth
  of what they were, a boss's level half. It is spent first on the vaults' guards and a newcomer's first showing, then on
  set pieces in a shuffled order (the watch, the webs, the buried, the imps, the gargoyles, the brood, the file, the
  roost, the ambush, the hunt, a band), then on posts and wanderers. Nothing is set within six steps of the altar, a
  puzzle's or a trial's door or a place to mend. What killed you on a level comes back a head short; a hunt that killed
  you isn't let loose again.
- **New combinations, one every two levels or so** (`docs/PLAN-ai.md` section 23): the watch and the dead (a wall skull
  with skeletons that hear its scream, from 3); the nest (hunting spiders over a web spider's corner, from 15); the knight
  and his hounds (17); the deacon's congregation (18); the brood's guard (19); the roost and the shade (21); the imps'
  brood at a vent (23); sentinels, gargoyles that wake each other (27); the Wraith's chill, packs keeping to their posts
  while it wails (9 on); bands of two kinds (29) and of three (33); a hunt that drives you into an ambush (37); and what
  you passed by gathering in the exit's room (39).

### Three new kinds

- **The Whipper-In** (from level 22): the Order's huntsman, never without three hounds (four in the Deep); his horn is the
  pack's call and is heard well off; his whip cracks as he sends each hound in; he keeps off and lashes what comes close.
  Fell him and the pack breaks, and his hounds run wild: quicker, careless. Three hunts of hounds in five from the
  Hollows are his.
- **The Sexton** (from 26): never hunts; walks his round and rings his handbell once at each turn of it. Seeing you, or
  struck, he raises it for half a second and rings it hard, and everything within fourteen steps goes to his place.
  Kill him with the bell raised, catch him unaware or freeze him, and it isn't rung.
- **The Lightless** (from 32): black glass with no eyes, which feels light instead: it sees all round, nine cells in the
  light and five in the dark; your lantern lit or a cast's flare within sixteen cells is as good as sight; a wisp or a
  fire on the floor is somewhere to go, and it puts out a wisp it reaches. Quick while it knows, it stands and listens
  when it finds nothing. Glassy chimes now and then.
- Each has its page in the bestiary, and the bestiary's pages now run to the Deep's kinds.

### And

- **Room for sixty-four creatures** a level (from thirty-two), for what is raised, hatched and called.
- **Doors sound like their run:** each run's doors have their own sound as they start to open and as they shut.
- **The dream's counters**, never a person's: it glances back at a sound behind it, reads an ambush's sign and sends a
  fire bolt through the door first, aims at a caller about to call and at a pack's leader before the rest, and holds a
  corner when a pack has it under half its health.

---

## Part 4. The pass over the screens

Planned on 3 November (`docs/PLAN-ui.md`) from five screenshots and the owner's brief (a carved tombstone and marble
look, keeping the colours, too much black; the same letter sizes and text budgets; the skill tree as a current
generation's, an icon to each slot coloured by its line, the tree on the left and a pane on the right), settled the same
day, its sheet approved before anything went into the game, and built in nine steps. Visual only: at every step the
dream and a person's sessions logged the same events, line for line, as the build before, and the pass whole gives the
same as the build it began from.

### The look

- **The marble.** One picture of weathered marble a level (veins, pits, stains), lent to every menu for the frame it is
  up: its floor's and ceiling's colours give way to two ramps of stone, a tenth towards the run's own, and the crypt
  behind keeps its look in the run's stone, dimmed in a four-pixel margin (the owner's choice).
- **Three faces of our own,** at today's sizes: an inscription face for titles at sixteen pixels, a body face for words
  at eight, both inlaid (set into the stone with its shadow), and the BIOS's own. Every menu's words go through them,
  but for paper (the documents' pages and the reckoning's slip keep their own ink).
- **A kit of carved pieces** (`src/crawl_ui.c`, `src/ui_art.c`): ledger stones, headstones and tablets with their
  mouldings; wells cut into them; plates; rules with a lozenge; arched niches; rows lit as chosen with gold notches;
  scroll grooves and thumbs; key caps for the hints (a pad's face buttons in their colours); sockets in six states
  (out of reach, ready, short, part bought, held, closed); grooves that fill; forty-eight icons, sixteen pixels square,
  cut or inlaid; and a pane for whatever is chosen. The owners' signs (a candle, a paw, bones, a key, a rose, an eye)
  are small, in the corners where there is room.
- **The fit checks.** In the debug build every glyph a menu draws is stamped, and one landing on another's ink or outside
  its stone is told; `tools/uishots.sh` takes every screen at each run's palette, with keys and with a pad, and gathers
  what they found. At the end: nothing.

### The screens

- **The Alignment** (the Lampstand, and Tab's fourth page): the line as a tree, its opening at the head, Efficiency and
  Insight, the ability, the two paths down to their ultimates, each node a socket with its icon in the line's colour,
  their grooves filling as they are bought, a node just bought running its colour down its groove; the six lines'
  seals across the top; the pane beside it with the chosen node large in its socket, its name, its kind, its cost, what it
  does and what it waits for. A warning (the other path closing, unbinding) takes the description's place in gold.
- **The cornerstones:** five sockets down a carved bow, the pane wide beside them. **Ascend:** the lines down the left nine
  pixels apart (they had been drawn over one another), each its mark, a dot for a step bought this visit, a scroll's
  thumb; the pane its cost, what is yours, what it does and the next step's worth. **The Hexed Totem:** its four
  abilities in rows of rank sockets, the chosen one's words in a band. **Her question** of what you were: the six marks
  in medallions across the top, the story and the gifts across the stone.
- **Tab's pages:** the wardrobe as the seven worn places on a carved figure (hood, amulet, robes at the heart, the
  bracers, the ring on the right hand, the boots) joined by grooves, the bag's places in a row, the portrait in a bevel,
  the chosen place and its piece in the pane; the caster, the elements and the documents in wells.
- **The bestiary:** the creature standing in an arched niche, lit from above, its ranks as lozenges at the niche's foot;
  the achievements with a groove and thumb. **The Codex:** its parts as small lozenges, its keys on caps.
- **Her hall:** her menu, the ledger (her book on a ledger stone with her paw in its corners), a stone's page (a
  headstone, with roses), the Compendium (a thumb for its scroll in place of *n MORE*), the Repository, the Portal (a list
  that scrolls, so its words no longer go missing), the Rune Shrine (its slots cut as cups), the Dead Bell and the
  Weeping Font (a tablet), the full bag.
- **The crypt's:** Esc's question a headstone, the question in its arch; the L list, the altar and the Black Shrine
  tablets; every card (a level's, a boss's, death's cost, the old mercy's, her claim's) a headstone; the tally a memorial
  tablet, its head on a plate, a carved line for the leaders, each row's notes on a line of their own under it.
- **The HUD,** in the run's own stone: the status bar a tablet with its plates sunk as wells and the souls' line in a
  groove; every word over the crypt (the target's readout, a boss's bar, the hourglass, the news, the readout, the
  experience, the toasts, the cards in her room) on a narrow band of dark stone in place of a black box; the sockets
  wells. No figure moved.

---

## Part 5. Faults found and mended on the way

**In play** (found in the dream's long runs; mended for both minds):
- A brood mother held back by the cap went on laying sacs until spiderlings filled every place on the level.
- A stope's firedamp, which hangs until fire sets it off, was marked as harm, so the dream waited at the doorway it
  filled until it gave the level up; a censer's swing kept the lane under it marked the whole swing, so the dream never
  crossed. Both now read as what is always there.
- A green thrower could peep at the dream from behind a corner for ten minutes; now three covers running at most.
- A creature whose way was stood in (a post in a hall a cell wide, a statue) had waited there for ever; it gives the
  place up after three seconds. This had been common, not rare: it fired about ten thousand times in a batch of runs.
- A pack broken while its call was under way stayed calling, and the dream's leaders-first aim held on its caller from
  any side for ever.
- The places for blasts were doubled to sixteen (they filled four times in eight runs); the dead's missiles stay at ten,
  since doubling them made the knight's level deadlier with either mind.

**On the screens:**
- Ascend's rows were drawn over one another; the tally's notes ran into its figures, and its CORRUPTED line sat below the
  stone; a stone's page let a long class (WITCHFINDER) run into its runes; the Portal cut its words to one line with three
  runs open, and its longest rows ran into their figures.
- The controls card lay over a ware's card in her room, and over death's card and her claim's for a person who died in
  their first seconds; a toast came over the controls card, a menu's corner and the tally's title; a level's card was
  drawn under a menu opened in its first seconds.

**In the harness:** `host apl` never played `HOST_INPUT`, so one check of a person's session (the pass's step 5) had
compared two dreams; it plays it now, and that check was made again under `shot` and holds.

---

## Part 6. The page and the README

- **Then and now:** a new section of single frames, the game's own 320 by 200, from the first beta (21 October), the
  thirteenth (31 October) and this one, each taken from its own build by `tools/gallery.sh`: the crypt (level 1, further
  in, her room, the first fireball, the Deep), the screens (Esc's question, the wardrobe, her menu, the Lampstand, the
  Codex, the tally, death's cost), the dead (nine of them in the bestiary), and what is new (the three new kinds, a
  boss's fight, the Totem, another past, a stone's page, the Backrooms). Click one to see it large; the arrows step
  through its set; the preview strips open in the same viewer.
- The README's ten previews that show a menu were taken again; their descriptions on the page say what they now show,
  and which frames are older.

---

## Part 7. For whoever tests it

New debug switches (the debug build only): `CRAWL_MIND=0` (the old minds), `CRAWL_PACK=k`, `CRAWL_HUNT=s`, `CRAWL_HUNTK=k`,
`CRAWL_AMBUSH=1`, `CRAWL_AMBT=k`, `CRAWL_CUNNING=r`, `CRAWL_TPHOLD=k`, `CRAWL_SPLASH=1`, `CRAWL_WELL=1`, `CRAWL_BUDGET=f`,
`CRAWL_KINDTRACE=k`, `CRAWL_HUNTTRACE=1`, `CRAWL_RUNID=n`, `CRAWL_TPMEND=1`, `CRAWL_HP=n`, `CRAWL_STAGE=k` and its
companions, `CRAWL_HANDFRAME=k`, `CRAWL_TPSHRINE=1`, `CRAWL_STONE=n`, `CRAWL_LETGO=1`, `CRAWL_HOLDCARD=1`, `CRAWL_CLAIM=1`,
`CRAWL_TOAST=1`, `CRAWL_SEENALL=1`. Tools: `tools/stage.py` (one creature down a hall at each distance), `tools/uishots.sh`
(every screen, `PAD=1` for a pad, `HOST=` for another build), `tools/gallery.sh` (the page's gallery), `build/uisheet fit`.
`STALL` lines in the debug log catch the dream standing still.

---

## Part 8. What has been tested, and how

Everything in the harness only. The Windows build compiled and smoke-tested by GitHub Actions on every branch.

- **Nothing moved that shouldn't have.** The art pass and the pass over the screens are visual only: each of their units
  and steps logged the same events, line for line, as the build before, in the dream and (for the screens) in scripted
  sessions of a person; the pass over the screens whole gives the same as the build it began from, in three dreams and
  six sessions. ASan (address and undefined-behaviour sanitisers) was clean over every unit and step, and at the end over
  every screen at two palettes with keys and one with a pad (113 sessions).
- **The minds** change how the game plays, and were weighed against the old minds on the same build and the same seeds
  (sixteen of them by the end; eight seeds' noise had been found to be a quarter either way). The dream from level 1,
  100 minutes a seed: a median of 9 levels cleared either way; with the new minds 56 deaths (33 on the knight's level,
  23 elsewhere) and the knight felled 7 times, against the old minds' 47 to 53 deaths a batch (36 or 37 on his level, 11
  to 16 elsewhere) and 5 or 6. So about a sixth harder for the dream in all, and the ordinary levels harder, as the owner
  asked; the knight's level is the noisiest number here, running from 28 to 49 deaths between builds that don't touch his
  fight. From levels 11, 21 and 31 (50 minutes a seed): no deaths in the last batch, medians of levels 17, 27 and 37
  reached, as before. A level takes about the same time (a median of six minutes).
- **How often the new ways show,** in the dream's runs: thousands of noticings and hearings, hundreds of searches given
  up, of packs calling, taking turns and breaking; hunts let loose on 25 levels in sixteen runs from level 1; ambushes on
  40 levels there, most found or shot from afar by the dream (an ambush is a person's trial more than the dream's); from
  the Deep, hundreds of dodges, flanks and feints. The dream seldom stands still, so the shade almost never strikes it;
  to a person who stops to look about, it will. The cap passed for a person doing well was tried only with its switch.
- **Frame time** rose a little with the HUD in stone (about a tenth of a millisecond, against a frame's fourteen at 70
  frames a second).

---

## Part 9. Known issues and what is still to come

- None of it has been played by hand or seen on a real screen. The new art (`media/sheets/artpass/review/`) and the new screens
  (`media/sheets/uipass/`) still wait for the owner's eye on a real monitor.
- Two of the dream's long runs stood within a cell of one place for about four minutes, casting at nothing it could kill,
  near where a Wraith had just been driven back. One cause was found and mended (the broken pack still calling); whether
  that was all of it isn't known. Runs by a sluice have stood still too (up to five and a half minutes, rarely).
- The knight is still the dream's wall on level 10.
- In a Backrooms pocket, a menu turns the four-pixel margin's yellow grey while it is up.
- Still to come: the art pass's runs III and IV's props and the Wraith Unbound; the setting to turn creature effects
  down; showing the minds (their sounds, captions, the bestiary's lines, the Codex, achievements); the lower storey, its
  edges, stairs and ladders (planned as 1.0.0b16).

---

## The owner's answers (3 November)

1. **The number:** 1.0.0b14. The plans' later numbers move down one: the minds' showing and the rest of the art come
   next, and the lower storey stays planned as 1.0.0b16 until it is renumbered.
2. **Latest:** a pre-release, as before, with nothing pinned; the workflow is unchanged.
3. **This document:** release notes cut to what a player needs to know, without the background work, the technicals or
   the debug switches; this record stays in `docs/`.
