# The music again: more chords, more voices, the rest of what an AdLib could do

*Begun 9 November 2026 on `claude/beautiful-einstein-tzed36`, from main at 1.0.0b21. Nothing in the game moves until the
owner has heard the attempts and chosen.*

## Status

Her hall first: five attempts at her piece are written and rendered for the owner's ear (`sh tools/music.sh shops`, into
`build/music/shop1.mp3` ... `shop5.mp3`). The game still plays 1.0.0b15's habanera (`MU_MERCHANT`); the candidates are
`MU_SHOP1`-`MU_SHOP5`, which only the harness plays. Run I and its fight come next, the same way, once her piece is
chosen. Harness only; nothing here has been heard by a person yet.

## What the owner asked (9 November)

The screensaver's music is fine; the game's should be better. Start with one set of pieces: her hall, then run I with
its fight. More chords, layered complexity, more voices, and the whole of what an AdLib could do, to give each piece
more structure. Five attempts at her hall first.

## The engine (`src/snd.c`)

What 1.0.0b15's pieces left unused of an OPL, and a few things a tracker did with one:

- **Tremolo** (`Patch.trem`, `tremm`, `trem_hz`): the chip's AM, on the carrier's level and the modulator's index, from
  one clock for every voice (`g_lfo_t`), as on the chip, so a chord breathes together. The bandoneon, the motor of the
  vibraphone, the muted trumpet.
- **Key scaling** (`Patch.kslm`, `kslc`, `ksr`): an octave above middle C the modulator loses so many dB (brightness), the
  carrier too (level), and the envelopes run quicker. The clarinet and guitar mellow and shorten as they climb.
- **OPL3's waveforms** (4-7: alternating, camel, square, derived square), for whoever wants them later.
- **Glides** (`Voice.glide`, `bend_end`): a note slid up to from a semitone, scooped from two, fallen onto from above.
- **The tracker's arpeggio** (`Voice.arp`, `narp`, `arp_hz`; `sc_arpvoice`): one voice running through a chord's notes
  faster than a hand, the AdLib's way of a chord on one channel. The waltz's clocks running down.
- **Rhythm mode** (`X_OBD`, `X_OSD`, `X_OHH`, `X_OCY`, `X_OTT`): the chip's bass drum (two operators), and its snare, hat
  and cymbal made as the chip makes them, from phase bits of two oscillators and its noise; the tom a sagging sine. Each
  can be pitched (`Sfx.f0`, `pdf`). And hands: castanets (a roll by `f0`), a tambourine, a brush (`f0` 1 a tap), a clap,
  a snap.
- **Twelve instruments** (`PAT_CLAR` ... `PAT_CHARP`): clarinet (the modulator an octave up, odd harmonics only), a
  bandoneon's reeds, a gut-strung guitar, a double bass plucked, the motor vibraphone, a muted trumpet, a harpsichord,
  an oboe (the carrier on the third harmonic), a woodblock, a string section, a solo violin or cello, a concert harp.
- **64 voices** (48), **128 notes waiting** (32; `sched_v`): a guitar's strum, a harp's roll, a piano's spread.
- **Scored pieces** (`ScSec`, `Ev`, `sc_parse`; `Piece.sc`, `step2`). A piece's parts are written as text, a string a
  part (chords, bass, tune, counter and three more), read once at start (`score_init`); `snd_check` reports any bar
  whose steps don't add up. A section is any number of bars, and the same music can come again dressed another way
  (`ScSec.var`). The notation is at `ScSec` in `snd.c`. Helpers: `sc_play`, `sc_play_ex` (spread, roll, a doubled
  voice a few cents sharp), `sc_musette`, `sc_strum`, `sc_arp`, `sc_now`, `sc_after` (the next chord, into the next
  section), `sc_bass_jazz` (a bass in two or walking, finding its own way: a chord tone, then a half step into the next
  root) and `sc_comp` (a rootless chord, anticipating the next).
- **The echo's time by the piece in front** (`Piece.dly`, in steps): the delay follows her piece's tempo in her hall.
- **For weighing a mix**: `snd_mute(mask)` and the harness's `HOST_MUTE` (patches by number, bit 63 the pieces' drums),
  `HOST_DRIPS=0` (the cave's drips and moans left out of a render).

The old pieces and the screensaver are untouched: the old merchant renders bit for bit as before.

## Her hall: the five attempts

| | Piece | Key, metre, pace | What carries it | Its form | Length |
|---|---|---|---|---|---|
| 1 | The habanera, grown | A minor, 4/4, 92 | pizzicato habanera over an A pedal while the chords slide A, G-sharp, G, F-sharp inside it; a gut-strung guitar on the off-beats; a bandoneon (tremolo, two reeds a few cents apart); the flute sliding to its notes, a clarinet under it; castanets, hands, the chip's tom | intro, A, A (with the clarinet), B (the circle of fifths on clarinet, to a stop), A (on bandoneon), her purr (the Andalusian cadence, a music box over guitar arpeggios), B (violin), A (everything), a turn | 2:42 |
| 2 | The clockwork waltz | C minor, 3/4, 138 | woodblocks ticking and tocking a beat; a music box with a celesta over it; pizzicato and harp for the oom-pah-pah; the B part in E-flat on violin, its minor fourth turning the light | intro (the clocks, the box wound up), A (16), B (16), A (violin and clarinet), the clocks run down (A-flat, glass harmonica over a tracker's arpeggio), B (flute), A (all), coda | 2:16 |
| 3 | The alley cat | F minor, swing, 120 | a muted trumpet on the tune; brushes, the chip's cymbal as a ride and its hat on two and four; an electric piano comping rootless ninths; a double bass in two, then walking | intro, A A B A (trumpet, brushes, bass in two but the bridge), A A B A (vibraphone on the tune, clarinet holding guide tones, the ride, the bass walking, the trumpet back for the last eight), tag | 2:24 |
| 4 | The lament ground | G minor, 3/4, 72 | a passacaglia: the bass falls G, F, E-flat, D on low strings and a harpsichord, and each variation lays more on it | the ground alone (harpsichord sarabande, a bell), the oboe's theme, the violin with the oboe against it, the ground made chromatic under a choir (flute and oboe through suspensions), divisions (running harpsichord, the ground walking, the organ), everything (organ, timpani), the chromatic ground quietly (music box, glass, harp) | 2:40 |
| 5 | The midnight nocturne | D-flat major, 4/4, 66 | a concert harp rolling wide chords (sevenths, ninths, a sharp eleventh); a music box; a cello under it; glass; a celesta and a choir; no drums but a finger cymbal | intro, A (music box), A (celesta, cello), B (B-flat minor on glass, the harp rippling), C (the chords sliding down by whole tones, all alike), A (vibraphone, everything), coda (G-flat minor, home) | 2:40 |

Each plays its whole order once and comes round to its first A (the intro isn't heard again). Each has her back rooms
written (`g_place == 2`): its bass and its tune on the music box, as before. The renders play each through once, a few
seconds of its top again, then her back rooms.

## What was checked

- No faults in the scores (`snd_check`): every bar adds up.
- A clash check over the parsed scores (a script, not in the repo): every minor second and minor ninth between a tune
  and anything else where one of them begins. What is left is meant: the line cliché's A against G-sharp, the major
  sevenths, E7's flat ninth, the passacaglia's sighs (B-flat to A and G to F-sharp over D). Ninths that sat a minor
  ninth under a tune's third were taken out of the chords.
- The balance, each instrument alone against its mix (K-weighted, while it plays): the tunes 3-5 dB under the mix, the
  basses 7-10, the chords and pads 8-12, the drums 10-14. Each piece's loudness matched to the old one's (-18.3 LUFS
  integrated, with the drips left out).
- Peak voices 15-45 of 64, none stolen. The synth costs about 2.7% of a core in her hall (1.5% for the old piece).
- Not heard: by me (I can't), or by the owner yet.

## Next

1. The owner hears the five and chooses one, or a mix of them (one's tune on another's harmony, say).
2. It takes `MU_MERCHANT`'s place; the others go.
3. Run I and its fight the same way: attempts to hear first.

## The hand-off log

- **9 November**: the engine's additions and the five candidates, harness only. `sh tools/music.sh shops` renders them;
  `build/host music shop1 ...` plays one through events as any piece. The game is unchanged.
