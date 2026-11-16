# HAUNTED.EXE: press kit

**A Halloween screensaver for Windows, built for OLED screens: a possessed 486 and a crypt you can take over.**

## The short version

HAUNTED.EXE is a free, open-source screensaver that runs in genuine VGA modes and makes its own sound with an FM
synthesiser, so it has no art or audio files at all. Two scenes take turns. **HAUNT.EXE** is a possessed 486: BIOS
POST, a DOS prompt, haunted programs, a BBS on the other side, and seven half-block ANSI scenes. **CATACOMB** is a
Wolfenstein 3-D style crypt crawl that plays itself, and that a person can take over by holding H for three seconds:
forty levels in four runs of ten, a boss at the end of each run, a cat who keeps a shop, and a ledger of everyone who
died there. It is one program of about 1.4 MB, with no installer and no runtime. It also plays in a browser, and,
held sideways, on a phone.

## Facts

| | |
|---|---|
| Name | HAUNTED.EXE |
| Version | 1.0.0b21.8, "The Antechamber" (a pre-release, November 2026) |
| Platform | Windows (`HAUNTED.EXE` runs in a window, `HAUNTED.scr` installs as a screensaver); a browser build at `/play/` |
| Modes | 80x25 text (720x400) and mode 13h (320x200, 256 colours), scaled by whole numbers up to 4K |
| Made with | C, the standard Windows DLLs and nothing else; the browser build is the same code through Emscripten |
| Sound | An OPL2-style FM synth, sequencer and reverb written for it; every piece of music and effect is generated |
| Size | About 1.4 MB |
| Licence | GNU GPL v3.0 |
| Author | ASP Design Labs |
| Page | https://aspdesignlabs.github.io/HAUNTED.EXE/ |
| Play now | https://aspdesignlabs.github.io/HAUNTED.EXE/play/ |
| Source, downloads | https://github.com/ASPDesignLabs/HAUNTED.EXE |

## Status, said plainly

It is a beta. The author plays it by hand on Windows and reports back; the release in the trailer (1.0.0b21.8) has been
tested in the project's headless harness but **not yet played by hand**, and not under Wine. The project page's
*Beta status* table and the README's *What has been verified* say exactly what was tested, how, and with what result,
including the bad news. The files are not code-signed, so Windows may show a SmartScreen warning the first time
(*More info, Run anyway*). Please keep this paragraph's meaning if you quote any of it.

## What is worth a paragraph

- **No assets.** Every wall texture, sprite, portrait, tune and sound is produced by the code at run time.
- **Real VGA.** Not an emulation of the look: the picture is 320x200 with a 256-colour palette, or 80x25 text, scaled
  by whole numbers so the pixels stay square.
- **It plays itself.** The crypt is run by an autopilot that explores, fights, buys, dies and is remembered. Take over at any time.
- **The dead are kept.** Death costs something, and the grave plot at the merchant's hall holds a stone for every run that ended.
- **A hand and a face.** The caster's hand and face, and the cat merchant, were traced from the author's photographs.
- **Seven art scenes, one BBS.** HAUNT.EXE's ANSI scenes include a jack-o'-lantern, a talking skull, a manor in a thunderstorm and a ghost that says boo.

## Assets

Everything is in this folder, made from the game itself by `tools/social.py` (see `README.md` here).

| Asset | Where | Notes |
|---|---|---|
| Trailer, 47 s, with sound | `video/haunted-trailer-16x9.mp4` | 1920x1080. Also `-1x1` (1080x1080) and `-9x16` (1080x1920) |
| One clip per scene, with the game's own sound | `clips/*.mp4` | 1920x1080, no captions |
| Loops | `gif/*.gif` | Silent, 640 to 720 px wide |
| Stills | `images/16x9`, `images/1x1`, `images/9x16` | Captioned in the game's own font |
| Repository social preview | `images/github/social-preview.png` | 1280x640 |
| Logo / title art | `site/cards/*.png` | The five 1200x630 title cards the page uses for link previews |

Screenshots and footage may be used freely in coverage of the game. The game is GPL-3.0; the fonts (Px437 IBM VGA 8x16
and IBM BIOS, from *The Ultimate Oldschool PC Font Pack* by VileR) are CC BY-SA 4.0, and the captions in these assets use them.

## Boilerplate

> HAUNTED.EXE is a free Halloween screensaver for Windows by ASP Design Labs. It runs in genuine VGA modes and
> makes its own sound with a software FM synthesiser, so the whole program, art and music included, is a single file
> of about 1.4 MB. A possessed 486 PC and a self-playing crypt crawl take turns on screen; hold H for three seconds
> and the crypt is yours to play. It is open source under the GPL-3.0 and is currently a pre-release.

## Footage note

The footage here was rendered by the project's Linux test harness with fixed seeds, not recorded from a Windows
session, so it shows no one's user name or date (the Windows HAUNT.EXE file manager does; see the README's
*Recording video* before sharing a capture of your own).
