Plain Words: the tenth point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It is the author's own wording, put in where the game's was.

### Before you upgrade

Your saves carry over, and nothing in them has changed. The settings are as they were. A copy of your saves costs a moment, and this is as good a time as any; from a Command Prompt:

```
reg export "HKCU\Software\HalloweenVGASaver" "%USERPROFILE%\Desktop\haunted-saves-backup.reg"
```

### New in this release

**New words.** About two hundred and forty of the game's lines are the author's now: the captions and the hints, what each creature and each boss is, the class stories, the Codex's pages, the merchant's, the altar's, the shrine's and the wisps', and what each node of a line and each ability does. Fire is called Flame throughout. The classes lose their THE (the Witchfinder is WITCHFINDER on its page), the shambler is UNDEAD and the shield-bearer a SKELETON GUARD, and the Seep is VILE OOZE. The longer lines needed longer room, and have it.

**Said as the game does it.** The descriptions of Shatter, Deep Chill, Impale, Ossified, Harvest, Wisp Host, the Major Soul Tether and the lightning's stun say what those abilities do now, not what they may come to do.

### What has been tested

Nobody has played this by hand. Everything is tested in the test harness only: nothing has been run under Wine, and the Windows and browser builds were compiled and smoke-tested by GitHub Actions and by nothing else.

In the harness, the screensaver played two seeds for 600 seconds of its time against the build before and logged the same lines to the line, but for the changed names; four more, for 1,500 seconds each, ran clean under AddressSanitizer and UBSan, and the pane-fit check found no overflow. That is fewer seeds than a change to the balance would be weighed on, and none of this moves the balance. A hand has not read each new line in its place: the boss, shop, controller and touch lines in particular are checked for fit by the numbers and not by eye.
