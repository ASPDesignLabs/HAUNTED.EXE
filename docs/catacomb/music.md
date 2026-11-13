# Music

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **The bosses' music** (`snd.c`: `BTHEMES`, `boss_step`; `snd_set_boss(theme, wave)`, `snd_boss_fall`).
  A slow waltz each on its own clock: eight bars of three beats, the last cut to two; a five-note
  ostinato that drifts against the bar; the bell on beat one; the tune rests one phrase in four and
  climbs an octave in another (the Wraith's voice sings a bar only now and then, `tune`). The knight's in
  D minor on brass, the spider's B-flat minor plucked with a skittering hat, the Deacon's G minor on the
  organ with a choir (`PAT_VOICE`), the Wraith's A minor on a drone. `boss_begin` starts it, `boss_wave`
  turns it up (at the next bar line: drums, the bass doubled, a semitone up, 8% quicker), the rise settles
  it, `boss_fall` fades it under three tolls; a death, a new level and the merchant's room fade it out.
  Every voice, drum and scheduled note belongs to a bus (`BUS_RUN`, `BUS_MUZAK`, `BUS_BOSS`, `BUS_SFX`, `BUS_SHOP`,
  `BUS_MENU`, set by `g_vbus` when it starts): the run's piece gives way to the waltz as it does to a pocket's room, and the
  fanfare and the stinger (`BUS_SFX`) play over either.

- **The music** (1.0.0b15, docs/PLAN-music.md; `src/core/snd.c` after the waltzes). A piece (`Piece`: pace, metre, root, four
  sections of eight bars as `MSec`, chords and a tune as `MNote`, an order, a step function, a gain) runs on a `Player` of
  its own clock and bus: the run's (`PL_RUN`, `BUS_RUN`; runs I and II are scored pieces since docs/PLAN-music-work.md,
  `gothic_step` and `descent_step`, runs III and IV still `hollows_step`, `deep_step`), her hall's (`habanera_step`, or
  `swing_step` after a perfect level, `BUS_SHOP`) and the menus' (`menu_step`, `BUS_MENU`, `g_menu_var` the run's guise; the
  ledger's opening plays `ground_step`, the Lampstand and ascending `estampie_step`). Voices and drums
  carry a layer (`LY_BASE`, `LY_FIGHT`, `LY_COLOUR`, set by `g_vlayer`). `music_tick` (once a sample) fades the buses
  (`g_busg`; `g_alive` a bus that is or is coming in, whose voices aren't cut) and runs the clocks: the run's stops in her
  hall and under a menu. The run's player takes the fight on a beat and lets it go at a bar line (`player_step`: a crash
  and timpani in, a closing chord out), quickens a twentieth a level (`player_pace`, and an eighth in a trial) and beats
  the failing's heart. The failing is a low-pass over the music buses (`g_flp`); `g_mix_music`, `g_mix_fx` the sliders.
  The game's side is `music_frame` (`crawl_frame` wraps `crawl_update`): the track by `crawl_set`, her room or back rooms
  (`her_screen` keeps her music for her own screens), a menu (the L list a short look), the fight (an alerted creature
  within nine cells, a missile, a skull charging, or a cast in the last three seconds with something in sight; level 2
  after 20 s or three engaged, 3 after 45 s, five engaged or under two fifths of health; out after 6 s; none in her hall,
  a pocket or a boss's fight), the failing (under a fifth, lifted over a quarter; not in her hall). The cave's drips and
  Severance's muzak keep the old sequencer clock (`crawl_step`). Debug: `EVENT music place|menu|fight|frail`. The
  harness: `build/host music`, `HOST_MUSIC`, `HOST_EFFECTS`; the registry's `MusicVolume`, `EffectsVolume`.
