# Multimedia package

Everything here is cut from the game itself (harness footage with fixed seeds, so it can be made again), captioned
in its own VGA font. Nothing is a mock-up. Words to post with are in `copy.md`; facts and boilerplate in `press-kit.md`.

| Folder | What | Made by |
|---|---|---|
| `video/` | The 47 s trailer as 16:9, 1:1 and 9:16, with sound | `python3 tools/social.py trailer [--music piece]` |
| `gif/` | Six silent loops, each under 5 MB, for X, Discord and the README | `... gifs` |
| `images/16x9`, `1x1`, `9x16` | Nine captioned stills in each shape | `... images` |
| `images/github/social-preview.png` | The repository's social preview, 1280x640 (Settings, Social preview) | `... images` |
| `clips/` (not in git) | One 1080p clip per scene, with the game's own sound | `... clips` |
| `music-candidates/` (not in git) | Eight pieces, each a minute through a fight, to choose the trailer's music from | `... music` |

Needs `ffmpeg`, Pillow and `make build/host_debug`. `python3 tools/social.py all` makes it all (about six minutes).
Change a caption, a cut or a seed at the top of `tools/social.py`.

**Sound.** The trailer's first thirteen seconds (the 486) are HAUNT.EXE's own sound; after that CATACOMB's run I piece
(`crypts`) rises through a fight to the knight and falls away at the card. `--music catacombs` (or `hollows`, `deep`,
`title`, ...) rebuilds it with another; nobody has chosen yet.

**How it was tested.** Made and checked in the Linux harness; the encoded files were looked at frame by frame in
contact sheets, not played in a player on every platform, and the sound has not been listened to by a person. Under
Wine and on Windows nothing was run. The page's trailer is the 16:9 file, `preload="none"`.
