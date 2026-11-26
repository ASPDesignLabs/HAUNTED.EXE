/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Side passages (docs/PLAN-side.md): what one is (SdLevel) and its text read (sd_read), the same text for those built in
 * (side_levels.c) and those the World Editor hands over. */

#define SD_W 256                                                   /* tiles across and down, at most */
#define SD_H 64
#define SD_THINGS 160
#define SD_TILE 16                                                 /* STS: a tile, in pixels */
enum { SDT_DOOR, SDT_PLAT, SDT_FALL, SDT_FLOW, SDT_MOB, SDT_GOLD, SDT_KEY, SDT_LIFE, SDT_SWITCH, SDT_TORCH,
       SDT_GATE, SDT_PLATE, SDT_ZONE, SDT_CURRENT, SDT_HAZ, NSDT };   /* the last five: 26 November, the owner's puzzles and the crypt's traps */
enum { SDF_GOAL = 1, SDF_LOOP = 2, SDF_TOGGLE = 4, SDF_OFF = 8, SDF_LOOSE = 16, SDF_POISON = 32, SDF_OPEN = 64, SDF_ZONE = 128, SDF_SAFE = 256 };
/* A thing, at a tile (x, y). DOOR: a its link, GOAL; it stands on the tile below, two tiles high. PLAT: w tiles wide, (a, b) its
 * way, f0 how far (tiles), f1 how fast (tiles a second), f2 the wait at each end (s), f3 where it begins (0 to 1); LOOP: on round
 * and back to its start at once, else back and forth. FALL: f0 the shaking before it goes, f1 how long till it is back. FLOW: w by
 * h tiles, f0 seconds full, f1 seconds dry, f2 where in that it begins. MOB: a MOB_* or -1 the run's choice, b a rank or -1.
 * GOLD: a an IT_*. KEY: a its colour. SWITCH: a the crypt's thing whose USE triggers it fires; TOGGLE: thrown back and forth. */
typedef struct { int kind, x, y, w, h, a, b, flags; float f[4]; char tag[16]; } SdThing;   /* tag: what a trigger names it by */
/* GATE x y w h: bars across w by h tiles, shut unless OPEN; a trigger's ON opens it. PLATE x y: pressed while the caster, or a loose
 * thing, stands on it. ZONE x y w h: pressed while the caster is in the box; never seen. CURRENT x y w h dx dy speed: water running
 * (tiles a second) through the box, carrying what is loose and pushing the caster; OFF: dry until a trigger. POISON: it bites.
 * HAZ kind x y: one of the crypt's traps (HZ_*), on the cells its HZC lines give in order; below, SdHaz. LOOSE on gold, a key or
 * a life: it falls, and water carries it. OFF on a platform, a flow or a trap: still until a trigger. */
enum { HZF_FALL, HZF_LINE, HZF_CREEP, HZF_CLOUD };
#define SD_HAZ 16
#define SD_HZC 512
typedef struct { int kind, fam, el, c0, nc, how, x0, y0, x1, y1, harm, chance; float every, give, first, warn, hold; } SdHaz;   /* how: HH_* */
#define SD_TRIGS 32
#define SD_ACTS 8
enum { TRM_ONCE, TRM_TOGGLE, TRM_HOLD, TRM_TIMED };
enum { TRV_ON, TRV_OFF, TRV_FLIP, TRV_CRYPT };
typedef struct { int src, mode, nact, target[SD_ACTS], verb[SD_ACTS], arg[SD_ACTS]; float secs; } SdTrig;   /* target: a thing, -1 the crypt */

/* What the owner tunes (the World Editor's sliders): every number of how the caster moves and what harms it, by name. A passage's
 * TUNE lines set its own; the rest are these defaults. Distances in pixels (a tile is 16), times in seconds, harm in shares of
 * health. extract.py reads this table for the editor, so its order and names are the text's. */
enum { TU_WALK, TU_RUN, TU_CREEP, TU_ACCEL, TU_BRAKE, TU_AIR, TU_JUMP, TU_GRAV_HELD, TU_GRAV, TU_FALL_MAX, TU_COYOTE, TU_JUMP_BUF,
       TU_HURT_SAFE, TU_KNOCK_X, TU_KNOCK_Y, TU_SPIKES, TU_BITE, TU_BITE_EVERY, TU_WADE, TU_CURRENT, TU_GATE_TIME,
       TU_FIRE_REACH, TU_FROST_REACH, TU_LIGHT_REACH, TU_SPIRIT_REACH, TU_FIRE_SPEED, TU_FROST_SPEED, TU_SPIRIT_SPEED, TU_BOLT_SPEED,
       TU_FLASK_NEAR, TU_FLASK_FAR, TU_FLASK_LIFT, TU_FLASK_HOLD, TU_DEAD_PACE, TU_DEAD_SIGHT, TU_CAM_LEAD, NTU };
static const struct { const char *name; float def, lo, hi, step; const char *what; } TUNE[NTU] = {
    { "walk", 72.0f, 20.0f, 200.0f, 1.0f, "walking pace, pixels a second" },
    { "run", 128.0f, 40.0f, 300.0f, 1.0f, "running pace (Shift, B, or the stick pushed hard)" },
    { "creep", 40.0f, 10.0f, 120.0f, 1.0f, "crouched pace" },
    { "accel", 900.0f, 100.0f, 4000.0f, 10.0f, "how fast it gets up to pace on the ground" },
    { "brake", 1100.0f, 100.0f, 5000.0f, 10.0f, "how fast it stops on the ground" },
    { "air", 560.0f, 0.0f, 3000.0f, 10.0f, "how much it can steer in the air" },
    { "jump", 236.0f, 100.0f, 500.0f, 1.0f, "the jump's speed upward" },
    { "grav_held", 520.0f, 100.0f, 2000.0f, 10.0f, "the pull down while jump is held and it rises (a longer jump)" },
    { "grav", 950.0f, 200.0f, 3000.0f, 10.0f, "the pull down otherwise" },
    { "fall_max", 340.0f, 100.0f, 800.0f, 5.0f, "the fastest it falls" },
    { "coyote", 0.08f, 0.0f, 0.3f, 0.01f, "grace to jump after running off an edge" },
    { "jump_buf", 0.10f, 0.0f, 0.3f, 0.01f, "a jump pressed this soon before landing still counts" },
    { "hurt_safe", 0.9f, 0.0f, 3.0f, 0.05f, "untouchable after a hurt" },
    { "knock_x", 110.0f, 0.0f, 400.0f, 5.0f, "thrown back by a hurt, sideways" },
    { "knock_y", 150.0f, 0.0f, 400.0f, 5.0f, "and upward" },
    { "spikes", 0.34f, 0.0f, 1.0f, 0.01f, "spikes take this share of health" },
    { "bite", 0.10f, 0.0f, 0.5f, 0.01f, "poison takes this share of health" },
    { "bite_every", 0.5f, 0.1f, 3.0f, 0.05f, "each this many seconds in it" },
    { "wade", 0.5f, 0.1f, 1.0f, 0.05f, "pace kept wading in poison or a current" },
    { "current", 0.5f, 0.0f, 2.0f, 0.05f, "a current's push on the caster, of its speed" },
    { "gate_time", 0.6f, 0.05f, 3.0f, 0.05f, "a gate's time to open or shut" },
    { "fire_reach", 6.0f, 1.0f, 20.0f, 0.25f, "fire's reach, in tiles" },
    { "frost_reach", 7.2f, 1.0f, 20.0f, 0.25f, "frost's reach, in tiles" },
    { "light_reach", 5.0f, 1.0f, 20.0f, 0.25f, "lightning's reach, in tiles (at once)" },
    { "spirit_reach", 3.0f, 1.0f, 20.0f, 0.25f, "spirit's reach, in tiles" },
    { "fire_speed", 230.0f, 40.0f, 600.0f, 5.0f, "fire's flight, pixels a second" },
    { "frost_speed", 190.0f, 40.0f, 600.0f, 5.0f, "frost's flight" },
    { "spirit_speed", 160.0f, 40.0f, 600.0f, 5.0f, "spirit's flight" },
    { "bolt_speed", 280.0f, 40.0f, 800.0f, 5.0f, "the bone's flight, and the rest" },
    { "flask_near", 60.0f, 0.0f, 300.0f, 5.0f, "the flask's throw, barely held" },
    { "flask_far", 200.0f, 20.0f, 500.0f, 5.0f, "the flask's throw, held a full while" },
    { "flask_lift", 150.0f, 0.0f, 400.0f, 5.0f, "the flask's throw upward" },
    { "flask_hold", 1.0f, 0.1f, 3.0f, 0.05f, "seconds held for the far throw" },
    { "dead_pace", 26.0f, 5.0f, 80.0f, 1.0f, "the dead's pace: their crypt pace times this" },
    { "dead_sight", 150.0f, 30.0f, 400.0f, 5.0f, "how far the dead see you, in pixels" },
    { "cam_lead", 24.0f, 0.0f, 100.0f, 1.0f, "how far ahead the view looks" },
};
static int sd_tune_of(const char *name) { for (int k = 0; k < NTU; k++) if (!strcmp(TUNE[k].name, name)) return k; return -1; }
typedef struct { char name[40]; int w, h; uint8_t t[SD_H][SD_W]; SdThing th[SD_THINGS]; int nth; float tune[NTU]; uint64_t tuned;
                 SdHaz hz[SD_HAZ]; int nhz; int16_t hzc[SD_HZC][2]; int nhzc; SdTrig tr[SD_TRIGS]; int ntr; } SdLevel;
/* the crypt's traps in side view: each kind in the shape of its own (docs/PLAN-side.md, 2d) */
static int sd_haz_family(int k) {
    switch (k) {
    case HZ_ROCK: case HZ_TIMBER: case HZ_GLASS: case HZ_CRYSTAL: return HZF_FALL;
    case HZ_BILE: case HZ_SILT: case HZ_DAMP: case HZ_SLUICE: case HZ_COLD: case HZ_DCOLD: case HZ_SAINT: case HZ_TITHE: return HZF_CREEP;
    case HZ_VOID: case HZ_ASH: case HZ_BREATH: case HZ_RELIQ: case HZ_MIRROR: return HZF_CLOUD;
    default: return HZF_LINE;                                      /* Hellrent, votive wax, fissure, lodestone, galvanic, cart, censer, snare */
    }
}

static int sd_solid_ch(int c) { return c == '#' || c == '%'; }
static int sd_name(const char *s, const char *prefix, int *v) {    /* a name in drawn_text.c's table, of the kind asked */
    if (strncmp(s, prefix, strlen(prefix))) return 0;
    for (unsigned i = 0; i < sizeof DXT_NAMES / sizeof DXT_NAMES[0]; i++) if (!strcmp(DXT_NAMES[i].n, s)) { *v = DXT_NAMES[i].v; return 1; }
    return 0;
}

/* one line in words: names and numbers, or a "string" whole; ; to the line's end is a note */
#define SD_WORDS 16
#define SD_WORD (SD_W + 8)
static int sd_words(const char *p, const char *end, char w[SD_WORDS][SD_WORD]) {
    int n = 0;
    while (p < end && n < SD_WORDS) {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
        if (p >= end || *p == ';') break;
        int k = 0;
        if (*p == '"') {
            w[n][k++] = '"'; p++;
            while (p < end && *p != '"' && k < SD_WORD - 1) w[n][k++] = *p++;
            if (p < end) p++;
        } else while (p < end && *p != ' ' && *p != '\t' && *p != '\r' && *p != ';' && k < SD_WORD - 1) w[n][k++] = *p++;
        w[n][k] = 0; n++;
    }
    return n;
}

/* the text into L; 0 and the reason, with its line, if it is not one */
static int sd_read(const char *text, SdLevel *L, char *err, int nerr) {
    memset(L, 0, sizeof *L);
    memset(L->t, '.', sizeof L->t);
    snprintf(L->name, sizeof L->name, "A SIDE PASSAGE");
    int line = 0, rows = 0, version = 0;
    static char trsrc[SD_TRIGS][16], tract[SD_TRIGS][SD_ACTS][16];   /* the triggers' names, till all is read */
    char w[SD_WORDS][SD_WORD];
#define SDBAD(...) do { char m_[96]; snprintf(m_, sizeof m_, __VA_ARGS__); snprintf(err, (size_t)nerr, "line %d: %s", line, m_); return 0; } while (0)
    for (const char *p = text; *p; ) {
        const char *e = strchr(p, '\n');
        if (!e) e = p + strlen(p);
        line++;
        int n = sd_words(p, e, w);
        p = *e ? e + 1 : e;
        if (!n) continue;
        const char *op = w[0];
        int iv[8] = { 0 }; float fv[8] = { 0 };
        for (int i = 1; i < n && i < 8; i++) { iv[i] = atoi(w[i]); fv[i] = (float)atof(w[i]); }
        if (!strcmp(op, "SIDE")) { version = iv[1]; if (version != 1) SDBAD("a side passage of version %d: this game reads 1", version); continue; }
        if (!version) SDBAD("SIDE 1 must come first");
        if (!strcmp(op, "NAME")) { if (n < 2 || w[1][0] != '"') SDBAD("NAME wants a \"name\""); snprintf(L->name, sizeof L->name, "%.39s", w[1] + 1); continue; }
        if (!strcmp(op, "SIZE")) {
            if (n < 3 || iv[1] < 20 || iv[2] < 10 || iv[1] > SD_W || iv[2] > SD_H) SDBAD("SIZE wants 20 to %d across and 10 to %d down", SD_W, SD_H);
            L->w = iv[1]; L->h = iv[2];
            continue;
        }
        if (!L->w) SDBAD("SIZE must come before the rest");
        if (!strcmp(op, "ROW")) {
            if (n < 2 || w[1][0] != '"' || (int)strlen(w[1] + 1) != L->w) SDBAD("a ROW is \"...\", %d letters long", L->w);
            if (rows >= L->h) SDBAD("more ROWs than SIZE gives (%d)", L->h);
            for (int x = 0; x < L->w; x++) {
                int c = w[1][1 + x];
                if (!strchr("#%,.=^~F", c)) SDBAD("a letter the game has no tile for: %c", c);
                if (c == 'F') {                                     /* a falling block: a thing, the dark behind it */
                    if (L->nth >= SD_THINGS) SDBAD("more than %d things", SD_THINGS);
                    L->th[L->nth++] = (SdThing){ SDT_FALL, x, rows, 1, 1, 0, 0, 0, { 0.45f, 4.0f, 0.0f, 0.0f }, "" };
                    c = '.';
                }
                L->t[rows][x] = (uint8_t)c;
            }
            rows++;
            continue;
        }
        if (!strcmp(op, "TUNE")) {                                  /* a passage's own number for something tuned */
            int k = n >= 3 ? sd_tune_of(w[1]) : -1;
            if (k < 0) SDBAD("TUNE wants a name the game tunes and a number: %.30s", n >= 2 ? w[1] : "");
            L->tune[k] = clampf(fv[2], TUNE[k].lo, TUNE[k].hi); L->tuned |= 1ull << k;
            continue;
        }
        if (!strcmp(op, "HZC")) {                                   /* a cell of the trap last named, in the order painted */
            if (!L->nhz) SDBAD("HZC before any HAZ");
            if (L->nhzc >= SD_HZC) SDBAD("more than %d trap cells", SD_HZC);
            if (n < 3 || iv[1] < 0 || iv[2] < 0 || iv[1] >= L->w || iv[2] >= L->h) SDBAD("HZC wants a cell in the map");
            L->hzc[L->nhzc][0] = (int16_t)iv[1]; L->hzc[L->nhzc][1] = (int16_t)iv[2];
            L->nhzc++; L->hz[L->nhz - 1].nc++;
            continue;
        }
        if (!strcmp(op, "TRIG")) {                                  /* TRIG source MODE [secs] tag:VERB ... (resolved once all is read) */
            if (n < 4) SDBAD("TRIG wants a source, a mode and something to do");
            if (L->ntr >= SD_TRIGS) SDBAD("more than %d triggers", SD_TRIGS);
            SdTrig *g = &L->tr[L->ntr];
            memset(g, 0, sizeof *g);
            static const char *const MODES[4] = { "ONCE", "TOGGLE", "HOLD", "TIMED" };
            g->mode = -1;
            for (int k = 0; k < 4; k++) if (!strcmp(w[2], MODES[k])) g->mode = k;
            if (g->mode < 0) SDBAD("a trigger's mode is ONCE, TOGGLE, HOLD or TIMED: %.20s", w[2]);
            int i = 3;
            if (g->mode == TRM_TIMED) { g->secs = fv[3]; i = 4; if (g->secs <= 0.0f) SDBAD("TIMED wants its seconds"); }
            snprintf(trsrc[L->ntr], sizeof trsrc[0], "%.15s", w[1]);
            for (; i < n && g->nact < SD_ACTS; i++) {
                char *c = strchr(w[i], ':');
                if (!c) SDBAD("a trigger's action is tag:ON, tag:OFF, tag:FLIP or CRYPT:n: %.20s", w[i]);
                *c = 0;
                const char *verb = c + 1;
                int a = g->nact++;
                if (!strcmp(w[i], "CRYPT")) { g->target[a] = -1; g->verb[a] = TRV_CRYPT; g->arg[a] = atoi(verb); continue; }
                g->verb[a] = !strcmp(verb, "ON") ? TRV_ON : !strcmp(verb, "OFF") ? TRV_OFF : !strcmp(verb, "FLIP") ? TRV_FLIP : -1;
                if (g->verb[a] < 0) SDBAD("a trigger's verb is ON, OFF or FLIP: %.10s", verb);
                snprintf(tract[L->ntr][a], sizeof tract[0][0], "%.15s", w[i]);
            }
            if (!g->nact) SDBAD("a trigger with nothing to do");
            L->ntr++;
            continue;
        }
        static const char *const OPS[NSDT] = { "DOOR", "PLAT", "FALL", "FLOW", "MOB", "GOLD", "KEY", "LIFE", "SWITCH", "TORCH",
                                               "GATE", "PLATE", "ZONE", "CURRENT", "HAZ" };
        static const int NEED[NSDT] = { 4, 8, 3, 7, 4, 4, 4, 3, 4, 3, 5, 3, 5, 8, 4 };
        int k = 0;
        while (k < NSDT && strcmp(op, OPS[k])) k++;
        if (k == NSDT) SDBAD("a word the game doesn't know: %.40s", op);
        if (n < NEED[k]) SDBAD("%.20s wants %d more", op, NEED[k] - n);
        if (L->nth >= SD_THINGS) SDBAD("more than %d things", SD_THINGS);
        SdThing t = { k, iv[1], iv[2], 1, 1, 0, 0, 0, { 0 }, "" };
        if (k == SDT_HAZ) { t.x = iv[2]; t.y = iv[3]; }                 /* HAZ kind x y: its kind comes first */
        if (t.x < 0 || t.y < 0 || t.x >= L->w || t.y >= L->h) SDBAD("%.20s outside the map", op);
        for (int i = 3; i + 1 < n; i++) if (!strcmp(w[i], "TAG")) snprintf(t.tag, sizeof t.tag, "%.15s", w[i + 1]);   /* any thing may be named */
        for (int i = 3; i < n; i++) {                              /* and these words on any that takes them */
            if (!strcmp(w[i], "OFF")) t.flags |= SDF_OFF;
            else if (!strcmp(w[i], "LOOSE")) t.flags |= SDF_LOOSE;
            else if (!strcmp(w[i], "POISON")) t.flags |= SDF_POISON;
            else if (!strcmp(w[i], "OPEN")) t.flags |= SDF_OPEN;
            else if (!strcmp(w[i], "TAG")) i++;
        }
        switch (k) {
        case SDT_DOOR:
            t.a = iv[3];
            if (t.a < 0 || t.a > 15) SDBAD("a doorway's link is 0 to 15");
            if (t.y < 1) SDBAD("a doorway stands two tiles high: not on the top row");
            for (int i = 4; i < n; i++) if (!strcmp(w[i], "GOAL")) t.flags |= SDF_GOAL;
            break;
        case SDT_PLAT:
            t.w = iv[3]; t.a = iv[4]; t.b = iv[5]; t.f[0] = fv[6]; t.f[1] = fv[7]; t.f[3] = 0.0f;
            if (t.w < 1 || t.w > 8) SDBAD("a platform is 1 to 8 tiles wide");
            if (t.a < -1 || t.a > 1 || t.b < -1 || t.b > 1 || (!t.a && !t.b)) SDBAD("a platform's way is -1, 0 or 1 each, not both 0");
            if (t.f[0] <= 0.0f || t.f[1] <= 0.0f) SDBAD("a platform goes some way, at some speed");
            for (int i = 8; i < n; i++) {
                if (!strcmp(w[i], "LOOP")) t.flags |= SDF_LOOP;
                else if (!strcmp(w[i], "WAIT") && i + 1 < n) t.f[2] = (float)atof(w[++i]);
                else if (!strcmp(w[i], "AT") && i + 1 < n) t.f[3] = (float)atof(w[++i]);
            }
            break;
        case SDT_FALL:
            t.f[0] = 0.45f; t.f[1] = 4.0f;
            for (int i = 3; i + 1 < n; i++) {
                if (!strcmp(w[i], "DELAY")) t.f[0] = (float)atof(w[++i]);
                else if (!strcmp(w[i], "BACK")) t.f[1] = (float)atof(w[++i]);
            }
            break;
        case SDT_FLOW:
            t.w = iv[3]; t.h = iv[4]; t.f[0] = fv[5]; t.f[1] = fv[6];
            if (t.w < 1 || t.h < 1 || t.x + t.w > L->w || t.y + t.h > L->h) SDBAD("a flow's box is outside the map");
            if (t.f[0] < 0.5f || t.f[1] < 0.5f) SDBAD("a flow is full and dry half a second each at the least");
            for (int i = 7; i + 1 < n; i++) if (!strcmp(w[i], "AT")) t.f[2] = (float)atof(w[++i]);
            break;
        case SDT_MOB:
            t.a = -1; t.b = -1;
            if (strcmp(w[3], "ANY") && !sd_name(w[3], "MOB_", &t.a)) SDBAD("not a kind: %.40s", w[3]);
            if (n > 4 && strcmp(w[4], "-1") && !sd_name(w[4], "RK_", &t.b)) SDBAD("not a rank: %.40s", w[4]);
            break;
        case SDT_GOLD:
            if (!sd_name(w[3], "IT_", &t.a) || t.a > IT_CHEST) SDBAD("not gold: %.40s", w[3]);
            break;
        case SDT_KEY:
            t.a = iv[3];
            if (t.a < 0 || t.a >= NKEYS) SDBAD("a key's colour is 0 to %d", NKEYS - 1);
            break;
        case SDT_SWITCH:
            t.a = iv[3];
            if (t.a < -1 || t.a >= EV_THINGS) SDBAD("a switch's thing is -1 or 0 to %d", EV_THINGS - 1);
            for (int i = 4; i < n; i++) if (!strcmp(w[i], "TOGGLE")) t.flags |= SDF_TOGGLE;
            break;
        case SDT_GATE: case SDT_ZONE:
            t.w = iv[3]; t.h = iv[4];
            if (t.w < 1 || t.h < 1 || t.x + t.w > L->w || t.y + t.h > L->h) SDBAD("%.10s's box is outside the map", op);
            break;
        case SDT_CURRENT:
            t.w = iv[3]; t.h = iv[4]; t.a = iv[5]; t.b = iv[6]; t.f[0] = fv[7];
            if (t.w < 1 || t.h < 1 || t.x + t.w > L->w || t.y + t.h > L->h) SDBAD("a current's box is outside the map");
            if (t.a < -1 || t.a > 1 || t.b < -1 || t.b > 1 || (!t.a && !t.b) || t.f[0] <= 0.0f) SDBAD("a current runs -1, 0 or 1 each way, at some speed");
            break;
        case SDT_HAZ: {
            if (L->nhz >= SD_HAZ) SDBAD("more than %d traps", SD_HAZ);
            SdHaz *z = &L->hz[L->nhz];
            memset(z, 0, sizeof *z);
            z->kind = -1;
            if (!sd_name(w[1], "HZ_", &z->kind) || z->kind == HZ_ARC || z->kind == HZ_GRASP) SDBAD("not one of the crypt's traps: %.20s", w[1]);
            t.x = iv[2]; t.y = iv[3];
            if (t.x < 0 || t.y < 0 || t.x >= L->w || t.y >= L->h) SDBAD("HAZ outside the map");
            z->fam = sd_haz_family(z->kind); z->el = HZ_ELEM[z->kind]; z->c0 = L->nhzc;
            z->harm = z->chance = -1; z->warn = -1.0f; z->hold = 6.0f;
            for (int i = 4; i < n; i++) {
                if (!strcmp(w[i], "ZONE")) t.flags |= SDF_ZONE;
                else if (!strcmp(w[i], "SAFE")) t.flags |= SDF_SAFE;
                else if (!strcmp(w[i], "CLOCK") && i + 3 < n) { z->how |= HH_CLOCK; z->every = (float)atof(w[i + 1]); z->give = (float)atof(w[i + 2]); z->first = (float)atof(w[i + 3]); i += 3; }
                else if (!strcmp(w[i], "ENTER") && i + 4 < n) { z->how |= HH_ENTER; z->x0 = atoi(w[i + 1]); z->y0 = atoi(w[i + 2]); z->x1 = atoi(w[i + 3]); z->y1 = atoi(w[i + 4]); i += 4; }
                else if (!strcmp(w[i], "HARM") && i + 1 < n) z->harm = atoi(w[++i]);
                else if (!strcmp(w[i], "CHANCE") && i + 1 < n) z->chance = atoi(w[++i]);
                else if (!strcmp(w[i], "WARN") && i + 1 < n) z->warn = (float)atof(w[++i]);
                else if (!strcmp(w[i], "STAYS") && i + 1 < n) z->hold = (float)atof(w[++i]);
                else if (!strcmp(w[i], "TAG")) i++;
            }
            if ((z->how & HH_CLOCK) && z->every < 0.5f) SDBAD("a trap's clock wants half a second at the least");
            t.a = L->nhz++;
            break;
        }
        default: break;
        }
        L->th[L->nth++] = t;
    }
    if (!version) SDBAD("no SIDE 1");
    if (rows != L->h) SDBAD("%d ROWs, and SIZE says %d", rows, L->h);
    int doors = 0;
    for (int i = 0; i < L->nth; i++) if (L->th[i].kind == SDT_DOOR) doors++;
    if (!doors) SDBAD("no doorway: there is no way in");
    for (int k = 0; k < L->ntr; k++) {                             /* the triggers' names, now all is read */
        SdTrig *g = &L->tr[k];
        int src = -1;
        for (int i = 0; i < L->nth; i++) if (L->th[i].tag[0] && !strcmp(L->th[i].tag, trsrc[k])) src = i;
        if (src < 0 || (L->th[src].kind != SDT_PLATE && L->th[src].kind != SDT_ZONE && L->th[src].kind != SDT_SWITCH))
            { snprintf(err, (size_t)nerr, "a trigger's source is no plate, zone or switch named %s", trsrc[k]); return 0; }
        g->src = src;
        for (int a = 0; a < g->nact; a++) {
            if (g->verb[a] == TRV_CRYPT) continue;
            int t = -1;
            for (int i = 0; i < L->nth; i++) if (L->th[i].tag[0] && !strcmp(L->th[i].tag, tract[k][a])) t = i;
            if (t < 0) { snprintf(err, (size_t)nerr, "a trigger names nothing called %s", tract[k][a]); return 0; }
            g->target[a] = t;
        }
    }
#undef SDBAD
    return 1;
}
