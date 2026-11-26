Three Left: the eleventh point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

The torches tell you when the end of a level is near, and underneath, the crypt has learnt to be drawn by hand.

### Before you upgrade

Your saves carry over and still load. One thing is added to them: a count of lives for the side passages to come, which an older save reads as three. The settings are as they were. A copy of your saves costs a moment; from a Command Prompt:

```
reg export "HKCU\Software\HalloweenVGASaver" "%USERPROFILE%\Desktop\haunted-saves-backup.reg"
```

### New in this release

**Three left.** When only three of a level's dead remain to be put down, every torch, lamp and chandelier turns a cold, pale ghost-light and breathes slowly, brighter and dimmer. It is only the picture: the dead see by the same light as before.

**Levels drawn by hand.** The crypt can now be laid from drawings made in the author's own editor: scripted events (torches that light a way, doors that open when a relic is set in its socket, levers and pressure plates), hazards placed where they are wanted, lights hung by hand, new hanging lights and twelve relics, and side passages, a small platformer behind a crypt doorway. None of it is in this release's levels yet; it is ready for the ones being drawn.

### What has been tested

Nobody has played this by hand. Everything is tested in the test harness only: nothing has been run under Wine, and the Windows and browser builds were compiled and smoke-tested by GitHub Actions and by nothing else.

In the harness, the screensaver on eight seeds for 3,000 seconds of its time logged the same lines as the build before, to the line, on the generated levels and the tutorial's two, and ran clean under AddressSanitizer and UBSan. The torches' change is drawing only and moves none of that. The drawn levels' machinery was tested on drawings made for the purpose, not on levels a player will meet.
