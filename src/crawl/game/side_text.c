/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Side passages (docs/PLAN-side.md): what one is (SdLevel) and its text read (sd_read), the same text for those built in
 * (side_levels.c) and those the World Editor hands over. */

#define SD_W 256                                                   /* tiles across and down, at most */
#define SD_H 64
#define SD_THINGS 160
#define SD_TILE 16                                                 /* STS: a tile, in pixels */
enum { SDT_DOOR, SDT_PLAT, SDT_FALL, SDT_FLOW, SDT_MOB, SDT_GOLD, SDT_KEY, SDT_LIFE, SDT_SWITCH, SDT_TORCH, NSDT };
enum { SDF_GOAL = 1, SDF_LOOP = 2, SDF_TOGGLE = 4 };
/* A thing, at a tile (x, y). DOOR: a its link, GOAL; it stands on the tile below, two tiles high. PLAT: w tiles wide, (a, b) its
 * way, f0 how far (tiles), f1 how fast (tiles a second), f2 the wait at each end (s), f3 where it begins (0 to 1); LOOP: on round
 * and back to its start at once, else back and forth. FALL: f0 the shaking before it goes, f1 how long till it is back. FLOW: w by
 * h tiles, f0 seconds full, f1 seconds dry, f2 where in that it begins. MOB: a MOB_* or -1 the run's choice, b a rank or -1.
 * GOLD: a an IT_*. KEY: a its colour. SWITCH: a the crypt's thing whose USE triggers it fires; TOGGLE: thrown back and forth. */
typedef struct { int kind, x, y, w, h, a, b, flags; float f[4]; } SdThing;
typedef struct { char name[40]; int w, h; uint8_t t[SD_H][SD_W]; SdThing th[SD_THINGS]; int nth; } SdLevel;

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
                    L->th[L->nth++] = (SdThing){ SDT_FALL, x, rows, 1, 1, 0, 0, 0, { 0.45f, 4.0f, 0.0f, 0.0f } };
                    c = '.';
                }
                L->t[rows][x] = (uint8_t)c;
            }
            rows++;
            continue;
        }
        static const char *const OPS[NSDT] = { "DOOR", "PLAT", "FALL", "FLOW", "MOB", "GOLD", "KEY", "LIFE", "SWITCH", "TORCH" };
        static const int NEED[NSDT] = { 4, 8, 3, 7, 4, 4, 4, 3, 4, 3 };
        int k = 0;
        while (k < NSDT && strcmp(op, OPS[k])) k++;
        if (k == NSDT) SDBAD("a word the game doesn't know: %.40s", op);
        if (n < NEED[k]) SDBAD("%.20s wants %d more", op, NEED[k] - n);
        if (L->nth >= SD_THINGS) SDBAD("more than %d things", SD_THINGS);
        SdThing t = { k, iv[1], iv[2], 1, 1, 0, 0, 0, { 0 } };
        if (t.x < 0 || t.y < 0 || t.x >= L->w || t.y >= L->h) SDBAD("%.20s outside the map", op);
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
        default: break;
        }
        L->th[L->nth++] = t;
    }
    if (!version) SDBAD("no SIDE 1");
    if (rows != L->h) SDBAD("%d ROWs, and SIZE says %d", rows, L->h);
    int doors = 0;
    for (int i = 0; i < L->nth; i++) if (L->th[i].kind == SDT_DOOR) doors++;
    if (!doors) SDBAD("no doorway: there is no way in");
#undef SDBAD
    return 1;
}
