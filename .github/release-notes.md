The Antechamber: the eighth point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It puts a vestibule before the first vault, and mends one doorway.

### Before you upgrade

Your saves carry over, and nothing in them has changed. The settings are as they were. A copy of your saves costs a moment, and this is as good a time as any; from a Command Prompt:

```
reg export "HKCU\Software\HalloweenVGASaver" "%USERPROFILE%\Desktop\haunted-saves-backup.reg"
```

### New in this release

**A vestibule before the first vault.** On about half the levels the first vault, the gold one, has an antechamber: an eleven-cell chamber with an altar at its heart and a brazier either side, the hall in one wall and the vault's locked door in the opposite one. The vault is a small room behind it. The key is found as it always was.

**A doorway mended.** On a rare level a bent secret room's way on, into the Backrooms or Severance, could touch no floor. It now stands beside the room.

### What has been tested

This one has not been played by hand: the author played the release before it, and asked for this on the strength of its checks. Everything is tested in the test harness only: nothing has been run under Wine, and the Windows and browser builds were compiled and smoke-tested by GitHub Actions and by nothing else.

In the harness, 1,800 levels were made with every exit, key, locked door and secret room tested for being reached, and no floor stranded: none failed. The screensaver played sixteen seeds for 3,000 seconds of its time three times over with the vestibule left out and put in: it died 120 times without it and 99 with it, and cleared 6.8 levels either way, so the vestibule is not what moves them. Against the release before it, the same way, it died more (130 deaths against 99, and 6.6 levels cleared against 6.9). That gap is not explained; the same build's total moves between 27 and 49 from run to run, so it is probably noise, but it is not shown to be. With the drawn rooms switched off it logs the build before's line for line. Clean under AddressSanitizer.
