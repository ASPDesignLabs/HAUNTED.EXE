The fifteenth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

The crypt has music of its own now: a piece for each run, the merchant's, the menus', and music that follows a fight and your health.

### Before you upgrade

Your saves and settings carry over, and nothing in the saves has changed, so going back to the fourteenth beta is safe too. The two new settings are simply ignored there.

### New in this beta

**A piece for each run of ten,** in place of the one march that played everywhere. Each piece has four parts, played in an order that takes three or four minutes to come round.

- **The Crypts:** D minor, slow, an organ and a low choir, and a bell. The old march's first phrase comes back as a chant, and there is a brass chorale.
- **The Catacombs:** a barcarolle in six, a harp rolling over the water, a bell under it, and drips. A vibraphone has the tune, then a low flute as if from under the surface.
- **The Hollows:** a work song. The bass swings like a pick, and there is an anvil, low brass and a hammered dulcimer.
- **The Deep:** a drone, glass harmonica, and a celesta glinting. Chords are laid over one another so that nothing settles.

**The merchant's hall** has a sly tango, which thins to a music box in her back rooms.

**The menus** play a lullaby made from the march's opening, in the run's key and on its instrument: a music box, a harp under water, a dulcimer, or glass. The run's piece waits where it stopped and takes up again when you close the menu. The console and the map don't count as menus.

**A fight** brings in drums, strings and brass on the next beat. The longer it lasts, the more there are against you and the lower your health, the more of it plays and the quicker it goes. Six quiet seconds, and it ends on a closing chord. A trial plays its fight from the start.

**When you are failing,** under a fifth of your health, the music goes muffled and thin over a heartbeat, and lifts above a quarter. The game's own sounds stay clear, so you can still hear what is coming.

**Music and Effects sliders** sit under Volume, in the screensaver's settings and in the browser's.

The bosses' waltzes, the Backrooms' hum and Severance's muzak are as they were. The dream plays exactly as before: the music only listens.

This is still a beta. The pieces were rendered as recordings for the author before they went in; in the game itself the music has only been measured in the harness, and nobody has played with it through real speakers yet, so whether the fights come in too often, and how the menus and the failing feel, is still to be judged. The Windows build is compiled and smoke-tested by GitHub Actions. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

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
