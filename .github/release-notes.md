The nineteenth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

Harm goes by its element now, and each element leaves its mark, on you and on the dead. It also mends the trial's door, hangs a lamp against the Wraith by each challenge, and lets you lay out your controller yourself.

### Before you upgrade

Your saves and settings carry over unchanged, and going back to an earlier beta is safe.

### New in this beta

**Harm by element.** Each element is kept off by its own resist: fire by Resist Burn, frost by Resist Slow, lightning, poison and spirit by theirs. Armour keeps off a share of a blow, not a set number, and half as much against the elements, so it counts as much deep down as near the top. A skeleton never throws quicker than every 0.6 seconds.

**What the elements leave on you**, shown as a row of marks by your portrait and in the view itself:

- **On Fire**: the hit's harm again over four seconds.
- **Chilled**: slower a step a stack, your sprint slow to come back, your missiles a little astray. Four stacks and you are **Frozen** for three seconds, unable to move or cast, and everything strikes a third harder.
- **Stunned**: no spells or runes for 1.2 seconds, and no sprint.
- **Poisoned**: your spells half again as slow, each dose adding, up to a minute.
- **Unbound**: no spells, runes or E for 1.5 seconds.
- **Knocked Down**: 1.5 seconds on the floor, the crypt leaning over you; while you are down, everything but what put you there does half.

They come from the dead's elemental attacks (more often the higher their rank), from hazards now and then, from the Wraith's touch, from bites, bile and the cold. None of the four that hold you comes twice running. Purify cures fire, cold and poison and breaks a freeze; Blink gets you off the floor or out of the ice.

**The dead's elements.** Every creature is weak to one element and strong against one; the higher ranks shrug off more of your lines, shown once you have seen them struck and cracked by a fifth of their health taken in that element. From green rank some are born to an element: their eyes burn in its colour, and one attack in three carries it.

**What your spells leave on them.** Fire sets them burning, lightning stuns them, spirit unbinds them so they cannot call or charge, frost chills and then freezes them, poison slows everything they do.

**New attacks.** Skeletons of green rank and up flash white and lob three bones over you. A shield-bearer stamps and charges along a clear lane, knocking you down unless you dodge it, ward it off or wear armour twice its harm. The Drowned swells and spews foul water in a narrow cone. A shambler's grip now and then holds you fast.

**Trials.** A trial's door no longer shuts on you half in the doorway: a trial begins only once you are a step and a half inside, and no door that seals itself shuts while you stand in it. Win, and the trial's hoard (three times the level's chest, glowing gold) and a draught that restores you in full appear in its room; the draught waits there until you need it.

**A lamp against the Wraith.** By each trial, sealed wing and sequence of things that want a spell hangs a soul-lamp. Offer it a soul for two minutes of light, up to five, and while it burns the Wraith can't set foot on that ground; it walks the edge instead.

**Bells you can tell apart**, by their shape, the bands round them, their metal and a numeral; the Catacombs' broken bells hang still.

**Your controller, laid out.** Esc's question now holds the controller's layout: every button's job but View's and Menu's can be traded with another's, the sticks swapped, the turning made slower or quicker, the rumble turned off. View and Menu together show the layout anywhere you play.

**And**: the wheels open at the merchant's, and props narrower than a cell take only half of it or a quarter, so you can pass beside them.

This is still a beta. All of it has been tested in the harness, by the screensaver's own play, by scripted play and on sheets of the art: nobody has played it by hand yet. The Windows and browser builds are compiled and smoke-tested by GitHub Actions. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

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
