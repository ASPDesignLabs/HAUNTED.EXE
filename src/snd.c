/* FM synth, effects and the two music programs. See snd.h. */
#include "snd.h"
#include "core.h"
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
static CRITICAL_SECTION g_cs;
static volatile LONG g_cs_ready;
#define LOCK()   EnterCriticalSection(&g_cs)
#define UNLOCK() LeaveCriticalSection(&g_cs)
#else
#define LOCK()   ((void)0)
#define UNLOCK() ((void)0)
#endif

#define SR ((float)SND_RATE)

/* ------------------------------------------------------------------ */
/* Sine table / waveforms (OPL2 had four: sine, half, abs, quarter)    */
/* ------------------------------------------------------------------ */

#define TBL 2048
static float g_sin[TBL + 1];

static inline float sin_cyc(float ph) {        /* ph in cycles, any range */
    ph -= floorf(ph);
    float x = ph * TBL;
    int i = (int)x;
    if ((unsigned)i >= TBL) { i = 0; x = 0.0f; }   /* a hair under zero wraps to exactly 1.0 (and NaN lands here too) */
    float fr = x - i;
    return g_sin[i] + (g_sin[i + 1] - g_sin[i]) * fr;
}

static inline float wave(int w, float ph) {
    float s = sin_cyc(ph);
    switch (w) {
    case 1:  return s > 0.0f ? s : 0.0f;
    case 2:  return s < 0.0f ? -s : s;
    case 3: {
        float p = ph - floorf(ph);
        if (p >= 0.5f) p -= 0.5f;
        return p < 0.25f ? fabsf(s) : 0.0f;
    }
    default: return s;
    }
}

static inline float nz(uint32_t *r) {           /* white noise -1..1 */
    return (float)(rng_next(r) >> 8) * (2.0f / 16777216.0f) - 1.0f;
}

/* ------------------------------------------------------------------ */
/* Patches                                                             */
/* ------------------------------------------------------------------ */

typedef struct Patch {
    float mc, mm;               /* carrier / modulator frequency multipliers */
    uint8_t wc, wm;             /* waveforms */
    float idx;                  /* peak modulation index (radians) */
    float fb;                   /* modulator feedback */
    uint8_t add;                /* 1 = additive instead of FM */
    float am, dm, sm, rm;       /* modulator envelope: attack, decay, sustain, release */
    float ac, dc, sc, rc;       /* carrier envelope */
    float vib_hz, vib_st, vib_delay;
    float level;
    float rev, dly;             /* effect sends */
} Patch;

enum { PAT_BELL, PAT_MBOX, PAT_DRONE, PAT_PAD, PAT_BASS, PAT_STAB,
       PAT_FLUTE, PAT_BRASS, PAT_HARP, PAT_ORGAN, PAT_EPIANO, PAT_VIBES, PAT_VOICE, PAT_COUNT };

static const Patch PATCHES[PAT_COUNT] = {
    /* bell: inharmonic 1:3.5, modulation dies away -> pure ring */
    [PAT_BELL]  = { .mc=1, .mm=3.5f, .wc=0, .wm=0, .idx=4.0f, .fb=0.0f,
                    .am=0.001f,.dm=1.1f,.sm=0, .rm=0.3f,  .ac=0.001f,.dc=2.8f,.sc=0, .rc=0.6f,
                    .vib_hz=0,.vib_st=0,.vib_delay=0, .level=0.30f, .rev=0.65f, .dly=0.30f },
    /* music box */
    [PAT_MBOX]  = { .mc=1, .mm=2.0f, .wc=0, .wm=0, .idx=2.4f, .fb=0.0f,
                    .am=0.001f,.dm=0.35f,.sm=0, .rm=0.2f, .ac=0.001f,.dc=1.2f,.sc=0, .rc=0.4f,
                    .vib_hz=0,.vib_st=0,.vib_delay=0, .level=0.26f, .rev=0.55f, .dly=0.20f },
    /* dark organ drone */
    [PAT_DRONE] = { .mc=1, .mm=2.0f, .wc=0, .wm=0, .idx=1.4f, .fb=0.35f,
                    .am=2.5f,.dm=1.0f,.sm=0.8f, .rm=3.0f, .ac=3.0f,.dc=1.0f,.sc=1.0f, .rc=4.0f,
                    .vib_hz=0.3f,.vib_st=0.10f,.vib_delay=0.5f, .level=0.075f, .rev=0.55f, .dly=0.0f },
    /* soft pad */
    [PAT_PAD]   = { .mc=1, .mm=1.0f, .wc=0, .wm=1, .idx=1.1f, .fb=0.2f,
                    .am=0.35f,.dm=1.0f,.sm=0.7f, .rm=0.6f, .ac=0.30f,.dc=0.8f,.sc=0.85f, .rc=0.7f,
                    .vib_hz=5.0f,.vib_st=0.05f,.vib_delay=0.5f, .level=0.12f, .rev=0.45f, .dly=0.10f },
    /* punchy bass */
    [PAT_BASS]  = { .mc=1, .mm=1.0f, .wc=0, .wm=0, .idx=3.0f, .fb=0.10f,
                    .am=0.001f,.dm=0.14f,.sm=0.12f, .rm=0.05f, .ac=0.001f,.dc=0.30f,.sc=0.65f, .rc=0.06f,
                    .vib_hz=0,.vib_st=0,.vib_delay=0, .level=0.36f, .rev=0.03f, .dly=0.0f },
    /* brass stab (stinger) */
    [PAT_STAB]  = { .mc=1, .mm=1.0f, .wc=0, .wm=0, .idx=3.4f, .fb=0.55f,
                    .am=0.03f,.dm=1.4f,.sm=0.2f, .rm=1.2f, .ac=0.02f,.dc=1.6f,.sc=0.3f, .rc=1.4f,
                    .vib_hz=4.5f,.vib_st=0.12f,.vib_delay=0.3f, .level=0.30f, .rev=0.60f, .dly=0.0f },
    /* breathy flute lead */
    [PAT_FLUTE] = { .mc=1, .mm=1.0f, .wc=0, .wm=0, .idx=0.9f, .fb=0.05f,
                    .am=0.06f,.dm=0.4f,.sm=0.7f, .rm=0.15f, .ac=0.05f,.dc=0.5f,.sc=0.85f, .rc=0.18f,
                    .vib_hz=5.2f,.vib_st=0.12f,.vib_delay=0.25f, .level=0.20f, .rev=0.35f, .dly=0.25f },
    /* OPL brass: the modulator swells in after the carrier */
    [PAT_BRASS] = { .mc=1, .mm=1.0f, .wc=0, .wm=0, .idx=3.2f, .fb=0.45f,
                    .am=0.09f,.dm=0.5f,.sm=0.7f, .rm=0.12f, .ac=0.03f,.dc=0.6f,.sc=0.8f, .rc=0.15f,
                    .vib_hz=4.8f,.vib_st=0.10f,.vib_delay=0.3f, .level=0.19f, .rev=0.30f, .dly=0.20f },
    /* harpsichord-ish pluck */
    [PAT_HARP]  = { .mc=1, .mm=3.0f, .wc=0, .wm=0, .idx=2.2f, .fb=0.10f,
                    .am=0.001f,.dm=0.25f,.sm=0, .rm=0.10f, .ac=0.001f,.dc=0.6f,.sc=0, .rc=0.15f,
                    .vib_hz=0,.vib_st=0,.vib_delay=0, .level=0.15f, .rev=0.25f, .dly=0.30f },
    /* soft church organ for chords */
    [PAT_ORGAN] = { .mc=1, .mm=2.0f, .wc=0, .wm=0, .idx=1.0f, .fb=0.15f,
                    .am=0.08f,.dm=1.0f,.sm=0.9f, .rm=0.40f, .ac=0.08f,.dc=1.0f,.sc=0.95f, .rc=0.45f,
                    .vib_hz=5.5f,.vib_st=0.04f,.vib_delay=0.4f, .level=0.075f, .rev=0.45f, .dly=0.0f },
    /* electric piano for the muzak: the tine's bark dies away, a round tone stays */
    [PAT_EPIANO] = { .mc=1, .mm=1.0f, .wc=0, .wm=0, .idx=1.8f, .fb=0.05f,
                    .am=0.001f,.dm=0.6f,.sm=0.15f, .rm=0.3f, .ac=0.002f,.dc=1.8f,.sc=0.3f, .rc=0.35f,
                    .vib_hz=4.0f,.vib_st=0.02f,.vib_delay=0.2f, .level=0.13f, .rev=0.40f, .dly=0.10f },
    /* vibraphone: a soft bell with a slow shimmer */
    [PAT_VIBES] = { .mc=1, .mm=4.0f, .wc=0, .wm=0, .idx=1.5f, .fb=0.0f,
                    .am=0.001f,.dm=0.35f,.sm=0, .rm=0.5f, .ac=0.001f,.dc=2.2f,.sc=0, .rc=0.6f,
                    .vib_hz=5.5f,.vib_st=0.03f,.vib_delay=0.0f, .level=0.17f, .rev=0.45f, .dly=0.20f },
    /* a voice, or a choir of them: an "oo" that swells in, wavers and dies slowly */
    [PAT_VOICE] = { .mc=1, .mm=2.0f, .wc=0, .wm=0, .idx=0.7f, .fb=0.25f,
                    .am=0.50f,.dm=1.0f,.sm=0.8f, .rm=0.8f, .ac=0.45f,.dc=1.0f,.sc=0.9f, .rc=0.9f,
                    .vib_hz=5.2f,.vib_st=0.14f,.vib_delay=0.35f, .level=0.14f, .rev=0.60f, .dly=0.15f },
};

/* ------------------------------------------------------------------ */
/* Voices                                                              */
/* ------------------------------------------------------------------ */

enum { ST_ATT, ST_DEC, ST_REL, ST_DONE };

typedef struct Env { int st; float lvl; } Env;

typedef struct Voice {
    const Patch *p;
    int active;
    uint32_t serial;
    float freq;                 /* Hz, before bend/vibrato */
    float bend, bend_rate;      /* semitones, semitones/sec */
    float phm, phc, incm, incc;
    float fb1, fb2;
    Env em, ec;
    float ram, rac, kdm, kdc, krm, krc;
    int gate, gate_left;        /* gate_left < 0: held until voice_off */
    float vel, pl, pr, t;
    int gainch;                 /* 1 = scaled by the drone gain */
    int ctl;
    int bus;                    /* which music it belongs to, which fades as a whole: BUS_* */
} Voice;

/* The march, Severance's muzak (in as the march goes), a boss's waltz (the same), and the notes the
 * game's effects play (the fanfare, the stinger), which neither fades. */
enum { BUS_MARCH, BUS_MUZAK, BUS_BOSS, BUS_SFX };
static int g_vbus;                  /* the bus new voices, drums and scheduled notes go to */

#define NV 36
static Voice g_v[NV];
static uint32_t g_serial;

static inline float coef_for(float seconds) {      /* ~1% after `seconds` */
    if (seconds < 0.002f) seconds = 0.002f;
    return expf(-4.6f / (seconds * SR));
}

static Voice *voice_on(int pat, int midi, float vel, float pan, int gate_samples) {
    Voice *v = NULL;
    for (int i = 0; i < NV; i++) if (!g_v[i].active) { v = &g_v[i]; break; }
    if (!v) {
        v = &g_v[0];
        for (int i = 1; i < NV; i++) if (g_v[i].serial < v->serial) v = &g_v[i];
    }
    const Patch *p = &PATCHES[pat];
    memset(v, 0, sizeof *v);
    v->p = p;
    v->active = 1;
    v->serial = ++g_serial;
    v->freq = 440.0f * exp2f((midi - 69) / 12.0f);
    v->em.st = ST_ATT; v->ec.st = ST_ATT;
    v->ram = 1.0f / (fmaxf(p->am, 0.001f) * SR);
    v->rac = 1.0f / (fmaxf(p->ac, 0.001f) * SR);
    v->kdm = coef_for(p->dm); v->kdc = coef_for(p->dc);
    v->krm = coef_for(p->rm); v->krc = coef_for(p->rc);
    v->gate = 1;
    v->gate_left = gate_samples;
    v->vel = vel;
    v->bus = g_vbus;
    float a = clampf(pan, 0.0f, 1.0f) * (PI_F * 0.5f);
    v->pl = cosf(a); v->pr = sinf(a);
    return v;
}

static inline float env_tick(Env *e, int gate, float ra, float kd, float kr, float sus) {
    if (!gate && e->st < ST_REL) e->st = ST_REL;
    switch (e->st) {
    case ST_ATT:
        e->lvl += ra;
        if (e->lvl >= 1.0f) { e->lvl = 1.0f; e->st = ST_DEC; }
        break;
    case ST_DEC:
        e->lvl = sus + (e->lvl - sus) * kd;
        if (sus <= 0.0f && e->lvl < 0.0004f) e->st = ST_DONE;
        break;
    case ST_REL:
        e->lvl *= kr;
        if (e->lvl < 0.0004f) e->st = ST_DONE;
        break;
    default: break;
    }
    return e->st == ST_DONE ? 0.0f : e->lvl;
}

static float g_drone_gain = 1.0f;

static inline void voice_run(Voice *v, float *outl, float *outr, float *rev, float *dly) {
    const Patch *p = v->p;
    if (v->gate_left > 0 && --v->gate_left == 0) v->gate = 0;

    if ((v->ctl++ & 15) == 0) {                     /* control rate */
        float st = v->bend;
        if (p->vib_hz > 0.0f && v->t > p->vib_delay)
            st += p->vib_st * sin_cyc(p->vib_hz * v->t);
        float f = v->freq * (st != 0.0f ? exp2f(st / 12.0f) : 1.0f);
        v->incc = f * p->mc / SR;
        v->incm = f * p->mm / SR;
    }
    v->t += 1.0f / SR;
    v->bend += v->bend_rate / SR;

    float em = env_tick(&v->em, v->gate, v->ram, v->kdm, v->krm, p->sm);
    float ec = env_tick(&v->ec, v->gate, v->rac, v->kdc, v->krc, p->sc);
    if (v->ec.st == ST_DONE) { v->active = 0; return; }

    float fbv = (v->fb1 + v->fb2) * 0.5f * p->fb;
    float m = wave(p->wm, v->phm + fbv * (1.0f / TAU_F) * 3.0f) * em;
    v->fb2 = v->fb1; v->fb1 = m;
    float c;
    if (p->add) {
        c = (wave(p->wc, v->phc) * ec + m * 0.6f);
    } else {
        c = wave(p->wc, v->phc + p->idx * m * (1.0f / TAU_F)) * ec;
    }
    v->phm += v->incm; if (v->phm >= 1.0f) v->phm -= 1.0f;
    v->phc += v->incc; if (v->phc >= 1.0f) v->phc -= 1.0f;

    float g = p->level * v->vel * (v->gainch ? g_drone_gain : 1.0f);
    float s = c * g;
    *outl += s * v->pl;
    *outr += s * v->pr;
    *rev += s * p->rev;
    *dly += s * p->dly;
}

/* ------------------------------------------------------------------ */
/* Sound effects (noise, PC speaker, percussion)                       */
/* ------------------------------------------------------------------ */

typedef enum {
    X_KICK = 1000, X_SNARE, X_HAT_C, X_HAT_O, X_CRASH, X_TOM
} XType;                                         /* percussion, after SFX_COUNT */

typedef struct Sfx {
    int type;
    int active;
    float t, dur;
    float a, b, c, d;           /* per-type scratch */
    float ph[5];
    float lp1, lp2, bp1, bp2, lo1, lo2;
    uint32_t rng;
    float gain, pan, rev;
    int bus;                    /* a drum's music */
} Sfx;

#define NS 28
static Sfx g_s[NS];
static uint32_t g_rng = 12345;
static int g_hold;                  /* the console is open: the game's effects and the crackle wait; the music and its drums play */
static int g_trial;                 /* a trial under way: the march a quarter quicker, its drums driving */
static float g_fx_gain = 1.0f;      /* the game's effects fade out and back in over a fiftieth of a second, so nothing clicks */
static int g_room;                  /* 1 the Backrooms: the music gone, a hum, more echo on the game's sounds; 2 Severance: muzak too */
static int g_muffle;                /* the void: everything as if heard through cloth */
static float g_muf_k, g_mlp[2];
static float g_room_k, g_muz_k;     /* how far into the room's sound, and into the muzak, 0..1, over a second */
static float g_hum_ph;

static Sfx *sfx_start(int type, float dur, float gain, float pan, float rev) {
    static Sfx none;
    if (g_hold && type < SFX_COUNT) { memset(&none, 0, sizeof none); return &none; }   /* nothing new while it waits */
    Sfx *s = NULL;
    for (int i = 0; i < NS; i++) if (!g_s[i].active) { s = &g_s[i]; break; }
    if (!s) s = &g_s[rng_next(&g_rng) % NS];
    memset(s, 0, sizeof *s);
    s->type = type; s->active = 1; s->dur = dur;
    s->gain = gain; s->pan = pan; s->rev = rev;
    s->rng = rng_next(&g_rng) | 1u;
    s->bus = g_vbus;
    return s;
}

static inline float svf_band(float x, float fc, float q, float *low, float *band) {
    float f = 2.0f * sinf(PI_F * fminf(fc, SR * 0.40f) / SR);
    float high = x - *low - q * *band;
    *band += f * high;
    *low += f * *band;
    return *band;
}

/* Returns the mono sample; stereo placement comes from s->pan. */
static float sfx_sample(Sfx *s) {
    float t = s->t, x = 0.0f;
    switch (s->type) {
    case SFX_BEEP_POST: {                         /* PC speaker */
        s->ph[0] += 987.0f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float e = fminf(1.0f, fminf(t / 0.003f, (s->dur - t) / 0.004f));
        x = (s->ph[0] < 0.5f ? 1.0f : -1.0f) * 0.22f * fmaxf(e, 0.0f);
        s->lp1 += 0.45f * (x - s->lp1); x = s->lp1;
        break;
    }
    case SFX_BEEP_ERR: {
        float u = t < 0.15f ? t : (t > 0.24f && t < 0.39f ? t - 0.24f : -1.0f);
        if (u >= 0.0f) {
            s->ph[0] += 311.0f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            float e = fminf(1.0f, fminf(u / 0.004f, (0.15f - u) / 0.006f));
            x = (s->ph[0] < 0.5f ? 1.0f : -1.0f) * 0.22f * fmaxf(e, 0.0f);
        }
        s->lp1 += 0.35f * (x - s->lp1); x = s->lp1;
        break;
    }
    case SFX_KEY:
    case SFX_ENTER: {
        if (t == 0.0f) { s->a = 0.85f + 0.3f * (nz(&s->rng) * 0.5f + 0.5f); }
        float k = s->type == SFX_ENTER ? 1.5f : 1.0f;
        float u = t, g = 1.0f;
        if (s->type == SFX_ENTER && t > 0.05f) { u = t - 0.05f; g = 0.7f; }
        float click = nz(&s->rng) * expf(-u * 1100.0f) * 0.55f;
        float body  = sin_cyc(150.0f * s->a * u) * expf(-u * 110.0f) * 0.45f;
        float tick  = sin_cyc(2300.0f * s->a * u) * expf(-u * 700.0f) * 0.12f;
        x = (click + body + tick) * g * k * 0.7f;
        break;
    }
    case SFX_FLOPPY: {                              /* seek rattle, then read whirr */
        if (t < 0.38f) {
            float rate = 52.0f + 20.0f * sinf(t * 21.0f);
            s->a += rate / SR;
            if (s->a >= 1.0f) { s->a -= 1.0f; s->b = 1.0f; s->c = 0.8f + 0.5f * (nz(&s->rng) * 0.5f + 0.5f); }
            s->b *= expf(-1.0f / (SR * 0.0045f));
            s->ph[0] += 190.0f * s->c / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            float saw = s->ph[0] * 2.0f - 1.0f;
            x = (saw * 0.55f + nz(&s->rng) * 0.45f) * s->b * 0.34f;
        } else {
            s->ph[1] += 63.0f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
            float sq = s->ph[1] < 0.5f ? 1.0f : -1.0f;
            s->lp1 += 0.05f * (sq - s->lp1);
            s->a += 11.0f / SR; if (s->a >= 1.0f) { s->a -= 1.0f; s->b = 1.0f; }
            s->b *= expf(-1.0f / (SR * 0.003f));
            s->lp2 += 0.2f * (nz(&s->rng) - s->lp2);
            float fade = fminf(1.0f, (s->dur - t) / 0.12f);
            x = (s->lp1 * 0.10f + s->lp2 * 0.10f + nz(&s->rng) * s->b * 0.14f) * fmaxf(fade, 0.0f);
        }
        break;
    }
    case SFX_HDD: {
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) { s->a = 0.012f + 0.05f * (nz(&s->rng) * 0.5f + 0.5f); s->b = 0.2f + 0.8f * (nz(&s->rng) * 0.5f + 0.5f); }
        s->lp1 += 0.14f * (nz(&s->rng) - s->lp1);
        float thump = t < 0.12f ? sin_cyc(55.0f * t) * expf(-t * 25.0f) * 0.5f : 0.0f;
        float fade = fminf(1.0f, (s->dur - t) / 0.1f);
        x = (s->lp1 * s->b * 0.55f + thump) * fmaxf(fade, 0.0f) * 0.7f;
        break;
    }
    case SFX_WHISPER: {
        float w = sinf(PI_F * t / s->dur); w *= w;
        s->c -= 1.0f / SR;                                    /* syllable gate */
        if (s->c <= 0.0f) { s->c = 0.07f + 0.2f * (nz(&s->rng) * 0.5f + 0.5f); s->d = (nz(&s->rng) > -0.25f) ? 1.0f : 0.15f; }
        s->b += (s->d - s->b) * 0.0015f;
        s->a += nz(&s->rng) * 0.0012f; s->a = clampf(s->a, -1.0f, 1.0f);   /* formant drift */
        float n = nz(&s->rng);
        float f1 = 650.0f + 380.0f * s->a, f2 = 1700.0f + 900.0f * s->a;
        float b1 = svf_band(n, f1, 0.22f, &s->lo1, &s->bp1);
        float b2 = svf_band(n, f2, 0.18f, &s->lo2, &s->bp2);
        x = (b1 * 0.8f + b2 * 0.55f) * s->b * w * 0.55f;
        break;
    }
    case SFX_CREAK: {
        float w = sinf(PI_F * t / s->dur);
        float f = 135.0f + 70.0f * sinf(t * 7.0f) + 55.0f * sinf(t * 2.3f + 1.0f) + 30.0f * t;
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float saw = s->ph[0] * 2.0f - 1.0f;
        s->a += (14.0f + 12.0f * sinf(t * 3.7f)) / SR; if (s->a >= 1.0f) s->a -= 1.0f;
        float stick = 0.35f + 0.65f * (s->a < 0.55f ? 1.0f : 0.15f);
        float b1 = svf_band(saw + nz(&s->rng) * 0.2f, 780.0f + 300.0f * sinf(t * 4.1f), 0.28f, &s->lo1, &s->bp1);
        float b2 = svf_band(saw, 1650.0f, 0.3f, &s->lo2, &s->bp2);
        x = (b1 * 0.55f + b2 * 0.3f) * stick * w * 0.55f;
        break;
    }
    case SFX_STATIC: {
        s->a -= 1.0f;
        if (s->a <= 0.0f) { s->a = 3.0f + 40.0f * (nz(&s->rng) * 0.5f + 0.5f); s->b = (nz(&s->rng) > -0.2f) ? 1.0f : 0.0f; }
        x = nz(&s->rng) * s->b * expf(-t * 8.0f) * 0.30f;
        break;
    }
    case SFX_BOOM: {
        float f = 26.0f + 62.0f * expf(-t * 3.2f);
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float cut = 0.01f + 0.25f * expf(-t * 3.0f);
        s->lp1 += cut * (nz(&s->rng) - s->lp1);
        x = (sin_cyc(s->ph[0]) * expf(-t * 1.9f) * 0.8f + s->lp1 * expf(-t * 2.6f) * 1.1f) * 0.8f;
        if (t < 0.003f) x *= t / 0.003f;
        break;
    }
    case SFX_GHOST_TICK: {
        if (t == 0.0f) s->a = 1100.0f + 500.0f * (nz(&s->rng) * 0.5f + 0.5f);
        float e = expf(-t * 20.0f);
        x = (sin_cyc(s->a * t) * 0.7f + sin_cyc(s->a * 2.01f * t) * 0.3f) * e * 0.18f * (0.7f + 0.3f * sin_cyc(30.0f * t));
        break;
    }
    case SFX_TICK: {
        x = sin_cyc(2100.0f * t) * expf(-t * 400.0f) * 0.22f + nz(&s->rng) * expf(-t * 900.0f) * 0.1f;
        break;
    }
    case SFX_SHIMMER: {
        float w = sinf(PI_F * t / s->dur); w *= w;
        float base = 640.0f + 1500.0f * (t / s->dur);
        static const float mul[5] = { 1.0f, 1.5f, 2.01f, 3.02f, 4.07f };
        for (int i = 0; i < 5; i++) {
            float f = base * mul[i] * (1.0f + 0.004f * sinf(t * (5.0f + i)));
            s->ph[i] += f / SR; if (s->ph[i] >= 1.0f) s->ph[i] -= 1.0f;
            x += sin_cyc(s->ph[i]) * (0.30f / (1.0f + i * 0.6f));
        }
        x *= w * 0.22f;
        break;
    }
    case SFX_WHOOSH: {
        float u = t / s->dur;
        float fc = 220.0f * expf(u * 3.3f);
        float amp = sinf(PI_F * u); amp = amp * amp;
        float b = svf_band(nz(&s->rng), fc, 0.55f, &s->lo1, &s->bp1);
        x = b * amp * 0.55f;
        break;
    }
    case SFX_ZAP: {
        float f = 2400.0f * expf(-t * 15.0f) + 110.0f;
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        x = (sin_cyc(s->ph[0]) * 0.6f + (s->ph[0] < 0.5f ? 0.2f : -0.2f)) * expf(-t * 10.0f) * 0.4f;
        break;
    }
    case SFX_HEART: {
        float u = t < 0.19f ? t : t - 0.19f, g = t < 0.19f ? 1.0f : 0.65f;
        s->lp1 += 0.05f * (nz(&s->rng) - s->lp1);
        x = (sin_cyc(58.0f * u) * 0.8f + s->lp1 * 0.4f) * expf(-u * 15.0f) * g * 0.9f;
        break;
    }
    case SFX_STEP: {                                /* thud plus a little grit */
        if (t == 0.0f) s->a = 0.85f + 0.3f * (nz(&s->rng) * 0.5f + 0.5f);
        s->lp1 += 0.15f * (nz(&s->rng) - s->lp1);
        float n = nz(&s->rng); float hp = n - s->lp2; s->lp2 = n;
        x = sin_cyc(72.0f * s->a * t) * expf(-t * 38.0f) * 0.55f + s->lp1 * expf(-t * 50.0f) * 0.9f
          + hp * expf(-t * 90.0f) * 0.08f;
        break;
    }
    case SFX_DOOR: {                                /* stone grinding, then a thunk */
        float w = sinf(PI_F * fminf(t / 0.85f, 1.0f));
        s->lp1 += 0.05f * (nz(&s->rng) - s->lp1);
        float wob = 0.6f + 0.4f * sinf(t * 75.0f) * sinf(t * 11.0f);
        float rumble = sin_cyc(44.0f * t) * 0.25f;
        float thunk = t > 0.86f ? sin_cyc(60.0f * (t - 0.86f)) * expf(-(t - 0.86f) * 18.0f) * 0.7f : 0.0f;
        x = (s->lp1 * 2.6f * wob + rumble) * w * 0.6f + thunk;
        break;
    }
    case SFX_CAST: {                                /* whoosh up + a magic tone */
        float u = t / s->dur;
        float b = svf_band(nz(&s->rng), 400.0f * expf(u * 2.2f), 0.5f, &s->lo1, &s->bp1);
        float amp = fminf(1.0f, t / 0.04f) * expf(-u * 3.0f);
        s->ph[0] += (300.0f + 700.0f * u) / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        x = (b * 0.8f + sin_cyc(s->ph[0]) * 0.18f) * amp;
        break;
    }
    case SFX_EXPLODE: {
        float cut = 0.02f + 0.4f * expf(-t * 6.0f);
        s->lp1 += cut * (nz(&s->rng) - s->lp1);
        float f = 40.0f + 80.0f * expf(-t * 9.0f);
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float crack = (nz(&s->rng) > 0.995f - 0.01f * expf(-t * 4.0f)) ? nz(&s->rng) * 0.6f : 0.0f;
        x = (s->lp1 * 1.4f * expf(-t * 4.0f) + sin_cyc(s->ph[0]) * 0.7f * expf(-t * 7.0f) + crack * expf(-t * 3.0f)) * 0.8f;
        if (t < 0.004f) x *= t / 0.004f;
        break;
    }
    case SFX_BONES: {                               /* clatter of dry bones */
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) {
            float density = 0.012f + 0.06f * (t / s->dur);
            s->a = density * (0.3f + (nz(&s->rng) * 0.5f + 0.5f));
            s->b = (0.5f + 0.5f * (nz(&s->rng) * 0.5f + 0.5f)) * (1.0f - t / s->dur);
            s->c = 1400.0f + 2600.0f * (nz(&s->rng) * 0.5f + 0.5f);
            s->d = 650.0f + 500.0f * (nz(&s->rng) * 0.5f + 0.5f);
            s->ph[0] = 0.0f;
        }
        s->b *= expf(-1.0f / (SR * 0.018f));
        float click = svf_band(nz(&s->rng) * s->b, s->c, 0.12f, &s->lo1, &s->bp1);
        s->ph[0] += s->d / SR;
        float knock = sin_cyc(s->ph[0]) * s->b * 0.35f;
        x = (click * 1.6f + knock) * 0.8f;
        break;
    }
    case SFX_GHOST_DIE: {                           /* a wail that falls away */
        float u = t / s->dur;
        float f = 880.0f * expf(-u * 1.1f) * (1.0f + 0.03f * sinf(t * 38.0f));
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        s->ph[1] += 2.0f * f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
        float amp = fminf(1.0f, t / 0.06f) * (1.0f - u) * (1.0f - u);
        float breath = svf_band(nz(&s->rng), f * 1.5f, 0.3f, &s->lo1, &s->bp1);
        x = (sin_cyc(s->ph[0]) * 0.45f + sin_cyc(s->ph[1]) * 0.12f + breath * 0.3f) * amp;
        break;
    }
    case SFX_BAT: {                                 /* three quick chirps */
        float u = fmodf(t, 0.09f);
        if (t < 0.27f && u < 0.03f) {
            float f = 6200.0f - 2200.0f * (u / 0.03f);
            s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            x = sin_cyc(s->ph[0]) * sinf(PI_F * u / 0.03f) * 0.22f;
        }
        break;
    }
    case SFX_HISS: {                                /* gas escaping through a crack: noise, high, swelling and dying */
        float u = t / s->dur, w = sinf(PI_F * fminf(1.0f, u * 1.6f)) * (1.0f - u * 0.4f);
        x = svf_band(nz(&s->rng), 4200.0f + 1800.0f * sinf(t * 3.0f), 0.35f, &s->lo1, &s->bp1) * w * w * 0.55f;
        break;
    }
    case SFX_CRACKLE: {                             /* a buzz under it, and sparks snapping, closer together as it builds */
        float u = t / s->dur;
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) { s->a = (0.05f - 0.04f * u) * (0.3f + 0.7f * (nz(&s->rng) * 0.5f + 0.5f)); s->b = 1.0f; }
        s->b *= expf(-1.0f / (SR * 0.003f));
        s->ph[0] += 120.0f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float buzz = (s->ph[0] < 0.5f ? 1.0f : -1.0f) * 0.06f * u;
        x = (svf_band(nz(&s->rng), 2600.0f, 0.3f, &s->lo1, &s->bp1) * s->b * 0.9f + buzz) * fminf(1.0f, (s->dur - t) / 0.05f);
        break;
    }
    case SFX_RUMBLE: {                              /* the cart: a low rolling rumble growing, the wheels clacking on the joins */
        float u = t / s->dur, w = fminf(1.0f, u * 2.5f) * fminf(1.0f, (s->dur - t) / 0.2f);
        s->lp1 += 0.02f * (nz(&s->rng) - s->lp1);
        s->ph[0] += 38.0f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float clack = fmodf(t * (5.0f + 6.0f * u), 1.0f), k = clack < 0.02f ? svf_band(nz(&s->rng), 1800.0f, 0.4f, &s->lo2, &s->bp2) : 0.0f;
        x = (s->lp1 * 3.2f + sin_cyc(s->ph[0]) * 0.25f + k * 0.6f) * w * 0.7f;
        break;
    }
    case SFX_HUM: {                                 /* a crystal singing: a glassy tone sliding up, beating, growing bright */
        float u = t / s->dur, f = 520.0f * (1.0f + 0.9f * u * u);
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        s->ph[1] += f * 2.003f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
        s->ph[2] += f * 3.01f / SR; if (s->ph[2] >= 1.0f) s->ph[2] -= 1.0f;
        float e = fminf(1.0f, t / 0.1f) * fminf(1.0f, (s->dur - t) / 0.08f);
        x = (sin_cyc(s->ph[0]) * 0.5f + sin_cyc(s->ph[1]) * (0.15f + 0.3f * u) + sin_cyc(s->ph[2]) * 0.15f * u) * e * 0.3f;
        break;
    }
    case SFX_RUSTLE: {                              /* a hand among dry things: crinkling grains, and a scrape under them */
        float u = t / s->dur;
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) { s->a = 0.012f + 0.05f * (nz(&s->rng) * 0.5f + 0.5f); s->b = 0.4f + 0.6f * (nz(&s->rng) * 0.5f + 0.5f); }
        s->b *= expf(-1.0f / (SR * 0.006f));
        float grain = svf_band(nz(&s->rng), 3400.0f, 0.35f, &s->lo1, &s->bp1) * s->b;
        s->lp1 += 0.05f * (nz(&s->rng) - s->lp1);
        float scrape = s->lp1 * (0.5f + 0.5f * sinf(t * 23.0f)) * sinf(PI_F * u);
        x = (grain * 0.8f + scrape * 1.4f) * fminf(1.0f, (s->dur - t) / 0.1f) * fminf(1.0f, t / 0.02f);
        break;
    }
    case SFX_PAGE: {                                /* a page: a quick rising flick of paper, and the soft slap of it settling */
        float f = 1400.0f + 4200.0f * fminf(1.0f, t / 0.12f);
        float flick = svf_band(nz(&s->rng), f, 0.3f, &s->lo1, &s->bp1) * sinf(PI_F * fminf(1.0f, t / 0.16f)) * (t < 0.16f);
        float u = t - 0.2f, slap = u > 0.0f ? svf_band(nz(&s->rng), 900.0f, 0.5f, &s->lo2, &s->bp2) * expf(-u * 40.0f) : 0.0f;
        x = flick * 0.7f + slap * 0.9f;
        break;
    }
    case SFX_SPLASH: {                              /* a foot in shallow water: the slap going in, the slosh as it lifts, a few drops falling back */
        if (t == 0.0f) s->a = 0.85f + 0.3f * (nz(&s->rng) * 0.5f + 0.5f);
        float slap = svf_band(nz(&s->rng), 1500.0f * s->a, 0.6f, &s->lo1, &s->bp1) * expf(-t * 34.0f) * fminf(1.0f, t / 0.002f);
        float u = t - 0.08f, slosh = 0.0f;
        if (u > 0.0f) slosh = svf_band(nz(&s->rng), 480.0f * s->a + 380.0f * expf(-u * 9.0f), 0.22f, &s->lp1, &s->lp2) * expf(-u * 13.0f) * fminf(1.0f, u / 0.025f);
        s->d -= 1.0f / SR;
        if (s->d <= 0.0f && t < 0.28f) { s->d = 0.035f + 0.06f * (nz(&s->rng) * 0.5f + 0.5f); s->c = 0.0f; s->b = 1400.0f + 1100.0f * (nz(&s->rng) * 0.5f + 0.5f); }
        s->c += 1.0f / SR;
        float drop = 0.0f;
        if (t > 0.06f && s->c < 0.025f) {
            s->ph[0] += s->b * (1.0f + 1.6f * s->c / 0.025f) / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            drop = sin_cyc(s->ph[0]) * expf(-s->c * 110.0f) * 0.22f;
        }
        x = slap * 2.3f + slosh * 1.1f + drop * 0.8f;
        break;
    }
    case SFX_DRIP: {
        if (t == 0.0f) s->a = 1200.0f + 700.0f * (nz(&s->rng) * 0.5f + 0.5f);
        float f = s->a * (1.0f + 0.5f * expf(-t * 70.0f));
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        x = sin_cyc(s->ph[0]) * expf(-t * 14.0f) * 0.25f;
        break;
    }
    case SFX_MOAN: {                                /* low "oooh" through two formants */
        float u = t / s->dur;
        float w = sinf(PI_F * u); w *= w;
        float f = 98.0f * (1.0f + 0.02f * sinf(t * 5.0f)) * (1.0f - 0.08f * u);
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float saw = s->ph[0] * 2.0f - 1.0f;
        float b1 = svf_band(saw, 320.0f, 0.2f, &s->lo1, &s->bp1);
        float b2 = svf_band(saw, 760.0f, 0.25f, &s->lo2, &s->bp2);
        x = (b1 * 0.7f + b2 * 0.35f) * w * 0.6f;
        break;
    }
    case SFX_COIN: {                                /* two bright chimes, a fifth apart */
        float u = t < 0.07f ? t : t - 0.07f, f = t < 0.07f ? 1318.5f : 1975.5f;
        float e = expf(-u * (t < 0.07f ? 30.0f : 7.0f));
        x = (sin_cyc(f * u) * 0.6f + sin_cyc(f * 2.0f * u) * 0.25f + sin_cyc(f * 3.01f * u) * 0.12f) * e * 0.5f;
        break;
    }
    case SFX_KEYGET: {                              /* metal jingle, then three rising chimes */
        if (t < 0.3f) {
            s->a -= 1.0f / SR;
            if (s->a <= 0.0f) { s->a = 0.035f + 0.03f * (nz(&s->rng) * 0.5f + 0.5f); s->b = 1.0f; s->ph[4] = 0.0f; }
            s->b *= expf(-1.0f / (SR * 0.02f));
            s->ph[4] += 1.0f / SR;
            float tt = s->ph[4];
            x = (sin_cyc(3150.0f * tt) + sin_cyc(4420.0f * tt) * 0.7f + sin_cyc(5980.0f * tt) * 0.5f) * s->b * 0.12f;
        }
        static const float N[3] = { 784.0f, 987.8f, 1568.0f };
        for (int i = 0; i < 3; i++) {
            float u = t - 0.25f - i * 0.11f;
            if (u > 0.0f) x += (sin_cyc(N[i] * u) * 0.5f + sin_cyc(N[i] * 2.0f * u) * 0.2f) * expf(-u * 5.0f) * 0.3f;
        }
        break;
    }
    case SFX_UNLOCK: {                              /* a heavy clunk, a ring, a chain falling */
        float clunk = sin_cyc(70.0f * t) * expf(-t * 22.0f) * 0.8f + nz(&s->rng) * expf(-t * 160.0f) * 0.5f;
        float ring = (sin_cyc(612.0f * t) + sin_cyc(1489.0f * t) * 0.5f) * expf(-t * 9.0f) * 0.15f;
        float chain = 0.0f;
        if (t > 0.25f && t < 0.9f) {
            s->a -= 1.0f / SR;
            if (s->a <= 0.0f) { s->a = 0.02f + 0.05f * (nz(&s->rng) * 0.5f + 0.5f); s->b = 1.0f - (t - 0.25f) / 0.7f; s->ph[4] = 0.0f; }
            s->b *= expf(-1.0f / (SR * 0.015f));
            s->ph[4] += 1.0f / SR;
            chain = (sin_cyc(2210.0f * s->ph[4]) + sin_cyc(3330.0f * s->ph[4]) * 0.6f) * s->b * 0.18f;
        }
        x = clunk + ring + chain;
        break;
    }
    case SFX_HURT: {                                /* body blow and a short grunt */
        float thud = sin_cyc((60.0f + 60.0f * expf(-t * 30.0f)) * t) * expf(-t * 18.0f) * 0.8f;
        float u = t - 0.03f, grunt = 0.0f;
        if (u > 0.0f && u < 0.28f) {
            float f = 135.0f * (1.0f - 0.25f * u / 0.28f);
            s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            float saw = s->ph[0] * 2.0f - 1.0f + nz(&s->rng) * 0.3f;
            float b1 = svf_band(saw, 560.0f, 0.25f, &s->lo1, &s->bp1), b2 = svf_band(saw, 1100.0f, 0.3f, &s->lo2, &s->bp2);
            grunt = (b1 * 0.7f + b2 * 0.3f) * sinf(PI_F * u / 0.28f) * 0.7f;
        }
        x = thud + grunt;
        break;
    }
    case SFX_THROW: {
        float u = t / s->dur, amp = sinf(PI_F * u);
        x = svf_band(nz(&s->rng), 700.0f + 1600.0f * u, 0.45f, &s->lo1, &s->bp1) * amp * amp * 0.6f;
        break;
    }
    case SFX_DEATH: {                               /* a long falling groan, then the body hits the floor */
        float u = t / 1.3f, groan = 0.0f;
        if (u < 1.0f) {
            float f = 160.0f * (1.0f - 0.5f * u) * (1.0f + 0.02f * sinf(t * 31.0f));
            s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            float saw = s->ph[0] * 2.0f - 1.0f;
            float b1 = svf_band(saw, 650.0f - 300.0f * u, 0.22f, &s->lo1, &s->bp1), b2 = svf_band(saw, 1050.0f - 300.0f * u, 0.25f, &s->lo2, &s->bp2);
            groan = (b1 * 0.7f + b2 * 0.35f) * sinf(PI_F * fminf(u * 1.4f, 1.0f)) * 0.75f;
        }
        float v = t - 1.15f, thud = 0.0f;
        if (v > 0.0f) { s->lp1 += 0.08f * (nz(&s->rng) - s->lp1); thud = (sin_cyc(52.0f * v) * 0.9f + s->lp1 * 1.2f) * expf(-v * 9.0f); }
        x = groan + thud;
        break;
    }
    case SFX_POTION: {                              /* three glugs and a rising shimmer */
        for (int i = 0; i < 3; i++) {
            float u = t - i * 0.13f;
            if (u > 0.0f && u < 0.12f) x += sin_cyc((220.0f + 900.0f * u) * u) * sinf(PI_F * u / 0.12f) * 0.45f;
        }
        float v = t - 0.38f;
        if (v > 0.0f) {
            float w = sinf(PI_F * fminf(v / 0.7f, 1.0f));
            x += (sin_cyc((880.0f + 900.0f * v) * v) * 0.3f + sin_cyc((1320.0f + 1350.0f * v) * v) * 0.18f) * w * 0.4f;
        }
        break;
    }
    case SFX_GROAN: {                               /* a rough, wet "uuurrgh" */
        float u = t / s->dur, w = sinf(PI_F * u); w = sqrtf(fmaxf(w, 0.0f));
        s->c -= 1.0f / SR;
        if (s->c <= 0.0f) { s->c = 0.008f + 0.02f * (nz(&s->rng) * 0.5f + 0.5f); s->d = 0.6f + 0.4f * (nz(&s->rng) * 0.5f + 0.5f); }
        float f = (78.0f + 18.0f * sinf(t * 2.1f) - 14.0f * u) * (1.0f + 0.05f * nz(&s->rng));
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float saw = (s->ph[0] * 2.0f - 1.0f) * s->d + nz(&s->rng) * 0.25f;
        float b1 = svf_band(saw, 380.0f + 140.0f * sinf(t * 1.3f), 0.2f, &s->lo1, &s->bp1);
        float b2 = svf_band(saw, 820.0f - 200.0f * u, 0.25f, &s->lo2, &s->bp2);
        x = (b1 * 0.8f + b2 * 0.4f) * w * 0.6f;
        break;
    }
    case SFX_DIRT: {                                /* soil cracking, then clods falling */
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) { s->a = 0.01f + 0.06f * (nz(&s->rng) * 0.5f + 0.5f) * (0.4f + t); s->b = (0.4f + 0.6f * (nz(&s->rng) * 0.5f + 0.5f)) * (1.0f - t / s->dur); }
        s->b *= expf(-1.0f / (SR * 0.03f));
        s->lp1 += 0.06f * (nz(&s->rng) - s->lp1);
        s->lp2 += 0.01f * (nz(&s->rng) - s->lp2);
        x = s->lp1 * s->b * 2.2f + s->lp2 * expf(-t * 3.0f) * 1.5f;
        break;
    }
    case SFX_XYLO: {                                /* bone xylophone: a quick run up and back */
        static const float N[8] = { 587.3f, 698.5f, 880.0f, 1174.7f, 1046.5f, 880.0f, 698.5f, 587.3f };
        for (int i = 0; i < 8; i++) {
            float u = t - i * 0.11f;
            if (u <= 0.0f || u > 0.6f) continue;
            float e = expf(-u * 16.0f);
            x += (sin_cyc(N[i] * u) * 0.6f + sin_cyc(N[i] * 3.93f * u) * 0.25f * expf(-u * 60.0f)) * e * 0.35f;
            if (u < 0.004f) x += nz(&s->rng) * 0.2f;
        }
        break;
    }
    case SFX_WAIL: {                                /* rises, hangs, falls away */
        float u = t / s->dur;
        float f = 520.0f + 420.0f * sinf(PI_F * u) - 160.0f * u;
        f *= 1.0f + 0.025f * sinf(t * 33.0f);
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        s->ph[1] += f * 1.5f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
        float amp = sinf(PI_F * u); amp *= amp;
        float breath = svf_band(nz(&s->rng), f * 2.0f, 0.25f, &s->lo1, &s->bp1);
        x = (sin_cyc(s->ph[0]) * 0.45f + sin_cyc(s->ph[1]) * 0.1f + breath * 0.25f) * amp * 0.7f;
        break;
    }
    case SFX_CAW: {                                 /* two hoarse caws */
        float u = t < 0.32f ? t : t - 0.42f;
        if (u > 0.0f && u < 0.3f) {
            float f = 520.0f * (1.0f - 0.2f * u / 0.3f);
            s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            float src = (s->ph[0] * 2.0f - 1.0f) * 0.6f + nz(&s->rng) * 0.6f;
            float b1 = svf_band(src, 1350.0f, 0.2f, &s->lo1, &s->bp1), b2 = svf_band(src, 2500.0f, 0.25f, &s->lo2, &s->bp2);
            float env = sinf(PI_F * u / 0.3f);
            x = (b1 * 0.7f + b2 * 0.4f) * sqrtf(fmaxf(env, 0.0f)) * 0.5f;
        }
        break;
    }
    case SFX_SHRIEK: {                              /* a high scream that tears and falls away */
        float u = t / s->dur;
        float f = (1350.0f - 700.0f * u * u) * (1.0f + 0.04f * sinf(t * 47.0f));
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        s->ph[1] += f * 1.51f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
        float n = nz(&s->rng);
        float b1 = svf_band(n + sin_cyc(s->ph[0]) * 0.6f, f * 2.1f, 0.18f, &s->lo1, &s->bp1);
        float b2 = svf_band(n, 3200.0f - 1400.0f * u, 0.25f, &s->lo2, &s->bp2);
        float amp = fminf(1.0f, t / 0.025f) * (1.0f - u) * (1.0f - u * 0.5f);
        x = (sin_cyc(s->ph[0]) * 0.35f + sin_cyc(s->ph[1]) * 0.12f + b1 * 0.5f + b2 * 0.3f) * amp * 0.7f;
        break;
    }
    case SFX_IGNITE: {                              /* a match struck, then the flame takes with a soft whump */
        float n = nz(&s->rng);
        if (t < 0.09f) x = svf_band(n, 3000.0f + 2000.0f * t / 0.09f, 0.3f, &s->lo1, &s->bp1) * (0.6f + 0.4f * nz(&s->rng)) * 0.9f;
        float u = t - 0.07f;
        if (u > 0.0f) {
            s->lp2 += 0.03f * (n - s->lp2);
            float env = (1.0f - expf(-u * 30.0f)) * expf(-u * 4.5f);
            x += s->lp2 * env * 4.0f + svf_band(n, 5200.0f, 0.4f, &s->lo2, &s->bp2) * expf(-u * 9.0f) * 0.25f;
        }
        break;
    }
    case SFX_WIND: {                                /* rushing noise with a hollow whistle in it */
        float u = t / s->dur, w = sinf(PI_F * u); w = w * w * (0.8f + 0.2f * sinf(t * 7.0f));
        float n = nz(&s->rng);
        float fc = 380.0f + 520.0f * sinf(PI_F * u) + 120.0f * sinf(t * 3.1f);
        float b = svf_band(n, fc, 0.35f, &s->lo1, &s->bp1);
        float wh = svf_band(n, fc * 2.6f, 0.05f, &s->lo2, &s->bp2);
        x = (b * 0.9f + wh * 0.06f) * w * 0.8f;
        break;
    }
    case SFX_CACKLE: {                              /* "heh-heh-heh-HAA-ha-aah", high and nasal */
        static const float ON[6] = { 0.00f, 0.17f, 0.34f, 0.52f, 0.82f, 1.06f }, LEN[6] = { 0.12f, 0.12f, 0.13f, 0.25f, 0.18f, 0.34f };
        static const float P[6] = { 330.0f, 345.0f, 360.0f, 430.0f, 360.0f, 300.0f };
        int i = -1;
        for (int k = 0; k < 6; k++) if (t >= ON[k] && t < ON[k] + LEN[k]) i = k;
        if (i >= 0) {
            float u = (t - ON[i]) / LEN[i];
            float f = P[i] * (1.0f - 0.18f * u) * (1.0f + 0.03f * nz(&s->rng));
            s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            float src = (s->ph[0] * 2.0f - 1.0f) + nz(&s->rng) * 0.35f;
            float ah = (i == 3 || i == 5) ? 1.0f : 0.0f;
            float b1 = svf_band(src, 560.0f + 260.0f * ah, 0.18f, &s->lo1, &s->bp1);
            float b2 = svf_band(src, 1950.0f - 700.0f * ah, 0.15f, &s->lo2, &s->bp2);
            float env = sqrtf(fmaxf(sinf(PI_F * u), 0.0f));
            x = (b1 * 0.55f + b2 * 0.45f) * env * (i == 3 ? 0.7f : 0.5f);
        }
        break;
    }
    case SFX_CHATTER: {                             /* dry clicks, fast and uneven */
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) { s->a = 0.055f + 0.03f * (nz(&s->rng) * 0.5f + 0.5f); s->b = 1.0f; s->c = 1500.0f + 900.0f * (nz(&s->rng) * 0.5f + 0.5f); }
        s->b *= expf(-1.0f / (SR * 0.004f));
        x = svf_band(nz(&s->rng), s->c, 0.12f, &s->lo1, &s->bp1) * s->b * 0.9f * fminf(1.0f, (s->dur - t) / 0.2f);
        break;
    }
    case SFX_KNOCK: {                               /* knuckles on an old table: a hollow thock */
        s->ph[0] += (190.0f - 60.0f * fminf(t / 0.03f, 1.0f)) / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        s->ph[1] += 523.0f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
        float n = svf_band(nz(&s->rng), 1800.0f, 0.5f, &s->lo1, &s->bp1);
        x = sin_cyc(s->ph[0]) * expf(-t * 38.0f) * 0.9f + sin_cyc(s->ph[1]) * expf(-t * 60.0f) * 0.25f + n * expf(-t * 300.0f) * 0.6f;
        break;
    }
    case SFX_DIAL: {                                /* dial tone, then 5-5-5-0-6-6-6 */
        static const float LO[7] = { 770, 770, 770, 941, 770, 770, 770 }, HI[7] = { 1336, 1336, 1336, 1336, 1477, 1477, 1477 };
        float fa = 0.0f, fb = 0.0f;
        if (t < 0.55f) { fa = 350.0f; fb = 440.0f; }
        else { int k = (int)((t - 0.6f) / 0.16f); float u = fmodf(t - 0.6f, 0.16f); if (t >= 0.6f && k < 7 && u < 0.1f) { fa = LO[k]; fb = HI[k]; } }
        if (fa > 0.0f) {
            s->ph[0] += fa / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            s->ph[1] += fb / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
            x = (sin_cyc(s->ph[0]) + sin_cyc(s->ph[1])) * 0.22f;
        }
        s->lp1 += 0.35f * (x - s->lp1); x = s->lp1;  /* the phone line's narrow band */
        break;
    }
    case SFX_MODEM: {                               /* answer tone, the dee-dee, the bongs, then the hiss of training */
        float f1 = 0.0f, f2 = 0.0f, hiss = 0.0f;
        if (t < 0.9f) f1 = 2100.0f;
        else if (t < 1.4f) { f1 = ((int)(t * 30.0f) & 1) ? 980.0f : 1180.0f; f2 = 1650.0f; }
        else if (t < 1.9f) { f1 = 1800.0f; f2 = 600.0f; }
        else hiss = fminf(1.0f, (s->dur - t) / 0.4f) * (0.7f + 0.3f * sinf(t * 47.0f));
        if (t > 0.45f && t < 0.46f) s->ph[0] += 0.5f;   /* the answer tone's phase reversal clicks */
        s->ph[0] += f1 / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        s->ph[1] += f2 / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
        x = (f1 > 0.0f ? sin_cyc(s->ph[0]) * 0.3f : 0.0f) + (f2 > 0.0f ? sin_cyc(s->ph[1]) * 0.18f : 0.0f);
        if (hiss > 0.0f) x += svf_band(nz(&s->rng), 1600.0f + 900.0f * sinf(t * 13.0f), 0.6f, &s->lo1, &s->bp1) * hiss * 0.55f;
        break;
    }
    case SFX_SHATTER: {                             /* a crack, then pieces falling */
        float n = nz(&s->rng);
        float crack = svf_band(n, 2600.0f, 0.3f, &s->lo1, &s->bp1) * expf(-t * 22.0f);
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) { s->a = 0.012f + 0.05f * (nz(&s->rng) * 0.5f + 0.5f) * (0.3f + t); s->b = 0.7f * (1.0f - t / s->dur); s->c = 1800.0f + 2400.0f * (nz(&s->rng) * 0.5f + 0.5f); }
        s->b *= expf(-1.0f / (SR * 0.006f));
        x = crack * 1.1f + svf_band(n, s->c, 0.15f, &s->lo2, &s->bp2) * s->b * 0.8f;
        break;
    }
    case SFX_SKITTER: {                             /* eight legs on stone, and a hiss */
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) { s->a = 0.018f + 0.02f * (nz(&s->rng) * 0.5f + 0.5f); s->b = 1.0f; }
        s->b *= expf(-1.0f / (SR * 0.002f));
        float n = nz(&s->rng);
        float hiss = svf_band(n, 5200.0f, 0.5f, &s->lo2, &s->bp2) * sinf(PI_F * fminf(t / 0.5f, 1.0f)) * (t < 0.5f ? 0.35f : 0.0f);
        x = svf_band(n, 3400.0f, 0.2f, &s->lo1, &s->bp1) * s->b * 0.9f + hiss;
        break;
    }
    case SFX_SCREAM: {                              /* a ragged, falling howl from a dry throat */
        float u = t / s->dur;
        float f = (330.0f + 260.0f * sinf(PI_F * fminf(u * 1.6f, 1.0f)) - 160.0f * u) * (1.0f + 0.06f * nz(&s->rng));
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float src = (s->ph[0] * 2.0f - 1.0f) * 0.8f + nz(&s->rng) * 0.6f;
        float b1 = svf_band(src, 900.0f - 300.0f * u, 0.2f, &s->lo1, &s->bp1), b2 = svf_band(src, 2400.0f, 0.25f, &s->lo2, &s->bp2);
        float amp = fminf(1.0f, t / 0.05f) * (1.0f - u) * (0.75f + 0.25f * sinf(t * 37.0f));
        x = (b1 * 0.7f + b2 * 0.35f) * amp * 0.75f;
        break;
    }
    case SFX_SWORD: {                               /* a swish, and the blade ringing */
        float u = t / 0.25f, n = nz(&s->rng);
        float swish = t < 0.25f ? svf_band(n, 900.0f + 2200.0f * u, 0.4f, &s->lo1, &s->bp1) * sinf(PI_F * u) : 0.0f;
        s->ph[0] += 2350.0f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        s->ph[1] += 3910.0f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
        float ring = t > 0.12f ? (sin_cyc(s->ph[0]) + 0.5f * sin_cyc(s->ph[1])) * expf(-(t - 0.12f) * 9.0f) * 0.12f : 0.0f;
        x = swish * 0.8f + ring;
        break;
    }
    case SFX_IMPLODE: {                             /* the air drawn in, rising, then a deep thud and a ringing */
        float n = nz(&s->rng);
        if (t < 0.8f) {
            float u = t / 0.8f;
            x = svf_band(n, 200.0f + 2600.0f * u * u, 0.5f, &s->lo1, &s->bp1) * u * u * 1.2f;
            s->ph[0] += (40.0f + 140.0f * u) / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            x += sin_cyc(s->ph[0]) * u * 0.5f;
        } else {
            float v = t - 0.8f;
            s->ph[0] += (55.0f * expf(-v * 2.0f) + 30.0f) / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
            s->ph[1] += 1760.0f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
            x = sin_cyc(s->ph[0]) * expf(-v * 3.0f) * 1.1f + sin_cyc(s->ph[1]) * expf(-v * 2.5f) * 0.12f + n * expf(-v * 30.0f) * 0.5f;
        }
        break;
    }
    case SFX_FROST: {                               /* glassy crackle */
        s->a -= 1.0f / SR;
        if (s->a <= 0.0f) { s->a = 0.006f + 0.03f * (nz(&s->rng) * 0.5f + 0.5f); s->b = 1.0f - t / s->dur; s->c = 3000.0f + 4000.0f * (nz(&s->rng) * 0.5f + 0.5f); }
        s->b *= expf(-1.0f / (SR * 0.004f));
        s->ph[0] += 2637.0f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        x = svf_band(nz(&s->rng), s->c, 0.1f, &s->lo1, &s->bp1) * s->b * 0.6f + sin_cyc(s->ph[0]) * expf(-t * 8.0f) * 0.1f;
        break;
    }
    case SFX_RUNE: {                                /* a soft bell and a breath of shimmer */
        s->ph[0] += 1046.5f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        s->ph[1] += 1568.0f / SR; if (s->ph[1] >= 1.0f) s->ph[1] -= 1.0f;
        float e = fminf(1.0f, t / 0.01f) * expf(-t * 5.0f);
        x = (sin_cyc(s->ph[0]) * 0.3f + sin_cyc(s->ph[1]) * 0.18f) * e + svf_band(nz(&s->rng), 6000.0f, 0.5f, &s->lo1, &s->bp1) * e * 0.1f;
        break;
    }
    case SFX_PURR: {                                /* about 25 rumbles a second, louder breathing out than in */
        float breath = 0.55f + 0.45f * sinf(TAU_F * t / 1.0f);
        s->ph[0] += 26.0f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float pulse = powf(fmaxf(0.0f, sinf(TAU_F * s->ph[0])), 3.0f);
        float n = nz(&s->rng);                      /* a low rumble, and enough rasp to carry on small speakers */
        float rum = svf_band(n, 160.0f, 0.5f, &s->lo1, &s->bp1) + 0.55f * svf_band(n, 520.0f, 0.35f, &s->lo2, &s->bp2);
        float edge = fminf(1.0f, fminf(t / 0.15f, (s->dur - t) / 0.15f));
        x = rum * pulse * breath * edge * 1.6f;
        break;
    }
    case SFX_TRILL: {                               /* "mrrp?" */
        float u = t / s->dur;
        float f = 420.0f + 380.0f * u;
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        float roll = 0.6f + 0.4f * sinf(TAU_F * 32.0f * t);
        float src = (s->ph[0] < 0.4f ? 1.0f : -0.6f) + nz(&s->rng) * 0.1f;
        float b = svf_band(src, 900.0f + 500.0f * u, 0.25f, &s->lo1, &s->bp1);
        x = b * roll * sinf(PI_F * u) * 0.5f;
        break;
    }
    case SFX_PUSHWALL: {                            /* a long grind with catches in it, a low rumble, then a thud */
        float w = fminf(1.0f, t / 0.12f) * fminf(1.0f, fmaxf(0.0f, 2.35f - t) / 0.2f);
        s->lp1 += 0.04f * (nz(&s->rng) - s->lp1);
        s->lp2 += 0.25f * (nz(&s->rng) - s->lp2);
        float catches = 0.55f + 0.45f * sinf(t * 37.0f) * sinf(t * 7.3f + 1.0f);
        float grit = (s->lp2 - s->lp1) * (sinf(t * 53.0f) > 0.6f ? 0.9f : 0.2f);
        float rumble = sin_cyc(33.0f * t) * 0.3f + sin_cyc(47.0f * t) * 0.15f;
        float thud = t > 2.38f ? (sin_cyc(52.0f * (t - 2.38f)) * 0.9f + nz(&s->rng) * 0.3f * expf(-(t - 2.38f) * 60.0f)) * expf(-(t - 2.38f) * 9.0f) : 0.0f;
        x = (s->lp1 * 2.8f * catches + grit * 0.6f + rumble) * w * 0.55f + thud;
        break;
    }
    case SFX_SECRET: {                              /* knock-knock on hollow stone, then four bells rising */
        float knock = 0.0f;
        for (int i = 0; i < 2; i++) {
            float u = t - i * 0.13f;
            if (u >= 0.0f && u < 0.12f) knock += (sin_cyc(140.0f * u) * 0.7f + nz(&s->rng) * 0.4f * expf(-u * 200.0f)) * expf(-u * 35.0f);
        }
        static const float N[4] = { 587.3f, 698.5f, 880.0f, 1174.7f };   /* D, F, A, high D */
        float bells = 0.0f;
        for (int i = 0; i < 4; i++) {
            float u = t - 0.32f - i * 0.12f;
            if (u > 0.0f) bells += (sin_cyc(N[i] * u) * 0.5f + sin_cyc(N[i] * 2.76f * u) * 0.18f + sin_cyc(N[i] * 5.4f * u) * 0.06f) * expf(-u * 2.6f);
        }
        x = knock * 0.6f + bells * 0.22f;
        break;
    }
    case X_KICK: {
        float f = 46.0f + 130.0f * expf(-t * 32.0f);
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        x = sin_cyc(s->ph[0]) * expf(-t * 9.0f) * 0.80f + nz(&s->rng) * expf(-t * 600.0f) * 0.22f;
        break;
    }
    case X_SNARE: {
        s->lp1 += 0.7f * (nz(&s->rng) - s->lp1);
        float n = nz(&s->rng) - s->lp1 * 0.6f;
        x = n * expf(-t * 20.0f) * 0.55f + sin_cyc(185.0f * t) * expf(-t * 28.0f) * 0.5f;
        break;
    }
    case X_HAT_C: case X_HAT_O: {
        float n = nz(&s->rng);
        float hp = n - s->lp1; s->lp1 = n;                 /* crude high-pass */
        x = hp * expf(-t * (s->type == X_HAT_C ? 85.0f : 16.0f)) * 0.30f;
        break;
    }
    case X_CRASH: {
        float n = nz(&s->rng);
        float hp = n - s->lp1; s->lp1 = n;
        x = hp * expf(-t * 2.6f) * 0.28f;
        break;
    }
    case X_TOM: {
        float f = 95.0f + 90.0f * expf(-t * 16.0f);
        s->ph[0] += f / SR; if (s->ph[0] >= 1.0f) s->ph[0] -= 1.0f;
        x = sin_cyc(s->ph[0]) * expf(-t * 10.0f) * 0.7f;
        break;
    }
    default: break;
    }
    s->t += 1.0f / SR;
    if (s->t >= s->dur) s->active = 0;
    return x * s->gain;
}

/* ------------------------------------------------------------------ */
/* Effects: Freeverb-style reverb, ping-pong delay, wind bed           */
/* ------------------------------------------------------------------ */

#define NCOMB 8
#define NAP 4
static const int COMB_LEN[NCOMB] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
static const int AP_LEN[NAP] = { 556, 441, 341, 225 };
#define SPREAD 23

typedef struct Reverb {
    float comb[2][NCOMB][1700 + SPREAD];
    float lpf[2][NCOMB];
    int   cpos[2][NCOMB];
    float ap[2][NAP][556 + SPREAD];
    int   apos[2][NAP];
} Reverb;

static Reverb *g_rv;
static float g_rv_fb = 0.82f, g_rv_damp = 0.25f, g_rv_wet = 0.30f;

#define DLY_MAX (SND_RATE)
static float g_dl[DLY_MAX], g_dr[DLY_MAX];
static int g_dpos, g_dlen = 14000;
static float g_dly_fb = 0.38f, g_dly_wet = 0.5f;

static void reverb_tick(float in, float *outl, float *outr) {
    Reverb *r = g_rv;
    float o[2];
    for (int ch = 0; ch < 2; ch++) {
        float acc = 0.0f;
        for (int i = 0; i < NCOMB; i++) {
            int len = COMB_LEN[i] + ch * SPREAD;
            float *buf = r->comb[ch][i];
            int p = r->cpos[ch][i];
            float y = buf[p];
            r->lpf[ch][i] = y * (1.0f - g_rv_damp) + r->lpf[ch][i] * g_rv_damp;
            buf[p] = in * 0.015f + r->lpf[ch][i] * g_rv_fb;
            if (++p >= len) p = 0;
            r->cpos[ch][i] = p;
            acc += y;
        }
        for (int i = 0; i < NAP; i++) {
            int len = AP_LEN[i] + ch * SPREAD;
            float *buf = r->ap[ch][i];
            int p = r->apos[ch][i];
            float b = buf[p];
            float y = -acc + b;
            buf[p] = acc + b * 0.5f;
            if (++p >= len) p = 0;
            r->apos[ch][i] = p;
            acc = y;
        }
        o[ch] = acc;
    }
    *outl = o[0]; *outr = o[1];
}

/* ------------------------------------------------------------------ */
/* Global state                                                        */
/* ------------------------------------------------------------------ */

static int    g_prog;
static double g_samples;            /* samples rendered since program start */
static double g_step_acc;           /* samples into the current step */
static double g_sps = 11025.0;      /* samples per 16th */
static uint32_t g_step;
static int    g_latency;

static float g_master = 0.7f, g_mg = 0.0f, g_mg_target = 1.0f, g_mg_slope = 1.0f / SND_RATE;
static float g_mood = 0.0f, g_mood_target = 0.0f;
static float g_wind_lp1, g_wind_lp2, g_wind_ph;
static float g_dcx[2], g_dcy[2];       /* DC-blocker state */
static float g_crk_env, g_crk_lp, g_crk_hiss;   /* torch crackle */
static float g_hb_timer;
static uint32_t g_mrng = 777;       /* music randomness */
static Voice *g_drone[3];
static int g_drone_on;

/* ------------------------------------------------------------------ */
/* HAUNT music: generative dark ambience in D harmonic minor           */
/* ------------------------------------------------------------------ */

static const uint8_t HAUNT_SCALE[] = { 74, 76, 77, 79, 81, 82, 85, 86, 88, 89, 69, 70, 73 };
static const uint8_t HAUNT_WEIGHT[] = { 5, 2, 3, 2, 5, 2, 3, 4, 2, 2, 3, 1, 2 };
static const uint8_t LULLABY[] = { 69, 74, 77, 76, 74, 77, 81, 79, 77, 76, 74, 73, 74, 0, 0, 0 };

/* Delayed notes (the lullaby needs spaced onsets between sequencer steps). */
typedef struct Sched { int when; int midi; float vel; int pat; int active; int gate; int bus; } Sched;
#define NSCHED 32
static Sched g_sched[NSCHED];
static void sched_note_g(int delay_samples, int pat, int midi, float vel, int gate) {   /* gate 0: two steps */
    for (int i = 0; i < NSCHED; i++) if (!g_sched[i].active) {
        g_sched[i].when = delay_samples; g_sched[i].midi = midi;
        g_sched[i].vel = vel; g_sched[i].pat = pat; g_sched[i].active = 1; g_sched[i].gate = gate;
        g_sched[i].bus = g_vbus;
        return;
    }
}
static void sched_note(int delay_samples, int pat, int midi, float vel) { sched_note_g(delay_samples, pat, midi, vel, 0); }
static void sched_run(void) {
    for (int i = 0; i < NSCHED; i++) if (g_sched[i].active && --g_sched[i].when <= 0) {
        g_sched[i].active = 0;
        int was = g_vbus;
        g_vbus = g_sched[i].bus;
        voice_on(g_sched[i].pat, g_sched[i].midi, g_sched[i].vel, 0.55f, g_sched[i].gate ? g_sched[i].gate : (int)(g_sps * 2));
        g_vbus = was;
    }
}

static int haunt_pick_note(void) {
    int total = 0;
    for (unsigned i = 0; i < sizeof HAUNT_WEIGHT; i++) total += HAUNT_WEIGHT[i];
    int r = (int)(rng_next(&g_mrng) % (uint32_t)total);
    for (unsigned i = 0; i < sizeof HAUNT_WEIGHT; i++) {
        r -= HAUNT_WEIGHT[i];
        if (r < 0) return HAUNT_SCALE[i];
    }
    return 74;
}

static void haunt_step(uint32_t step) {
    if (step == 0) {
        g_drone[0] = voice_on(PAT_DRONE, 38, 1.0f, 0.35f, -1);
        g_drone[1] = voice_on(PAT_DRONE, 45, 0.8f, 0.65f, -1);
        g_drone[2] = voice_on(PAT_DRONE, 50, 0.45f, 0.5f, -1);
        for (int i = 0; i < 3; i++) { g_drone[i]->gainch = 1; g_drone[i]->freq *= (i == 1) ? 1.002f : 1.0f; }
        g_drone_on = 1;
    }
    float mood = g_mood;
    /* sparse bells on the quarter-note grid */
    if ((step & 3) == 0) {
        float pbell = 0.08f + 0.20f * mood;
        if (rng_f(&g_mrng) < pbell) {
            int n = haunt_pick_note();
            float vel = 0.6f + 0.4f * rng_f(&g_mrng);
            voice_on(PAT_BELL, n, vel, 0.15f + 0.7f * rng_f(&g_mrng), 4 * (int)g_sps);
            if (rng_f(&g_mrng) < 0.05f + 0.10f * mood)               /* unsettling second note */
                voice_on(PAT_BELL, n + ((rng_next(&g_mrng) & 1) ? 6 : 1), vel * 0.6f, 0.5f, 4 * (int)g_sps);
        }
    }
    /* tritone swell every ~48 s */
    if (step % 192 == 96) {
        Voice *a = voice_on(PAT_PAD, 44, 1.0f, 0.3f, (int)(g_sps * 28));
        Voice *b = voice_on(PAT_PAD, 50, 0.9f, 0.7f, (int)(g_sps * 28));
        if (a) a->gainch = 1;
        if (b) b->gainch = 1;
    }
    /* music box lullaby about once a minute */
    if (step % 240 == 120 && step > 0 && rng_f(&g_mrng) < 0.75f) {
        for (int i = 0; i < 16 && LULLABY[i]; i++)
            sched_note((int)(i * 2 * g_sps), PAT_MBOX, LULLABY[i], 0.7f);
    }
}

/* ------------------------------------------------------------------ */
/* CRAWL music: 92 BPM, D minor, a 32-bar AdLib-style adventure march */
/* ------------------------------------------------------------------ */

typedef struct ChordDef { int8_t bass; int8_t minor; } ChordDef;
#define CH_DM { 38, 1 }
#define CH_BB { 34, 0 }
#define CH_C  { 36, 0 }
#define CH_GM { 43, 1 }
#define CH_A  { 33, 0 }
#define CH_F  { 41, 0 }
#define CH_EB { 39, 0 }
static const ChordDef CHORDS[32] = {
    CH_DM, CH_DM, CH_BB, CH_C,  CH_DM, CH_GM, CH_A,  CH_A,     /* A: march intro          */
    CH_DM, CH_F,  CH_C,  CH_DM, CH_BB, CH_GM, CH_A,  CH_A,     /* B: flute melody         */
    CH_GM, CH_DM, CH_A,  CH_DM, CH_BB, CH_F,  CH_GM, CH_A,     /* C: brass melody         */
    CH_DM, CH_EB, CH_DM, CH_EB, CH_BB, CH_GM, CH_A,  CH_A,     /* D: the crypt, then a roll */
};
#undef CH_DM
#undef CH_BB
#undef CH_C
#undef CH_GM
#undef CH_A
#undef CH_F
#undef CH_EB

typedef struct LeadNote { uint8_t bar, step, len, midi, pat; } LeadNote;
#define FL PAT_FLUTE
#define BR PAT_BRASS
static const LeadNote LEAD[] = {
    { 8,0,4,69,FL},{ 8,4,4,74,FL},{ 8,8,6,77,FL},{ 8,14,2,76,FL},
    { 9,0,4,77,FL},{ 9,4,4,81,FL},{ 9,8,4,79,FL},{ 9,12,4,77,FL},
    {10,0,6,76,FL},{10,6,2,74,FL},{10,8,4,72,FL},{10,12,4,76,FL},
    {11,0,12,74,FL},{11,12,4,69,FL},
    {12,0,4,70,FL},{12,4,4,74,FL},{12,8,8,77,FL},
    {13,0,4,79,FL},{13,4,4,77,FL},{13,8,4,74,FL},{13,12,4,70,FL},
    {14,0,4,73,FL},{14,4,4,76,FL},{14,8,8,81,FL},
    {15,0,4,79,FL},{15,4,4,77,FL},{15,8,4,76,FL},{15,12,4,73,FL},

    {16,0,6,86,BR},{16,6,2,84,BR},{16,8,4,82,BR},{16,12,4,79,BR},
    {17,0,6,81,BR},{17,6,2,79,BR},{17,8,4,77,BR},{17,12,4,74,BR},
    {18,0,4,76,BR},{18,4,4,81,BR},{18,8,8,85,BR},
    {19,0,12,86,BR},
    {20,0,4,82,BR},{20,4,4,86,BR},{20,8,4,84,BR},{20,12,4,82,BR},
    {21,0,4,81,BR},{21,4,4,84,BR},{21,8,4,81,BR},{21,12,4,77,BR},
    {22,0,4,79,BR},{22,4,4,82,BR},{22,8,4,81,BR},{22,12,4,79,BR},
    {23,0,4,81,BR},{23,4,4,76,BR},{23,8,4,73,BR},{23,12,4,76,BR},

    {28,0,8,74,FL},{28,8,8,77,FL},
    {29,0,8,79,FL},{29,8,8,82,FL},
    {30,0,16,81,FL},
    {31,0,8,85,FL},{31,8,8,81,FL},
};
#undef FL
#undef BR

enum { D_KICK = 1, D_SNARE = 2, D_HAT = 4, D_GHOST = 8, D_CRASH = 16, D_TOM = 32, D_ROLL = 64 };

static int crawl_drums(int sec, int s) {
    int m = 0;
    if (s == 0 && (sec == 0 || sec == 8 || sec == 16)) m |= D_CRASH;
    if (sec < 24) {                                   /* march */
        if (s == 0 || s == 8) m |= D_KICK;
        if (s == 4 || s == 12) m |= D_SNARE;
        if ((s & 1) == 0) m |= D_HAT;
        if ((sec & 1) && (s == 14 || s == 15)) m |= D_GHOST;
        if ((sec == 7 || sec == 15 || sec == 23) && s >= 8 && (s & 1) == 0) m |= D_TOM;
    } else if (sec < 28) {                            /* the crypt: timpani only */
        if (s == 0) m |= D_TOM;
        if (s == 8 && (sec & 1)) m |= D_TOM;
    } else if (sec < 30) {
        if (s == 0 || s == 8) m |= D_KICK;
        if (s == 4 || s == 12) m |= D_SNARE;
        if ((s & 3) == 2) m |= D_HAT;
    } else {                                          /* snare roll into the loop */
        if ((s & 1) == 0 || sec == 31) m |= D_ROLL;
        if (s == 0 || s == 8) m |= D_KICK | D_TOM;
    }
    return m;
}

static const uint8_t ARP8[8] = { 0, 1, 2, 3, 2, 1, 2, 1 };

/* Severance's muzak: eight bars of major sevenths, slightly too calm. An electric piano comps in a
 * bossa rhythm, a soft bass walks root and fifth, a vibraphone carries a tune nobody asked for. */
static void muzak_step(uint32_t step) {
    static const int8_t CH[8][5] = {                              /* bass, then four chord tones */
        { 41, 53, 57, 60, 64 }, { 40, 52, 55, 59, 62 }, { 38, 50, 53, 57, 60 }, { 43, 53, 55, 59, 62 },
        { 36, 52, 55, 59, 60 }, { 45, 52, 55, 57, 60 }, { 38, 50, 53, 57, 60 }, { 43, 53, 55, 59, 62 } };
    static const int8_t MEL[][4] = {                               /* bar, step, note, length in steps */
        { 0, 0, 69, 4 }, { 0, 4, 72, 4 }, { 0, 8, 76, 8 }, { 1, 0, 74, 6 }, { 1, 6, 71, 2 }, { 1, 8, 67, 8 },
        { 2, 0, 69, 4 }, { 2, 4, 72, 4 }, { 2, 8, 77, 6 }, { 2, 14, 76, 2 }, { 3, 0, 74, 12 },
        { 4, 0, 76, 4 }, { 4, 4, 79, 4 }, { 4, 8, 71, 8 }, { 5, 0, 72, 6 }, { 5, 6, 69, 2 }, { 5, 8, 76, 8 },
        { 6, 0, 77, 4 }, { 6, 4, 76, 4 }, { 6, 8, 74, 4 }, { 6, 12, 72, 4 }, { 7, 0, 71, 8 }, { 7, 8, 74, 8 } };
    int bar = (int)(step / 16) & 7, s = (int)(step & 15), sps = (int)g_sps;
    const int8_t *ch = CH[bar];
    Voice *v;
    if (s == 0 || s == 6 || s == 8 || s == 14) {                   /* the bass: root, and the fifth on the off-beat */
        int n = (s == 6 || s == 14) ? ch[0] + 7 : ch[0];
        if ((v = voice_on(PAT_BASS, n, s == 0 ? 0.55f : 0.42f, 0.5f, sps * (s == 6 || s == 14 ? 2 : 5)))) v->bus = BUS_MUZAK;
    }
    if (s == 0 || s == 3 || s == 6 || s == 10 || s == 12)          /* the piano, comping */
        for (int k = 1; k < 5; k++)
            if ((v = voice_on(PAT_EPIANO, ch[k], s == 0 ? 0.62f : 0.42f, 0.3f + 0.12f * k, sps * 2))) v->bus = BUS_MUZAK;
    for (unsigned i = 0; i < sizeof MEL / sizeof MEL[0]; i++)
        if (MEL[i][0] == bar && MEL[i][1] == s && (v = voice_on(PAT_VIBES, MEL[i][2], 0.8f, 0.62f, MEL[i][3] * sps - sps / 3))) v->bus = BUS_MUZAK;
}

static float g_boss_k;              /* how far into a boss's waltz, 0..1 (below) */

static void crawl_step(uint32_t step) {
    if (g_room == 2) muzak_step(step);
    if ((g_room && g_room_k >= 1.0f) || g_boss_k >= 1.0f) return;  /* the march has faded: none of it, nor the cave's drips */
    int bar = (int)(step / 16), sec = bar & 31, s = (int)(step & 15);
    const ChordDef *ch = &CHORDS[sec];
    int third = ch->minor ? 3 : 4;
    int sps = (int)g_sps;

    int dm = g_trial ? crawl_drums(8 + (sec & 7), s) | ((s & 1) ? D_HAT : 0) : crawl_drums(sec, s);   /* a trial's: the march's, harder */
    if (dm & D_KICK)  sfx_start(X_KICK, 0.4f, 0.65f, 0.5f, 0.05f);
    if (dm & D_SNARE) sfx_start(X_SNARE, 0.3f, 0.70f, 0.5f, 0.22f);
    if (dm & D_GHOST) sfx_start(X_SNARE, 0.2f, 0.28f, 0.52f, 0.15f);
    if (dm & D_ROLL)  sfx_start(X_SNARE, 0.2f, 0.25f + 0.55f * ((sec - 30) * 16 + s) / 31.0f, 0.5f, 0.2f);
    if (dm & D_HAT)   sfx_start(X_HAT_C, 0.08f, (s & 3) == 0 ? 0.45f : 0.28f, 0.6f, 0.04f);
    if (dm & D_CRASH) sfx_start(X_CRASH, 1.6f, 0.45f, 0.5f, 0.3f);
    if (dm & D_TOM)   sfx_start(X_TOM, 0.6f, sec >= 24 && sec < 28 ? 0.9f : 0.6f, 0.45f, 0.35f);

    /* bass: a marching eighth-note figure; a held pedal D in the crypt */
    if (sec >= 24 && sec < 28) {
        if (s == 0) voice_on(PAT_BASS, 38, 0.9f, 0.5f, sps * 15);
    } else if ((s & 1) == 0) {
        static const int8_t FIG[8] = { 0, 0, 7, 0, 0, 0, 7, 12 };
        voice_on(PAT_BASS, ch->bass + FIG[s >> 1], (s & 7) == 0 ? 0.95f : 0.7f, 0.5f, sps * 2 - sps / 3);
    }

    /* organ chord each bar */
    if (s == 0) {
        int root = ch->bass + 12 + (ch->bass < 36 ? 12 : 0);
        float v = sec >= 24 && sec < 28 ? 1.0f : (sec < 8 ? 0.7f : 0.45f);
        int len = sps * 15;
        voice_on(PAT_ORGAN, root,         v, 0.35f, len);
        voice_on(PAT_ORGAN, root + third, v, 0.50f, len);
        voice_on(PAT_ORGAN, root + 7,     v, 0.65f, len);
    }

    /* harpsichord arpeggios in the second half of the intro and under the brass */
    if (((sec >= 4 && sec < 8) || (sec >= 16 && sec < 24)) && (s & 1) == 0) {
        int r = ch->bass + 24;
        int tones[4] = { r, r + third, r + 7, r + 12 };
        voice_on(PAT_HARP, tones[ARP8[s >> 1]], (s & 3) == 0 ? 0.9f : 0.65f, (s & 2) ? 0.65f : 0.35f, sps * 2);
    }

    for (unsigned i = 0; i < sizeof LEAD / sizeof LEAD[0]; i++)
        if (LEAD[i].bar == sec && LEAD[i].step == s)
            voice_on(LEAD[i].pat, LEAD[i].midi, 1.0f, 0.5f, LEAD[i].len * sps - sps / 4);

    /* the cave itself: drips and, rarely, something moaning far away; not in a pocket's room */
    if (g_room) return;
    if (rng_f(&g_mrng) < 0.022f) sfx_start(SFX_DRIP, 0.6f, 0.5f + 0.5f * rng_f(&g_mrng), 0.1f + 0.8f * rng_f(&g_mrng), 0.75f);
    if (s == 0 && rng_f(&g_mrng) < 0.06f) sfx_start(SFX_MOAN, 2.6f, 0.45f, 0.2f + 0.6f * rng_f(&g_mrng), 0.8f);
}


/* ------------------------------------------------------------------ */
/* The bosses' music: a slow waltz each, off-kilter, in a minor key    */
/* ------------------------------------------------------------------ */

/* Eight bars of three beats, the last cut to two so the phrase stumbles back to its start; an ostinato
 * of five notes that never sits square on the bar; a tritone in the tune and a bell on the first beat.
 * While the adds are out the same theme turns up: drums, a doubled bass, a semitone higher, a little
 * quicker; it settles back at the next bar line once the last add falls. */
typedef struct BNote { int8_t bar, step, semi, len; } BNote;
typedef struct BossTheme {
    int root;                       /* the tonic, in the bass */
    float bpm, tune, lead_vel;      /* quarters a minute; the chance a bar of the tune is sung */
    int lead, lead_at;              /* the tune's patch, and how far above the root it sits */
    int ost, ost_at, ost_every;     /* the ostinato's patch (-1 none), register, steps between notes */
    int8_t ost5[5];
    int chord, chord_at, held;      /* the chords (-1 none); held a bar, or oom-pah-pah on beats two and three */
    int pad, pad_at;                /* a voice holding each chord (-1 none) */
    int bell_at, bass, drone, skitter;
    float bell;
    int8_t prog[8][3];              /* each bar's chord, over the root */
    const BNote *mel; int nmel;
} BossTheme;

static const BNote MEL_KNIGHT[] = {                            /* D minor, on brass */
    {0,0,7,6},{0,6,8,2},{0,8,7,4}, {1,0,3,4},{1,4,2,4},{1,8,0,4}, {2,0,12,6},{2,6,10,2},{2,8,8,4},
    {3,0,2,4},{3,4,8,8}, {4,0,7,6},{4,6,11,2},{4,8,14,4}, {5,0,15,8},{5,8,12,4},
    {6,0,13,6},{6,6,12,2},{6,8,8,4}, {7,0,11,4},{7,4,7,4} };
static const BNote MEL_SPIDER[] = {                            /* B-flat minor, plucked, creeping */
    {0,0,0,2},{0,2,3,2},{0,4,7,2},{0,6,6,2},{0,8,7,2},{0,10,3,2}, {1,0,1,4},{1,4,5,2},{1,6,8,2},{1,8,5,4},
    {2,0,12,2},{2,2,10,2},{2,4,7,2},{2,6,6,2},{2,8,3,4}, {3,0,2,4},{3,4,8,4},{3,8,2,4},
    {4,0,5,2},{4,2,8,2},{4,4,12,2},{4,6,13,2},{4,8,12,4}, {5,0,13,4},{5,4,8,2},{5,6,5,2},{5,8,1,4},
    {6,0,11,2},{6,2,7,2},{6,4,11,2},{6,6,14,2},{6,8,13,2},{6,10,11,2}, {7,0,12,6} };
static const BNote MEL_DEACON[] = {                            /* G minor, a hymn on the organ */
    {0,0,7,8},{0,8,8,4}, {1,0,12,8},{1,8,10,4}, {2,0,8,8},{2,8,5,4}, {3,0,7,12},
    {4,0,3,8},{4,8,2,4}, {5,0,8,4},{5,4,2,8}, {6,0,1,8},{6,8,5,4}, {7,0,11,4},{7,4,7,4} };
static const BNote MEL_WRAITH[] = {                            /* A minor, hardly a tune: a voice, now and then */
    {0,0,7,12}, {1,0,8,8},{1,8,7,4}, {2,0,6,12}, {4,0,12,12}, {5,0,13,8},{5,8,12,4}, {6,0,6,12} };
#define MEL(m) m, (int)(sizeof m / sizeof m[0])

static const BossTheme BTHEMES[] = {
    { .root = 38, .bpm = 66, .tune = 1.0f, .lead_vel = 0.9f, .lead = PAT_BRASS, .lead_at = 24,
      .ost = PAT_HARP, .ost_at = 12, .ost_every = 2, .ost5 = { 0, 7, 3, 6, 7 },
      .chord = PAT_ORGAN, .chord_at = 24, .held = 0, .pad = -1, .bell_at = 36, .bass = 1, .bell = 0.40f,
      .prog = { {0,3,7}, {0,3,7}, {5,8,12}, {2,5,8}, {7,11,14}, {8,12,15}, {1,5,8}, {7,11,14} }, MEL(MEL_KNIGHT) },
    { .root = 34, .bpm = 76, .tune = 1.0f, .lead_vel = 0.95f, .lead = PAT_HARP, .lead_at = 36,
      .ost = PAT_MBOX, .ost_at = 24, .ost_every = 2, .ost5 = { 0, 1, 3, 1, 6 },
      .chord = PAT_HARP, .chord_at = 24, .held = 0, .pad = -1, .bell_at = 36, .bass = 1, .skitter = 1, .bell = 0.32f,
      .prog = { {0,3,7}, {1,5,8}, {0,3,7}, {2,5,8}, {5,8,12}, {1,5,8}, {7,11,14}, {0,3,7} }, MEL(MEL_SPIDER) },
    { .root = 43, .bpm = 58, .tune = 1.0f, .lead_vel = 1.2f, .lead = PAT_ORGAN, .lead_at = 24,
      .ost = PAT_MBOX, .ost_at = 36, .ost_every = 4, .ost5 = { 0, 7, 6, 7, 3 },
      .chord = PAT_ORGAN, .chord_at = 12, .held = 1, .pad = PAT_VOICE, .pad_at = 24, .bell_at = 24, .bass = 1, .bell = 0.38f,
      .prog = { {0,3,7}, {8,12,15}, {5,8,12}, {7,11,14}, {0,3,7}, {2,5,8}, {1,5,8}, {7,11,14} }, MEL(MEL_DEACON) },
    { .root = 33, .bpm = 50, .tune = 0.45f, .lead_vel = 0.9f, .lead = PAT_VOICE, .lead_at = 24,
      .ost = -1, .chord = -1, .pad = PAT_PAD, .pad_at = 12, .bell_at = 24, .bass = 0, .drone = 1, .bell = 0.50f,
      .prog = { {0,3,7}, {0,3,7}, {1,4,8}, {0,3,7}, {0,3,6}, {0,3,7}, {1,5,8}, {0,6,12} }, MEL(MEL_WRAITH) },
};
#define NBTHEME ((int)(sizeof BTHEMES / sizeof BTHEMES[0]))
#undef MEL

static int g_boss_on, g_bth;        /* a boss's waltz playing, and which (1-4) */
static int g_bwave, g_bwave_next;   /* the adds' section, and what it turns to at the next bar line */
static int g_bbar, g_bpos, g_bost;  /* bars played, the step in this one, ostinato notes played */
static int g_bsing, g_bhat;         /* this bar's tune sung; a second hat due in so many samples */
static double g_bsps, g_bacc;       /* the waltz's own clock */
static float g_bfade;               /* how fast it fades in or out */
static Voice *g_bdrone[2];
static uint32_t g_bdrone_id[2];

static double boss_sps(void) {
    const BossTheme *T = &BTHEMES[g_bth - 1];
    return 60.0 / (T->bpm * (g_bwave ? 1.08 : 1.0)) / 4.0 * SND_RATE;
}

static void boss_drone(int on, int r) {
    for (int k = 0; k < 2; k++) {
        Voice *v = g_bdrone[k];
        if (v && v->active && v->serial == g_bdrone_id[k]) v->gate_left = 1;   /* let the old one go */
        g_bdrone[k] = NULL;
        if (!on) continue;
        int len = (int)(g_bsps * (7 * 12 + 8));
        v = voice_on(PAT_DRONE, r + 7 * k, k ? 0.7f : 1.0f, k ? 0.65f : 0.35f, len);
        if (v) { g_bdrone[k] = v; g_bdrone_id[k] = v->serial; }
    }
}

static void boss_step(void) {
    const BossTheme *T = &BTHEMES[g_bth - 1];
    int b = g_bbar & 7, s = g_bpos, blen = b == 7 ? 8 : 12, phrase = (g_bbar / 8) & 3;
    g_vbus = BUS_BOSS;
    if (s == 0 && g_bwave != g_bwave_next) {                       /* the adds come, or the last has fallen */
        g_bwave = g_bwave_next;
        g_bsps = boss_sps();
        if (g_bwave) sfx_start(X_CRASH, 1.6f, 0.45f, 0.5f, 0.3f);
        if (T->drone) boss_drone(1, T->root + g_bwave);
    }
    int r = T->root + g_bwave, sps = (int)g_bsps;
    const int8_t *ch = T->prog[b];
    if (s == 0) {
        if (T->drone && b == 0) boss_drone(1, r);
        /* the tune rests one phrase in four and climbs an octave in another; the adds' section sings it all */
        g_bsing = g_bwave || (phrase != 2 && rng_f(&g_mrng) < T->tune);
        voice_on(PAT_BELL, r + T->bell_at, T->bell * (b == 0 || b == 4 ? 1.0f : 0.55f) * (phrase == 2 ? 1.3f : 1.0f), 0.2f + 0.6f * rng_f(&g_mrng), sps * 12);
        if (T->pad >= 0)
            for (int k = 0; k < 3; k++) {
                Voice *v = voice_on(T->pad, r + T->pad_at + ch[k], T->pad == PAT_VOICE ? 0.45f : 0.75f, 0.3f + 0.2f * k, sps * (blen - 1));
                if (v && k == 2) v->freq *= 1.004f;
            }
        if (T->chord >= 0 && T->held)
            for (int k = 0; k < 3; k++) voice_on(T->chord, r + T->chord_at + ch[k], 0.35f, 0.35f + 0.15f * k, sps * (blen - 1));
    }
    if (T->chord >= 0 && !T->held && (s == 4 || s == 8))             /* oom-pah-pah */
        for (int k = 0; k < 3; k++) voice_on(T->chord, r + T->chord_at + ch[k], T->chord == PAT_ORGAN ? 0.9f : 0.5f, 0.35f + 0.15f * k, sps * 2);
    if ((s & 3) == 0 && (T->bass || g_bwave) && (s == 0 || g_bwave)) {   /* the bass, and doubled under the adds */
        int n = r + (ch[0] > 6 ? ch[0] - 12 : ch[0]);
        voice_on(PAT_BASS, n, s == 0 ? 0.9f : 0.65f, 0.5f, sps * (g_bwave ? 3 : blen - 1));
        if (g_bwave) voice_on(PAT_BASS, n + 12, 0.45f, 0.55f, sps * 3);
    }
    if (T->ost >= 0 && s % T->ost_every == 0) {
        voice_on(T->ost, r + T->ost_at + T->ost5[g_bost % 5], phrase == 2 ? 0.7f : 0.5f, 0.25f + 0.5f * (float)(g_bost & 1), sps * T->ost_every);
        g_bost++;
    }
    if (g_bsing)
        for (int i = 0; i < T->nmel; i++)
            if (T->mel[i].bar == b && T->mel[i].step == s) {
                int n = r + T->lead_at + T->mel[i].semi + (phrase == 3 && !g_bwave && T->lead != PAT_VOICE ? 12 : 0);
                Voice *v = voice_on(T->lead, n, T->lead_vel, 0.5f, T->mel[i].len * sps - sps / 4);
                if (v && T->lead == PAT_VOICE) {                    /* two voices, not quite together */
                    Voice *w = voice_on(T->lead, n, T->lead_vel * 0.6f, 0.62f, T->mel[i].len * sps - sps / 4);
                    if (w) w->freq *= 1.006f;
                }
            }
    if (T->skitter && rng_f(&g_mrng) < (g_bwave ? 0.45f : 0.28f)) {   /* something many-legged */
        sfx_start(X_HAT_C, 0.06f, 0.10f + 0.12f * rng_f(&g_mrng), 0.2f + 0.6f * rng_f(&g_mrng), 0.08f);
        if (rng_f(&g_mrng) < 0.3f) g_bhat = sps / 2;
    }
    if (g_bwave) {                                                 /* drums: one, and two, three */
        if (s == 0) sfx_start(X_KICK, 0.4f, 0.7f, 0.5f, 0.05f);
        if (s == 4 || s == 8) {
            if (T->drone) sfx_start(X_TOM, 0.6f, s == 4 ? 0.5f : 0.7f, 0.45f, 0.35f);
            else sfx_start(X_SNARE, 0.3f, 0.42f, 0.5f, 0.22f);
        }
        if (!(s & 1)) sfx_start(X_HAT_C, 0.08f, (s & 3) == 0 ? 0.32f : 0.2f, 0.6f, 0.04f);
        if (b == 7 && s >= 4 && !(s & 1)) sfx_start(X_TOM, 0.6f, 0.45f + 0.08f * (float)s, 0.4f, 0.35f);
    }
    g_vbus = BUS_MARCH;
    if (++g_bpos >= blen) { g_bpos = 0; g_bbar++; }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

static void reset_voices_and_fx(void) {
    memset(g_v, 0, sizeof g_v);
    memset(g_s, 0, sizeof g_s);
    memset(g_sched, 0, sizeof g_sched);
    memset(g_rv, 0, sizeof *g_rv);
    memset(g_dl, 0, sizeof g_dl); memset(g_dr, 0, sizeof g_dr);
    g_dpos = 0;
    g_drone_on = 0;
    g_wind_lp1 = g_wind_lp2 = 0;
    g_dcx[0] = g_dcx[1] = g_dcy[0] = g_dcy[1] = 0;
}

void snd_init(void) {
#ifdef _WIN32
    if (!g_cs_ready) { InitializeCriticalSection(&g_cs); g_cs_ready = 1; }
#endif
    for (int i = 0; i <= TBL; i++) g_sin[i] = sinf(TAU_F * i / TBL);
    if (!g_rv) g_rv = (Reverb *)calloc(1, sizeof(Reverb));
    LOCK();
    reset_voices_and_fx();
    g_prog = SND_PROG_NONE;
    UNLOCK();
}

void snd_set_volume(float v) { LOCK(); g_master = clampf(v, 0.0f, 1.0f); UNLOCK(); }

void snd_fade_master(float target, float seconds) {
    LOCK();
    g_mg_target = target;
    g_mg_slope = 1.0f / (SR * fmaxf(seconds, 0.01f));
    UNLOCK();
}

void snd_start_program(int prog) {
    LOCK();
    reset_voices_and_fx();
    g_prog = prog;
    g_samples = 0; g_step_acc = 0; g_step = 0;
    g_mrng = 777u + (uint32_t)prog * 31u;
    g_hb_timer = 0; g_hold = 0; g_fx_gain = 1.0f; g_room = 0; g_room_k = g_muz_k = 0.0f; g_muffle = 0; g_muf_k = 0.0f;
    g_boss_on = 0; g_boss_k = 0.0f; g_bhat = 0; g_bdrone[0] = g_bdrone[1] = NULL; g_vbus = BUS_MARCH;
    if (prog == SND_PROG_HAUNT) {
        g_sps = 0.25 * SND_RATE;                      /* 60 BPM sixteenths */
        g_rv_fb = 0.86f; g_rv_damp = 0.30f; g_rv_wet = 0.36f;
        g_dlen = (int)(0.5f * SND_RATE); g_dly_fb = 0.40f; g_dly_wet = 0.45f;
        g_mood = g_mood_target = 0.0f;
    } else if (prog == SND_PROG_CRAWL) {
        g_sps = 60.0 / 92.0 / 4.0 * SND_RATE;
        g_rv_fb = 0.84f; g_rv_damp = 0.30f; g_rv_wet = 0.30f;      /* a big stone hall */
        g_dlen = (int)(g_sps * 3.0); g_dly_fb = 0.30f; g_dly_wet = 0.30f;
        g_mood = g_mood_target = 0.0f;
    }
    g_mg = 0.0f;                                      /* program starts silent, caller fades up */
    UNLOCK();
}

void snd_set_mood(float m) { LOCK(); g_mood_target = clampf(m, 0.0f, 1.0f); UNLOCK(); }
void snd_hold(int on) { LOCK(); g_hold = on != 0; UNLOCK(); }
void snd_set_room(int room) { LOCK(); g_room = room; UNLOCK(); }
void snd_set_muffle(int on) { LOCK(); g_muffle = on != 0; UNLOCK(); }
void snd_set_trial(int on) { LOCK(); g_trial = on != 0; UNLOCK(); }

void snd_set_boss(int theme, int wave) {
    LOCK();
    if (theme < 1 || theme > NBTHEME) {
        if (g_boss_on) g_bfade = 1.0f / (3.0f * SR);
        g_boss_on = 0;
    } else {
        if (!g_boss_on || g_bth != theme) {                        /* from the top */
            g_bth = theme; g_bbar = g_bpos = g_bost = 0;
            g_bwave = g_bwave_next = wave != 0;
            g_bsps = boss_sps(); g_bacc = g_bsps - 1.0;
            g_bdrone[0] = g_bdrone[1] = NULL;
        }
        g_boss_on = 1; g_bwave_next = wave != 0;
        g_bfade = 1.0f / (1.5f * SR);
    }
    UNLOCK();
}

void snd_boss_fall(void) {
    LOCK();
    if (g_boss_on) {                                               /* the waltz fades; the bell tolls three times */
        const BossTheme *T = &BTHEMES[g_bth - 1];
        g_boss_on = 0; g_bfade = 1.0f / (4.0f * SR);
        g_vbus = BUS_SFX;
        for (int i = 0; i < 3; i++) sched_note_g((int)((2.4f + 1.8f * i) * SND_RATE), PAT_BELL, T->root + 24, 0.75f - 0.15f * i, 3 * SND_RATE);
        g_vbus = BUS_MARCH;
    }
    UNLOCK();
}
void snd_set_latency(int frames) { LOCK(); g_latency = frames; UNLOCK(); }

/* duration, gain, default pan (negative: random spread of that width), reverb send */
typedef struct SfxDef { float dur, gain, pan, rev; } SfxDef;
static const SfxDef SFXDEF[SFX_COUNT] = {
    [SFX_BEEP_POST]  = { 0.20f, 1.00f,  0.50f, 0.00f },
    [SFX_BEEP_ERR]   = { 0.42f, 1.00f,  0.50f, 0.00f },
    [SFX_KEY]        = { 0.12f, 0.85f, -0.08f, 0.03f },
    [SFX_ENTER]      = { 0.20f, 1.00f,  0.50f, 0.04f },
    [SFX_FLOPPY]     = { 1.35f, 1.00f,  0.50f, 0.02f },
    [SFX_HDD]        = { 0.85f, 1.00f,  0.50f, 0.02f },
    [SFX_WHISPER]    = { 2.40f, 1.00f, -0.60f, 0.75f },
    [SFX_CREAK]      = { 1.70f, 1.00f, -0.50f, 0.45f },
    [SFX_STATIC]     = { 0.32f, 1.00f,  0.50f, 0.05f },
    [SFX_BOOM]       = { 2.80f, 1.00f,  0.50f, 0.35f },
    [SFX_STINGER]    = { 0.00f, 1.00f,  0.50f, 0.00f },
    [SFX_GHOST_TICK] = { 0.28f, 1.00f, -0.60f, 0.70f },
    [SFX_TICK]       = { 0.05f, 1.00f,  0.50f, 0.00f },
    [SFX_SHIMMER]    = { 3.20f, 1.00f,  0.50f, 0.65f },
    [SFX_WHOOSH]     = { 1.10f, 1.00f,  0.50f, 0.30f },
    [SFX_ZAP]        = { 0.30f, 1.00f,  0.50f, 0.10f },
    [SFX_HEART]      = { 0.60f, 0.60f,  0.50f, 0.15f },
    [SFX_STEP]       = { 0.20f, 0.55f, -0.06f, 0.12f },
    [SFX_DOOR]       = { 1.00f, 0.90f,  0.50f, 0.30f },
    [SFX_CAST]       = { 0.50f, 1.10f,  0.50f, 0.25f },
    [SFX_EXPLODE]    = { 0.90f, 1.00f,  0.50f, 0.40f },
    [SFX_BONES]      = { 0.90f, 0.50f,  0.50f, 0.30f },
    [SFX_GHOST_DIE]  = { 1.40f, 0.80f,  0.50f, 0.60f },
    [SFX_BAT]        = { 0.30f, 1.30f,  0.50f, 0.30f },
    [SFX_DRIP]       = { 0.60f, 0.70f, -0.40f, 0.75f },
    [SFX_MOAN]       = { 2.60f, 0.50f, -0.30f, 0.80f },
    [SFX_COIN]       = { 0.60f, 0.85f,  0.50f, 0.35f },
    [SFX_KEYGET]     = { 0.95f, 0.90f,  0.50f, 0.40f },
    [SFX_UNLOCK]     = { 0.95f, 1.00f,  0.50f, 0.35f },
    [SFX_HURT]       = { 0.40f, 0.90f,  0.50f, 0.15f },
    [SFX_THROW]      = { 0.28f, 0.55f,  0.50f, 0.25f },
    [SFX_DEATH]      = { 1.70f, 1.00f,  0.50f, 0.45f },
    [SFX_POTION]     = { 1.10f, 0.85f,  0.50f, 0.40f },
    [SFX_GROAN]      = { 2.00f, 0.80f, -0.40f, 0.45f },
    [SFX_DIRT]       = { 1.30f, 0.80f, -0.40f, 0.30f },
    [SFX_XYLO]       = { 1.50f, 0.80f,  0.50f, 0.35f },
    [SFX_WAIL]       = { 2.60f, 0.70f, -0.50f, 0.70f },
    [SFX_CAW]        = { 0.80f, 0.60f, -0.70f, 0.50f },
    [SFX_FANFARE]    = { 0.00f, 1.00f,  0.50f, 0.00f },
    [SFX_SHRIEK]     = { 1.10f, 0.75f,  0.50f, 0.70f },
    [SFX_IGNITE]     = { 0.75f, 0.50f,  0.50f, 0.25f },
    [SFX_WIND]       = { 3.20f, 1.10f,  0.50f, 0.40f },
    [SFX_CACKLE]     = { 1.45f, 0.48f,  0.50f, 0.60f },
    [SFX_CHATTER]    = { 1.40f, 0.60f, -0.50f, 0.35f },
    [SFX_KNOCK]      = { 0.30f, 0.70f,  0.45f, 0.45f },
    [SFX_DIAL]       = { 1.75f, 0.60f,  0.50f, 0.05f },
    [SFX_MODEM]      = { 3.50f, 0.70f,  0.50f, 0.05f },
    [SFX_SHATTER]    = { 0.70f, 0.60f,  0.50f, 0.30f },
    [SFX_SKITTER]    = { 0.70f, 0.60f,  0.50f, 0.25f },
    [SFX_SCREAM]     = { 1.20f, 0.70f,  0.50f, 0.55f },
    [SFX_SWORD]      = { 0.60f, 0.70f,  0.50f, 0.25f },
    [SFX_IMPLODE]    = { 2.40f, 0.58f,  0.50f, 0.60f },
    [SFX_FROST]      = { 0.60f, 0.70f,  0.50f, 0.35f },
    [SFX_RUNE]       = { 0.80f, 0.70f,  0.50f, 0.55f },
    [SFX_PURR]       = { 2.00f, 0.90f,  0.50f, 0.10f },
    [SFX_TRILL]      = { 0.35f, 0.70f,  0.50f, 0.30f },
    [SFX_PUSHWALL]   = { 2.90f, 0.80f,  0.50f, 0.35f },
    [SFX_SECRET]     = { 1.90f, 0.85f,  0.50f, 0.55f },
    [SFX_MIRRORBELL] = { 0.00f, 1.00f,  0.50f, 0.00f },
    [SFX_DEEPCHIME]  = { 0.00f, 1.00f,  0.50f, 0.00f },
    [SFX_ATONE]      = { 0.00f, 1.00f,  0.50f, 0.00f },
    [SFX_SPLASH]     = { 0.38f, 0.60f, -0.06f, 0.22f },
    [SFX_HISS]       = { 1.40f, 0.55f,  0.50f, 0.35f },
    [SFX_CRACKLE]    = { 1.20f, 0.60f,  0.50f, 0.30f },
    [SFX_RUMBLE]     = { 2.00f, 0.80f,  0.50f, 0.40f },
    [SFX_HUM]        = { 1.60f, 0.55f,  0.50f, 0.50f },
    [SFX_RUSTLE]     = { 0.95f, 0.55f,  0.50f, 0.25f },
    [SFX_PAGE]       = { 0.40f, 0.55f,  0.50f, 0.25f },
};

void snd_tone(int glass, int midi, float gain, float pan) {      /* a struck bell or a singing crystal, at its own note */
    LOCK();
    g_vbus = BUS_SFX;
    voice_on(glass ? PAT_VIBES : PAT_BELL, midi, gain, clampf(pan, 0.0f, 1.0f), (int)((glass ? 1.4f : 2.2f) * SND_RATE));
    if (glass) voice_on(PAT_BELL, midi + 12, gain * 0.35f, clampf(pan, 0.0f, 1.0f), (int)(1.0f * SND_RATE));
    UNLOCK();
}
void snd_sfx_ex(int id, float gain, float pan) {
    if (id < 0 || id >= SFX_COUNT) return;
    LOCK();
    const SfxDef *d = &SFXDEF[id];
    if (pan < 0.0f) pan = d->pan >= 0.0f ? d->pan : 0.5f + d->pan * (rng_f(&g_rng) * 2.0f - 1.0f);
    g_vbus = BUS_SFX;
    if (id == SFX_FANFARE) {                       /* level complete: a brass flourish up to a held D major */
        static const short F[][3] = { {0, 62, 110}, {120, 66, 110}, {240, 69, 110}, {360, 74, 420},
                                      {840, 71, 110}, {960, 74, 110}, {1080, 78, 1300}, {1080, 74, 1300}, {1080, 69, 1300}, {1080, 50, 1300} };
        for (int i = 0; i < (int)(sizeof F / sizeof F[0]); i++)
            sched_note_g(F[i][0] * SND_RATE / 1000 + 1, F[i][1] < 55 ? PAT_BASS : PAT_BRASS, F[i][1], gain * (F[i][1] < 55 ? 0.8f : 0.95f), F[i][2] * SND_RATE / 1000);
        sfx_start(X_TOM, 0.6f, 0.7f * gain, 0.45f, 0.35f);
        sfx_start(X_CRASH, 1.6f, 0.35f * gain, 0.55f, 0.3f);
    } else if (id == SFX_MIRRORBELL) {            /* struck, not rung: two bells a little apart, and the strike */
        static const short B[][2] = { { 88, 100 }, { 95, 55 }, { 89, 45 } };
        for (int i = 0; i < 3; i++) {
            Voice *v = voice_on(PAT_BELL, B[i][0], gain * B[i][1] / 100.0f, clampf(pan + (i - 1) * 0.12f, 0.0f, 1.0f), (int)(1.2f * SND_RATE));
            if (v && i == 2) v->freq *= 0.993f;                  /* beating against the first */
        }
        sfx_start(X_HAT_O, 0.35f, 0.5f * gain, clampf(pan, 0.0f, 1.0f), 0.4f);
    } else if (id == SFX_DEEPCHIME) {             /* low, and slowly beating */
        static const short B[][2] = { { 45, 100 }, { 52, 60 }, { 57, 35 } };
        for (int i = 0; i < 3; i++) {
            Voice *v = voice_on(PAT_BELL, B[i][0], gain * B[i][1] / 100.0f, clampf(pan + (i - 1) * 0.15f, 0.0f, 1.0f), (int)(2.5f * SND_RATE));
            if (v && i == 1) v->freq *= 1.004f;
        }
    } else if (id == SFX_ATONE) {                 /* the choir on a diminished chord, settling into the minor under a high bell */
        static const short C[][4] = { { 0, 50, 55, 1300 }, { 0, 53, 45, 1300 }, { 0, 56, 45, 1300 },   /* ms, note, gain %, ms held */
                                      { 650, 50, 65, 1800 }, { 650, 53, 50, 1800 }, { 650, 57, 50, 1800 }, { 650, 62, 35, 1800 } };
        for (int i = 0; i < (int)(sizeof C / sizeof C[0]); i++)
            sched_note_g(C[i][0] * SND_RATE / 1000 + 1, PAT_VOICE, C[i][1], gain * C[i][2] / 100.0f, C[i][3] * SND_RATE / 1000);
        voice_on(PAT_BELL, 38, gain * 0.8f, 0.5f, (int)(2.8f * SND_RATE));   /* a deep bell under it all */
        sched_note_g(650 * SND_RATE / 1000, PAT_BELL, 74, gain * 0.45f, (int)(1.6f * SND_RATE));
        sched_note_g(950 * SND_RATE / 1000, PAT_BELL, 69, gain * 0.3f, (int)(1.8f * SND_RATE));
        sfx_start(SFX_WHISPER, 2.4f, 0.5f * gain, 0.5f, 0.75f);
    } else if (id == SFX_STINGER) {
        int notes[4] = { 38, 44, 45, 50 };
        for (int i = 0; i < 4; i++) {
            Voice *v = voice_on(PAT_STAB, notes[i], gain, 0.2f + 0.2f * i, (int)(1.6f * SND_RATE));
            if (v) v->bend_rate = -0.55f;
        }
        sfx_start(SFX_BOOM, 2.8f, 0.55f * gain, 0.5f, 0.35f);
    } else {
        float g = d->gain * gain;
        if (id == SFX_HEART) g = (0.6f + 0.5f * g_mood) * gain;
        sfx_start(id, d->dur, g, clampf(pan, 0.0f, 1.0f), d->rev);
    }
    g_vbus = BUS_MARCH;
    UNLOCK();
}

void snd_sfx(int id) { snd_sfx_ex(id, 1.0f, -1.0f); }

static inline float softclip(float x) {
    if (x > 3.0f) x = 3.0f; else if (x < -3.0f) x = -3.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

static void render_locked(int16_t *out, int frames) {
    for (int n = 0; n < frames; n++) {
        /* sequencer clock */
        if (g_prog != SND_PROG_NONE) {
            g_step_acc += 1.0;
            double sps = g_trial && g_prog == SND_PROG_CRAWL ? g_sps * 0.78 : g_sps;
            if (g_step_acc >= sps) {
                g_step_acc -= sps;
                if (g_prog == SND_PROG_HAUNT) haunt_step(g_step);
                else crawl_step(g_step);
                g_step++;
            }
            if (g_boss_on && (g_bacc += 1.0) >= g_bsps) { g_bacc -= g_bsps; boss_step(); }
            if (g_bhat > 0 && --g_bhat == 0) { g_vbus = BUS_BOSS; sfx_start(X_HAT_C, 0.05f, 0.09f, 0.3f + 0.4f * rng_f(&g_mrng), 0.08f); g_vbus = BUS_MARCH; }
            sched_run();
            g_samples += 1.0;
        }
        /* slow controls */
        g_mood += (g_mood_target - g_mood) * (1.0f / (SR * 1.5f));
        g_drone_gain = 0.55f + 0.45f * g_mood;
        if (g_mg < g_mg_target) { g_mg += g_mg_slope; if (g_mg > g_mg_target) g_mg = g_mg_target; }
        else if (g_mg > g_mg_target) { g_mg -= g_mg_slope; if (g_mg < g_mg_target) g_mg = g_mg_target; }

        float L = 0.0f, R = 0.0f, rev = 0.0f, dly = 0.0f;

        g_room_k = g_room ? fminf(1.0f, g_room_k + 1.0f / SR) : fmaxf(0.0f, g_room_k - 1.0f / SR);
        g_muz_k = g_room == 2 ? fminf(1.0f, g_muz_k + 1.0f / SR) : fmaxf(0.0f, g_muz_k - 1.0f / SR);
        g_boss_k = g_boss_on ? fminf(1.0f, g_boss_k + g_bfade) : fmaxf(0.0f, g_boss_k - g_bfade);
        float bus[4];                                              /* the march, and its drums, fade in a pocket's room */
        bus[BUS_MARCH] = (1.0f - g_room_k) * (1.0f - g_boss_k);    /* and under a boss's waltz; */
        bus[BUS_MUZAK] = 0.6f * g_muz_k;                           /* Severance's muzak, kept well back under the hum; */
        bus[BUS_BOSS] = g_boss_k;
        bus[BUS_SFX] = g_fx_gain;                                  /* the fanfare and the stinger, with the game's sounds */
        {
            float bl[4] = { 0 }, br[4] = { 0 }, brev[4] = { 0 }, bdly[4] = { 0 };
            for (int i = 0; i < NV; i++) {
                Voice *v = &g_v[i];
                if (!v->active) continue;
                if (bus[v->bus] <= 0.0f && v->bus != BUS_SFX) { v->active = 0; continue; }
                voice_run(v, &bl[v->bus], &br[v->bus], &brev[v->bus], &bdly[v->bus]);
            }
            for (int k = 0; k < 4; k++) { L += bl[k] * bus[k]; R += br[k] * bus[k]; rev += brev[k] * bus[k]; dly += bdly[k] * bus[k]; }
        }

        g_fx_gain = g_hold ? fmaxf(0.0f, g_fx_gain - 50.0f / SR) : fminf(1.0f, g_fx_gain + 50.0f / SR);
        for (int i = 0; i < NS; i++) if (g_s[i].active) {
            Sfx *s = &g_s[i];
            int game = s->type < SFX_COUNT;
            if (game && g_fx_gain <= 0.0f) continue;               /* held where it is */
            float x = sfx_sample(s) * (game ? g_fx_gain : bus[s->bus]);
            float a = s->pan * (PI_F * 0.5f);
            L += x * cosf(a) * 1.0f; R += x * sinf(a) * 1.0f;      /* cheap pan */
            rev += x * (s->rev + (game ? 0.25f * g_room_k : 0.0f));
        }
        if (g_room_k > 0.0f && g_fx_gain > 0.0f) {                 /* the tubes' hum: the mains, and a buzz on top */
            g_hum_ph += 60.0f / SR; if (g_hum_ph >= 1.0f) g_hum_ph -= 1.0f;
            float p = g_hum_ph;
            float h = 0.55f * sin_cyc(p * 2.0f) + 0.25f * sin_cyc(p * 4.0f) + 0.12f * sin_cyc(p * 6.0f + 0.2f) + 0.06f * sin_cyc(p * 10.0f);
            h *= 0.012f * g_room_k * g_fx_gain;
            L += h; R += h;
        }

        if (g_prog == SND_PROG_HAUNT) {
            /* wind bed */
            float nzv = nz(&g_mrng);
            g_wind_lp1 += 0.012f * (nzv - g_wind_lp1);
            g_wind_lp2 += 0.0035f * (g_wind_lp1 - g_wind_lp2);
            g_wind_ph += (0.07f + 0.05f * g_mood) / SR; if (g_wind_ph >= 1.0f) g_wind_ph -= 1.0f;
            float gust = 0.55f + 0.45f * sin_cyc(g_wind_ph) * sin_cyc(g_wind_ph * 2.7f + 0.3f);
            float w = (g_wind_lp1 * 3.0f + g_wind_lp2 * 6.0f) * gust * (0.06f + 0.10f * g_mood);
            L += w; R += w * 0.9f; rev += w * 0.3f;
            /* heartbeat when tense */
            if (g_mood > 0.55f) {
                g_hb_timer -= 1.0f / SR;
                if (g_hb_timer <= 0.0f) {
                    g_hb_timer = 1.5f - 0.75f * g_mood;
                    sfx_start(SFX_HEART, 0.6f, 0.35f + 0.5f * g_mood, 0.5f, 0.15f);
                }
            }
        }

        if (g_prog == SND_PROG_CRAWL && g_mood > 0.01f && g_fx_gain > 0.0f) {
            /* torch crackle: sparse pops, louder when torches are close */
            if (rng_f(&g_mrng) < 0.0002f + 0.0016f * g_mood) g_crk_env = 0.4f + 0.6f * rng_f(&g_mrng);
            g_crk_env *= 0.9965f;
            float n = nz(&g_mrng), hp = n - g_crk_lp; g_crk_lp = n;
            g_crk_hiss += 0.02f * (nz(&g_mrng) - g_crk_hiss);
            float c = (hp * g_crk_env * 0.30f + g_crk_hiss * 0.05f) * g_mood * g_fx_gain;
            L += c * 0.9f; R += c; rev += c * 0.1f;
        }

        /* ping-pong delay */
        float dl = g_dl[(g_dpos + DLY_MAX - g_dlen) % DLY_MAX];
        float dr = g_dr[(g_dpos + DLY_MAX - g_dlen) % DLY_MAX];
        g_dl[g_dpos] = dly + dr * g_dly_fb;
        g_dr[g_dpos] = dl * g_dly_fb;
        if (++g_dpos >= DLY_MAX) g_dpos = 0;
        L += dl * g_dly_wet; R += dr * g_dly_wet;
        rev += (dl + dr) * 0.25f;

        float rl, rr;
        reverb_tick(rev, &rl, &rr);
        L += rl * g_rv_wet * 3.0f; R += rr * g_rv_wet * 3.0f;

        /* DC blocker (~15 Hz): the low sines in kicks/bass are not zero-mean */
        float yl = L - g_dcx[0] + 0.9979f * g_dcy[0]; g_dcx[0] = L; g_dcy[0] = yl;
        float yr = R - g_dcx[1] + 0.9979f * g_dcy[1]; g_dcx[1] = R; g_dcy[1] = yr;
        g_muf_k += ((float)g_muffle - g_muf_k) * (5.0f / SR);     /* the void: a low-pass coming down over everything */
        if (g_muf_k > 0.002f) {
            float a = 1.0f - 0.94f * g_muf_k;
            g_mlp[0] += a * (yl - g_mlp[0]); g_mlp[1] += a * (yr - g_mlp[1]);
            yl = g_mlp[0] * (1.0f - 0.25f * g_muf_k); yr = g_mlp[1] * (1.0f - 0.25f * g_muf_k);
        } else { g_mlp[0] = yl; g_mlp[1] = yr; }

        float gm = g_master * g_mg;
        float ol = softclip(yl * gm * 1.15f), or_ = softclip(yr * gm * 1.15f);
        out[2 * n]     = (int16_t)(ol * 32000.0f);
        out[2 * n + 1] = (int16_t)(or_ * 32000.0f);
    }
}

void snd_render(int16_t *out, int frames) {
    LOCK();
    render_locked(out, frames);
    UNLOCK();
}

void snd_advance(double seconds) {
    static int16_t scratch[2048 * 2];
    int total = (int)(seconds * SND_RATE + 0.5);
    LOCK();
    while (total > 0) {
        int n = total > 2048 ? 2048 : total;
        render_locked(scratch, n);
        total -= n;
    }
    UNLOCK();
}

void snd_state(SndState *st) {
    LOCK();
    memset(st, 0, sizeof *st);
    double pos = g_samples - g_latency;
    if (pos < 0) pos = 0;
    st->song_time = pos / SND_RATE;
    st->mood = g_mood;
    if (g_prog != SND_PROG_NONE && g_sps > 0) {
        st->step = (int)(pos / g_sps);
        st->bar = st->step / 16;
    }
    UNLOCK();
}
