/* Sound: a small OPL2-flavoured FM synth, noise/PC-speaker effects and the
 * two music "programs" (one per scene).  Everything is synthesised at runtime;
 * there are no audio assets. */
#ifndef SND_H
#define SND_H

#include <stdint.h>

#define SND_RATE 44100

enum { SND_PROG_NONE = 0, SND_PROG_HAUNT, SND_PROG_CRAWL };

enum {
    SFX_BEEP_POST,      /* PC-speaker POST beep                      */
    SFX_BEEP_ERR,       /* low double beep                           */
    SFX_KEY,            /* keyboard click                            */
    SFX_ENTER,          /* heavier click                             */
    SFX_FLOPPY,         /* floppy seek + read                        */
    SFX_HDD,            /* hard-disk crunch                          */
    SFX_WHISPER,        /* formant-swept noise "voice"               */
    SFX_CREAK,          /* door creak                                */
    SFX_STATIC,         /* static burst                              */
    SFX_BOOM,           /* low boom                                  */
    SFX_STINGER,        /* dark FM chord stab, pitch dive            */
    SFX_GHOST_TICK,     /* spectral typing blip                      */
    SFX_TICK,           /* progress-bar tick                         */
    SFX_SHIMMER,        /* rising shimmer swell                      */
    SFX_WHOOSH,         /* noise sweep (transitions)                 */
    SFX_ZAP,            /* falling zap                               */
    SFX_HEART,          /* heartbeat lub-dub                         */
    SFX_STEP,           /* footstep on stone                         */
    SFX_DOOR,           /* heavy door grinding open                  */
    SFX_CAST,           /* spell whoosh                              */
    SFX_EXPLODE,        /* fireball impact                           */
    SFX_BONES,          /* skeleton collapsing: bone clatter         */
    SFX_GHOST_DIE,      /* falling wail                              */
    SFX_BAT,            /* squeaks                                   */
    SFX_DRIP,           /* water drip in a cave                      */
    SFX_MOAN,           /* distant moan                              */
    SFX_COIN,           /* treasure: two bright chimes               */
    SFX_KEYGET,         /* key picked up: jingle and a rising chime  */
    SFX_UNLOCK,         /* heavy lock clunk and a chain rattle       */
    SFX_HURT,           /* the player is hit: thud and a grunt       */
    SFX_THROW,          /* a bone thrown: short whoosh               */
    SFX_DEATH,          /* the player dies: falling groan and a thud */
    SFX_POTION,         /* glug and a rising shimmer                 */
    SFX_GROAN,          /* zombie groan                              */
    SFX_DIRT,           /* earth breaking and crumbling              */
    SFX_XYLO,           /* a run on a bone xylophone                 */
    SFX_WAIL,           /* long ghostly wail                         */
    SFX_CAW,            /* a crow                                    */
    SFX_FANFARE,        /* level complete: brass flourish            */
    SFX_SHRIEK,         /* a wraith recoils: a piercing scream        */
    SFX_IGNITE,         /* a match struck and a flame taking         */
    SFX_WIND,           /* a gust through dry leaves                 */
    SFX_CACKLE,         /* a high, nasal "heh-heh-heh-HAA"           */
    SFX_CHATTER,        /* teeth chattering                          */
    SFX_KNOCK,          /* one knock on a wooden table               */
    SFX_DIAL,           /* dial tone, then 555-0666 in DTMF          */
    SFX_MODEM,          /* a 14.4k modem handshake                   */
    SFX_SHATTER,        /* something breaking: a shield, a ward, an urn */
    SFX_SKITTER,        /* a spider: legs clicking, a hiss           */
    SFX_SCREAM,         /* a wall skull wakes                        */
    SFX_SWORD,          /* a sword swung                             */
    SFX_IMPLODE,        /* the BFS9000: everything drawn in, then a thud */
    SFX_FROST,          /* ice crackling                             */
    SFX_RUNE,           /* a rune flaring on the caster's hand       */
    SFX_PURR,           /* the merchant purring, about two breaths   */
    SFX_TRILL,          /* the merchant's chirrup                    */
    SFX_PUSHWALL,       /* a block of stone grinding back, then settling */
    SFX_SECRET,         /* a secret found: a hollow knock, then bells  */
    SFX_COUNT
};

/* Where the music is, for visuals that want to follow it. */
typedef struct SndState {
    double song_time;   /* seconds since the program started (latency-corrected) */
    int    step;        /* 16th-note index */
    int    bar;
    float  mood;
} SndState;

void snd_init(void);
void snd_set_volume(float v);               /* master, 0..1 */
void snd_fade_master(float target, float seconds);
void snd_start_program(int prog);           /* hard reset into a program */
void snd_set_mood(float m);                 /* HAUNT: intensity; CRAWL: nearby torches (crackle) */
void snd_sfx(int id);
void snd_sfx_ex(int id, float gain, float pan); /* pan 0..1, or < 0 for the effect's default */
void snd_hold(int on);                      /* effects and the torches' crackle wait; the music plays on */
void snd_set_latency(int frames);           /* device latency, for visual sync */
void snd_render(int16_t *out, int frames);  /* interleaved stereo */
void snd_advance(double seconds);           /* run the clock with no audio device */
void snd_state(SndState *s);

#endif
