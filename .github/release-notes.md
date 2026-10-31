The thirteenth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

Each run of ten now keeps things of its own: hazards, puzzles, trials, and a reckoning on the way out.

### Before you upgrade

**Coming from the twelfth or eleventh beta:** your saves carry over. Going back is another matter: the twelfth can't read a piece that has rolled the new Resist Lightning, and drops it, and it forgets the record of puzzles and trials. If you might go back, keep a copy first, from a command prompt:

```
reg export HKCU\Software\HalloweenVGASaver haunted-backup.reg
```

Double-clicking `haunted-backup.reg` puts it back. Your settings carry over either way.

**Coming from the tenth or earlier:** the eleventh beta changed saves one way, and this one keeps that change. The tenth and earlier can't read what this writes, and the first time it opens a page from before the eleventh, that page's spells are forgotten for good (an Alignment Point for each Level of Atonement past the first instead) and the merchant asks what you were. Keep a copy first, as above.

### New in this beta

- **Each run's own hazards.** The Crypts, the Catacombs, the Hollows and the Deep each have six or seven things that hurt. Some are fixed in the walls and rooms and keep a rhythm you can learn: a censer on its chain, a burial slab breathing, a sluice gate, a lodestone seam, a fissure, a black mirror. Some arise near you: votive racks falling, silt, choking ash, glass that spikes. There are more as the run goes on, and now and then an earlier run's. Each harms by an element, never more than a quarter of your health at a stroke, and always warns first. From the sixth level of each run, a few drink your souls while you stand in them. **Resist Lightning** is a new stat, with a line of its own in the points.
- **Puzzles.** On the even levels from the sixth, but never a boss's, a sealed wing with a hoard inside, and on one puzzle level in three the way on. Only thought opens it: skulls carried to their niches, bells rung in order, cisterns poured, stepping stones, rails switched, prisms turned into the magma's beam, a magic square; twenty-seven kinds in all. Everything is done with E and your feet, and you can carry one thing and still cast. After three wrong tries a whisper gives a nudge.
- **The answer, written down.** Somewhere on the level, hidden in one of the run's things: a candle-box, a satchel in the silt, a miner's boot, a broken geode. E searches one, and the right one glints when you are close, more so with Treasure Hunter. A **Black Shrine** will, for souls, show where it lies through the walls and on the map. What you find, and every clue you read, goes into a new page of Tab, **documents**, until the level ends.
- **Trials.** On every level from the third, a door marked with the run's skull and crossed bones. A step inside and it seals, the room turns, and an hourglass gives you a minute with no numbers. Seventeen of them, four or five to a run: the ossuary waking, a vigil, last rites, a knight's wager, the toll; rising water, a brood, the sluices, the saints' lamps; a cave-in, a charnel feast, a rite, firedamp; the molten rise, the dark hours, a court of glass, singing glass. Win, and you are made whole and get a fifth more experience from your kills for the rest of the level. Let the sand run out and a curse takes a fifth from everything instead.
- **The reckoning.** Going out with anything still on the level (creatures, secrets, the sealed wing, gold, the trial), a slip on the exit's door says what, and asks whether to descend. It starts on *not yet*, so a quick E never leaves by mistake.
- **The record and the Codex** count puzzles solved and trials won, with achievements for them, and the Codex has a page for each hazard met, each kind of puzzle solved and each trial won.

Against the twelfth beta, the screensaver's caster does about as well: from level 1 a median of nine levels in 100 minutes, as before, with a few of its deaths now in trials; deeper starts reach as far, and a level takes a little longer. It wins most trials with more time to spare than planned. That, and the knight it still finds hard, will be tuned once a person has played them.

This is still a beta. Everything new has been tested only in the harness, which draws exactly what the game draws: none of it has been played by hand, run on Windows or under Wine, and the new art hasn't been seen on a real screen. The Windows build is compiled and smoke-tested by GitHub Actions. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H, or a controller's button, to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `SHA256SUMS.txt` | Checksums of the three files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/
