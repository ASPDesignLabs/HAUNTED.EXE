The second beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H** for three seconds.

### New in this beta

- **A wider crypt.** Main halls two and three cells wide, big chambers and colonnades, with a few narrow side passages.
- **Set dressing.** Swaying chains and meat hooks, gibbet cages, candle chandeliers, roots and cobweb curtains, graveyards of tombstones, and more light: fire pits, lit totems and a light in every room.
- **Secrets.** One to three walls a level give when pressed (E or Space). Their cracks show only when you are close. Behind each is a closet of treasure and often a potion, never a key, and it doesn't count towards the level's treasure. The tally counts the secrets found.
- **The will-o'-wisp.** Take ten souls on a level and it kindles on the back of your hand, leaning the way out (by way of a vault key if the exit needs one) and lighting the crypt around you. It goes out with the level.
- **Three save graves** in the Esc menu: carve a gravestone to save the run, rise from one to load it.
- **Balance for the open halls.** Bones thrown from far across a hall fly wider, and each death on a level makes its creatures hit a little softer until it is cleared.

This is still a beta. The first beta was played by hand on the author's own Windows PC in a few different configurations; the new crypt, the secrets, the wisp and the graves have so far been tested by the harness and under Wine. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `SHA256SUMS.txt` | Checksums of the three files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

Upgrading from the earlier *Halloween VGA* builds (`HalloweenSaver.scr`): your settings and saved crypt progress carry over. The old entry stays in Screen Saver Settings until you run `uninstall.cmd` from the zip, which removes both.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/
