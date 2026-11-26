The Dead's Gallery: the twelfth point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

The dead have learnt their way about the side passages, and the browser will take you into one.

### Before you upgrade

Your saves carry over and still load; nothing is added to them. The settings are as they were. A copy of your saves costs a moment; from a Command Prompt:

```
reg export "HKCU\Software\HalloweenVGASaver" "%USERPROFILE%\Desktop\haunted-saves-backup.reg"
```

### New in this release

**The dead in the side passages.** The side passages behind crypt doorways, a small platformer with lives of its own, are not in the levels yet. In a passage the dead now go each their own way. Skeletons keep their distance and lob bones in a high, slow arc after half a second's warning, so a step aside clears them. Bats circle above, flutter and dive, low enough to be struck. Hounds and rats run you down and leap, spiders drop on a thread, the deacon raises the fallen and the whipper's lash reaches. Every blow flashes white before it lands. What strikes, and what is struck, is the creature's own pixels and the shot's own shape, not a box round either.

**A passage in the browser.** [play/?side=0](https://aspdesignlabs.github.io/HAUNTED.EXE/play/?side=0) starts a new run on level 1 straight in the first passage, *The Ossuary Stair*; `?side=1` to `?side=4` open the others, the last *The Dead's Gallery*, a kind or two of the dead to each bay at their first rank, and `&level=` a deeper level. It keeps its saves apart from the page's own. The dream plays it until you hold H.

### What has been tested

Nobody has played this by hand. Everything is tested in the test harness only: nothing has been run under Wine, and the Windows build was compiled and smoke-tested by GitHub Actions and by nothing else. The browser build was also run in a headless Chromium.

In the harness, a scripted person seven tiles from a skeleton was struck by 5 of 6 bones standing still and by 1 of 7 stepping back and forth. Each kind of the dead was put alone in a passage and the dream reached the goal every time. Over the four passages there were before, on eight seeds each, the dream lost no life, before this release or after it. The crypt itself logged the same lines as the build before, to the line, and ran clean under AddressSanitizer and UBSan.
