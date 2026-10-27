The eighth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or now any button on a game controller, for three seconds.

### New in this beta

- **A game controller.** An Xbox controller, or any pad that speaks XInput (most PC pads do, some in an Xbox mode), in the screensaver and in the window. Hold any button three seconds and the dweller is yours, or HAUNT.EXE cuts to the crypt; a tap does nothing, so a pad bumped on the desk won't end the screensaver.
- **Played by the pad**, all of it: the left stick walks (click it to run), the right stick turns, RT casts, LT uses a boss's gift, the bumpers turn the spellbook, A opens and buys, B dodges, X readies the next ability or holds a ware, Y the wisps, View the wardrobe, Menu the question, the D-pad the bestiary, the Codex and the status bar. In the menus the D-pad and stick choose, A takes, B goes back.
- **The hints speak its language.** When the pad was the last thing you touched, every menu's hints name its buttons, A in green, B in red, X in blue, Y in gold, and the controls card is the pad's. Touch a key and they go back.
- **Rumble** when you are hurt (by how much), when a rock lands near, a secret wall grinds, a boss falls, or the BFS9000 goes off.
- **The display stays awake** while you play with the pad, since Windows doesn't count a controller as activity; the window also refuses the screensaver while a pad plays. The settings have a box to turn the controller off.

A PlayStation or Switch pad needs a translator such as DS4Windows for now.

This is still a beta. The controller has only been tested in the harness, with a pretend controller scripted into it: no real controller has touched this build, and it hasn't been run on Windows or under Wine, so how the sticks feel and how hard it rumbles are untested. Nothing of the seventh beta has been played by hand yet either. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version, graphics card and controller.

### Before you upgrade

Your settings and saves carry over, and this beta changes nothing in the save, so the seventh can still read it. If you are coming from the sixth or earlier, its graves open as pages of the merchant's ledger, and the sixth and earlier can't read the saves this one writes. If you might go back that far, keep a copy first, from a command prompt:

```
reg export HKCU\Software\HalloweenVGASaver haunted-backup.reg
```

Double-clicking `haunted-backup.reg` puts it back.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H, or a controller's button, to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `SHA256SUMS.txt` | Checksums of the three files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/
