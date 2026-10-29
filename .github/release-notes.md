The tenth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

### New in this beta

Each of CATACOMB's four runs of ten levels is now a place of its own, built in its own stone with its own rooms, doors, keys, light and treasure, and rotting in its own way from its first level to its tenth. The walls are laid a stretch at a time, by the room each faces; where one material gives way to another the cell between is a blend, never a hard edge; floors and ceilings come from the room.

- **The Crypts** (levels 1 to 10): the burial vaults of a forgotten order, in dressed ashlar, coursed rubble, painted plaster and bone. Burial vaults with niches in rows, chapter houses with banners and a lozenge floor, knights' vaults with their slabs, undercrofts cut from the rock; torches in iron sconces, oak doors under iron, keys with a trefoil bow. Kept, then damp, then falling in.
- **The Catacombs** (11 to 20): wet slate beneath them, flooded and drained. Saints' catacombs, cisterns and canals with standing water (every step in it splashes), silted naves and spinners' galleries; lanterns on hooks, sluice doors gone to rust, coin gone black in the silt, reliquaries and pearls. A flood line climbs the walls through the run, and webs come onto them from its sixth level.
- **The Hollows** (21 to 30): caverns and an abandoned mine where the dead were tipped. Timbered galleries with rails and carts, stopes, charnel pits, fossil halls and a cult's chapel; oil lamps on spikes, mine doors, keys with a tally tag, nuggets, raw gems and a strongbox. Late in the run something has dragged its claws down the stone.
- **The Deep** (31 to 40): black glass, amethyst and magma. Geodes, glass halls, cinder fields, magma vents and a throne room; magma fissures for torches and geodes alight, black iron doors with the fire behind their grilles, keys of glass, coin run molten and an idol of black glass. It glitters, then burns, then runs molten, then goes dark for the Wraith.
- **The bosses** fight in their run's rooms: the knight in a knights' vault, the spider in a spinners' gallery, the Deacon in his chapel and the Wraith on its throne.
- **The merchant's hall** is rebuilt once in the Crypts' ashlar under the Order's banners, the same before every run. The red brick has gone.

The pockets behind the secrets (the Backrooms and Severance) keep their own looks. How the game plays is unchanged, except that the room types now decide which rooms keep resident dead: the Deep holds about a tenth more creatures a level than before.

This is still a beta. Everything new has been tested only in the harness, which draws exactly what the game draws: none of it has been played by hand, run on Windows or under Wine, or seen on a real screen. The owner approved the first two runs' art; the last two went in without a review first, at the owner's word. The map and Levels of Atonement from the ninth beta, the controller from the eighth and everything in the seventh haven't been played by hand yet either. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Before you upgrade

Your settings and saves carry over, both ways: this beta changes nothing in them, so the ninth can still read what this one writes. If you are coming from the sixth or earlier, its graves open as pages of the merchant's ledger, and the sixth and earlier can't read the saves this one writes. If you might go back that far, keep a copy first, from a command prompt:

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
