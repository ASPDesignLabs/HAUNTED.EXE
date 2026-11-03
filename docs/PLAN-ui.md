# The carved stone: a pass over CATACOMB's screens

**Status: settled (3 November 2026). The owner took my answer to every question but two: U7 (b), this pass whole
before PLAN-ai's step 11, and U10 (b), full screens keep the crypt dimmed in a 4 px margin round them. Being built on
`claude/gifted-rubin-q8up37` from main (1.0.0b13 with the art pass so far and PLAN-ai's steps 0-10). Step 0 is done (the
before sheets in `docs/uipass/`). Step 1's sheet was approved by the owner (3 November). Step 2 is built (harness only): every menu, card and the tally
wear the stone. Step 3 is built (harness only): the Alignment, at the Lampstand and on Tab's fourth page, is the
approved stone. Step 4 is built (harness only): the cornerstones, the Hexed Totem, ascend and her question of what you were. Step 5 is built (harness only): Tab's pages, the bestiary and the Codex. Step 6 is built (harness only): her hall's screens. Step 7 is built (harness only): the crypt's screens, the cards and the tally. Step 8 is built (harness only): the HUD in the run's stone. Step 9, the checks, is done: the pass is built. Like the art pass, it is visual only: nothing in the simulation, the saves or the
autopilot's numbers may move.**

## Decided (3 November 2026)

| | Answer |
|---|---|
| U1 | (a) a marble of our own for every menu, borrowed into `R_FLOOR` and `R_CEIL` while a menu is up, leaning a tenth to the run's stone; the HUD in the run's stone |
| U2 | (a) weathered grey marble, mid-dark, pale veins, light text cut in it, the colours inlaid |
| U3 | (a) three new faces; the body face only once it reads as well as the BIOS one |
| U4 | (a) the slab's own words cut, anything coloured inlaid with a dark rim, small print plain |
| U5 | (a) forty 16x16 icons by hand, cut when unbought, inlaid when bought |
| U6 | (a) no names in the tree; the pane names the chosen node |
| U7 | **(b) this pass whole first, then PLAN-ai's step 11** |
| U8 | (a) part of 1.0.0b15, with the minds |
| U9 | (a) ledger stones, headstones, tablets |
| U10 | **(b) full screens as now: the crypt dimmed in a 4 px margin round them** (so the remap of the crypt's floor and ceiling in section 2.2 covers every menu, full or small; the slab's moulding sits 4 px in from the edge) |
| U11 | (a) a light pass over the HUD in the run's stone |
| U12 | (a) the owners' signs, small |
| U13 | (a) no mouse in menus this pass |

## What the owner asked for (3 November 2026)

- A pass over the UI. Most of it is plain, with little theming, and the screens are a place to tell the story.
- Much of it set in carved tombstone and marble, keeping the coloured elements. *Too much black.*
- Letter sizes and spacing kept as they are. New fonts of our own are welcome, but the sizes and the text budgets are right.
- Some screens overhauled, a polish that shows structured UI elements. The skill tree (the Alignment at the Lampstand) is
  the example and the main focus: it looks legacy and should look like a skill tree of today. Each slot its own icon,
  coloured by its line; the whole thing in one frame, the tree on the left, and on the right the title, description, cost
  and other text of whichever slot is chosen.
- The Alignment is where the carved marble look gets tuned, before it goes anywhere else.

Five screenshots came with the request: the Alignment (Spirit, rank 12), level 11's tally, the shade's page of the
bestiary, the Compendium's spells, and Esc's question.

## 1. What the screens are now

### 1.1 How they are drawn

- **Two frames for everything.** Every menu draws `dim_view` (every other pixel of the crypt set to 0), then `panel`
  (a black rectangle with two grey lines, `R_STONE + 4..11`) or `card_box` (the same, for cards and the tally). There is no
  other furniture: no wells, no headers, no list rows, no dividers but a dotted line drawn by hand in each screen.
- **Black.** Both fill with palette index 0. The tally clears the whole frame to 0. A full menu is
  mostly black: its text and two lines are all that is not.
- **Two fonts.** `gfx_text8` (the IBM BIOS 8x8) and `gfx_text16` (the IBM VGA 8x16), both from `tools/gen_font.py`, 8 px
  advance, lines 8, 9, 10 or 16 px apart, and `gfx_text16` at scale 2 for the tally's title. `hint8` colours the keys in a
  hint; `small_digits` draws 3x5 figures; `eerie_text` an overlevelled number.
- **Layout by hand.** Each of the two dozen screens places its text by pixel, so each has its own margins, line gaps and wraps
  (`text_wrap` at 36, 34, 32 columns...). Nothing checks that text stays in its box.
- **The palette** is full: sixteen ramps of 16 and the golds. `R_STONE` is tinted and dimmed by run (`level_palette`: blue-grey
  limestone yellowing in the Crypts, damp green to slate in the Catacombs, sandstone to rust in the Hollows, ash to violet in
  the Deep, a tenth darker each run). No menu uses `R_FLOOR` or `R_CEIL`. `tint()` works on the palette, so it touches the
  menus too.

### 1.2 What is wrong, screen by screen (from the screenshots and the code)

- **The Alignment** (`render_align`, `align_box`). Boxes as wide as their names, so the tree is ragged; the chained nodes
  have a chain drawn through their text and can hardly be read; points as pips glued to the names; the element tabs a
  glyph and a figure; the description at the foot, far from the node it describes. No icons.
- **The tally** (`render_tally`). A black card. The EXPERIENCE row's notes run into its figure when the font's +10% and a
  trial's boon both show (in the screenshot, `BOON +20%` under `+2030`): the notes start after the label and the figure
  is right-aligned, and nothing stops them meeting.
- **The bestiary** (`render_bestiary`). The creature stands in a black box; the page is text in four colours.
- **The Compendium** (`render_compmenu`). A list of names and red prices, `1 MORE` as the only sign of scrolling, an empty
  pane under it for most of the page.
- **Esc's question** (`render_quit`). Five or six rows crowded into a box 176 high; the note under the chosen row sits on
  the frame's lower edge.
- **Ascend** (`render_ascend`). Its fourteen lines are set 7 px apart in an 8 px face, so each row's foot is drawn over
  by the next (step 0's sheets).
- **Overlaps between screens** (step 0's sheets): the controls card, shown as a person takes over in her room, is drawn
  over the shop's card; an achievement's toast is drawn over the tally's title. Section 2.4's fit checks are to catch
  these too: a toast or card waits for room, or goes where it doesn't cover a title.
- And generally: no sense of where you are. The Lampstand's screen, her ledger, the bell and the font, the altar and the
  shrine all wear the same two grey lines.

### 1.3 The screens, all of them

| Screen | Where | What it needs |
|---|---|---|
| The Alignment (`UI_ALIGN`, Tab's fourth page) | the Lampstand | **overhaul**: the tree with icons, the pane (section 5) |
| Cornerstones (`UI_STONES`, Tab's third page) | at each tenth level | **overhaul** on the same pattern: five sockets and the pane |
| The Hexed Totem (`UI_TOTEM`) | her back room | **overhaul**: four abilities by rank, sockets and the pane |
| Ascend (`UI_ASCEND`) | her menu | **overhaul**: fourteen lines as carved rows with pips, the pane for the chosen one |
| The class (`UI_CLASS`) | her question | **overhaul**: six marks as medallions, the chosen one's past and gifts beside |
| The wardrobe (`UI_INVENTORY` page 0) | Tab | **overhaul**: seven worn sockets and the bag as sockets with icons, the card in the pane |
| The caster, the elements, documents (Tab pages 1, 4, 5) | Tab | restyle: wells and rows |
| The bestiary (`UI_BESTIARY`) | B | **overhaul**: a niche for the creature, ranks as tabs, its weaknesses as inlaid glyphs |
| The Codex (`UI_CODEX`) | C | restyle: parts as tabs, entries as rows, the page in a well |
| The tally | each level's end | **overhaul**: a memorial tablet; the overlap mended |
| Esc's question (`UI_QUIT`) | Esc | **overhaul**: a headstone, the rows spaced as they fit |
| Her menu, the ledger, a stone's page (`UI_MERCHANT`, `UI_LEDGER`, `UI_STONE`) | her room | restyle; the stone's page on a headstone |
| The Compendium, the Repository, the Portal, the shrine, the bell and font (`UI_COMP`, `UI_REPO`, `UI_PORTAL`, `UI_SHRINE`, `UI_HALL2`) | her hall | restyle: list rows, a scroll mark, the pane filled |
| The L list, the altar, the Black Shrine, the full bag (`UI_WISPS`, `UI_ALTAR`, `UI_BSHRINE`, `UI_LETGO`) | the crypt | restyle |
| The cards: a level's card, death's cost, her claim (`render_card`, `render_claim`) | the crypt | restyle on a headstone |
| The shop's cards over the pedestals (`render_shop_card`) | her room | restyle: a small tablet |
| The HUD: the status bar, the readout, toasts, the target, the boss's bar, sockets, the XP line | in play | light restyle in the run's stone (section 7) |
| The map, a document's page, the reckoning's slip | | already things in the world (parchment, paper, a slip): keep; frame only where they have a frame |
| The console | the key left of 1 | keep as it is: a plain plate in the BIOS font is right for it |
| HAUNT.EXE | | not touched |

## 2. The look

### 2.1 The idea

Every screen is a stone in the crypt, cut for its purpose. A full screen is a **ledger stone**: a slab laid flat in a
floor, its edge a moulding. A question or a card is a **headstone**: arched at the top. A toast is a small **tablet**.
Text is cut into the stone; what is coloured is inlaid (enamel, gilt, or a light in the groove, the element's own ramp,
kept full-bright as now). Wells are sunk where lists and pages sit, a step darker than the slab, so there is somewhere
quiet to read. Black is left for the deepest grooves, a shadow under a moulding, and the crypt seen dimly behind a small
menu.

The marble is **weathered grey**, mid-dark (its body at about a quarter to a third of white), with paler veins, hairline
cracks, chips at the corners, and a little lichen low on the slab (two or three `R_MOSS` pixels, not more). Light text
reads on it; the elements' full-bright colours glow out of it.

Each stone carries a small sign of whose it is: the Lampstand's corners a candle each, her stones a cat's paw, the
bestiary bones, the Codex a key, the Portal its violet. Small (an 8x8 ornament at the corners or in the title's
plate), never in the way of text.

### 2.2 The palette (U1)

The menus can't simply use `R_STONE`: it changes colour and darkens run by run (the Deep's is about two thirds as bright
as the Crypts'), and the walls behind use it. Three ways:

- **(a) Her marble, borrowed for the menus (my choice).** While a menu is up, the frame's `R_FLOOR` and `R_CEIL` ramps
  become two marble ramps of our own: the slab (`R_CEIL`: sixteen greys from groove to vein) and the wells and shadows
  (`R_FLOOR`: a darker, cooler stone, never quite black). The crypt already drawn behind is first remapped, each floor
  and ceiling pixel to the `R_STONE` or `R_WARM` entry nearest its brightness (one pass over the frame, cheap, and hidden
  under the dimming). No menu uses those two ramps today, so nothing else changes. The marble leans a tenth towards the
  run's stone, so a menu in the Hollows is a warmer grey than one in the Crypts, but always as bright. The HUD, drawn
  without a menu, keeps the run's stone.
- **(b) The run's own stone.** Draw everything in `R_STONE`. No palette work; the menus darken and change colour with
  the run (the Deep's violet ash is dim and muddy for text).
- **(c) A fixed ramp taken from elsewhere.** Free sixteen entries by merging two ramps the game uses. That touches the
  art everywhere; no.

### 2.3 Text (U3, U4)

Sizes and spacing stay exactly: every glyph on an 8 px advance, 8x8 and 8x16 cells, the line gaps each screen has now, the
tally's title at scale 2. What changes is the faces and how they are cut.

- **Three faces of our own**, drawn by hand in `src/font_ui.c` (as the art is: original, no font licence needed), capitals,
  figures and the punctuation the game uses, plus the hint's button codes (`\1`...):
  - **Inscription 8x16**: Roman capitals with serifs and thick and thin strokes, as cut on a headstone. Titles, the
    tally, big figures.
  - **Inscription 8x8**: the same in small, for headers, tabs and labels.
  - **Body 8x8**: a plain serif small capital, stroked a little lighter than the BIOS face but still two pixels where it
    must be, for the reading text. It must read at 320x200 on a real screen: the BIOS face reads because it is bold, so
    this one is drawn and checked against it before it replaces it anywhere.
  - Each face leaves its eighth column and eighth (or sixteenth) row empty, so a cut letter's highlight stays in its cell
    and widths don't move.
- **Ways to set them** (`ui_text`): *cut* (the letter in shadow, a pixel of the slab's highlight below and right of each
  stroke: incised), *inlaid* (the letter in its colour with a dark rim: for every coloured word, so a gold price or an
  element's name stays its colour), *plain* (as now, for small print and figures). The default is cut for the slab's own
  words (titles, labels) and inlaid for anything coloured.
- The BIOS and VGA faces stay for the console, HAUNT.EXE and anything not yet moved.
- A face is chosen by a switch (`gfx_face8`, `gfx_face16`) that `render_ui` sets and puts back, so the many
  `gfx_text8` calls in the menus pick up the new faces without each being touched, and the HUD and console keep theirs.

### 2.4 The kit (`src/crawl_ui.c`)

The structured elements the owner asked for, one file, used by every screen, so that each screen is laid from the same
pieces and no screen draws its own lines again:

- `ui_slab(x, y, w, h, kind)`: the marble cut to a shape (`SLAB_LEDGER` square-cornered, `SLAB_HEAD` arched, `SLAB_TABLET`
  small), with a bevelled moulding (light above and left, shadow below and right), chipped corners and the owner's
  ornament. The marble is one picture of the whole screen (`g_marble`, made once a level from its seed: veins by a
  turbulent field, cracks by a few random walks, chips), so two stones side by side are one stone cut twice, not a
  pattern repeated.
- `ui_well(x, y, w, h)`: a sunk panel, a step darker, a pixel of shadow along its top and left and of light along its foot
  and right.
- `ui_rule(x0, x1, y, orn)`: a carved line, with an ornament at its middle if asked.
- `ui_title(y, text, plate)`: a title cut in the inscription face, on a raised plate if asked.
- `ui_tabs(...)`: a row of tabs (the six elements, the Codex's parts, Tab's pages, the bestiary's ranks): the chosen one
  raised and lit, the rest sunk.
- `ui_list(...)`: rows in a well with one height, the chosen row lit (a warm wash in the groove and a gilt bracket at
  either end, in place of today's candle, gold frame or grey bar by screen), a scroll mark (a carved thumb on a groove)
  where today it says `1 MORE`.
- `ui_socket(x, y, size, icon, state)`: a round or square socket with an icon in it, in the states of section 5.3.
- `ui_pane(...)`: the right-hand pane (section 5.2), its parts in order, each wrapped in its own space.
- `ui_meter(...)`: a groove that fills (health, the sprint, a boss, experience, a slow thing's work).
- `ui_keys(...)`: the hint line, its keys on small raised caps behind the letters (the caps fit behind the 8 px cells, so
  `hint_width` is unchanged), in the pad's colours as now.
- **Fit checks.** In the debug build every `ui_text` knows the box it was set in and logs `UI overflow <screen> <text>`
  when a line runs out of it or over another; a script opens every screen with long values (gold in millions, the longest
  names, every note at once) and fails on any such line. The tally's overlap would have been caught by it.

## 3. Motion and sound

Small and in keeping: a candle at the Lampstand's corners flickers; a node that can be bought breathes a faint glow; buying
one runs its colour along the grooves from the node it hangs from (a third of a second) with the sound buying makes now; a
chosen tab or row eases in over two frames. Nothing that holds up the hand of a person in a hurry: every key does what it
did the frame it is pressed.

## 4. Ground rules

- Visual only. The keys, the pad's buttons, the order of rows and what Enter does on each stay as they are, unless a
  question below changes one.
- Every word on a screen today is on it after, at its size; a description may wrap differently in a narrower pane, never
  be cut. Where a screen's text wouldn't fit its new layout, the layout gives way.
- 320x200 and the 8-bit palette, nothing else. No new ramps but the two borrowed while a menu is up.
- The HUD and every card must still read over the crypt at its darkest (the Deep's last levels) and brightest (a geode's
  glow, the Backrooms' yellow, Severance's white).
- Works with the pad: every layout below is walked by the D-pad and stick in the directions it shows.

## 5. The Alignment (the focus)

### 5.1 The layout

One ledger stone over the whole screen but a 4 px margin of the dimmed crypt (U10). Sizes in pixels at 320x200:

```
  0   8                                   172 178                                311 319
  4 +-----------------------------------------------------------------------------------+  moulding, 4 px
 10 |  (F)17  (C)20  (L)-   (P)-   (S)12  (X)-                         0 POINTS        |  the six seals, y 10-24
 28 |---------------------------------------- rule ------------------------------------|
 32 | SPIRIT  RANK 12   [mark]             | [ICON ] SOUL TETHER                          |
    |                                      | [32x32] ULTIMATE . PATH A                    |
 40 |               (o)                    |         5 POINTS                             |  row 0  the opening
    |              /   \                   |-------------------------------------------- |
 64 |          (o)       (o)               | HOLD TO KEEP THE BEAM ON,                    |  row 1  Efficiency, Insight
    |              \   /                   | DRAINING AND SLOWING; IT                     |
 88 |               (O)                    | LEAPS ON WHEN WHAT IT                        |  row 2  the ability
    |              /   \                   | HOLDS DIES.                                  |
112 |          (o)       (o)               |                                              |  row 3  Efficiency II, Insight II
    |           |         |                |                                              |
136 |          (O)       (O)               |                                              |  row 4  path A's, path B's
    |           |         |                |-------------------------------------------- |
162 |         ((*))     ((*))              | THE OTHER PATH WILL CLOSE.                   |  row 5  the ultimates
    |                                      | AGAIN TO TAKE THIS ONE.                      |  (the state, or the warning)
182 | [1-6]  [WASD]  [ENTER] BUYS  [SPACE] UNBINDS                                       |  the keys, y 182-190
195 +-----------------------------------------------------------------------------------+
```

- **The seals** (y 10-24, across the top): each element's mark as a carved medallion 14 across (today's 7x7 glyph,
  redrawn at 12x12), its rank beside it inlaid in its colour, or a cut dash if the line isn't open. The chosen one raised
  and lit, the class's own with a gilt ring. Points at the right, gold, breathing while there are any to spend (the
  Witchfinder's Fire count as now).
- **The tree** (x 8-172): in a well. Its title line (the line and its rank, the class's mark) at the top. Ten sockets in
  six rows on three columns (x 52, 90, 128): the opening; Efficiency and Insight; the ability; then path A down the left
  (Efficiency II, its second, its ultimate) and path B down the right (Insight II, the Mastery, its ultimate). The common
  nodes 20 across, the ability and the paths' seconds 22, the ultimates 26 with a double ring. **No names in the tree**
  (U6): the icons, and the pane says what the chosen one is. Under a node of more than one point, its points as pips cut in
  the stone (filled in its colour as bought). The grooves between them are cut channels; a groove fills with the element's
  colour once the node above it is held, so a line's progress reads at a glance from across the room.
- **The pane** (x 178-311, 16 columns): in its own well.
  1. The node's icon at twice its size (32x32) in a socket, and beside it (12 columns) its name in the inscription face,
     wrapping to two lines (`BURNING INTENSITY`), and under it its kind: `OPENING`, `PASSIVE`, `ABILITY`, `PATH A`,
     `PATH B`, `ULTIMATE . PATH A`.
  2. Its cost: `5 POINTS`, or `2 OF 3` with pips, or `YOURS`, inlaid gold.
  3. A rule, then its description (the `does` text as now), 16 columns, up to ten lines at 9 px; the longest today is 121
     letters, about eight lines.
  4. At its foot, its state, one line or two: `NEEDS COMMUNION`, `THE OTHER PATH IS CHOSEN`, `TWO POINTS SHORT`, or the
     warnings that today replace the description (the other path closing, unbinding and its two coins), in gold; a
     warning takes lines from the description if it must, never the other way.
- **The keys** (y 182-190): today's hint lines, on caps (`1-6  WASD  ENTER BUYS  SPACE UNBINDS`; Tab's view `A/D PAGE
  1-6 LINE  W/S NODE  TAB`).
- Moving: as now (`update_align` already goes by row and column: W/S up and down, A/D across, the pad's D-pad and stick
  the same, 1-6 the lines); the sockets keep today's rows and columns, so nothing about it changes.

### 5.2 Tab's view

Tab's fourth page shows the same stone, smaller in nothing: the same tree and pane inside Tab's own frame (its page tabs
across the top, so the seals move down 4 px and the tree's rows close up by 2 px each). Nothing can be bought there, so
the pane's foot says where it is bought (`AT THE LAMPSTAND`).

### 5.3 A socket's states

The story of a socket is a carving filled in. Unbought, the icon is **cut into the stone**: the slab's own greys, a
relief. Bought, it is **inlaid**: the carving filled with its element's colour, lit from within, a gilt ring round the
socket.

| State | Socket | Icon | Groove into it |
|---|---|---|---|
| Out of reach (the node above it not held) | sunk, plain rim | cut in stone, dim | cut, empty |
| Can be bought now (held above, points enough) | rim in its colour, breathing | cut, a glow in the cut | cut, empty |
| Held above but too few points | rim pale | cut | cut, empty |
| Part bought (some of its points) | rim in its colour | inlaid at half light | filling |
| Bought (yours) | gilt ring, a soft halo for an ultimate | inlaid, full colour | filled in its colour |
| Closed (the other path taken) | cracked, a chain across the **socket, not the icon** | cut, very dim | cracked |
| Chosen | a gilt bracket at four corners, pulsing gently | as its state | |

A Witchfinder's free Fire and a line kept from a class given up (`RUN.al_gift`) show as bought, with the class's mark in
the pane.

### 5.4 The icons (U5)

Sixteen pixels square, drawn by hand in the art's way (an `ICON_ART` of strings, each letter a tone of the element's ramp
or the stone's), ten a line:

- **Shared shapes, tinted by line:** Efficiency (a drop poured, less), Insight (an eye half-open, opening), Efficiency II
  and Insight II (the same with a second mark).
- **Each line's own:** its opening (the spell's form: the fireball, the shard, the bolt, the flask, the wisp, the spear),
  its ability (Firestarter, Deep Chill, Ionization, Seep, Harvest, Impale), path A's second and ultimate (Burning
  Intensity and Firewall, Permafrost and Shatter...), path B's second and ultimate (the Masteries and Flamewalker, Winter's
  Lance, Amperage, Contagion, Wisp Host, the Ballista).

That is four shared and thirty-six of the lines' own: forty icons. The same icons serve Tab's view, the Codex's lines and the
sockets of section 6 where an ultimate appears.

Each is drawn in five tones (`ICON_ART` letters a-e), mapped to the stone's greys when cut and to the element's ramp when
inlaid, so one drawing makes every state.

## 6. The other screens

### 6.1 The same pattern: sockets on the left, the pane on the right

- **Cornerstones** (`UI_STONES` and Tab's third page): the five as large sockets on a carved arc (Second Wind,
  Premonition, Cartographer, Lantern of the Dead, Grave Sense), each its icon; the pane as section 5's: what it does, when
  it comes (at each tenth level), whether Second Wind is spent and its coin.
- **The Hexed Totem** (`UI_TOTEM`): four columns, a boss each (the knight, the spider, the Deacon, the Wraith), three
  sockets a column for the ranks; the scrip hanging on the totem's hooks as the column's head; the pane its rank's numbers,
  its price in that boss's scrip, and the readied one.
- **Ascend** (`UI_ASCEND`): its fourteen lines as carved rows down the left, each its small icon, its steps as pips, the
  overlevelled with `eerie_eye`; the pane the chosen line's numbers now and after one more step, its cap, the points left.
  It keeps its scroll, now shown by a thumb.
- **The class** (`UI_CLASS`): six medallions in two rows of three (today's `class_mark` redrawn at 24x24), the chosen
  one's name, past and gifts in the pane, her question at the top.
- **The wardrobe** (Tab's first page): the seven worn slots as sockets round a carved figure (hood at the head, amulet at
  the throat, robes, bracer and ring at the hands, boots at the feet), each with its piece's icon in its quality's colour;
  the bag's places as a row of sockets under it; the pane the chosen piece's card (`piece_card` as now). Pieces need
  icons: one per kind (seven), tinted by quality, at first; a look per ability later if wanted.

### 6.2 Restyled on the kit

- **The bestiary:** the creature stands in a carved niche (a well with an arched top and a little light from above, so its
  eyes show as the art pass wants), its ranks as tabs above the niche, `SLAIN` as a meter in the well, *what hurts it* as
  the six elements' glyphs inlaid (more, as usual, less, not learnt), the words beside.
- **The tally:** a memorial tablet. `LEVEL 11 COMPLETE!` cut in the inscription face across its head, the level's name
  under it as an epitaph, inlaid; each row a label cut in the stone, leader dots as a carved line, the figure inlaid gold.
  The notes (`HALF`, `+10%`, `BOON +20%`, `CURSE -20%`, `1 TRIAL FAILED`) go on a line of their own under their row, so
  they can't meet the figure.
- **Esc's question:** a headstone, arched, `LEAVE THE CRYPT?` cut in its arch, her line under it; the rows in a well,
  spaced to fit (so a sixth row for the pad doesn't crowd them), the chosen one lit with its two candles (they suit a
  headstone); the note on its own line inside the frame.
- **Her hall's lists** (the Compendium, the Repository, the Portal, the shrine, the bell and the font, her menu): their
  rows on `ui_list`, a scroll thumb, prices inlaid gold or blood, and the pane under or beside the list always holding
  something (the chosen thing's card, or what the place is for, in her voice) so it is never a black space.
- **The ledger and a stone's page:** her book stays a book where it is one; the stone's page is a headstone, as the plot's
  stones are.
- **The cards** (a level's card, death's cost, her claim): headstones over the dimmed crypt.
- **The L list, the altar, the Black Shrine, the full bag:** tablets with `ui_list`.
- **The Codex:** its parts as tabs, the entries as rows, the page in a well, each entry's glyph (the elements', the
  abilities') inlaid.

## 7. The HUD (U11)

Drawn over the crypt with no menu, so in the run's own stone (`R_STONE`), not the marble. A light pass: the status bar
keeps its Wolfenstein layout and its figures, its plates cut and bevelled in the run's stone in place of the flat grey, the
portrait in a carved frame, the souls' line in a groove; toasts as small tablets; the target's readout and the boss's bar on
a carved band; the sockets of the boss abilities and runes as `ui_socket`. Nothing moves or grows; every figure stays where
it is.

## 8. Steps

Each step committed, pushed and logged at the end of this plan, so it can be handed on between sessions.

0. **Before.** A script (`tools/uishots.sh`) that opens every screen in the harness with the debug switches (the Lampstand,
   the Totem, her hall, the bestiary with everything met, the tally with every note, Esc with a pad), at each run's palette
   (levels 1, 11, 21, 31), and lays them out as contact sheets in `docs/uipass/`. Run now for the *before*, and after each
   step.
1. **The sheet** (approved). `make sheets` draws `ui.png`: the two marble ramps (and the run's stone beside, for U1), the slab, well,
   headstone and tablet with their mouldings and ornaments, the three faces in each way of setting them beside the BIOS
   ones at 1x and 2x, the kit's pieces, the Alignment laid out for all six lines in every socket state, and the forty
   icons cut and inlaid. **For the owner's approval before anything goes in the game.** Tuning happens here.
2. **The kit and the faces in the game** (built). `crawl_ui.c`, `font_ui.c`, the palette borrowed while a menu is up, `panel` and
   `card_box` rerouted to `ui_slab`: every menu wears the marble at once, laid as it is today. The fit checks.
3. **The Alignment**, both its screens, from the approved sheet (built).
4. **Cornerstones, the Totem, ascend, the class**, on the same pattern (built; the Totem and the class with their words
   in a band below, see the log).
5. **Tab:** the wardrobe and its icons, the caster, the elements, documents; **the bestiary; the Codex.** (built).
6. **Her hall:** her menu, the Compendium, the Repository, the Portal, the shrine, the bell and font, the ledger, a stone's
   page, the full bag (built).
7. **The crypt's:** Esc's question, the L list, the altar, the Black Shrine, the cards, the tally (built).
8. **The HUD** (built).
9. **The checks** (done): every screen at each run's palette, with keys and with a pad, at the darkest and brightest crypt behind;
   the fit script clean; a dream run with `CRAWL_RUNID` gives the same `EVENT` lines before and after (nothing in the
   simulation moved); ASan over sessions that open every screen; the README's previews retaken where they show a menu.

Steps 3-8 each end with a sheet or frames from the game for the owner, as the art pass did.

## 9. Questions for the owner

My answer is marked **(mine)**; the owner's take it over.

- **U1. The marble's colour.** (a) **(mine)** a marble of our own for every menu, borrowed into the floor and ceiling ramps
  while a menu is up and leaning a tenth to the run's stone; the HUD in the run's stone. (b) Everything in the run's own
  stone, darkening by run. (c) Something else.
- **U2. Which stone.** (a) **(mine)** a weathered grey marble, mid-dark, pale veins, light text cut in it, the colours
  inlaid. (b) A pale white marble with dark text cut in it (closest to a new headstone, but coloured text loses contrast on
  it). (c) A black marble veined in gold (rich, but it is the black the owner wants less of).
- **U3. The faces.** (a) **(mine)** all three new (inscription 8x16, inscription 8x8, body 8x8), the body face only once it
  reads as well as the BIOS one on a real screen. (b) New title faces only; the BIOS face kept for reading text. (c) Keep
  both old faces and only cut them (shadow and highlight).
- **U4. How text is cut.** (a) **(mine)** the slab's own words cut, anything coloured inlaid with a dark rim, small print
  plain. (b) Everything inlaid. (c) Everything plain on the marble.
- **U5. The icons.** (a) **(mine)** 16x16, drawn by hand, forty of them, cut when unbought and inlaid when bought. (b) 12x12,
  quicker to draw, less to say. (c) 16x16 but each line's own only, the shared four as plain marks.
- **U6. Names in the tree.** (a) **(mine)** no: icons only, the name in the pane. (b) The chosen node's name under it as
  well. (c) Every name under its socket in the small face (there is room for about ten letters).
- **U7. When.** PLAN-ai's step 11 (showing the minds: captions, the bestiary's lines, the Codex, achievements) is next, and
  the art pass waits after it. (a) **(mine)** this pass's steps 0-2 now (the kit), then PLAN-ai's step 11 built on the kit,
  then the rest of this pass. (b) This pass whole, then step 11. (c) Step 11 first, then this.
- **U8. Its number.** (a) **(mine)** part of 1.0.0b15 with the minds. (b) A release of its own after it.
- **U9. Shapes.** (a) **(mine)** ledger stones for full screens, headstones for questions and cards, tablets for small
  menus and toasts. (b) One shape for all.
- **U10. Full screens.** (a) **(mine)** to the edge of the screen, the moulding at its rim (no crypt peeping round a full
  menu). (b) As now, the crypt dimmed in a 4 px margin.
- **U11. The HUD.** (a) **(mine)** a light pass in the run's stone, nothing moved. (b) Leave it as it is. (c) Redraw it in
  the marble's manner too (it would need the borrowed ramps in play, which the floor and ceiling use).
- **U12. The owners' signs** (a candle, a paw, bones, a key on each stone). (a) **(mine)** yes, small. (b) No.
- **U13. The mouse in menus** (hover to choose, click to buy). (a) **(mine)** not in this pass. (b) Yes, while the screens are
  being redone.

## 10. Hand-off log

- 3 November 2026: the plan drafted on `claude/gifted-rubin-q8up37`, and settled the same day (the table at the top).
- 3 November 2026, step 0: `tools/uishots.sh <label> [levels]` opens eighteen screens in the debug harness (the
  Lampstand, the Totem, the Portal, the Compendium, the Repository, her menu, ascend, the bell, the font, Esc's question,
  Tab's wardrobe, caster, cornerstones and Alignment pages, the bestiary, the Codex, the controls card, the tally on a
  corrupted level) at the palettes of levels 1, 11, 21 and 31, and lays them out as `docs/uipass/<label>-L<level>.png`
  (frames in `build/uishots/`). The *before* sheets are `docs/uipass/before-L*.png`. Not reached by it yet: the shrine
  (no switch stands you before it), a stone's page (a fresh run has none), the ledger, the class question, the L list,
  the altar, the Black Shrine, the full bag and the cards; they are added as their steps come. They showed three faults
  more (section 1.2: ascend's rows, the controls card over the shop's, a toast over the tally).
- 3 November 2026, step 1: the kit is `src/crawl_ui.c` (`crawl_ui.h`: the marble's ramps and picture, `ui_lend`, slabs,
  wells, plates, rules, ornaments, the faces' ways, wrapping, key caps, icons, sockets, grooves, and `ui_tree`, the
  Alignment's stone laid from a `UiTree` the game or the sheet fills) and `src/ui_art.c` (the two 8x8 faces and the icons
  as strings; `ui_face_init` builds the tables and stretches the inscription face to 8x16). Both are in the game's build,
  not yet called. `make sheets` now also runs `build/uisheet all`, drawing `ui.png` (six Alignments in their states, the
  faces, the stones and sockets, the icons cut and inlaid, the marble) and `ui_runs.png` (the marble's lean at levels 1,
  15, 25 and 38); copies are `docs/uipass/sheet-ui.png` and `sheet-ui-runs.png`. `build/uisheet fit` lays every node of
  every line in the pane with its longest note and each warning and fails on any line that doesn't fit (0 now). Choices
  of mine, for the owner to overrule: words cut into the stone read only at 16 px on a smooth plate (`ui_plate`), so the
  8x8 is always inlaid and cutting is kept for titles; a warning takes the description's place in the pane, as it does
  today, rather than squeezing under it; an ultimate's kind reads ULTIMATE (its side of the tree says which path);
  Physical's first node on path A is Bone Plate, not Efficiency II, so there are forty-one icons, not forty; Lightning's
  two Masteries share a name (their numbers differ), and their icons tell them apart (one bolt, two). Still first drafts:
  the icons (Arc, Ionization and Soul Tether are thin at 16 px), and the 8x16 face, which is the 8x8 stretched and is to be
  drawn by hand once the look is settled. The three 8x8 faces differ little at a glance: the body face is kept as heavy as
  the BIOS one so it reads. Next: the owner's word on the sheet, then step 2.
- 3 November 2026: the owner approved the sheet.
- 3 November 2026, step 2: the kit in the game. `gfx.c` has two hooks (`gfx_hook8`, `gfx_hook16`): while set, a glyph with no
  background goes through them. `ui_on(f, marble, where)` sets them, makes the level's marble if it hasn't been (it is made
  in `start_level` after `crawl_art_level`, 9 ms, so never on a frame in play) and, for a menu, lends the floor and
  ceiling ramps once a frame (`ui_lent`); `ui_off` clears them. `render_ui`, `render_tally` and `render_claim` are wrapped
  in it with the marble; `render_card` (the level's card, a boss's, a death's) with the run's stone (`ui_ramps(0)`: `UI_M`
  and `UI_W` are then `R_STONE`), since it lies over the live crypt and lending would recolour the floor behind it for
  three seconds. Through the hooks every `gfx_text8` is the body face inlaid and every `gfx_text16` the inscription face
  inlaid, and a colour from the run's stone becomes the marble's grey of the same rank, so greys read alike in every run.
  `panel` and `card_box` are `ui_slab` with `SLAB_CALM` (the inside's shades pressed into 2-8, so words greyed for
  black still read; a choice of mine, until each screen gets its own wells); no owners' signs yet, since today's titles sit
  where they would go. Paper keeps its own ink: the document's page and the reckoning's slip turn the hooks off
  (`UI_PLAIN_BEGIN`/`END`). The tally's hand-drawn drop shadow went (the inlay is its shadow). The fit checks in the game:
  in the debug build every glyph the kit draws is stamped (`ui_check`, `ink`) and one landing on another's ink or outside
  the last stone cut is told, once a row, as `UI overflow <screen> overlap|outside on row y`; `tools/uishots.sh` gathers
  them into `build/uishots/<label>/L<level>/fit.txt`. At all four palettes the eighteen screens gave one: the tally's
  CORRUPTED line, which sits under its card on purpose (step 7 lays the tally anew). The checks do not see what is drawn
  after the menus (toasts, the console), so a toast over a menu's title (seen in a Backrooms pocket's wardrobe) is for
  step 8. Checked: a 900 s dream run (seed 5, `CRAWL_RUNID=77`) and a person's session through the reckoning, the tally,
  Tab, Esc and the bestiary give the same `EVENT` lines as step 1's build, line for line; ASan clean over the menus, the
  tally and a 400 s dream; `make smoke` 1.39 ms a frame, 1.6-1.7 ms with a menu up. The Windows build was compiled and
  smoke-tested by CI on the branch (run 81, started by hand). The after sheets are
  `docs/uipass/step2-L*.png`. Nothing played by hand. Next: step 3, the Alignment.
- 3 November 2026, step 3: the Alignment. `render_align(f, view)` fills a `UiTree` from the line as it stands and hands it
  to `ui_tree`; `align_box` and the old tree are gone. A node's state comes from the functions the old screen used: held
  (the opening by `align_open`, so Fire and the class's line show bought), closed (`align_chained`, the other path taken),
  part bought (points in it, not full), ready or short (`align_ready`, against `align_funds`), else out of reach. The
  pane's foot says why: `ONE POINT SHORT.`, `NEEDS EFFICIENCY AND INSIGHT, BOTH FULL.`, `NEEDS <the node above>.`, `OPEN
  <line> FIRST.`, `THE OTHER PATH IS CHOSEN.`, `YOURS BY WHAT YOU WERE.` (an opening a class or a past gave), and on Tab's
  page, where nothing is bought, `BOUGHT AT THE LAMPSTAND.` The three warnings (`G.al_arm`: the other path closing,
  unbinding, too few coins to unbind) take the description's place, in gold, as they did. The hint line goes through `pk`,
  so a pad's words and its face buttons' colours come out on the caps (`ui_keys` reads the `\1` marks;
  `ui_keys_width`). The kit gained `UiNode.steps` (pips only under nodes bought a point at a time: `align_point`; the
  ultimates had shown five pips for a price paid at once) and `UiNode.grow` (a node just bought runs its colour down its
  groove in a third of a second: `al_buy_t/e/n`, set in `update_align`). Names wrap at ten letters, not eleven (SOUL TETHER
  touched the well's lip); a line not open has its seal's mark cut pale, not near black. The moving keys are as they were
  (`update_align` already went by row and column). Checked: the fit check at four palettes finds nothing new (the tally's
  line, as at step 2); `build/uisheet fit` still 0 over the sixty nodes; a person buying an opening and Efficiency's two
  points, then unbinding the line (with `CRAWL_ALPTS=200`, `CRAWL_COINS=3`), and a 900 s dream give the same `EVENT` lines
  as step 2's build; ASan clean over that session and 200 s of the dream from level 21; the Windows build compiled and smoke-tested by CI on
  the branch (run 82, started by hand). Frames: `docs/uipass/step3-L*.png`
  (`lamp` and `alignment`). Nothing played by hand. Next: step 4, the cornerstones, the Totem, ascend and the class.
- 3 November 2026, step 4: the cornerstones, the Totem, ascend and the class. The kit gained `ui_screen` (the slab, a title
  and a figure across the top, a well and the pane's well from a split), `ui_pane` (the pane taken out of `ui_tree`, which
  now uses it; its width sets its columns; a name with a word too long to sit beside the socket goes under it), its wide
  form `ui_pane_wide` (a band across the stone: the mark, the name, the kind on the same line when it fits, the cost at the
  right), `ui_socket_mark` and `ui_mark` (the game's own marks, cut or inlaid like the icons, doubled when the socket has
  room) and `UI_LMARK` (eleven seven-square marks for ascend's lines; the five resists take the elements' glyphs). No new
  art otherwise: the sockets hold the marks the screens had (`CS_MARK`, `BA_MARK`, `CLASS_MARK`). *Cornerstones* (`UI_STONES`
  and Tab's third page): five sockets down a carved bow in a narrow well, their groove lit between two held, the pane wide
  (227 px) since their words run to 140 letters; the state on the cost line (YOURS, ENTER CHOOSES IT, NEEDS THE MAP, AT
  ATONEMENT n), Second Wind spent told at the foot, the asking twice in the description's place. *Ascend*: the sixteen lines
  down the left, nine pixels apart (they had been eight, drawn over one another), each its mark (the dead's eye once
  overlevelled), a dot for a step bought this visit, a scroll's groove and thumb; the pane the line's cost, what is yours,
  what it does, and its total with the next step's worth; an overlevel's word or a warning stands in for what it does, as it
  did; the EXP line drops the next level's need once the figures reach the millions. *The Totem* and *the class* depart
  from the plan's left and right, my choice, since their words would not fit a pane at the side: Tangled Web's next rank
  with whose scrip it wants is twelve lines at nineteen columns, a past and its gifts over thirty. The Totem: its four
  abilities in rows across a well at the top (the mark, the name, three rank sockets along a groove, the scrip held, a SELL tag
  lit while A/D has the sale), an ability not yet won dim with NOT FALLEN; under them the band. The class: the six marks
  in medallions across the top (the one you are inlaid, the chosen glowing), the past's name in the title's band, its
  story and gifts across a well. Keys and what each does are as they were. Checked: the fit checks find nothing new at four
  palettes (`tools/uishots.sh` now opens the class question too, and gives the Totem scrip); a 900 s dream and a person's
  sessions (ascending and buying steps, another past for five coins, a rank raised and scrip sold at the Totem, a
  cornerstone chosen) give the same `EVENT` lines as step 3's build; ASan clean over those, an overlevel and the Totem with
  nothing to teach; the Windows build compiled and smoke-tested by CI on the branch (run 83). Frames: `docs/uipass/step4-L*.png`. Nothing played by hand. Next: step 5, Tab's wardrobe and pages, the
  bestiary, the Codex.
- 3 November 2026, step 5: Tab's pages, the bestiary and the Codex. Tab's six pages share a frame (`tab_frame`: the slab,
  the page's title and its figure across the top, the hint on caps under them, where the hints were, since these pages are
  full to the foot). *The wardrobe*: the seven worn places as sockets on a figure (the hood, the amulet, the robes at the
  heart, the bracers either side, the ring on the right hand, the boots), joined by grooves in bone, a held socket's piece
  drawn in its quality's colour, an anointed one ringed as an ultimate is; the bag's places in a row of small sockets along
  the well's foot (or *NO BAG*); the portrait in a bevel by the hood; on the right the chosen place large in a socket, its
  name and what Enter does there, and the piece's card as it was, under a rule. The kit gained seven icons, one for each kind
  of piece (bracer, boots, amulet, hood, robes, ring, bag), so the sockets aren't empty when nothing is worn. *The caster*, *the
  elements*: their rows as they were, in a well; the dotted line between the elements' parts a rule. *Documents*: the list in a
  well on the left, the chosen row lit; the page in a well on the right, the paper and its ink untouched (`UI_PLAIN`). *The
  bestiary*: the creature stands in an arched niche (`ui_niche`, new: a well with a rounded head, lit a little from above), its
  ranks as lozenges at the niche's foot, its words in a well beside it and what hurts it in a well across the foot; the
  achievements in a well with a groove and thumb for the scroll in place of the dots. *The Codex*: `ui_screen` with the
  entry's name as the title and its place in the part on the right, the part's name under it and the parts as small lozenges
  at the right of that line (the gold one yours; at 9 px apart they had run under the longest part's name), the keys on caps
  along the foot. What each key does and every word are as they were. The caster's runes, when more of them are held than the
  well has room for, stop and tell the fit checks (`caster runes`) rather than run off the stone; none did in the shots.
  Checked: the fit checks at four palettes find only the tally's line, as before (`tools/uishots.sh` now opens the elements,
  documents and achievements pages too); a 900 s dream and a person's session through every Tab page, the wardrobe's moves,
  the bestiary's creatures and ranks, the achievements and the Codex's parts give the same `EVENT` lines as step 4's build;
  ASan clean over three such sessions (levels 1, 21 and 31). Frames: `docs/uipass/step5-L*.png`. Nothing played by hand.
  Next: step 6, her hall's menus.
- 3 November 2026, step 6: her hall's screens. The kit gained `ui_row` (a list's chosen row: the well's stone pressed paler, gold
  notches at its ends, as ascend's had been) and `ui_scroll` (a groove and its thumb). The game has `hall_frame` (the slab, its
  title and a figure at the right) and `hall_keys`/`hall_word` (the keys on caps along the foot, or a word of hers in their
  place). *Her menu*: its eight rows in one well, what the chosen one means in a well under it. *The ledger*: her book on a
  ledger stone with her paw in its corners, the three pages in a well. *A stone's page*: a headstone, as the plot's stones
  are, with roses in its shoulders; the epitaph centred under the arch, the date and which stone at either end of a line,
  what was carried in a well on the left and the chosen thing on the right. Its runes' column moved right: WITCHFINDER had run
  into it (the fit checks found it, the first time a stone was shown). *The Compendium*: the entries in a well with a thumb
  for the scroll in place of `n MORE`, the card in a well under it, the price's factor or a passing word at the foot. *The
  Repository*: the places in a well on the left, the money and its rows on the right; the keys at the foot, a passing word
  in their place as it already did when choosing what goes in. *The Portal*: its rows and headings as one list of nine lines
  that scrolls by the chosen row, and what the chosen one does in a fixed well under it. With three runs open and the
  levels' row its words had had one line (cut off), and with more runs none; now five. Its long rows (RUN II with HERE) had
  run into their figures by 4 px; they are moved apart. *The shrine*: the nine slots as cups cut in a well (the chosen one or
  one filling ringed as it was), the rows and the note in another, what you hold across the foot. *The bell and the font*: a
  tablet, the rows in one well, what the chosen one does in another. *The full bag*: the bag's places in a well, the spare's
  card beside it. Words and keys are as they were, but for `n MORE` in the Compendium, now its thumb.
  Three debug switches so the shots reach what they hadn't: `CRAWL_TPSHRINE=1` (before the Rune Shrine), `CRAWL_STONE=n` (n
  stones in the plot from what is held) and `CRAWL_LETGO=1` (the full bag's question over the first ware for a worn slot, as
  you take over in her room); `tools/uishots.sh` opens the shrine, the ledger, a stone's page (and a piece on it) and the full
  bag, and the Portal with three runs, the challenges and the levels' row.
  A fault in the harness, found here: `host apl` didn't play `HOST_INPUT` (only `shot`, `wav`, `dump` and `bench` did), so
  step 5's "person's session" had been run under `apl` and compared two dreams; step 2's to 4's were made with `shot` and
  stand. `apl` now plays it. Step 5's check was made again: a session in her room through every Tab page with two pieces
  moved into a full bag and out, the bestiary, the achievements and the Codex, then her ledger signed, gives the same `EVENT`
  lines and the same signed page, byte for byte, as step 4's build. ASan's two `apl` sessions at step 5 were dreams too; the
  `shot` one wasn't, and this step's ASan sessions all take over.
  Checked: the fit checks at four palettes find only the tally's line; ten sessions of a person in her hall (her menu and the
  ledger signed, a pedestal bought; the Compendium twice bought from; a piece and a coin put in the Repository, a place bought;
  the Portal to run III, and a level replayed; a pull and a take at the shrine; the bell corrupted, the font cursed; a stone's
  page; a spare let go) give the same `EVENT` lines as step 5's build (given the same three switches), and the signed page and
  the progress are the same bytes; a 900 s dream the same; ASan clean over the ten and a 300 s dream from level 21. Frames:
  `docs/uipass/step6-L*.png` (and `before6-L1.png`, the newly reached screens as they were). Nothing played by hand. Next: step 7,
  the crypt's screens.
- 3 November 2026, step 7: the crypt's screens. *Esc's question*: a headstone, LEAVE THE CRYPT? in its arch, her line under it,
  the rows in a well (seventeen pixels apart, fifteen with the pad's sixth), the chosen one lit with its two candles, a rule
  with a lozenge between leaving and the ledger, the note on its own line inside the stone. The headstone's own candles in
  its shoulders were tried and taken off: they sat on her line. *The L list*, *the altar*, *the Black Shrine*: tablets, the
  rows in a well, what the chosen one means in another (the altar's, the shrine's) or the toggle under it (the list's: a
  cup cut in the stone with a gold cross in it when on, for `[X]`), the keys on caps along the foot (`hall_keys`). *The
  cards* (a level's, a boss's, death's cost, the old mercy's, her claim): headstones (`card_box` is `SLAB_HEAD`), their
  first line moved down a few pixels into the arch. A level's and a boss's stay over the live crypt, not dimmed, my choice
  against the plan's "over the dimmed crypt": they come while you walk, and dimming the view for three seconds would hide
  what is coming; death's and her claim's were dimmed already. *The tally*: a memorial tablet, LEVEL n COMPLETE! on a smooth
  plate at its head, the level's name under it, CORRUPTED: HALF EXPERIENCE under that (it had sat below the stone, the
  fit checks' one finding since step 2); each row's label and figure as they were with a line cut in the stone between them
  for the leader dots, and each row's notes (HALF, +10%, BOON +20%, CURSE -20%, n TRIALS FAILED) on a line of their own under
  it, each part in its colour; the rows spaced to fit (fifteen pixels to twelve), checked at the worst case (a boss's row,
  the bell, the font, a boon and two trials failed, with a copy of the source patched for it). The labels stay inlaid in
  their colours rather than cut, so EXPERIENCE keeps its violet.
  Two faults found and mended, both drawing: the controls card shown as you take over was drawn over death's card and her
  claim's (a person dying in the first seconds); and a level's card went on being drawn under a menu opened in its first
  seconds, its letters through the menu's (the fit checks found it on the altar's screen). Neither card shows now while the
  other or a menu is up. Two debug switches for the shots: `CRAWL_HOLDCARD=1` (a level's card and a boss's held up once
  shown) and `CRAWL_CLAIM=1` (her card as you take over, with a claim's words and nothing taken); `tools/uishots.sh` now
  opens the L list, the altar and the Black Shrine (on a level of the sheet's run with an altar and a puzzle), a level's card,
  a boss's, death's cost, the old mercy's and her claim's. At level 31 the altar's recipe dies before it reaches the altar;
  levels 1, 11 and 21 show it.
  Checked: the fit checks at four palettes find nothing, the first time since step 2; nine sessions of a person (Esc's rows
  and back; sleepwalking; the L list with its toggle and a cast; the altar woken; the Black Shrine; a death and the waking;
  her claim read; the tally and the reckoning; a boss's antechamber) and a 900 s dream give the same `EVENT` lines as step 6's
  build (given the same two switches); ASan clean over the nine. Frames: `docs/uipass/step7-L*.png` (and `before7-L1.png`,
  `before7-L21.png`). Nothing played by hand. Next: step 8, the HUD.
- 3 November 2026, step 8: the HUD, a light pass in the run's stone (U11), nothing moved and no figure changed. The frame starts
  with the kit's ramps on the run's stone (`ui_ramps(0)` in `crawl_render`; a menu lends the marble as before), so whatever the
  HUD cuts is `R_STONE`. *The status bar*: a tablet of the run's stone (`ui_slab`, `SLAB_TABLET`, its foot below the screen) in
  place of the flat grey, its plates sunk in it as wells (`bevel` is now `ui_well`, which also gives the face's frame, the
  keys' box, the ability sockets, the carried thing's box, and the wardrobe's portrait in the marble), the souls' line in a
  groove cut along its plate's foot; the labels, figures and their places as they were, in the VGA type (the kit's faces are
  for the stone's own words). *Words over the crypt* (the target's readout, a boss's bar and its name, the hourglass's, the
  news line, the read panel, the readout, the experience lines and the banner, a ware's card and her room's other cards, the
  toasts) sit on `ui_band`, new in the kit: a narrow band of dark stone lit along its top and left, the veins only just there,
  in place of the black boxes; a toast keeps its gold (or grey) edge inside the band. The controls card is one band behind all
  its lines, not a box a line. *The runes' sockets*: wells, edged in the rune's quality as before. The plan's `ui_socket` for
  the sockets was not taken: a round socket has no room for the line along the foot that shows the wait, so they stay square,
  cut as wells. The sprint's slim bar and the HOLD H plate (the app's, not the game's) are as they were.
  Two faults from section 1.2 mended, both drawing: a toast came over the controls card's first lines (it now waits while the
  card is up, as it waits out a boss's fight: `update_toasts`; its chime comes when it shows); and the controls card lay over a
  ware's card in her room (it gives way while a person looks at something there: `shop_carded`). A debug switch for the shots,
  `CRAWL_TOAST=1` (a pop-up as you take over); `tools/uishots.sh` now takes the HUD with the sockets, the runes, a toast and the
  lantern; a boss's fight at each run's boss (seed 7, the person walking in); the target's readout (`CRAWL_TPHOLD=0`); the dream's
  readout raised over HOLD H; a ware's card.
  Checked: the fit checks at four palettes find nothing; four sessions of a person (the HUD with its sockets, casting and an
  ability; a boss's fight; her room at the start; a target struck) and a 900 s dream give the same `EVENT` lines as step 7's
  build; ASan clean over the four and a 300 s dream from level 31. A frame costs a little more: a person walking with the bar
  up 1.74-1.82 ms against 1.61-1.67 (two runs of `bench` each), the dream 1.52-1.55 against 1.41-1.44. Frames:
  `docs/uipass/step8-L*.png`. Nothing played by hand. Next: step 9, the checks, with keys and with a pad, and the README's
  previews where they show a menu.
- 3 November 2026, step 9: the checks. `tools/uishots.sh` gained `PAD=1` (every recipe played on a pad: the takeover held on X, E and
  Enter on A, Esc on Menu, Tab on View, Space and L on Y, the D-pad for W/S/A/D in menus and for B, C and H in play; the recipes
  that walk are left out) and `HOST=` (another build, an ASan one), and a recipe for the wardrobe in a Backrooms pocket, the
  brightest crypt behind a menu. Every screen at the four palettes, with keys (`docs/uipass/step9-L*.png`) and with a pad
  (`step9pad-L*.png`): the fit checks find nothing. In a pocket the menu reads, the yellow carpet and paper go grey in the 4 px
  margin while the marble is lent, and come back as the menu closes. One more fault, mended: a toast went on being drawn over a
  menu's corner and over the tally's title (section 1.2 had found it there at step 0); toasts now wait while a menu or the tally is
  up, as they wait for a boss's fight and the controls card. ASan clean over every recipe at levels 1 and 31 with keys and at 21
  with a pad (113 sessions). The pass whole, against the build it began from (main at the minds' steps 0-10, its `apl` given the
  harness's fix for `HOST_INPUT`): two 900 s dreams, a 600 s dream from level 21 and six sessions of a person (the ledger signed,
  Tab's pages and pieces moved, the Lampstand bought from, the tally and the reckoning, Esc and the L list, a pad through Tab and
  the bestiary) give the same `EVENT` lines, line for line. A debug switch, `CRAWL_SEENALL=1` (every kind met, some slain), for
  the bestiary's shots.
  The README's previews that show a menu were taken again (ten strips: trials, alignment, atonement, pad, ledger, hall, shrine,
  bestiary, wisps, play, items): their menus are new frames of the same screens, the frames without a menu kept from the old
  strips; the old strips are in the history. The page's descriptions of them say what they now show, and its note says which
  frames are older. Nothing played by hand. The pass is built; what is left is the owner's look, and the release.
