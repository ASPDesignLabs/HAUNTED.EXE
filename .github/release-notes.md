The sixteenth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

This one is about the quiet stretches of the early levels, and what you do with the spells you have.

### Before you upgrade

Your saves and settings carry over. A caster keeps every line it paid for, Fire included, but nobody has the fireball for nothing any more (below). Going back to the fifteenth beta is safe: it simply forgets the Lampstand's gifts, and offers them again if you come back.

### New in this beta

**One line to begin with.** No class starts with the fireball now. Each begins with its own line, its first rank costing no spirit, and the Lampstand gives you a second line free after level 4 and a third after level 12: the merchant holds the way on until you have taken it. The Headsman's line waits for the first of those gifts. The Bone Spear with no bones throws splinters, eight at most, one coming back every six seconds. Any spell now drives the wandering Wraith back for a moment.

**Things that want a spell.** Along the way, wherever it runs long with nothing in it, things in the crypt want a spell you hold, each with a small iron seal that draws motes of its colour in. Strike it with the right one and it gives a little: gold, gems, a potion, bones, souls, light, a short way through. The wrong spell rings dull, and the first time on a level you are told what it wants.

- On the first levels: cold prickets, votive racks, an urn of coals, a galvanic coil, a knot of roots, a soul bound in a lantern, a bone barricade, and coffers and cages in any line.
- From each run's fifth level they move, time and want two spells: a corpse-light that shies off, a skull that rolls away from you, a seal that fire heats and frost breaks, three bells that must ring at once, a portcullis winch, a dying hearth, a steam vent. Each run has its own besides.
- From level 13, sequences: a tablet's order, a turning reliquary, a ward-stone whose glyph changes, a relay of lamps.
- In the rooms, things for fights: lamp oil to fire, loose masonry to bring down, a font to fill with bile, the Order's drum to silence, standing water to shock, a floor to rime.

**Warded creatures.** From level 5 some creatures come warded against one of your lines, which then does only a small share of its damage: a hard rim in its colour, its glyph barred over its head, a dull ring when struck by it. Never your class's own line while you hold two, never every line you hold, and a fifth of its health taken in that element cracks the ward.

**The Mirror by hand.** The Mirror rune rises only when you raise it, on the right mouse button. What it meets in its first moment goes home to its thrower twice as hard. Q turns between it and the boss abilities, and held opens a wheel of them. The Rune Shrine's first curse gives you a Common Mirror if you have none.

**On a controller**, holding the D-pad up opens a wheel of the runes and abilities, and holding it down a wheel of the spells, while the crypt slows and greys. RB raises the Mirror and LB uses the ability readied.

**The Plague Doctor** bites harder each tick, and its flask strikes a quarter harder; no one else's does.

This is still a beta. All of it has been tested in the harness, by the screensaver's own play and by scripted players, and seen in frames: nobody has played it by hand yet, so how the gifts, the things, the wards and the Mirror feel is still to be judged. The Windows and browser builds are compiled and smoke-tested by GitHub Actions. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H, or a controller's button, to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `HAUNTED-<version>-web.zip` | The same game built for the browser, the page that plays it, to serve yourself. |
| `SHA256SUMS.txt` | Checksums of the files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/

Play it in the browser, with nothing to download: https://aspdesignlabs.github.io/HAUNTED.EXE/play/ (Space casts there, since Ctrl is the browser's; the ledger is kept in the browser).
