# The music work: more chords, more voices, the rest of what an AdLib could do

*Begun 9 November 2026 on `claude/beautiful-einstein-tzed36`, from main at 1.0.0b21. This is the plan and its log, kept so
that whoever picks the work up can follow it: what the owner asked, what they chose, what is built, what waits.*

## Status

- **Her hall: done, harness only.** Five pieces were written and heard by the owner, who gave every one of them a place
  (below). In the game: the habanera is her hall's, the alley cat plays there after a perfect level, the lament ground
  while the ledger is opened to choose a page. The waltz and the nocturne are kept in the code, unused.
- **Run I and its fight: done, harness only.** Five attempts were written and heard; the owner placed three: the ill-fated
  is run I's piece, the descent replaces run II's, the estampie plays while the Lampstand or ascending is open. The office
  and the ossuary are kept, unused. 1.0.0b15's run I and run II pieces are retired.
- Runs III-IV, the menus and the bosses' waltzes come after, the same way, if the owner wants them.
- Nothing here has been heard on a real machine but the renders the owner listened to.
- **Merged into main, unreleased** (9 November, at the owner's word), squashed as one commit, so that a title screen can be
  built on main with the music in. `VERSION` stays 1.0.0b21, so no release comes of it.

**For whoever builds the title screen.** The music is reached through `snd_set_menu` (`src/core/snd.h`), called each frame from
`music_frame` at the end of `scene_crawl.c` with whatever screen is up: 3 the lament ground (`MU_TITLE`: "press start,
select a grave"; today the ledger opened from Esc), 5 the waltz (`MU_SPECIAL`, for a special screen), 6 the nocturne
(`MU_ENDING`, for an ending), 4 the estampie (the Lampstand), 1 or 2 the menus' tune; 0 lets the run's piece back. One
piece gives way to the next in a third of a second. `build/host music crypts 40 out.wav title@5 waltz@15 back@30` hears the
switches.

## What the owner asked (9 November)

The screensaver's music is fine; the game's should be better. Start with one set of pieces: her hall, then run I with its
fight ("its combat alternatives"). More chords, layered complexity, more voices, and the whole of what an AdLib could do,
to give each piece more structure. Five attempts at her hall first.

## What the owner chose (9 November)

They heard the five and used them all:

| Piece | Its place | In the code |
|---|---|---|
| The habanera, grown | her hall, as it was (1.0.0b15's habanera is retired) | `MU_MERCHANT`, `SC_HABANERA`, `habanera_step` |
| The alley cat | her hall after a perfect level; the habanera again after the next level that isn't | `MU_PERFECT`, `SC_SWING`, `swing_step` |
| The lament ground | a title screen ("press start, select grave"); there is none yet, so for now the ledger opened to choose a page | `MU_TITLE`, `SC_GROUND`, `ground_step` |
| The clockwork waltz | kept for a special menu screen, not yet used | `MU_SPECIAL`, `SC_WALTZ`, `waltz_step` |
| The midnight nocturne | kept for an ending theme, which the game hasn't got yet; unused | `MU_ENDING`, `SC_NOCTURNE`, `nocturne_step` |

Taken as defaults, to be put right if wrong: a *perfect* is the tally's PERFECT! (every treasure and every creature on
the level: `level_perfect`); *the run after* is the next level (her hall comes between every two), so one perfect level
gives one visit of the alley cat. The lament ground plays wherever the ledger is opened to choose a page (Esc's OPEN THE
LEDGER), in the crypt or in her hall; signing the ledger in her hall keeps her piece.

## The engine (`src/core/snd.c`)

What 1.0.0b15's pieces left unused of an OPL, and a few things a tracker did with one:

- **Tremolo** (`Patch.trem`, `tremm`, `trem_hz`): the chip's AM, on the carrier's level and the modulator's index, from
  one clock for every voice (`g_lfo_t`), as on the chip, so a chord breathes together.
- **Key scaling** (`Patch.kslm`, `kslc`, `ksr`): an octave above middle C the modulator loses so many dB (brightness), the
  carrier too (level), and the envelopes run quicker.
- **OPL3's waveforms** (4-7: alternating, camel, square, derived square).
- **Glides** (`Voice.glide`, `bend_end`): a note slid up to from a semitone, scooped from two, fallen onto from above.
- **The tracker's arpeggio** (`Voice.arp`, `narp`, `arp_hz`; `sc_arpvoice`): one voice running through a chord's notes.
- **Rhythm mode** (`X_OBD`, `X_OSD`, `X_OHH`, `X_OCY`, `X_OTT`): the chip's bass drum, and its snare, hat and cymbal made
  as the chip makes them, from phase bits of two oscillators and its noise; the tom a sagging sine. Each can be pitched
  (`Sfx.f0`, `pdf`). And hands: castanets (a roll by `f0`), a tambourine, a brush (`f0` 1 a tap), a clap, a snap.
- **Twelve instruments** (`PAT_CLAR` ... `PAT_CHARP`): clarinet, bandoneon, gut-strung guitar, plucked double bass, motor
  vibraphone, muted trumpet, harpsichord, oboe, woodblock, string section, solo violin or cello, concert harp.
- **64 voices** (48), **128 notes waiting** (32; `sched_v`): strums, rolled chords, a piano's spread.
- **Scored pieces** (`ScSec`, `Ev`, `sc_parse`; `Piece.sc`, `step2`). A part is written as text (the notation is at
  `ScSec` in `snd.c`), read once at start (`score_init`); `snd_check` reports any bar whose steps don't add up. A section
  is any number of bars, and the same music can come again dressed another way (`ScSec.var`). Helpers: `sc_play`,
  `sc_play_ex`, `sc_musette`, `sc_strum`, `sc_arp`, `sc_now`, `sc_after`, `sc_bass_jazz`, `sc_comp`; for the fights
  `sc_root`, `sc_figure`, `fight_lvl`.
- **The echo's time by the piece in front** (`Piece.dly`, in steps).
- **The menu's piece** (`snd_set_menu(3)`, `g_menu_mu`, `g_menu_sw`): a menu can play the title's piece, the one going and
  the other coming in a third of a second.
- **For weighing a mix**: `snd_mute(mask)` and the harness's `HOST_MUTE` (patches by number, bit 63 the pieces' drums),
  `HOST_DRIPS=0` (the cave's drips and moans left out of a render).

The old run pieces, the menus' tune and the screensaver render bit for bit as before.

## The game's side

- `RUN.perfect` (not saved): set by `rec_cleared` from `level_perfect` (the tally reads the same), cleared by
  `start_level`. `music_frame` says `snd_set_shop(RUN.perfect ? MU_PERFECT : MU_MERCHANT)` before her place; the piece
  changes only at her next visit. Debug: `CRAWL_PERFECT=1` (every level counted perfect), `EVENT music hall
  merchant|perfect`.
- `music_frame`'s menu is 3 while `UI_LEDGER` is open in `led_mode` 1 (opening), wherever it is; 4 while `UI_ALIGN`,
  `UI_ASCEND` or `UI_STONES` is open (`snd_set_menu(4)`, the estampie). Debug: `EVENT music menu 3|4`.

## The steps

1. **Her hall** (done): five attempts, the owner's choice, wired in. Harness only.
2. **Run I and its fight** (done, the owner's choice wired in): five attempts at run I's piece, each written with its fight's three levels (what
   comes in at each, and the pace a twentieth quicker a level, as before), rendered calm and through a fight that grows,
   ends and fails. The owner picks; the chosen one takes `MU_CRYPTS`'s place.
3. Whatever the owner asks next (runs II-IV, the menus' guises, the bosses).

## Her hall's pieces, as written

| Piece | Key, metre, pace | What carries it | Its form | Length |
|---|---|---|---|---|
| The habanera, grown | A minor, 4/4, 92 | pizzicato habanera over an A pedal while the chords slide A, G-sharp, G, F-sharp inside it; gut-strung guitar on the off-beats; bandoneon; the flute sliding to its notes, a clarinet under it; castanets, hands, the chip's tom | intro, A, A (clarinet), B (the circle of fifths, to a stop), A (bandoneon), her purr (the Andalusian cadence, a music box), B (violin), A (everything), a turn | 2:42 |
| The clockwork waltz | C minor, 3/4, 138 | woodblocks ticking and tocking; a music box with a celesta over it; pizzicato and harp; the B part in E-flat on violin, its minor fourth turning the light | intro, A, B, A, the clocks run down (glass harmonica over a tracker's arpeggio), B, A, coda | 2:16 |
| The alley cat | F minor, swing, 120 | muted trumpet; brushes, the chip's cymbal as a ride and its hat on two and four; electric piano comping rootless chords; a double bass in two, then walking (its own notes each time) | intro, A A B A (trumpet), A A B A (vibraphone, the trumpet back for the last eight), tag | 2:24 |
| The lament ground | G minor, 3/4, 72 | a passacaglia on the falling bass G, F, E-flat, D | the ground alone, the oboe's theme, violin and oboe, the ground made chromatic under a choir, divisions, everything with organ and timpani, the chromatic ground quietly | 2:40 |
| The midnight nocturne | D-flat major, 4/4, 66 | a concert harp rolling wide chords; music box, cello, glass, celesta, a choir; no drums | intro, A, A, B (B-flat minor on glass), C (chords sliding by whole tones), A, coda | 2:40 |

## What the owner chose for run I (9 November)

"run1b and its combat music is amazing, the exact kind of thing I was looking for" (1.0.0b15's run I piece had felt like a
lullaby). They heard the five and placed three:

| Piece | Its place | In the code |
|---|---|---|
| B, the ill-fated | run I's piece, with its fight (1.0.0b15's is retired) | `MU_CRYPTS`, `SC_GOTHIC`, `gothic_step` |
| D, the descent | run II's piece, with its fight (1.0.0b15's drowned barcarolle is retired) | `MU_CATACOMBS`, `SC_DESCENT`, `descent_step` |
| E, the estampie | while the Lampstand is open (`UI_ALIGN`) and while ascending (`UI_ASCEND`, and `UI_STONES` on the way in) | `MU_LAMP`, `SC_ESTAMPIE`, `estampie_step` |
| A, the office of the dead | kept, unused | `MU_OFFICE`, `SC_OFFICE`, `office_step` |
| C, the ossuary | kept, unused | `MU_OSSUARY`, `SC_OSSUARY`, `ossuary_step` |

Taken as defaults: the descent stays in D minor (run I's key too) as it was heard; the menus' tune keeps its run II guise
(the harp under water, in C minor). The estampie plays on the menus' bus, so her piece waits under it and takes up again
where it stopped when the screen closes; it begins from its top each time.

## Run I's attempts (step 2)

Each is D minor or its modes, as run I's piece has been, and each is written with its fight: the fight's layer (`LY_FIGHT`)
comes in on a beat at level 1 and grows to 3, the pace a twentieth quicker a level, a crash and a timpani stroke going in
and a closing chord at a bar line coming out, as 1.0.0b15's pieces do (`player_step`); what each level adds is the
piece's own. The failing (the low-pass, the colour thinned, the heartbeat) and the menus are as before. Candidates are
were `MU_RUN1A`-`MU_RUN1E` until the owner chose (`run_piece` says which pieces the run's player may take).

| | Piece | Key, metre, pace | Calm | Fighting: level 1, 2, 3 | Length |
|---|---|---|---|---|---|
| A | The office of the dead | D minor (Dorian at times), 4/4, 64 | a chant on the choir over the organ's pedal; the chant again in organum (a fourth under it, over the drone alone); a chorale for the choir, ending on D major; a motet (the chant low, a flute's descant, a cello); the crypt (E-flat leaning on D, bells) | timpani on the bass, low strings driving; sackbuts on the chant, the organ's chords struck on brass, a snare; the chant in canon on brass, hats, timpani every beat | 3:15 |
| B | The ill-fated | D minor, 4/4, 80 | a horn's call; the march's phrase grown into a romantic theme on the violins over a harp and strings, the bass falling D, C, B-flat, A; the horn takes it; a soaring B part (F, then E-flat, the Neapolitan); a storm of tremolo over a chromatic fall; all of it; the phrase on a celesta | tremolo and timpani; a snare's march, brass struck on the chords, a cymbal; a drum on every beat, hats, brass with the tune | 2:48 |
| C | The ossuary | D minor, 4/4, 72 | slow chords that turn aside (B-flat major seven, B-flat minor's sixth, G minor, E-flat) in glass and strings over a low pad, a bell for the tune; the choir; the tracker's arpeggio rippling under a cello; the crypt over a pedal | the bass pulsing in sixteenths, the chip's bass drum; its snare and an arpeggio buzzing the chord on one voice, as an AdLib did; hats in sixteenths, brass on the tune, a tom's fill | 3:20 |
| D | The descent | D minor (its flat second in the B part), 4/4, 96 | the dungeon crawlers' music of the AdLib's day: a harpsichord's pluck running the chords in sixteenths, a pad, a bass in eighths, the tune on brass; a B part on the flute; a bridge where a cello sings over a walking bass | the chip's bass drum and snare; hats, the bass galloping, brass struck on the off-beats; hats in sixteenths, cymbals, a muted trumpet on the tune, the toms' fill | 2:30 |
| E | The estampie of the bone choir | D Dorian, 6/8, 50 the dotted beat | a consort: a hurdy-gurdy's drone, a lute striking the chord on each beat and picking between, a recorder; the dance in puncta played twice, open then closed; the third on a shawm; a slow interlude on a viol with an E-flat in it | the tabor's full rhythm and a tambourine; the shawm with the tune, a frame drum, a psaltery running the chords; the tambourine shaken through, the psaltery with the tune, the viol on the drone | 2:20 |

They were rendered once through calm and through a fight (calm, fight 1 at 15 s, 2 at 40 s, 3 at 65 s, over at 95 s,
failing from 105 s to 125 s). Now `sh tools/music.sh crypts` and `catacombs` render runs I and II as any run's piece,
`hall` the estampie (`lamp`) with her hall's, and `spare` the two kept unused.

Checked as her hall's were: no score faults; the clash check (what is left is meant: the chant's flat-sixth neighbour,
a flat ninth on A7); the balance calm and at the fight's height (tunes 0-5 dB under the mix, basses 7-10, pads 8-14,
drums 5-9 at level 3); loudness matched to 1.0.0b15's run I piece (-18.4 LUFS calm; a fight brings each up 1-3 dB, the old
one 1.5); peak voices 13-45 of 64.

## What was checked

- No faults in the scores (`snd_check`): every bar adds up.
- A clash check over the parsed scores (a script, not in the repo): every minor second and minor ninth between a tune
  and anything else where one of them begins. What is left is meant (the line cliché, major sevenths, flat ninths on
  dominants, the passacaglia's sighs). Ninths that sat a minor ninth under a tune's third were taken out of the chords.
- The balance, each instrument alone against its mix (K-weighted, while it plays): tunes 3-5 dB under the mix, basses
  7-10, chords and pads 8-12, drums 10-14. Each piece's loudness matched to the old habanera's (-18.3 LUFS).
- Peak voices 15-45 of 64, none stolen; the synth about 2.7% of a core in her hall (1.5% before). ASan and UBSan clean
  over all five, leaving her hall and coming back. `make smoke` passes.
- In the game (the debug build): her hall's piece is the habanera, and the alley cat after a level counted perfect; the
  ledger's opening brings the lament ground and Esc's question the menus' tune again; the Lampstand and ascending bring
  the estampie, her menu keeps the habanera; runs I and II play the ill-fated and the descent, fights coming and going.

## How to hear them

`sh tools/music.sh hall` renders her hall's pieces into `build/music/` (`merchant`, `perfect`, `title`, `special`,
`ending`); `build/host music <name> <seconds> out.wav [event@t ...]` plays any one through events as any piece.

## The hand-off log

- **9 November, step 1**: the engine's additions and the five candidates, harness only; the renders sent to the owner.
- **9 November, step 1 wired**: the owner's choices in; the 1.0.0b15 habanera retired; the plan renamed from
  `PLAN-music2.md` at the owner's word. Step 2 begun.
- **9 November, step 2**: run I's five attempts (`SC_OFFICE`, `SC_GOTHIC`, `SC_OSSUARY`, `SC_DESCENT`, `SC_ESTAMPIE`; the fight's
  helpers `sc_root`, `sc_figure`, `sc_stab`, `sc_late`, `timp_hz`), harness only, rendered and sent. Next: the owner's choice;
  the chosen one takes `MU_CRYPTS`'s place (the menus' tune keeps its guise for run I), the others go or are kept as the
  owner says.
- **9 November, step 2 wired**: the owner's choice in: the ill-fated is run I's (`MU_CRYPTS`), the descent run II's
  (`MU_CATACOMBS`), the estampie the Lampstand's and ascending's (`MU_LAMP`, `snd_set_menu(4)`); the office and the ossuary
  kept (`MU_OFFICE`, `MU_OSSUARY`); 1.0.0b15's `crypts_step` and `catac_step` retired (the D chords stay for the menus'
  tune). Harness only. Next: whatever the owner asks (runs III and IV, the menus, the bosses).
- **9 November, merged**: `snd_set_menu` 5 and 6 added (the waltz and the nocturne, for screens to come; the harness's
  `waltz` and `nocturne` events), and the work squashed onto main at the owner's word, unreleased, for the title screen's
  agent. Harness only; CI compiles the Windows and browser builds.
