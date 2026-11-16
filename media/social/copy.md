# Words to post with

Drafts, in the README's voice (plain, a little dry, British spelling). Change what you like; nothing here is
scheduled or sent. Every file named is under `media/social/`. Character counts are for X's 280 (links count as 23).

**Link to use:** https://aspdesignlabs.github.io/HAUNTED.EXE/ (the page) · play in the browser:
https://aspdesignlabs.github.io/HAUNTED.EXE/play/ · downloads: https://github.com/ASPDesignLabs/HAUNTED.EXE/releases

**Say what it is, once, and say it is a beta.** It is a pre-release (1.0.0b21.8). The page and README carry the
honest status; none of these posts should claim more than they do.

---

## The launch post (trailer)

**X / Bluesky / Mastodon** (attach `video/haunted-trailer-16x9.mp4`)

> HAUNTED.EXE: a Halloween screensaver for Windows.
>
> A possessed 486 in real VGA text mode, and a Wolfenstein-style crypt that plays itself until you hold H for three
> seconds. One 1.4 MB file. Every picture, sound and note is made by the code.
>
> Free, GPL-3.0. Plays in a browser too.
> https://aspdesignlabs.github.io/HAUNTED.EXE/

Alt text for the video: *A trailer. A 486's BIOS counts its memory; an antivirus finds a virus it cannot clean; a
pumpkin is carved on a green-black screen; a title screen reads "HAUNTED.EXE"; then a pixel-art crypt crawl in the
style of Wolfenstein 3-D, with a cat merchant, a hand casting fireballs, a skeleton knight boss, and a card that
reads "You have perished".*

**Mastodon version** adds the tags: `#screensaver #retrocomputing #pixelart #Halloween #indiedev #gamedev #DOS #VGA`

**Bluesky** is the same text; drop the hashtags to two at most.

## Thread (X / Mastodon), one post each, each with its loop from `gif/`

1. `gif/haunt-virus.gif`: "The PC is already infected. The antivirus says so. It cannot clean the host: the host is already dead."
2. `gif/haunt-pumpkin.gif`: "A pumpkin, carved for you, in half-block ANSI at 720x400. There are seven of these scenes."
3. `gif/title.gif`: "Hold H for three seconds and the screensaver stops being one."
4. `gif/merchant-hall.gif`: "There is a merchant. She is a cat. Her prices are not."
5. `gif/crypt-cast.gif`: "The crypt plays itself. You can take the wand off it at any time."
6. `gif/boss-knight.gif`: "Forty levels in four runs of ten, a boss at the end of each. This one rode down to fetch the order's dead and never rode back up."
7. Close: the link, and "It is a pre-release. The page says what has and has not been played by hand."

Alt texts, in order: *Antivirus 2.0 in a red box reading "VIRUS FOUND ... Cannot clean: the host is already dead."* ·
*A pixel-art jack-o'-lantern grinning under a full moon, with "happy halloween. i have been carved for you." typed
beneath.* · *A title screen: HAUNTED.EXE in dripping red letters over a moonlit graveyard, "Press any key".* · *A
stone hall with red banners, pedestals of wares with prices, and a cat sitting on a chest.* · *A first-person crypt
corridor; a hand throws an orange fireball at a skeleton.* · *A boss arena in the dark with a health bar reading "The
Ill-Fated Skeleton Knight" and a chandelier of candles and skulls.*

## Short, for TikTok / YouTube Shorts / Reels (`video/haunted-trailer-9x16.mp4`)

> It's 1994 and your PC is haunted. Screensaver, free, 1.4 MB. Link in bio.

Tags: `#screensaver #retro #pixelart #halloween #oldschoolgaming #wolfenstein #dos`

(The same file is `video/haunted-trailer-1x1.mp4` for a square feed.)

## Reddit

Pick the sub by its rules, and read them first; several want the self-promotion limits observed and the free,
open-source part said up front.

**Title:** *I made a Halloween screensaver in C: a haunted 486 in real VGA modes, and a crypt crawl you can take over
(free, GPL, one 1.4 MB file)*

**Body:**

> It runs in the real VGA modes (80x25 text, and mode 13h at 320x200, 256 colours), scales them by whole numbers, and
> makes its own sound on a software OPL2-style synth, so there are no asset files at all. Two scenes take turns: a
> possessed 486 with a BBS on the other side, and a Catacomb 3-D style crypt that plays itself. Hold H for three
> seconds and you play it. Forty levels, four runs of ten, bosses, a merchant, a ledger of the dead.
>
> It is a beta (1.0.0b21.8). Windows is the target; the browser build is the same code compiled to WebAssembly. The
> page says which parts have been played by hand and which only in my test harness.
>
> Source and downloads: https://github.com/ASPDesignLabs/HAUNTED.EXE

Use `images/1x1/` or `images/16x9/` stills as the gallery; `video/haunted-trailer-16x9.mp4` uploaded natively
rather than linked, if the sub allows it.

## Hacker News / lobste.rs style

> Show HN: HAUNTED.EXE, a Halloween screensaver in C with no dependencies, real VGA modes, and an FM synth

First comment: the technical shape: a software raycaster, an OPL2-style synth, a procedural art generator (every
texture, sprite and note is made at run time), one translation unit for the crawl cut into parts, a headless Linux
harness that renders frames and WAVs for testing. Say it is a pre-release.

## GitHub

- **Social preview** (Settings, Social preview, upload): `images/github/social-preview.png`, 1280x640.
- **Release notes** can embed `gif/title.gif` or `gif/crypt-cast.gif` by raw URL once the files are on `main`.
- **Topics** to set on the repository: `screensaver`, `halloween`, `retro`, `vga`, `raycasting`, `wolfenstein`,
  `pixel-art`, `fm-synthesis`, `c`, `windows`, `webassembly`, `procedural-generation`.
- **Description** (the repository's short line): *A Halloween screensaver for Windows: a possessed 486 and a crypt you can take over, in real VGA modes.*

## Alt text for the stills (`images/`)

| Still | Alt text |
|---|---|
| `title` | The title screen: HAUNTED.EXE in dripping red letters over a moonlit graveyard, a crypt wall below, "Press any key". |
| `virus` | A DOS antivirus window with a red box: VIRUS FOUND, cannot clean, the host is already dead. |
| `pumpkin` | A pixel jack-o'-lantern under a full moon in a pumpkin patch, drawn in half-block characters. |
| `bbs` | A bulletin-board screen: THE OTHER SIDE in red block letters over a login prompt. |
| `merchant` | A stone hall with red banners and pedestals of wares priced in dollars; a cat sits on a chest. |
| `cast` | A first-person crypt corridor; a pale hand throws a fireball at a skeleton in the dark. |
| `knight` | A boss arena: the Ill-Fated Skeleton Knight's health bar across the top, a chandelier hung with skulls. |
| `fireball` | A fireball bursting in a boss arena; a banner reads "Low on spirit: spells cast weak". |
| `perished` | A red screen, "You have perished. Level 21 begins again", with a rat's face behind the card. |

## Don't

- Don't write the console's codes anywhere (CLAUDE.md's standing rule).
- Don't post a recording of the Windows build without checking the HAUNT.EXE file manager scene: it shows your
  Windows user name and today's date (the README's *Recording video*). The footage in this package was made in the
  Linux harness and shows neither.
- Don't call it finished. It is a pre-release, and the page's *Beta status* table says so.
