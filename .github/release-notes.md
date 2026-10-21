The first public build of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H** for three seconds.

This is a beta. It has been tested in a headless harness and under Wine, not yet on real Windows hardware. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

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
