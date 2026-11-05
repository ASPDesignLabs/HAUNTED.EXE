The eighteenth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

The dead learnt to think in the fourteenth beta. This one lets you see it.

### Before you upgrade

Your saves and settings carry over. Each page of the ledger now also keeps the dead's ways you have met and your deeds against them; earlier betas simply step over that part, so going back is safe.

### New in this beta

**Their backs.** Every creature that can turn away now shows its back, and has no eyes from behind: one looking away hasn't seen you, and you may pass it.

**The roost** hangs asleep from the ceiling wrapped in its wings, and stirs as you come near.

**The ambushes' signs.** Each run marks a kill room with a sign of its own by the door: raked bones in the Crypts, a wet drag in the Catacombs, fresh scree in the Hollows, warm cinders in the Deep. A run's first ambush, its torch snuffed, has eyes in the dark just inside.

**Named, and written down.** Each of the dead's tactics (the file of the Order, the hounds' pack, the roost, the wake, the hunt, the ambush, the nest, the Sexton's bell and the rest) is named the first time it shows itself in each run of ten. Once met, it has a page in the Codex, *The Ways of the Dead*: what it does, how you hear it coming, and how it is answered. In the bestiary, **Space** turns a creature's page from its lore to its ways.

**New sounds**: a scavenger at its meal, a bait's yelp, a web thrumming as the nest drops, the dead's bones settling as they turn wary, a hound scenting the air.

**Eleven new achievements**: ambushes come through without a death, hunts shaken off, packs broken by their leader's fall, callers killed before they could call, and a file of the Order beaten without a single hurt.

This is still a beta. All of it has been tested in the harness, by the screensaver's own play and on sheets of the art: nobody has played it by hand yet. The Windows and browser builds are compiled and smoke-tested by GitHub Actions. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

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
