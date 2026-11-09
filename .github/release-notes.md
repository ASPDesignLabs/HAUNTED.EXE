The Title Screen Update: a point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It puts a title screen in front of the crypt, and new music in the merchant's hall, the first two runs and at the Lampstand.

### Before you upgrade

Your saves and settings carry over. This release adds one byte to the controller's settings, to remember which title screen came last; earlier betas ignore it. Going back to 1.0.0b21 is safe.

### New in this release

**A title screen.** Taking over no longer drops you straight into the run the screensaver was dreaming: a title screen comes up first. It has three scenes, and each start of the program shows the next in turn:
- **Beneath**: a graveyard in cross-section under a rising moon, its dead in the earth, and a walker with a lantern crossing the crypt buried under it.
- **The Walk**: the first run's hall in the first person, walked slowly into the fog, seen through an arch whose lintel carries the name.
- **The Risen**: one great headstone close up, the name cut in it, and a hand clawing out of the grave beside it.

The name is in a gothic, bloody hand that drips, and lightning comes now and then. **PRESS ANY KEY** (or **PRESS ANY BUTTON** when a controller is in use): any key or button does it, but not a stick's movement. A menu then comes into the middle while the scene goes on:
- **Choose a grave**: the ledger's three pages as graves, stone for a written page and timber for an empty one. Each stone gives its class, level and date; Enter raises it.
- **Begin a new run**: a fresh run, from her room.
- **The controls**: the keys' whole list, or the controller's own screen when a controller is in use.
- **Help**: not written yet.

Esc (B on a controller) goes back a step. Left untouched on its prompt for a minute and a half, the title gives the screen back to the screensaver.

**New music.** The merchant's hall plays a habanera, and an alley cat's swing after a perfect level. The first run's piece is now *the ill-fated*, the second's *the descent*. The Lampstand and ascending play an estampie. The title screen, and the ledger opened to choose a page, play the lament ground, a passacaglia in G minor. The third and fourth runs keep their pieces.

### What has been tested

The title screen was driven down every path in the test harness, with the keys and with a scripted controller, and checked with AddressSanitizer. The music was tested in the harness. None of it has been played by hand or run on Windows yet. A title frame is heavier to draw than a frame of the crypt (19 to 28 ms on the test machine): if it stutters on yours, please say.
