The twelfth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

A small one, from the first look at the eleventh.

### Before you upgrade

**Coming from the eleventh beta:** your saves carry over both ways; nothing in them has changed.

**Coming from the tenth or earlier:** the eleventh beta changed saves one way, and this one keeps that change. The tenth and earlier can't read what this writes, and the first time it opens a page from before the eleventh, that page's spells are forgotten for good (an Alignment Point for each Level of Atonement past the first instead) and the merchant asks what you were. Keep a copy first, from a command prompt:

```
reg export HKCU\Software\HalloweenVGASaver haunted-backup.reg
```

Double-clicking `haunted-backup.reg` puts it back. Your settings carry over either way.

### New in this beta

- **Begin a new run, from Esc.** The screensaver takes a class at random for a run it starts, the moment the merchant's room opens, so someone who took over a fresh run was never asked *WHAT WERE YOU?* Esc now has a fifth answer under *OPEN THE LEDGER*: **BEGIN A NEW RUN**. It asks twice, because what you haven't signed is lost, as in fleeing; then you stand in her room before level 1 with nothing, and she asks what you were at once. Your signed pages stay in the ledger, and a page the dead are watching keeps their mark. On a controller, A twice. It works from the crypt, her rooms, or inside a pocket.
- **The souls' line** on the status bar, which shows how full your vessel is, sits under the number now instead of running through it.

Everything from the eleventh beta is here as it was: the six elements and their glyphs, spells as lines at the Lampstand, wind-ups and charging, what you were, gear that rolls the elements, souls that can run dry, Soul Sap, and the map at a walk. Against the tenth beta the screensaver's caster finds the knight harder; that is unchanged and will be tuned once a person has played it.

This is still a beta. Everything new has been tested only in the harness, which draws exactly what the game draws: none of it has been played by hand, run on Windows or under Wine. The Windows build is compiled and smoke-tested by GitHub Actions. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H, or a controller's button, to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `SHA256SUMS.txt` | Checksums of the three files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/
