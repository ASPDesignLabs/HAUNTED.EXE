# The first art pass: faces, eyes, wear and spirit (1.0.0b14)

**Status: settled (31 October 2026): the owner answered every question the same day (see *Decided* below), and work has
begun on `claude/dazzling-dirac-20cr2y`. Section 13, *Where it stands*, is the log of what is built and the notes for
whoever picks it up next; read that first.** The owner looked at the creatures in 1.0.0b13 and found many of them basic:
the skeletons' bones are good, but their skulls, the wall skulls, the heads of the imps and gargoyles look "almost like a
child's drawing", and the Drowned look goofy where they should frighten. This plan is the art direction for putting that
right, written for whoever builds it (it is meant to give them something firmer to work from than "make it more
gothic"). Read it with `CLAUDE.md`, [`PLAN-bestiary.md`](PLAN-bestiary.md) (the creatures, their ranks and the colour
ladder) and [`PLAN-b10.md`](PLAN-b10.md) (how each run's look was drawn and approved: sheets first, then frames from the
game).

**Numbering.** This pass takes the place of 1.0.0b14. The lower storey, its edges, stairs and ladders (steps 9-13 of
[`PLAN-b13.md`](PLAN-b13.md), which still calls it 1.0.0b14) move to **1.0.0b15**, untouched. `CLAUDE.md` says so.

## Decided (31 October 2026)

The owner's answers to section 12's questions. Where an answer differs from the recommendation, the sections that
follow are read in its light (the differences are in bold).

- **A1** (b): every skull the game draws: the wall skull creature, the piles, heaps, totems, niches and the ossuary's
  bone walls.
- **A2 (c): all four bosses get bespoke redraws in this pass**, not only the Deacon and the Wraith; their new art is
  drawn after their kind's own (section 9).
- **A3** (a): no dressing by run in this pass; the kit and the table of points are built so it can come later.
- **A4 (c): every creature is re-proportioned**, not only the Drowned, the imp and the gargoyle. The skeletons' bone
  work (the owner's "pretty good") is kept in spirit: limbs may lengthen and the stance change, but the bones are drawn
  as they are drawn now.
- **A5** (a): heads may grow by up to a quarter.
- **A6** (b): a rank tints the shadows and mid-tones; the highlights keep the material.
- **A7** (a): vapour from rank III (a thread), IV a plume, V a cloak, gilded motes.
- **A8** (a): breakage, rot, rust, frost and old stains; no wet viscera.
- **A9** (a): the gilded are the Order's honoured dead: gold leaf and a death-mask over the same body, gold motes.
- **A10** (a): eyes are hidden by walls. **A11** (a): sleepers and statues are dark or ember until they wake.
  **A12** (a): the kind's own colour at rank I, the rank's from II. **A13** (a): eyes flare when alerted and striking.
- **A14** (a): a setting, *CREATURE EFFECTS: FULL, CALM, OFF*; the eyes are not touched by it.
- **A15** (a), the owner's: **sheets for each step, then frames in the game, and the work done creature by creature,
  then each run's assets**, so that updates and reviews stay small and the work can be handed on between sessions if
  usage runs out mid-way. Each creature is one unit: drawn, put in the game, committed and pushed, and logged in
  section 13 before the next begins.
- **A16**: greyscale legibility is enough for now.
- **A17**, the owner's: 1.0.0b13's content was built; **the lower storey moves to 1.0.0b15 and this pass is 1.0.0b14**.
  `PLAN-b13.md` was renumbered to match (a note at its head says so) and `CLAUDE.md` points here.

## What the owner asked for (31 October 2026)

1. **Better artistic direction for the builder** around adjusting many of the art assets. The NPCs "look pretty basic".
   What is good: the bony structure of the skeletons. What is not: their skulls, the wall skulls, the heads on the
   imps and the gargoyles ("incredibly primitive"). The Drowned look "incredibly goofy, not at all scary".
2. **A more gothic presentation, throughout.**
3. **One constant: eyes bright and visible from anywhere, whatever the light around them, for every NPC that has
   them. Shades are the only exception.**
4. **Rank told in more than colour.** Tattered appearances, battle damage, "some kind of spirit vapour emanating from
   higher ranks, coloured if it helps add variety".
5. **No code and no assets in this document**: a plan, structured, with questions, and rich enough in its
   descriptions to draw from. Flavour text is welcome wherever the plan runs short (the last section asks for some).

## 1. What the game does today

Read from `src/crawl_art.c`, `src/crawl.h` and `render_sprites` in `src/scene_crawl.c` at 1.0.0b13, and from the
frames in `docs/preview_catacomb_creatures.png` and `docs/preview_catacomb_bestiary.png`.

### 1.1 Why the faces look like a child's drawing

- **Heads are built from the same few primitives.** The shared skull (`cv_skull`, used 29 times: skeletons,
  shield-bearers, the deacon and his staff, bone heaps, skull piles, totems) is two smooth ellipses (dome and jaw), two
  flat ellipses for sockets, one pixel for a nose and a row of five for teeth. On a standing skeleton it is about 12
  pixels across in a 64-pixel sprite. The imp's head is one ellipse about 19 pixels wide, two eyes of two pixels each
  and a parabola for a grin. The gargoyle's is an ellipse 13 pixels wide with a smaller one for a muzzle, and **one
  pixel per eye** (none at all when it is perched). The wall skull is a dome, a jaw, two flat sockets and a triangle.
- **Nothing has a brow, a cheekbone, a temple, a nostril or a chin.** The shapes are filled with smooth gradients, so
  two planes never meet at an edge for the light to catch; the result is plastic and egg-like. And every pair is
  mirrored exactly about the middle. That is what a child's face is: a list of features set on a blank oval.
- **At the sizes in play the head is 6 to 20 screen pixels.** A feature that is not placed by hand at that scale is
  lost, and a feature built from a procedural ellipse is never quite where it should be.
- **The Drowned are goofy because of their proportions**, not their rank colour: a belly 29 pixels across (over 40% of
  the sprite's width), short thick legs, a navel, a small round head sunk between the shoulders, puffed lids with
  "milky" patches, barnacles as scattered dots and weed for hair. It reads as a toddler or a balloon figure. And it
  is drawn in the walls' own stone ramp, so it is also the colour of the wall it stands against.
- **Heads are the same size at every rank and in every set.** Nothing about a face says how old, how hurt or how
  hungry it is.

### 1.2 Why eyes are lost

- They are baked into the sprite as one or two pixels: a skeleton's two single pixels (`R_FLAME + 10`, or cold blue
  `R_GLOW + 13` past a decay of 0.55: "the eyes go cold"), the gargoyle's one, the imp's two, the deacon's one each in
  his hood. The Drowned's are not bright at all: patches of the ghost ramp, which darkens with the light.
- Full-bright ramps (`R_FLAME`, `R_GLOW`, `C_WHITE`) escape the lightmap, but not the rest. **Fog** dithers any pixel,
  bright or not, into haze (up to 70% of them). **Minification** samples the sprite by nearest texel, so at a distance
  a one-pixel eye falls between samples and goes. **Translucency** (the spectre, the Wraith in the dark) dithers them
  with the body, and **frost** and the **Backrooms glitch** overdraw them.
- The only creature with a proper eye pass is the web spider waiting in its web: `DR_EYES` draws two red points in
  screen space, straight into the frame, behind nothing but the z-buffer, blinking out for a moment every few seconds.
  **That is the precedent, and it works; it is simply special to one creature.**
- Rank does not touch the eyes. PLAN-bestiary left "rank shown in the eyes" for the renderer; it was never done.

### 1.3 What a rank is today

A rank is a recolour: `crawl_rank_lut` redraws a creature's whole body ramp in another (II **ROTTEN** green, III
**COLD** pale teal-grey, IV **BLOODY** red, V **BLACK** near-black, **GILDED** gold; bone that is already dark goes
pale instead, plate darkens), and `RANK_SIZE` makes it 4% bigger a rank (V 16%, gilded 8%): "a second cue, for eyes
that mix up the colours". The strength behind it:

| Rank | Health | Blow | Pace | Souls |
|---|---|---|---|---|
| I | x1 | x1 | x1 | x0.5 |
| II | x1.6 | x1.15 | x1.05 | x1 |
| III | x2.4 | x1.3 | x1.1 | x1.5 |
| IV | x3.5 | x1.5 | x1.15 | x2 |
| V | x5 | x1.7 | x1.15 | x3 |
| Gilded | x3 | x1.2 | x1 | x3 |

Every kind ranks except the spectre, the Wraith, the shade and the egg sac (rank I only); bats stop at IV; gilded ones
come from level 7, about one level in three. So a rank V skeleton is the same drawing as a rank I skeleton, poured
into a darker ramp and a sixth larger. The previews show the result: a red shambler like a flayed doll, a green
ghoul like a puppet, a blue-grey Drowned like a balloon. The word on the tin is right (rotten, cold, bloody, black,
gilded) but nothing is *done* to the picture to earn it.

### 1.4 How a creature reaches the screen (for the builder)

- Sprites are 64 by 64, column-major, drawn procedurally at start-up (`crawl_art_init`). The skeleton family, the
  shield-bearer, the bestiary's bones and the side views are **drawn again at every level** with `g_decay` (bones go
  missing, crack, yellow and gather moss deeper down), so any new skull must keep taking `g_decay`.
- Per frame, each creature becomes an entry in a depth-sorted list; each pixel is sampled (through the rank LUT),
  shaded by the lightmap and distance, then fogged. Flags change how: `DR_EYES`, `DR_BRIGHT`, `DR_EDGE` (a dithered
  outline), `DR_ICE`, `DR_GLITCH` (the Backrooms), `DR_LIT` (Severance), `DR_FLIPV` (the hunting spider on the
  ceiling), `DR_SHINE` (the knight boss's polish), `DR_COMMUNE`, `DR_ALLY` and others. Several write straight to the
  frame, which is how something glows without a palette entry.
- **The palette has no free entries**: 256, in 16-step ramps (`R_STONE ... R_GLOW`), a gold ramp of 13, white and the
  clear colour. `R_FLAME` (dark red to orange to pale yellow) and `R_GLOW` (deep blue to azure to pale cyan) are
  full-bright. `R_STONE`, `R_FLOOR`, `R_CEIL` are tinted per run and `R_BONE` yellows with depth, so a creature
  drawn in them changes colour with the level. Anything new uses these ramps.
- Eighteen kinds have **side views** (`SP_*_SA`, `SP_*_SB`): the bestiary page draws them large, and
  PLAN-bestiary meant creatures to face the way they walk one day. A new head is therefore **two drawings** (front and
  profile), and the bestiary page is where the new art will be judged most closely.

## 2. Rules for the whole pass

1. **Visual only.** Nothing the simulation reads may move: hit radii, `scale`, speed, health, damage, saves, the
   autopilot's numbers, or the game's random stream (effects take their randomness from the renderer's own hashing, as
   the glitch does, never from the game's `rng`). **Proof**: a same-seed dream run's event log matches 1.0.0b13's.
2. **Keep what works.** The skeletons' bone work and walking silhouettes; every creature's poses and frame counts
   (death, rise, burst, leap, feed, perch and wake); the web spider's eyes-only ambush; `g_decay`'s ageing.
3. **No new palette entries.** Vapour, glints and halos use the existing ramps and are written straight to the frame
   where they must glow.
4. **Sprites stay 64 by 64.** The VGA look is the point; the cure for 12-pixel heads is to spend the pixels better
   (hand-placed, a little larger) and not to add resolution.
5. **Eyes follow section 5**, without exceptions beyond those listed there.
6. **Rank is legible without colour** (section 6): a rank can be told from the next one in greyscale, by the marks
   on the creature and how it moves.
7. **A sensory budget** (section 7.6): slow, low-contrast motion; nothing flashing more than three times a second; a
   cap on how much glows at once; and a setting. Eyes are legibility, not effect, and stay on.
8. **Sheets before art in the game**, each one for the owner's eye, as since 1.0.0b5. New sheet groups for
   `tools/sheet.c` are proposed in section 9.
9. **Honest status.** Tested in the harness only until the owner has played it; `README.md`, the page's *Beta status*
   table and the previews say so, and the two creature previews are taken again for the release.
10. **An initial pass.** Dressing by run (miners' helmets in the Hollows, glass-crusted dead in the Deep), bespoke boss
    redraws beyond the Deacon and the Wraith, and the decorations beyond the skull are questions below, not promises.

## 3. The gothic brief

**What gothic means here.** Not cobwebs and bats. Cathedral and charnel: the Order's crypt as a place made to hold
the dead *beautifully*, and the dead as what became of that care. Thin, vertical, pointed; ornamented and ruined at
once; memento mori carved on the very stone. The creatures should look like they belong in the same building as the
niches, the banners and the Order's marks, as though they were *carved and then neglected*.

**Shape.** Verticals, lancets and spikes: tall hoods with a peak, horns that sweep rather than curl, long fingers,
pointed ears, narrow shoulders under a heavy head. **Nothing is symmetrical**: a dropped shoulder, a head canted,
one arm longer, one eye socket deeper, a tooth gone. Give every creature a line of action (a lean, a hang or a
stoop) so that its silhouette reads at ten pixels tall.

**Light.** One key light from the upper left on every sprite (the walls' lighting is ambient, so this is baked). The
darkest values on a creature belong to its **cavities**: sockets, under the brow, inside the hood, between the ribs,
under the jaw. The brightest belong to a few edges: a cheekbone, a brow ridge, a horn tip. Shade in **bands of four
or five values per material** and break between them with a dither, not a smooth gradient: smooth gradient fills are
what make the current heads look inflated.

**Material.** Each stuff has its own rules, so that bone no longer looks like plastic:

| Stuff | What it looks like |
|---|---|
| Bone | pitted, porous at the ends, stained brown in its cracks, the colour of old teeth; never a clean white |
| Flesh | dead grey and wax, drawn tight over the cheek and brow, loose at the throat; no saturated reds |
| Cloth | a woven edge, frayed hems, stiff folds, stains at the bottom; the Order's colours bleached |
| Iron and plate | dented, rust blooming at the joints, a dull edge-light; a few bright scratches |
| Stone | chipped at edges, mineral stains running down from every recess, lichen in the shade |
| Chitin | glossy only at the highlights, banded joints, hair at the knees |
| Water and damp | dark wet runs, a sheen on the lowest folds, drips |

**Ornament.** The Order is already in the game: the mitre, helm, tonsure, crown, key, wheel, arrows, tower, lamb, lamp,
eye, hand, picks, moon, star and skull (`PZ_MARKS`, 7 by 7 pixels each, carved on the puzzle things). The dead may
wear them, as a tabard's device, a medal, a scratched pauldron or a brand on a brow: it gives them a past and ties
them to the walls. Add prayer-strips tied on with wax, rusted rosaries, funeral coins, manacles and chains.

**Mood.** Dread rather than gore. Damage is shown as breakage, rot, rust, frost and old dark stains, not wet viscera
(question A8). This is a Halloween screensaver that people leave running in a room.

**Restraint.** Bodies keep their own material colours; hue is for the **accents**: eyes, vapour, trim, a crown. That
is what makes a coloured thing worth looking at.

### The child's-drawing checklist

Every head, in every sheet, answers these before anyone shows it to the owner:

1. Is it perfectly symmetrical? Break it somewhere.
2. Is there a brow, with a shadow under it?
3. Are there at least three planes (forehead and brow, cheek, jaw) catching the light differently?
4. Are the cavities the darkest pixels on the sprite?
5. Does the silhouette of the head differ from the body's (a neck, a gap, a crest, a horn) and survive at eight
   pixels tall?
6. Do the eyes sit *in* shadow (a socket, a brow, a hood), not on the surface?
7. Is there one detail that tells a story (a notched crown, a chipped tooth, a stitched lip, a rusted rivet)?
8. At three times, once and a third of the size: still a face?

## 4. Faces

### 4.1 One skull, drawn by hand

Replace the shared skull with a **hand-pixelled master**, drawn as grids of characters in the source in the way
`RUNE_GLYPH`, `PZ_MARKS` and `CLASS_MARK` already are, and stamped in the body's ramp. The reasons are practical: the
artist can *see and edit every pixel*, and at 12 to 16 pixels a hand-placed pixel beats an ellipse every time.
Views: **front** (standing, hunting, throwing), **profile** (side views, the hound), **broken** (jaw hanging, jaw
gone, cracked open, a hole in the brow) and **helmed** (the knight's visor, the deacon's hood). The skull piles, bone
heaps, totems and niches take the same master at their own sizes so that the whole building agrees.

**Size.** A skeleton's skull is about 12 by 12 today. Let it grow by about a quarter (to 15 or 16 pixels tall): a
heavy head on narrow shoulders is the gothic proportion, and the extra pixels are what make a nose and a tooth row
possible. The body's frames are unchanged (question A5).

**Anatomy, as direction** (front view, key light upper left):

- The cranium: a broad dome with a visible brow ridge and **recessed temples**, narrowing to the cheekbones; the
  widest point is the cheekbone, not the dome.
- **Orbits**: squarish with rounded corners, 3 to 4 pixels across at this size, set under the ridge so the top third
  of each is in shadow; the left orbit's rim catches the light, the right is deeper. Nowhere in the skull is darker.
- **Nasal opening**: an inverted pear or heart, two pixels wide and three tall, with a bright bridge above it.
- **Cheek planes**: a lit ridge under each orbit sloping to a dark hollow before the jaw.
- **Teeth**: the upper row set in the maxilla as a lit band, individual teeth 1 pixel wide with dark gaps; **the lower
  jaw is a separate piece with a one-pixel dark seam** between them, and may hang a pixel or two open. Break the
  row somewhere: a missing tooth, a chip.
- One **suture or crack**, from the brow across the crown or down through a socket, stained dark; one chip out of the
  rim. Deeper levels add more through `g_decay`, as the old skull's cracks do.

### 4.2 What each size must show

| Head on screen | What it must carry |
|---|---|
| 6 pixels or fewer (far) | the silhouette, a dark brow band, two dark sockets and **two bright eye points** (the eye pass, section 5, not the body) |
| 7 to 14 (mid) | and the nasal hollow, the tooth band and the jaw's seam |
| 15 to 30 (near) | and cheek planes, cracks, stains, the chipped tooth |
| 40 and up (the bestiary page) | and pitting, sutures and the ornament; the page is where the owner will look hardest |

### 4.3 The other heads

Section 8 has the brief for each. In all of them, **the head leads**: the imp, the gargoyle and the wall skull each
become a *design* (a face with a character) rather than a body with a ball on it.

## 5. Eyes

**The rule.** Every creature with eyes shows them **bright, at least two pixels wide each, from any distance the
creature is drawn at, in any light and through any fog**, for as long as it is present. The Shade is the only
exception, and a few states below (sleepers, the dead) are not exceptions but the moments the eyes begin and end.

### 5.1 How the eyes are made (design, not code)

1. **Two layers.** The sprite keeps eye pixels, in full-bright ramps, so that pages that draw a sprite directly
   (the bestiary, sheets, the previews) still show them. **In the world, a separate eye pass puts them on the screen
   a second time**, in screen space, after the body is drawn and after fog, as `DR_EYES` does for the web spider. So
   a pixel lost to minification, fog, frost, glitch or alpha dither is simply drawn again on top.
2. **A table of eye points per sprite**, not per kind, because a head moves between poses (the skeleton's throw, the
   imp's lob, the gargoyle's swoop, the Drowned's rise). The sprite generators, which know where they put the head,
   record the points as they draw, as `find_glints` finds the gold's brightest pixel today; the table also holds the
   crown of the head, the mouth and the shoulders, which the vapour and the dressing in sections 6 and 7 need.
3. **A floor**: far off (under about 0.85 screen pixels a texel) each eye is one pixel, and a pair is never closer than
   two pixels apart, so it reads as two points with a gap; nearer, each eye is a square of 2 to 4 pixels and the rank's
   form shows round it (5.2). *As built: `render_eyes` in `src/scene_crawl.c`.*
4. **Never dimmed** by the lightmap, fog, depth, the rank LUT or frost. **Hidden only** by walls, doors and other
   solid things (the z-buffer), and by the creature's own absence.
5. **Follows the body's presence, not its light**: a spectre's eyes fade as it fades out, a Wraithwalking boss's go
   when it goes into the stone; but a boss Wraith drawn at a third of its strength in the dark keeps eyes at full. In
   the dark it will be **two lights and a faint shape**, which is wanted.
6. **Rides with the sprite's flags**: flipped with `DR_FLIPV`; shifted with the boss's scale and `RANK_SIZE`; turned
   off under `DR_COMMUNE` (red through walls); left alone by the Backrooms glitch (or nudged a pixel; the builder's
   call, but never sheared away).
7. **A blink** may be kept: no more than a tenth of a second dark, no more than once in four seconds, and not on
   creatures that are hunting you.
8. **Cost**: a handful of pixel writes per creature; the existing 20-creature cap bounds it.

### 5.2 Eye form tells rank (so it survives a small screen)

Colour is the weak cue at two pixels; **form** is the strong one when the eye is big enough to show it, and colour
carries when it is not.

| Rank | Eye form | Eye colour (from II up) |
|---|---|---|
| I | two bare points, the kind's own colour | the kind's own |
| II | a point with a one-pixel glow beside it | sick green |
| III | a short vertical glint, a "cold" slit | cold white-blue |
| IV | a point with a **streak** back along the brow towards the temple, like a guttering flame | red-orange |
| V | a white-hot centre with a **four-armed flare**, slowly flickering | white, a violet rim |
| Gilded | a bright point with a small gold cross-glint | gold |

**The kinds' own colours at rank I** (proposal): the dead's amber (skeletons, shield-bearers, hounds, the knight's
visor in crimson), cold white-blue for spirits and the drowned (spectre, Wraith, Drowned), sulphur for the buried
(shamblers, ghouls), ember red for the eight-eyed and the winged (spiders, bats), white-gold for the fire and the
stone once awake (imp, gargoyle), pale lights in shadow for the priest (deacon). The bestiary pages draw the rank's
eye beside its name.

**State** (optional, A13): an alerted creature's eyes gain one pixel of halo and a steadiness (no blink); one that
strikes flares for a moment. They are never *less* visible when it is calm, only less loud.

### 5.3 Exceptions and edges

| Case | Eyes |
|---|---|
| **Shade** | none in the dark: its eyes show only where the light shows it (the one exception the owner named) |
| Gargoyle on its plinth or in an alcove | **dark, stone**: the statue trick depends on it; they **ignite when it wakes**, with a shed of dust |
| Wall skull asleep | a low ember pair, still visible; they flare as it wakes, fire gathers in them while it charges |
| Shambler and Drowned buried or rising | none while buried; they open as the head clears the ground |
| A creature dying | the eyes **snuff out first**, then the body falls: a clean, readable beat |
| Held in a web, frozen | stay lit; the ice may rime the body but not them |
| Spectre, Wraith | hollow eyes in a pale face / hood, lit; a spectre's follow its fade |
| Spiders | eight-eye cluster, the brood mother's larger; spiderlings' two points; egg sacs none |
| Web spider waiting | unchanged: it is the template |

## 6. Rank beyond colour

### 6.1 The idea

**Each rank adds to the one below it**, so rank can be read by counting what has been done to a creature, in
shape and in motion, as well as by hue. The story to draw from: *the longer a dead thing stands, the more it has
been through, and the more of what keeps it standing is showing.* The five words are already the instruction: rotten,
cold, bloody, black, gilded. Make each literal.

| Rank | The word | Dressing and growth | Damage | Silhouette | Eyes | Vapour |
|---|---|---|---|---|---|---|
| I | (none) | whole, a little grime, a scrap of the Order's cloth or a rusted buckle | a chip or a crack | the kind's own | bare points | none |
| II | **ROTTEN** | + rags hanging from hem and shoulder, grave-dirt, moss and pale fungus in the hollows, a cobweb | + a deeper crack, a missing piece | ragged lower edge | + glow (green) | none (a few spores, optional) |
| III | **COLD** | + frost on every upper edge, icicles under jaw and ribs, the cloth stiff and glittering | + a notch, a split plate | spiked with rime | + slit glint (white-blue) | a **thread** from the eyes, pale blue, rising |
| IV | **BLOODY** | + old blood soaked down the cloth and bone, rust, a torn tabard bleeding colour | + **wounds**: a cleaved brow, an arrow shaft, a hacked limb, a pierced chest | + spurs, bone spikes, a chain | + streaks (red) | a **plume** from the eyes and chest, red-orange, with embers falling off |
| V | **BLACK** | + charred, scorched cloth in streamers, tar, a **broken crown or iron halo**, shackles with a length of chain | + heavy: half a jaw, ribs broken out | tallest and widest; a great torn cloak | + flare (white-hot) | a **cloak** of violet-black smoke, a pale rim, a few white sparks |
| Gilded | **GILDED** | **not wounded: dressed**: gold leaf over the body, a gilt death-mask or collar, a fine chain | clean, polished | ornate, balanced | gold cross-glint | **gold motes** circling, a glint now and then |

**As built (31 October):** the *structural* marks add up rank by rank (rags from II, damage from III, wounds from IV,
regalia at V), so counting them gives the rank, while the *condition* is the rank's own and does not carry upward (II
moss, III frost, IV old blood, V char): a rank V creature is charred, not mossy and frosted and bloodied at once.

Gilded is a branch, not the top of the ladder (its health lies between III's and IV's and its blow between II's and
III's; it runs when hurt and drops treasure): it is the Order's
**honoured dead**, dressed rather than ruined. Its tell is *neatness* in a place where every other rank is messier.

### 6.2 The kit, translated for each material

Each creature draws from one shared kit, so that all of them agree about what "bloody" means.

| Kit | Bone (skeletons, hounds) | Flesh (shambler, ghoul) | Stone and clay (gargoyle, imp) | Chitin (spiders) | Robe (deacon) | Plate (knight) | Drowned |
|---|---|---|---|---|---|---|---|
| Tatters | tabard scraps, strips of shroud | burial linen shreds, loose skin | none (moss ribbons) | torn web trailing | frayed hem, torn sleeve | torn tabard, mantle | sodden shroud, weed |
| Ruin | cracks, chips, lost teeth | gaping jaw, sunken belly | chipped horn, cracked brow | a broken leg, a dented carapace | burnt sleeve | dents, holed plate | splitting skin |
| Wounds | cleaved brow, a rib out | a stitched wound, a bolt | a fractured wing | gouged eye | a slit cowl | a broken blade lodged | the rope's mark |
| Grave-goods | chain, funeral coin, rosary | a coin on the tongue, a wedding band | an iron collar | none | a skull-topped staff, chain | the Order's device | an anchor-weight, rope |
| Growth | moss, mushrooms, frost | roots, mould | lichen, frost, mineral crust | spores, mould | mildew | rust, moss | barnacles, weed, shells |
| Gold | mask, leaf on the ribs | gilt teeth | gold in the cracks | gilt-dusted legs | gold thread, gold stole | gilt trim | gold in the weed |

### 6.3 Reading it

Three kinds of cue, so that no one of them is load-bearing: **shape** (how many marks and how spiked the
silhouette), **motion** (vapour's speed, the eyes' flicker) and **colour** (the vapour, the eyes, the trim). The
test in section 10 is that a rank V creature and a rank II creature of any kind can be told apart in greyscale at a
glance, and that a rank III and a rank IV can be told apart with their colour taken out. The existing `RANK_SIZE`
cue stays.

### 6.4 How it is layered (options for the builder)

- **A. Bake it into the sprite**, through a `wear` parameter beside `g_decay`, for every pose and rank. Simplest to
  draw; but the skeleton family is redrawn at every level, so five more variants of it would multiply that
  load-time cost, and body decay and rank wear would run together.
- **B. A dressing layer**: for each kind and pose, a second 64 by 64 picture per rank (II, III, IV, V, Gilded), `C_CLEAR`
  where the body shows, composited by the sprite loop at the same texel. This is how `T_FEAT` works on walls and
  how the marks on the puzzles' things work. The base keeps its per-level decay; the dressing is drawn once, at
  start-up. About 15 ranking kinds x 6 poses x 5 ranks is ~450 layers, ~1.8 MB.
- **C. Hybrid**: B for cloth, gold, growth and wounds; baked into the base only what changes the body itself (a jaw
  lost, a horn broken).

**Recommendation: B**, with the layers drawn cumulatively by one kit per kind, so rank V draws everything rank IV
does and more, and a rule that the rank-I base stays clean. Measure `crawl_art_init` and the per-level reload before
and after (section 10), and fall back to C if the reload grows.

### 6.5 What happens to the body colour (question A6)

The current LUT turns a bone skeleton green, blue, red, black or gold *all over*, which is why they look like
painted figures. If the dressing, the eyes and the vapour carry the hue, the body does not need to: the options are
to keep it, to let only the shadows and mid-tones take the rank's tint so the bone still looks like bone (the
recommended middle), or to drop it. Whatever is chosen, `RANK_SIZE` stays and the bestiary's rank word keeps naming
the colour.

### 6.6 In the bestiary

Each rank gets one line under its name, in the plain dry voice, naming its marks, so the ladder can be learnt:
for instance **ROTTEN**: *RAGS, MOULD AND A GREEN LIGHT IN THE EYE.* **COLD**: *FROST ON THE BONE, AND A BREATH OF
IT.* **BLOODY**: *OLD WOUNDS, AND A RED SMOKE FROM THEM.* **BLACK**: *BURNT, CROWNED IN IRON, SMOKING.* **GILDED**: *THE
ORDER'S HONOURED DEAD. THEY RUN WHEN HURT.* These are samples to be written properly in step 9.

## 7. Spirit vapour

### 7.1 What it is

**The soul showing.** A creature with more spirit in it than its body can hold lets it out. This agrees with the game:
rank decides the souls a kill gives (x0.5 at I to x3 at V and gilded), a kill leaves a mote ("a pale flame with a
face in it"), and the soul economy of 1.0.0b11 makes how much spirit a thing carries worth knowing at a glance.
So vapour grows with rank, and **at the creature's death it gathers into the mote** (a short beat: the eyes snuff,
the vapour streams to a point at its chest and the mote lifts out).

### 7.2 When it starts and how it grows

| Rank | Vapour | Colour |
|---|---|---|
| I, II | none (II may leak a few green spores) | green |
| III | a **thread**: one slow wisp from the eyes | pale blue, a frost glitter at its tip |
| IV | a **plume** from eyes and chest, with falling embers | red-orange |
| V | a **cloak**: a standing column behind and around it, a pale rim, a few white sparks | violet-black with a violet edge |
| Gilded | **motes** circling, a glint now and then | gold |

(The question is whether it should begin at II: A7.) A black vapour is invisible against a dark crypt, so it is drawn
as **a darkening of what is behind it** (pixels pulled down a few steps within its shape) with a bright violet rim, so
it reads against both lit and unlit stone. Its colour is shown over ramps that are written straight to the frame, so
a coloured vapour in a dark room glows without needing a palette entry.

### 7.3 Where it comes from, by kind (the variety)

| Kind | Emission | Style |
|---|---|---|
| Skeleton, shield-bearer | the eye sockets and the ribcage | rising threads, curling |
| Bone hound | eyes and jaws | a breath plume streaming back as it runs; eye-trails |
| Shambler | mouth and any wound | damp steam, slow, clinging low |
| Ghoul | mouth | hunger-smoke curling forward when it leaps and feeds |
| Drowned | the mouth, the skin's seams, the feet | water and gas: vapour that **drips and bubbles** |
| Knight | the visor slit, the gorget and plate joins | **jets** of steam, pulsing with its stride |
| Deacon | the cowl and his staff's skull | a slow cowl-smoke, motes circling the staff; thick when he chants |
| Imp | shoulders and back | heat-shimmer and sparks (it *is* the fire; its vapour is heat) |
| Gargoyle | the cracks in the stone | a **dust-and-light seep** rather than smoke, coming from fissures |
| Spiders, brood mother | joints and fangs | spores and venom-drips, thickest on the brood mother |
| Bats | wing-tips (to rank IV) | a trailing mist behind the wings |
| Spectre, Wraith | (they are spirit) | none; a rank-less creature needs nothing added |

### 7.4 Drawing it

- **Dithered translucency**, the engine's own idiom (the Bayer matrix), so it matches everything else on screen.
- **Written after the body, with the eye pass above it**, so vapour never covers an eye.
- **Slow**: drift under about half a cell a second, nothing sudden; the same few hundred hashed particles each frame
  from a seed per creature, no state to keep and no game randomness.
- **Fogged as the body is** (it dims with distance), never drawn through a wall.
- **Anchored to the table of points** (section 5.1.2), so it comes from the right place in every pose and follows a
  swoop, a leap, a lob or a rise.

### 7.5 What it is not

It does not hide the creature, change what a spell hits, or count as a hazard. It is never a cue the simulation reads.

### 7.6 The sensory budget

This game is looked at by one person in a dim room for a long time, and may be left running. So:

- Motion is **slow and low in contrast**. No pattern flashes more than three times a second, ever.
- **A cap on what glows at once**: the nearest handful of creatures get full vapour; farther ones thin to a thread,
  then to nothing, and ten creatures in a hall do not become ten bonfires.
- **A setting** in the existing settings dialog (question A14): *CREATURE EFFECTS: FULL, CALM, OFF*. CALM halves the
  vapour and slows it; OFF draws none. **The eyes are not an effect and stay on at every setting**: they are the
  game's legibility.
- Vapour and glow must not carry meaning that nothing else carries. Rank is also shape (6.3); a creature that is
  about to strike is also its pose.

## 8. The creatures, one by one

Each entry: what is there now, what is wanted, and what is special about its ranks, eyes and vapour. They are
proposals for the owner's eye. Tier A is what the owner named; tier B is the rest of the faces; tier C is the lighter
touch.

### Tier A: what the owner named

**The skeleton and the shield-bearer** (and their remains, `SP_SKEL_D*`). *Now*: good bones, a plastic skull.
*Wanted*: **keep every bone, change the head**: the master skull of 4.1, bigger, brow-heavy, a chipped tooth, the
jaw a little open when it throws; the *hang* of the body kept. A shield-bearer's helm-less head sits lower under the
shield. *Dressing at I*: a scrap of the Order's tabard on one hip, an iron buckle, one hand with a ring-bone. The
walking pair, the winding-up throw and the three-frame collapse all carry it. *Ranks*: as table 6.1: moss in the
ribs (II); frost on the shoulders and a rimed shroud (III); a cleaved brow and blood down the tabard (IV); burnt,
crowned in a broken iron band, a shackle's chain trailing (V); a gold death-mask and leaf on the ribs (Gilded).
*Eyes*: amber, from sockets so deep that only the points are seen; the vapour leaves through them.

**The wall skull.** *Now*: a dome, a jaw, flat sockets and a triangle; drawn at 0.62 of a body. *Wanted*: a **hero
asset**. It never moves, so it can have more detail than anything else in the crypt: **a carved skull in a pointed
arch**, half bone and half the Order's masonry, a keystone for a brow, tracery on its frame, a crack that runs like a
vein. It should read as *something built to watch* (the Order's watcher). Its states, which exist already: asleep
(embers deep in the sockets), woken (the fire gathers in the eyes and mouth), blazing (**light along its cracks and
through its teeth**) and shattering. The scale may grow if the owner agrees (A4), though its hit radius cannot.
*Ranks*: the stone and bone around it age (moss, frost, blood on the keystone, soot, gilding on the frame); its
eyes' fire colour is the rank's. *Vapour*: smoke from the nasal hollow and the cracks, as it charges.

**The imp.** *Now*: a pot-bellied cartoon with a big round head and a lit grin; its cracks showing fire are the one
good idea. *Wanted*: **a sinewy, soot-black thing that is still cooling**: a crust of black over lava, the fissures
glowing orange and white, wiry limbs ending in talons, a narrow skull-like head with a long muzzle, needle teeth, and
**swept-back horns that glow at the tips**, large bat ears with a torn edge, ragged leather wings and a tail whose
tip is an ember. The grin stays: it is the only friendly thing about it, and it should look *hungry*. A hunched,
sprung stance; asymmetry (one horn broken). Its eyes are white-gold slits. *Ranks*: more of it glows and less of it
is crust; V is nearly all fissure, a body about to come apart into fire. *Vapour*: heat-shimmer and sparks.

**The gargoyle.** *Now*: an ellipse head, a muzzle ellipse and one pixel per eye. *Wanted*: **a true grotesque**:
heavy overhanging brow slab, deep-set eyes, a broad flat snout with flared nostrils, a wide lipless mouth set in a
snarl with two tusks, swept-back fin ears, horns that curve *forward and down* like a bull's, a thick neck and
hunched, slab-like shoulders. Wings of bat bones with the membrane **torn at the edges and stained**. The stone
should look *weathered*: dark streaks running from the eyes and mouth like rain-stain, lichen in the recesses,
chipped claws, a cracked horn. A rainspout's mouth, because that is what it was. *The wake*: perched, it is a
statue with blank stone eyes (and **must stay indistinguishable from the decorative statues**); as it wakes, dust
sheds off, the cracks light, and the eyes ignite. *Ranks as stone*: II mossy limestone; III frost-glazed granite; IV
red sandstone with rust-coloured streaks; V black basalt with veins of light; Gilded marble with gold in the
fissures (a stone creature changes its *material*, not its paint). Drawn so that it stands out from the wall behind
it, which today it does not. *Vapour*: a dust-and-light seep.

**The Drowned.** *Now*: a pot-bellied giant baby in the colour of the wall. *Wanted*: **the body the water made of
a man**, long and wrong. Keep the one true idea (**the swollen belly, tight as a drum, the skin at its seams**) but
put it on a **gaunt, too-tall frame**: sloping shoulders, thin arms that hang past the knees, long wrinkled hands
(the fingertips washerwoman-white), a head that hangs forward on a slack neck, **the hair plastered flat and dark**
in strands, the mouth a black slack hole, **the skin waxen green-white** (flesh and ghost ramps, never the wall's
stone; the wall ramp is what made it vanish), veins showing, a dark bruise at the throat where something held it.
Clothes: the Order's sodden shroud or a rotted tunic dragged on one side; a rope round the ankle with a stone or an
anchor's weight; weed, shells and a crab or two. Eyes **small and cold** and *bright* (pale cyan-white): a dead fish's,
that follow you. It **drips**: from the fingers, the hair, the mouth; and a dark patch of wet on the floor where it
stands. *Rise* (from a puddle, `SP_DROWN_RISE`): a hand, then the hair, then the face. *Burst*: the shroud splits and
a cloud of gas and water goes out, the skin hanging in strips. *Ranks*: II mould and weed; III frost-furred, ice in
the hair; IV waterlogged and bleeding from the seams; V black, a thing of the deep, a crown of rope and a chain;
Gilded sodden, with the Order's gold gone green. *Vapour*: gas from the seams and drips from the fingers.

### Tier B: the other faces

- **Shambler.** Now a red flayed doll with an orange orb. Wanted: the *buried*: grave-dirt clotted on a grey-yellow
  body, a burial linen in rags, roots through the ribs, the jaw dropped, hair in clumps; arms always reaching.
  Ranks as the kit. Eyes: sulphur. Vapour: damp steam.
- **Ghoul.** Now a bulky green puppet with red horns. Wanted: *starved*: **long, grey-green, skin tight over ribs and
  spine**, a hunched ape stance, a jaw that unhinges, claws, loose lips, a ridge of vertebrae, long ears. Less bulk,
  more hunger. The feeding pose gets a mess of bone.
- **Bone hound.** Now good bones; the skull is the shared one. Wanted: the *dog's* skull (long snout, a hinged jaw
  with canine teeth), a studded iron collar and a broken chain, ribs and a hanging tail. Vapour from jaws like breath.
- **Skeleton knight.** Now a black helmed figure with a blue sword. Wanted: a **great helm with a Y-slit and breath
  holes**, a stump of plume, rivets, the Order's device on a tabard, a visor-slit glow in crimson; the three stages
  of plate hacked away stay (a breastplate, half, none): with the skull *visible under the broken helm* in the last
  stage. Steam jets at the joints. The blade keeps its steel colour at every rank.
- **Deacon.** Now a green-robed priest with a skull staff. Wanted: a **taller, thinner** robe with a peaked hood
  in the lancet shape, the skull face a pale gleam deep in shadow, **two lights for eyes**, the stole and the staff's
  skull given the master skull's detail, a rosary of knuckle-bones. The robe's colours bleached by rank, as the kit
  has it. The chant pose brings the vapour up.
- **Spectre.** A long drawn face in a trailing shroud, mouth open in a wail, hollow eyes lit, hands of mist; it is
  translucent and has no rank.
- **Wraith.** A peaked hood over a void that is nothing but **two lights**, a body of tattered black cerements that
  end in wisps, long thin hands. Its boss is the same, larger.

### Tier C: lighter touch

- **Bat.** A pointed, tiny face, large ears, fine fangs, red eyes; wings with a bone structure and torn membrane.
- **Web spider and hunting spider.** Pitted carapace, banded legs, hair at the joints, a cluster of eight eyes, fangs
  with a drop on them. The spider boss's pale skull-mark is the template for a rank-V mark.
- **Brood mother, spiderling, sac.** The brood mother heavier and more pitted, a marked abdomen; the sacs glisten.
- **Shade.** Unchanged and eyeless in the dark; the rank-less exception. Check it still shows only in light.
- **The bosses.** They are scaled forms of these sprites and inherit the new heads, eyes and layers. The Deacon and the
  Wraith were to get **new art** (`CLAUDE.md`, 1.0.0b7's leftovers): the heavy-gothic subjects here, so they are the
  first candidates to fold in (A2). Bespoke boss vapour, beyond what rank V gives, is a later pass.

### Tier D: what shares the skull

Skull piles, bone heaps, hung skulls on totems (`SP_TOTEM_*`, the Hexed Totem), the Black Shrine's skull, the trial
doors' skulls, the niches and the bone walls (`T_SKULLS` and the runs' bone walls, which show rows of the same flat
skull in cream). Whether the walls are in scope is the first question (A1); the props that take the master skull come
free.

## 9. Building it

Sheets before art in the game, each unit ending with a sheet or frames for the owner. Sheet groups (`make sheets`,
`tools/sheet.c`): `artpass.png` (every creature across its ranks, front and side, at 1x and 3x, with a greyscale strip),
then each creature's own as it is drawn, and `eyes.png` (eye forms by rank over lit, dark and fogged stone). Frames from
the game are taken with `CRAWL_MEET`, `CRAWL_MEETN` and `CRAWL_RANK`.

**Part one: the groundwork** (shared by every creature, built once)

1. **Baseline.** Freeze "before": the review sheet of every creature at rank I and V, frames of each in light and
   dark, the bench numbers (`build/host bench crawl 60`). The before-pictures are kept in `docs/artpass/before/` so
   that the owner and the next session can compare. **No game change.**
2. **Eyes for everyone.** The table of points, the eye pass, forms and colours by rank, the alert flare and the
   exceptions of 5.3, for every creature as it stands today (their points found from today's sprites), so the owner's
   one constant holds from the first commit. A frame strip in the dark at six distances.
3. **The rank's tint** (A6 b): the LUT that keeps the highlights' material.
4. **The dressing layer and the vapour** (6.4 option B, section 7), built with the skeleton as the pilot, and the
   *CREATURE EFFECTS* setting.

**Part two: creature by creature** (each one: re-proportioned body, head, side view, eye points, dressing for II to V
and gilded, vapour emitters; its sheet, then the game; committed, pushed and logged in section 13 as one unit)

5. The skeleton (and with it the skull master), 6. the shield-bearer, 7. the wall skull, 8. the imp, 9. the gargoyle,
10. the Drowned, 11. the shambler, 12. the ghoul, 13. the bone hound, 14. the skeleton knight, 15. the deacon, 16. the
spectre, 17. the Wraith, 18. the bat, 19. the web spider, 20. the hunting spider, 21. the brood mother, spiderling and
sac, 22. the shade (its eyes stay the exception).

**Part three: the bosses** (A2 c), each after its kind: 23. the Ill-Fated Knight (its three stages and its planted
pose), 24. the Wandering Spider, 25. the Black Deacon, 26. the Wraith Unbound.

**Part four: each run's assets** (A1 b): 27. the skull's props (piles, heaps, totems, the Hexed Totem, the Black
Shrine, the trial doors' skulls); 28-31. the niches and bone walls of runs I, II, III and IV, one run at a time.

**Part five:** 32. the bestiary's rank lines, the previews and README strips retaken; 33. the checks (section 10) and
the release as 1.0.0b14, at the owner's word, squashed onto main as one commit like the betas before it.

**How to work on the art.** Draw heads as hand-placed pixel grids, view them at 1x, 3x and a third; compare *before and
after* side by side; run the child's-drawing checklist on every head; look at each sprite in the real sizes of 4.2,
and against lit stone, dark and fog; ask for reference or flavour text where this document runs dry rather than fill
the gap with a default. A sheet is not finished when it looks right large.

## 10. The checks

- **Eyes from anywhere.** For every kind that has them, at rank I and V, standing in an unlit, fogged corridor at 2, 4, 8,
  12, 16, 20 and 23 cells: the harness counts the eye-coloured pixels and fails if there are fewer than two for each
  eye. It stays in `make smoke` so that it cannot regress. Shades are asserted *not* to show.
- **Rank without colour.** The `ranks.png` sheet in greyscale, and with colours confused for the usual forms of
  colour-vision difference: each rank must be distinguishable from its neighbours by shape or mark. *Question A16.*
- **Nothing in the simulation moved.** The event logs of the baseline's four dream runs, run again on the same seeds
  for ten minutes, are identical; the saves still load; `make smoke` passes.
- **Cost.** Frame time with ten creatures in sight within 5% of the baseline; `crawl_art_init` and the per-level
  reload no slower than the baseline plus a figure agreed in step 1 (fall back to option C of 6.4 if the reload
  grows); the ASan build clean.
- **The pages.** Every bestiary page, at every rank, front and side, draws without a gap.
- **The settings.** FULL, CALM and OFF each look as described; the eyes the same in all three.
- **Windows.** The cross-compiled build builds and runs the smoke test in CI. The art has not been seen on a real
  screen, nor under Wine, until the owner says so; `README.md` and the page say that.

## 11. Risks

- **Ambush changes.** Eyes that show from anywhere make the dark a place where things look back at you. That is the
  request; but the web spider's waiting, the risers, the statues and the shade rely on *not* being seen, and 5.3
  keeps each of them. Check them by eye in the dark.
- **Load time.** Option B is about 450 extra layers; measure before trusting it (section 10).
- **Colour is still the long-range cue.** Past about twenty cells nothing but the eyes' colour reaches the screen;
  that is why the eyes take the rank's colour from II (A12), with the body's marks as the short-range cue.
- **The bestiary pages** will show every flaw: the sprites are drawn large there.
- **One look is not a style.** Eight heads drawn by hand can still disagree: the master skull's rules (4.1) and the
  checklist are what hold them together.
- **Reference.** The hand, the face and the cat were traced from the owner's photographs and are the best-looking art
  in the game; the creatures have no such source. Reference from the owner (a sketch, a photograph of a carving) is
  worth more than any rule here.

## 12. Questions

Each has a recommended answer (*). The first five change the work most.

**Scope**

- **A1. Which skulls?** (a) the wall skull creature only; **(b)\* every skull the game draws: the creature, the piles,
  heaps, totems, niches and the ossuary's bone walls (`T_SKULLS` and the runs' bone walls), so the whole building
  agrees**;
  (c) creatures and props, but leave the walls' textures.
- **A2. The bosses and 1.0.0b7's leftovers.** (a) leave them out; **(b)\* the bosses inherit the new heads, eyes and
  layers, and the Deacon's and the Wraith's new art (which 1.0.0b7 promised) is folded in at step 9**; (c) all four
  bosses get bespoke redraws now.
- **A3. Dressing by run** (miners' helmets and lamps in the Hollows, silt and weed in the Catacombs, glass and cinder in
  the Deep). (a)\* not in this pass, but the kit and the table of points are built so it can come later; (b) in this
  pass (it multiplies the sprites by four); (c) never.

**Direction**

- **A4. How far may a silhouette change?** (a) heads only; **(b)\* heads, and the Drowned, the imp and the gargoyle
  re-proportioned, since their bodies are part of what reads as childish**; (c) every creature re-proportioned.
- **A5. May heads grow?** A skeleton's skull from about 12 pixels to 15 or 16. **(a)\* yes, up to a quarter**; (b) keep
  today's sizes.
- **A6. How much of the body takes the rank's colour?** (a) all of it, as now, with the new layers on top; **(b)\* the
  shadows and mid-tones, so bone still looks like bone**; (c) none: the eyes, vapour and trim carry the hue.
- **A7. Where does vapour begin?** **(a)\* III a thread, IV a plume, V a cloak, gilded motes**; (b) at II;
  (c) only at IV.
- **A8. How much damage on show?** **(a)\* breakage, rot, rust, frost and old stains, no wet viscera**; (b) frank
  wounds; (c) cleaner than that.
- **A9. Gilded.** (a)\* the Order's honoured dead: gold leaf and a death-mask over the same body, gold motes;
  (b) gold-plated bone and armour; (c) your picture of it (flavour wanted).

**Eyes**

- **A10. Through walls?** **(a)\* no, only past darkness and fog**; (b) a faint glimmer through a wall within a few cells,
  like Commune's.
- **A11. Sleepers and statues** (the wall skull asleep, the gargoyle on its plinth, a riser under its mound).
  **(a)\* dark or embers until they wake, then they ignite**; (b) lit always.
- **A12. Eye colour.** **(a)\* the kind's own at rank I, the rank's colour from II**; (b) always the kind's own.
- **A13. State.** Eyes gain a pixel of halo and a steadiness when alerted, and flare as it strikes. **(a)\* yes**; (b) no.

**Settings and process**

- **A14. A setting** in the settings dialog, *CREATURE EFFECTS: FULL, CALM, OFF*, which does not touch the eyes.
  **(a)\* yes**; (b) no; (c) CALM and OFF only if you find FULL tiring.
- **A15. Approvals.** **(a)\* a sheet for each step, then frames in the game, as for each run's look**; (b) approve
  steps 2 to 5 and leave the rest to my judgement, as you did for runs III and IV; (c) one look at the end.
- **A16. Colour vision.** `RANK_SIZE`'s comment says the ladder was meant for "eyes that mix up the colours". Is
  greyscale legibility enough, or is there a particular form of colour-vision difference I should test the sheets for?
- **A17. Housekeeping.** (a)\* `CLAUDE.md` records that the lower storey is now 1.0.0b15 and `PLAN-b13.md` is left alone;
  (b) `PLAN-b13.md` is edited too, so that its "1.0.0b14" reads 1.0.0b15.

### Flavour wanted (open, in your own words)

Anything you can say in a sentence or two helps more than a rule does:

1. **Touchstones.** Games, films, paintings, carvings or sprites whose creatures you love or loathe (the Doom, Heretic
   and Hexen sprites, Castlevania, Dark Souls, Bloodborne, Darkest Dungeon, Hammer films, Doré's engravings, the
   medieval *danse macabre* and the carved memento mori are the sort of thing I mean).
2. **How frightening.** Spooky and wry, or properly dreadful? Where is your line on faces that look *hurt*?
3. **The Drowned.** What do you want to feel when one stands up out of the water? Pity, disgust, or plain fear?
4. **The gargoyle's temper.** Smug, furious, patient, sorrowful?
5. **The Order.** Who were they to the dead who wear their marks, jailers, saints, a guild? (It decides which marks
   go on which creature and whether the gilded dead are honoured or condemned.)
6. **Reference.** A sketch, a photograph of a skull, a stone carving or a gargoyle, as with the hand, the face and
   the cat. I can describe a pixel grid from it; I cannot put a face in it that nobody has shown me.
7. **Anything that already feels right** that I should not touch (I have kept the skeleton's bones, the web spider's
   eyes and the imp's cracks of fire for now).

## 13. Where it stands, and notes for whoever continues

This section is the hand-off. It is updated in the same commit as each unit of work, so that a session that runs out
of usage leaves the next one a true account. **Read it before anything else in this plan.**

### Log

| Unit | State | Where | Notes |
|---|---|---|---|
| Plan settled | done | `docs/InitialArtPass.md`, `docs/PLAN-b13.md` renumbered, `CLAUDE.md` | 31 October |
| 1. Baseline | done | `docs/artpass/before/` (every creature's sheet and the line-up, before; `eyes_dark_skeleton.png`) | bench 2.104 ms a frame (`build/host bench crawl 60`) |
| Test stage | done | `CRAWL_STAGE=k` (+ `CRAWL_STAGEDIST`, `CRAWL_STAGEDARK`, `CRAWL_STAGEFOG`, `CRAWL_STAGESUB`) in `src/scene_crawl.c`; `tools/stage.py` makes strips | the caster goes straight into the level, still; the kind stands frozen down the longest hall |
| 2. Eyes for everyone | done, for the owner's eye | `EyeAt crawl_eyes[]` (`src/crawl.h`), `cv_eye`/`cv_eye_mark` in each creature's sprite function, `render_eyes` and `EYE_*` tables in `src/scene_crawl.c`; review: `docs/artpass/review/eyes_*.png` | every kind's eyes on today's sprites; frame time 2.014 ms; a 15-minute dream run's 390 events identical to 1.0.0b13's; smoke passes. Not yet: the harness check of 10 (it is done by eye with `tools/stage.py --dark 0.15 --fog`) |
| 3. The rank's tint | done, for the owner's eye | `build_rank_luts` in `src/crawl_art.c` (`KEEP[]`: the step from which a rank leaves the material alone) | every family at once: bone stays bone in its lights, the rank in its hollows (rotten, cold, bloody, charred, gilt) |
| 4. Dressing layer, vapour, the setting | in progress: the layer and the vapour done; the setting not yet | the layer: `crawl_dress`, `crawl_dress_ix` (`src/crawl.h`), `dress_begin`/`dress_store` and the kit (`kit_rags`, `kit_frost`, `kit_moss`, `kit_blood`, `kit_char`, `kit_gold`, `kit_arrow`, `kit_crown`, `kit_chain`, `kit_mask`) in `src/crawl_art.c`; composited in `render_sprites` (`Draw.ov`, untinted) and in `tools/sheet.c`. The vapour: `render_vapour`, `vap_px`, `g_fx` in `src/scene_crawl.c` (`Draw.vap`; the nearest four full, the rest half) | **Still to do**: the *CREATURE EFFECTS* setting (A14): `g_fx` is fixed at 0 (full). It wants `AppConfig` (`src/app.h`), the registry and the settings dialog (`src/win_main.c`, which this container cannot compile: no mingw; CI builds it), and `tools/host.c` (an env var). And the vapour gathering into the mote at a death (7.1). |
| 5. The skeleton, with the skull master | done, for the owner's eye (but the death's vapour into the mote) | `SKULL_FRONT`, `SKULL_SIDE` (15 by 16, hex steps of the bone ramp, `@` an eye) and `skull_stamp` in `src/crawl_art.c`; `skel_draw` (a neck, the shoulders two pixels lower, the right a pixel lower still), `skel_side`; review: `docs/artpass/review/skeleton.png`, `skeleton_game.png`, `sides.png` | the skull keeps its face through the level's grime (`g_skbox`); fallen, it lies in profile. Dressing (`skel_dress`): II rags and mould, III frosted rags and a notch out of the crown, IV bloodied rags, an arrow in the ribs and a cut brow, V charred, an iron crown broken, a shackle and chain, a torn cloak behind; gilded a death-mask, gold leaf, a clean shroud, a gold chain. Vapour in the game: `docs/artpass/review/skeleton_vapour_game.png`. Frame time 2.03-2.06 ms; a dream run's events unchanged |
| 6. The shield-bearer | done, for the owner's eye | `spr_shieldskel` (shares `skel_draw` and `skel_dress`; blade, guard and shield moved down with the arm; the dressing keeps off the shield and blade) | as the skeleton |
| 7. The wall skull | done, for the owner's eye | `spr_wskull` (the master skull three times over by `grid_scale3`, Scale3x; grain and cracks at the finer scale; fire filling the sockets, then the mouth, the nose and the cracks; broken, split with the right half dropped), `wskull_dress` (II moss, III frost, IV tears of old blood, V char and an iron crown, gilded gold and a mask), `spr_wskull_side`; review `docs/artpass/review/wskull.png`, `wskull_game.png` | asleep, ember eyes (A11), seen at 13 cells in the dark. The old `skull_side` is gone. Frame time unchanged (benched back to back against the commit before); events unchanged |
| Checks after unit 7 | done | CI run 41221041012 on the branch (started by hand, `workflow_dispatch`): the Windows build compiled, the smoke test passed. ASan (`-fsanitize=address,undefined`, as `CLAUDE.md` gives it): a 400 s dream run at level 21 with every creature at rank IV, six kinds staged at gilded in the dark and fog, and a scripted player at level 13: clean | the art has not been seen on a real screen, nor under Wine |
| 8. The imp | done, for the owner's eye | `IMP_HEAD` (17 by 17), `IMP_SIDE` (15 by 15), `imp_crust`, `spr_imp`, `spr_imp_side`, `imp_dress` in `src/crawl_art.c`; `grid_stamp` (any hand-drawn grid: hex the body's ramp, g-v a second ramp, A-P the fire, 0, @, .); review `docs/artpass/review/imp.png`, `imp_game.png`, `sides4.png` | re-proportioned (A4 c): no pot belly; a narrow chest, wiry arms to the knee with talons, reverse-jointed legs, ragged bat wings behind, a tail with an ember; the head a horned, torn-eared, long-muzzled face with slit eyes and a lit grin of needles, the right horn broken. Ranks: II mould and torn wings, III frost and a chipped horn, IV the fissures opened and a wound in the chest, V black crust veined all over with fire under an iron collar and chain, gilded gold in every crack. Events unchanged, ASan clean |
| 9. The gargoyle | done, for the owner's eye | `GARG_FACE` (15 by 17), `GARG_SIDE` (14 by 15), `garg_head` (bull's horns, the right one broken short; fin ears), `garg_wing_front`, `garg_weather` (rain stains from the eyes and mouth, lichen in the hollows, the cracks alight once awake), `spr_gargoyle`, `garg_dress`, and the side view's head in `spr_garg_side`; `grid_stamp` draws `@` as blank stone when its eye is 0; review `docs/artpass/review/gargoyle.png`, `gargoyle_game.png`, `gargoyle_v_game.png`, `sides4.png` | re-proportioned: slab shoulders, a hunched chest, wings folded high round it on its plinth like a cloak, spread and torn in flight. Perched, its eyes are blank stone and nothing shows in the dark (A11); the statues in the alcoves are the same sprite. Ranks: II moss and torn wings, III frost and a chipped horn, IV rust run down it and a wing cracked through, V black basalt with thin veins of cold light under an iron collar, gilded gold in its veins. A bounds fault in the rain's loop was found by UBSan and fixed before the commit; ASan clean after; events unchanged |
| 10. The Drowned | done, for the owner's eye | `DROWN_HEAD` (13 by 15), `spr_drowned`, `drowned_dress`, `spr_drowned_side` in `src/crawl_art.c`; a rank family of its own, `PF_DROWN` (`src/crawl.h`, the `BODY` table in `build_rank_luts`, `KINDS[MOB_DROWNED]` in `src/scene_crawl.c`, `tools/sheet.c`); review `docs/artpass/review/drowned.png`, `drowned_game.png`, `sides2.png` | re-proportioned: a gaunt, too-tall frame round a belly swollen tight with veins and a splitting seam; ribs, sloping shoulders, thin arms past the knees with long fingers; a long face, hair plastered down it, small cold eyes in deep sockets, a slack black mouth; a bruise at the throat; the Order's shroud sodden down its left side; a rope round the ankle to a stone; weed, barnacles, drips. Its skin is the ghost ramp, never the walls' stone. Rank III goes bone-pale (the cold rank's ramp is its own skin's, so the old rule sends it pale). Ranks: II mould and more of the shroud torn, III frost and the seam split, IV bleeding from its seams and mouth, V black, an iron crown and a chain to its stone, gilded gold gone green. Rise, burst and pool kept. `drowned_pool` was lost in the rewrite and restored from git before the commit. The side view is better (a long face, a smaller belly) but not yet as good as the front |
| 11. The shambler | done, for the owner's eye | `SHAMB_HEAD` (13 by 16), `spr_shambler`, `shambler_dress`, `spr_shambler_side` in `src/crawl_art.c`; review `docs/artpass/review/shambler.png`, `shambler_game.png`, `sides2.png` | the buried: a hand-drawn face (sunken sockets with a sulphur light, the nose rotted to holes, the jaw dropped on a few yellowed teeth, hair in clumps); its grey shirt now burial linen wound in bands, its legs bound in it; grave-dirt clotted on it, more towards the feet; roots out of the tear in its chest. Its poses (walk, grab, the two rising, kneeling, down) kept. Ranks: II more linen and grave-mould, III frost and an ear gone, IV an arrow in the shoulder and old blood from the tear and mouth, V charred under an iron circlet with a grave's shackle, gilded gold leaf and gilt teeth. The first head was a square with a grille of teeth and was redrawn narrower before the commit. Events unchanged, ASan clean |
| 12. The ghoul | done, for the owner's eye | `GHOUL_HEAD` (15 by 15), `spr_ghoul`, `ghoul_dress` in `src/crawl_art.c`; review `docs/artpass/review/ghoul.png`, `ghoul_game.png` | starved: a narrow arched back with the spine beaded and the shoulder blades jutting, deep ribs over sunken flanks, long thin arms with knobbed elbows and claws twice as long, thinner hind legs; a hand-drawn face (close-set eyes deep in their sockets, slit nostrils at the muzzle's tip, lips peeled back from fangs, the jaw hanging open), broad torn ears (the first ears crossed the face as a red X and were redrawn). The feeding pose bloodies the mouth. The side view was already gaunt and spined, and is kept. Ranks: II loose skin and mould, III frost and an ear torn through, IV a gash down its back and its claws and mouth bloodied, V charred in a chained beast's collar, gilded gold leaf and gilt fangs. Events unchanged, ASan clean |
| 13. The bone hound | done, for the owner's eye | `DOG_UPPER` (17 by 8), `DOG_JAW` (15 by 4), `dog_skull_at` (the jaw hinged: its front drops further than its back, the gape dark behind the teeth; mirrored for the side view), `dog_skull`, `spr_hound`, `hound_dress`, `spr_hound_side` in `src/crawl_art.c`; review `docs/artpass/review/hound.png`, `hound_game.png`, `sides2.png` | the bones kept as they were (the owner's "pretty good"); the skull by hand: a long muzzle, the nasal opening at its tip, a deep orbit, the cheekbone's arch, a canine hanging; a broken chain from the studded collar. Late in a run its eye goes cold, as the skeleton's does. Ranks: II a strip of hide from the collar and moss, III frost and the skull cracked, IV an arrow in the ribs and old blood about the jaws, V charred under a spiked muzzle-strap with its master's chain, gilded a gold collar and gold leaf. Events unchanged, ASan clean |
| 14. The skeleton knight | done, for the owner's eye | `KNIGHT_HELM` (15 by 16; hex R_IRON, g-v R_BLOOD for the plume), `spr_knight`, `knight_dress` in `src/crawl_art.c`; review `docs/artpass/review/knight.png`, `knight_game.png` | a great helm by hand: a ridged crest and a plume's stump, rivets, a cross-shaped slit with a crimson light in it, breath-holes; the master skull under it once it is hacked off (the rim left at half plate); the Order's tabard below the plate with its pale cross, torn at half plate, gone when stripped; rivets on the pauldrons and down the plate. The blade stays steel. The bones' grime now spares any hand-drawn skull (`g_skbox` is reset by `cv_begin` and read by `cv_bone_decay`), so the deacon's and the knight's skulls keep their faces deeper down. Ranks: II rust and moss on the plate, III frost and dents, IV a broken blade lodged in the breastplate and an arrow in the pauldron with old blood, V charred plate under an iron crown and a chain, gilded gilt edges and rivets. The planted poses (the boss's) take the new helm; the boss's own art is unit 23. Events unchanged, ASan clean (staged, and a run through level 10's arena) |
| 15. The deacon | done, for the owner's eye | `SKULL_SMALL` (9 by 10), `spr_deacon`, `deacon_dress` in `src/crawl_art.c`; review `docs/artpass/review/deacon.png`, `deacon_game.png` | a peaked hood in the lancet's shape with a pale rim; in its mouth a small hand-drawn skull, dimmed as in shadow, two lights for eyes; the staff's skull the same, lit, its glow its own (not recorded as the deacon's eyes); the robe taller and thinner, in long falling folds; a rosary of knuckle-bones on its cord with a small cross. Ranks: II mildew and a torn hem, III frost, IV blood soaked up the hem, V the robe charred and a black chain of office, gilded gold thread at the robe's edges (its stole keeps its red: the layer stays off what is not the robe). The Black Deacon is drawn from these sprites; its own art is unit 25. The side view is unchanged. Events unchanged, ASan clean (staged, and a run through level 30's arena) |
| 16. The spectre | done, for the owner's eye | `SPECTRE_FACE` (13 by 21), `SPECTRE_SIDE` (9 by 21), `mist_hand`, `spr_spectre` (in place of `spr_wraith`, which drew it as a small Wraith), `spr_ghost_side` in `src/crawl_art.c`; review `docs/artpass/review/spectre.png`, `spectre_game.png`, `sides.png` | it has a face now, so it no longer reads as a small Wraith: long and drawn, the brow knitted, sockets drooping at their outer corners with a pinprick of light low in each, the cheeks fallen in, the mouth dragged open in a long gape to a narrow chin; pale in the mouth of a hood whose brim lies over the brow; the shroud past the shoulders torn at its edges and narrowing to a tail that sways and frays in streaks; hands of mist (a pale palm, four long fingers whose tips go to mist, motes falling). The shriek flings the arms up and flares the shroud. In profile the face looks forward from the hood, which falls into a shroud streaming back. Translucent in play as before; no rank, so no dressing or vapour. The sheet's heading no longer says "today's Wraiths". Events unchanged, ASan clean (staged, and a run with six spectres a level) |
| (aside) The caster's hand | done, for the owner's eye | `HAND_AXIS`, `HAND_POSE`, `crawl_hand_frame` in `src/crawl_art.c`; `hand_frame`, `hand_fr` in `src/scene_crawl.c`; `draw_hand` in `tools/sheet.c` (`make sheets`: `hand.png`); review `docs/artpass/review/hand.png`, `hand_cast.gif` | asked for on its own (31 October), not one of the plan's units: an idle of three frames (1-2-3-2, a third of a second each, while the hand is up between casts) and a cast of four (drawn in as it rises, a claw while it gathers or a charge is held, flung open as it goes, easing after). The traced bitmap is not redrawn: each frame bends it at the knuckles, a finger turning about its knuckle and drawn in along its length for a curl seen from behind, its tip darkened; resampled three by three a pixel, the outline put back. The traced pose is the first idle frame, unchanged. Frames are made from the dressed hand (sleeve, ring, bracer, tattoos) when first wanted after a change, about 6 ms each. The spell forms where each frame's fingertips are. Events unchanged, ASan clean, no cost on the bench |
| 17. The Wraith | done, for the owner's eye | `wraith_hand`, `spr_wraith_big`, `spr_wraith_side` in `src/crawl_art.c`; review `docs/artpass/review/wraith.png`, `wraith_game.png` (lit, then in the dark and thickest fog), `sides2.png` | the hood falls to the shoulders, ragged over them, with folds from its peak and a cold rim; its mouth a lancet of nothing, lined a shade off black, with the two lights in it. The body is wound in cerements (bands round it, loosely, black between the turns, a cold edge on some as they turn away), torn through here and there, and below the waist comes loose into strips of their own lengths with pointed ends, wisps curling off them. Rags hang from the sleeves, which stand a shade off the body with a shadow on it; the hands are thinner and grey, four fingers of three joints curling as they go, knuckles catching the light, nails rimed with frost. Less frost falls. The side view the same: the void a deep notch with one light, the wrappings slantwise, strips and rags streaming back. In the dark it is two lights and a faint shape, as section 5 wants. No rank. The Wraith Unbound is drawn from these until unit 26. Events unchanged, ASan clean (staged, a level with the Wraith at 20 s, and a minute of level 40's fight drawn every frame) |
| 18. The bat | done, for the owner's eye | `BAT_FACE` (13 by 13), `BAT_SIDE` (9 by 11), `bat_wing_front`, `bat_body`, `spr_bat`, `bat_dress`, `spr_bat_side` and `bat_wing` in `src/crawl_art.c`; `g_dress_ramp2` lets a dressing lie on a second ramp (the bat's fur as well as its wings); `render_latch` in `src/scene_crawl.c` now lays the rank's marks over the latched bat; review `docs/artpass/review/bat.png`, `bat_game.png` (ranks I and IV in the crypt, then a rank II bat latched), `sides.png` | a hand-drawn face: ears taller than its head, flesh inside them, a small pointed face, a leaf of a nose, the mouth open on two fine white fangs, eyes like coals; a furred barrel of a body, paler at the breast, its feet drawn up with claws; the wings boned (an arm to the wrist, three long fingers with a knuckle each, a hooked thumb), the membrane scalloped between the fingers and notched at its edge, its shading dithered and thinner towards the tips. Dead, its wings fold about it. Ranks (it stops at IV): II holes worn through the wings and mould in the fur, III frost on the ears and bones, a finger broken back and a wing ripped through, IV old blood about the jaws and down the fangs, the wings' edges shredded and dark; gilded gold on its bones and ear rims. Its side view has the face in profile. Events unchanged, ASan clean (staged at three ranks, a latch, a level with latches drawn every frame) |
| 19. The web spider | done, for the owner's eye | `SPIDER_HEAD` (13 by 8), `chitin_leg`, `spider_abdomen`, `spider_fangs`, `spr_spider`, `spider_dress`, `spr_spider_side` in `src/crawl_art.c` (`SKULL_SMALL` moved up beside them); review `docs/artpass/review/spider.png`, `spider_game.png` (ranks I and V come down from the web at 2.5 and 6 cells, then at 1.4), `sides.png` | seen as from a standing height: the head before it with a hand-drawn cluster of eight eyes like coals (two great ones low in front, two above, two smaller, two at the sides), jaws' bulges and pale fangs curving in, a green drop of venom on each (another falling as it bites); the abdomen looming behind, a shell pitted all over and lit from above, bristles round its rim, its mark an hourglass in blood; legs in two long joints, banded where the plates meet, bristling at the knee, the front pair raised to bite. Dead on its back, its legs curl over it. Ranks: II silk clinging in strands and mould, III frost and its shell cracked, IV old blood on the fangs and a pale scar across its back, V charred and the pale skull on its back on a dark oval (the Wandering Spider's mark, in place of the hourglass), gilded gold in its bands and along its shell. In its web it is still only its eyes. Its side view the same, the face drawn over the near legs. Events unchanged, ASan clean (staged at four ranks, and four met each level for a minute drawn every frame) |
| 20. The hunting spider | done, for the owner's eye | `HUNTER_HEAD` (15 by 10), `hunter_abdomen` (mark: chevrons, the skull, or plain), `spr_hunter(id, frame, marked)`, `hunter_dress`, `spider_side` in `src/crawl_art.c`; `chitin_leg` bands dark in a band ramp; the Wandering Spider is now `spr_hunter(SP_BSPIDER_*, f, 1)`, its skull drawn from `SKULL_SMALL` on a dark oval where the stamp of fixed pixels was; review `docs/artpass/review/hunter.png`, `hunter_game.png` (ranks I and V down at 1.8 and 5 cells, then on the ceiling), `sides3.png`, `b7.png` (the boss) | a wolf spider's face by hand: two great eyes shining in front, rounded, two above, a row of four small below; fur on its fore-body, palps, jaws' bulges and dark fangs each with a green drop; the abdomen furred in strokes, a dark stripe and chevrons, bristles round its rim; legs banded dark and bristling at the knee. Ranks: II silk and mould, III frost and its back cracked, IV old blood on the fangs and a bald scar torn in its fur, V charred and the pale skull on its back, gilded gold in its fur. Its side view (shared with the brood mother, whose side view changes with it: pitted, its eggs and red mark kept) has its fur, banded legs and one great eye drawn over the near legs. The drops' white glint is gone, here and on the web spider: on the ceiling, in the dark, the pair read as a second pair of eyes. Events unchanged, ASan clean (staged at three ranks, four met a level for a minute, a minute of level 20's fight, all drawn every frame) |
| 21. The brood mother, spiderling and egg sac | done, for the owner's eye | `brood_abdomen`, `spr_brood`, `brood_dress`, `spr_sac`, `spr_ling` in `src/crawl_art.c` (the old `spider_leg` gone; `g_dress_ramp3` and `dress_on` let a dressing lie on a third ramp); review `docs/artpass/review/brood.png`, `brood_game.png` (the mother at ranks I and V at 2.2 and 5 cells, spiderlings, a sac), `sides3.png` | the mother is the web spider's kind at the size of a dog: her abdomen the pitted shell stretched over sixteen eggs pressing pale through the skin, each with the skin drawn dark under it, veins of blood running between them, her hourglass at the front; the eight eyes, pale fangs with venom, thick banded legs bristling at the knee. Laying, a sac wound in silk comes down on its thread before her jaws; dead, she lies on her back with her legs curled over her. Ranks: II silk and mould, III frost and her shell cracked, IV old blood on her fangs and an egg burst with its yolk run, V charred and the pale skull on her back (over her mark and her eggs), gilded gold in her bands. The sac is wound strand on strand, the young pressed dark against it inside; stirring, their legs at the silk and a few eyes catching; hatched, its lips torn outwards and strands trailing. The spiderling: banded knees, a small hourglass, its two great eyes and small ones beside them, fine fang tips. Her side view was reworked in unit 20. Events unchanged, ASan clean (each staged at two ranks, three mothers a level for a minute and a half drawn every frame) |
| 22. The shade | done, a check | nothing changed: `spr_shade` and `spr_shade_side` as they were (its sheet is the same, pixel for pixel, as `docs/artpass/before/shade.png`); review `docs/artpass/review/shade_game.png` (lit, at half the light, and at a seventh, each at 1.8 and 4 cells) | the exception holds: the eye pass leaves the shade out (`render_sprites` sets no `d->eye` for `MOB_SHADE`), so its eyes are its sprite's own two dim ones, drawn as faintly as `shade_alpha` shows the rest of it. Lit, a dark shape with its violet edge and its eyes; at half the light, faint and see-through; in the dark, nothing, eyes and all (it is not drawn under an alpha of 0.04, and shows for a moment after its touch, as it always did). No rank, no dressing, no vapour. ASan clean (staged lit and dark) |
| 23. The Ill-Fated Knight | next | the knight boss's sprites (`SP_KNIGHT*` and the planted poses `SP_KNIGHT_P0..2`, drawn `DR_SHINE` at the boss's scale); a bespoke set would want its own `SP_*` and `mob_sprite`'s boss case | A2 (c): a bespoke redraw after its kind's own (unit 14). Its three stages (full plate, half, the breastplate gone) and its planted pose; the owner to be asked for the look first (big work, and the plan's brief is thin) |

### How to pick it up

1. `git log --oneline` on `claude/dazzling-dirac-20cr2y`, then this log: the last row that is not *done* is where to
   start; a row marked *in progress* says what was left half-made.
2. `make host build/host_debug build/sheet` (about a minute), then `make sheets` and look at `build/sheets/artpass.png`
   (or the creature's own sheet) with the before-picture in `docs/artpass/before/` beside it.
3. Each creature is a unit: its sprite functions in `src/crawl_art.c` (named in section 8 and the log), its eye points
   recorded as it is drawn, its dressing and vapour from the kit; then `make smoke`, a frame from the game with
   `CRAWL_MEET=k` (the `MOB_*` number) and `CRAWL_RANK=r`, and the sheet sent to the owner.
4. Commit and push after each unit with this log updated; never leave the branch with a creature half in the game.
5. Frames: `python3 tools/stage.py <MOB_* number> out.png --dists 2.5,6,13,23 --dark 0.15 --fog` (one creature at each
   distance, in the dark and the thickest fog; `--rank r`, `--level n`, `--sub s` for a state, such as a wall skull
   charging). The level's seed is 5 unless `--seed` says otherwise, so frames are comparable between sessions.
7. **Lessons from the first units** (31 October):
   - Keep a head to about a quarter of the body's height or less: at 1 to 3.4 the skeleton read as a cartoon, so the
     skull was cut to 16 rows.
   - The level's grime (`g_decay`) darkens two in five bone pixels by three steps and erased the hand shading, so faces
     are spared (`g_skbox`); do the same for any hand-drawn face that is aged by the level.
   - A face drawn by hand at 15 pixels can be used bigger with `grid_scale3` (Scale3x), as the wall skull is, and then
     given grain at the finer scale, so that every skull in the game is the same skull.
   - A grid declared `[SKULL_H]` with a row missing reads past its end and crashes `build/sheet`: count the rows.
   - A creature's dressing is a function beside its sprite's (`skel_dress`, `wskull_dress`), called after `cv_store`,
     drawing ranks II to gilded with the kit and storing each with `dress_store(id, rank, body ramp)`. The layer is not
     tinted by the rank, and is kept off whatever is not the body (a shield, a blade).
   - Vapour comes from the eye points by itself; a creature with none vapours from (32, 16) of its sprite.
   - `tools/stage.py` takes the `MOB_*` number, and the enum in `src/scene_crawl.c` runs over three lines: the shambler is
     6, the Drowned 7, the hound 8, the Wraith 9, the hunting spider 10, the ghoul 11, the knight 12, the brood mother 13,
     the spiderling 14, the sac 15, the shade 16, the imp 17, the deacon 18, the gargoyle 19.
   - Keep a matching pair of pale or full-bright pixels off a face except for its eyes: the venom drops' white glints,
     four pixels apart below the eyes, read in the dark (and upside down on a ceiling) as a second pair of eyes.
   - The bench varies by a fifth between runs in this container: compare against the commit before, back to back, with
     a `git worktree`.
   - The settings dialog is in `src/win_main.c`, which only CI can compile here (no mingw in the container); push and
     read the build's result before calling the setting done.
8. The hand's frames are bent from `crawl_hand_img` by the axes in `HAND_AXIS` (knuckle to tip through each finger's
   middle, fitted to the traced hand): if the hand is ever traced again, fit them again, or the bends will tear it.
   `CRAWL_HANDFRAME=k` (with `CRAWL_SHOWHAND=0`) holds frame k on screen; `CRAWL_STAGE` now lets a person who has taken
   over act (the creatures stay still), so a cast can be filmed on the stage.
9. A sprite's eyes are wherever its function calls `cv_eye` (or `cv_eye_mark` for an eye drawn as a shape); a redrawn
   creature must call it for each eye, in each pose that has them open, or it will lose its eyes in the game.

