/* Stamps (docs/PLAN-stamps.md, track A, step A1): reading a drawn map from its text, turning it eight ways, judging it, and listing a
 * kind's stamps. Pure: no game state, no random numbers. The interface and the glyphs are in crawl_stamp.h. */
#include "crawl_stamp.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const int DXS[4] = { 1, 0, -1, 0 }, DYS[4] = { 0, 1, 0, -1 };   /* the game's DX/DY: east, south, west, north */

static void note(char *why, size_t n, const char *fmt, ...) {
    va_list ap;
    if (!why || !n) return;
    va_start(ap, fmt); vsnprintf(why, n, fmt, ap); va_end(ap);
}

static int is_mark(char c) { return c == 'a' || c == 'b' || c == 'c' || c == 's' || c == 't' || (c >= '0' && c <= '9'); }
static int in_box(const Stamp *s, int x, int y) { return x >= 0 && y >= 0 && x < s->w && y < s->h; }
static int is_floor(const Stamp *s, int x, int y) { return in_box(s, x, y) && (s->fl[y][x] & SF_FLOOR); }

/* the lane: floor at index 1 mod 4 along either side of the box, which is the middle of a block when the box starts on the plan's lattice */
static void set_lane(Stamp *s) {
    for (int y = 0; y < s->h; y++)
        for (int x = 0; x < s->w; x++) {
            s->fl[y][x] &= (uint8_t)~SF_LANE;
            if ((s->fl[y][x] & SF_FLOOR) && ((x & 3) == 1 || (y & 3) == 1)) s->fl[y][x] |= SF_LANE;
        }
}

/* which way out of the box a cell on its edge faces: 0..3, or 4 if it is not on exactly one edge. A port in a corner is on two; it faces the
 * way its run of ports does not run (a row of ports along the top faces north, whatever corner it ends in) */
static uint8_t edge_way(const Stamp *s, int x, int y) {
    int n = 0; uint8_t d = 4;
    if (x == s->w - 1) { n++; d = 0; }
    if (y == s->h - 1) { n++; d = 1; }
    if (x == 0) { n++; d = 2; }
    if (y == 0) { n++; d = 3; }
    if (n == 1) return d;
    if (n == 2 && (s->fl[y][x] & SF_PORT)) {
        int along_x = (x > 0 && (s->fl[y][x - 1] & SF_PORT)) || (x + 1 < s->w && (s->fl[y][x + 1] & SF_PORT));
        int along_y = (y > 0 && (s->fl[y - 1][x] & SF_PORT)) || (y + 1 < s->h && (s->fl[y + 1][x] & SF_PORT));
        if (along_x != along_y) {
            if (along_x) return y == 0 ? 3 : (y == s->h - 1 ? 1 : 4);
            return x == 0 ? 2 : (x == s->w - 1 ? 0 : 4);
        }
    }
    return 4;
}

/* the way from a floor cell beside (x,y) into it, or 4 if none of its four neighbours is floor */
static uint8_t way_into(const Stamp *s, int x, int y) {
    for (int k = 0; k < 4; k++) if (is_floor(s, x + DXS[k], y + DYS[k])) return (uint8_t)((k + 2) & 3);
    return 4;
}

int stamp_parse(const StampDef *def, int index, Stamp *out, char *why, size_t whylen) {
    memset(out, 0, sizeof *out);
    if (!def || !def->name || !def->rows) { note(why, whylen, "no definition"); return -1; }
    size_t nl = strlen(def->name);
    if (!nl || nl >= STAMP_NAME) { note(why, whylen, "%s: the name must be 1 to %d letters", def->name ? def->name : "?", STAMP_NAME - 1); return -1; }
    if (def->kind >= ST_KINDS) { note(why, whylen, "%s: unknown kind %d", def->name, def->kind); return -1; }
    if (!def->w || !def->h || def->w > STAMP_MAXW || def->h > STAMP_MAXH) { note(why, whylen, "%s: %dx%d is not 1 to %dx%d", def->name, def->w, def->h, STAMP_MAXW, STAMP_MAXH); return -1; }
    if (def->minstep > def->maxstep || def->maxstep > 9) { note(why, whylen, "%s: steps %d to %d are not within 0 to 9", def->name, def->minstep, def->maxstep); return -1; }
    if (def->weight < 1 || def->weight > 9) { note(why, whylen, "%s: weight %d is not 1 to 9", def->name, def->weight); return -1; }
    memcpy(out->name, def->name, nl);
    out->kind = def->kind; out->w = def->w; out->h = def->h;
    out->minstep = def->minstep; out->maxstep = def->maxstep; out->weight = def->weight; out->runs = def->runs;
    out->def = (int16_t)index; out->xf = 0; out->norient = 1;
    const char *p = def->rows;
    for (int y = 0; y < out->h; y++) {
        for (int x = 0; x < out->w; x++, p++) {
            char c = *p;
            if (!c || c == '\n') { note(why, whylen, "%s: row %d has %d glyphs, not %d", def->name, y + 1, x, out->w); return -1; }
            uint8_t f = 0;
            if (c == '#') { }
            else if (c == '.') f = SF_FLOOR;
            else if (c == 'P' || c == 'p') f = SF_FLOOR | SF_PORT;
            else if (c == 'E') f = SF_ENTRY;
            else if (c == 'D') f = SF_DOOR;
            else if (is_mark(c)) f = SF_FLOOR;
            else { note(why, whylen, "%s: unknown glyph '%c' at row %d, column %d", def->name, c, y + 1, x + 1); return -1; }
            out->g[y][x] = c; out->fl[y][x] = f;
        }
        if (*p == '\n') p++;
        else if (y < out->h - 1) { note(why, whylen, "%s: row %d is longer than %d glyphs", def->name, y + 1, out->w); return -1; }
    }
    if (*p) { note(why, whylen, "%s: extra text after row %d", def->name, out->h); return -1; }
    for (int y = 0; y < out->h; y++)                               /* the lists, once every cell is known (an E or D looks at its neighbours) */
        for (int x = 0; x < out->w; x++) {
            char c = out->g[y][x];
            if (out->fl[y][x] & SF_FLOOR) out->nfloor++;
            if (c == 'P' || c == 'p') {
                if (out->nport >= STAMP_MAXPORTS) { note(why, whylen, "%s: more than %d port cells", def->name, STAMP_MAXPORTS); return -1; }
                out->port[out->nport++] = (StampPort){ (int8_t)x, (int8_t)y, edge_way(out, x, y), (uint8_t)(c == 'P') };
            } else if (c == 'E') {
                if (out->nentry >= STAMP_MAXENTR) { note(why, whylen, "%s: more than %d entrances", def->name, STAMP_MAXENTR); return -1; }
                out->entry[out->nentry++] = (StampSpot){ (int8_t)x, (int8_t)y, (int8_t)way_into(out, x, y) };
            } else if (c == 'D') {
                if (out->ndoor >= STAMP_MAXENTR) { note(why, whylen, "%s: more than %d doorways", def->name, STAMP_MAXENTR); return -1; }
                out->door[out->ndoor++] = (StampSpot){ (int8_t)x, (int8_t)y, (int8_t)way_into(out, x, y) };
            } else if (is_mark(c)) {
                if (out->nmark >= STAMP_MAXMARKS) { note(why, whylen, "%s: more than %d marks", def->name, STAMP_MAXMARKS); return -1; }
                out->mark[out->nmark++] = (StampMark){ (int8_t)x, (int8_t)y, c };
            }
        }
    set_lane(out);
    return 0;
}

/* ---------------------------------------------------------------- turning */

static uint8_t flip_way(uint8_t d) { return d == 0 ? 2 : d == 2 ? 0 : d; }   /* a way (4 stays 4) in a mirror in x */
static uint8_t turn_way(uint8_t d) { return d < 4 ? (uint8_t)((d + 1) & 3) : d; }   /* and a quarter turn clockwise */

static void mirror_x(const Stamp *in, Stamp *out) {
    *out = *in;
    for (int y = 0; y < in->h; y++)
        for (int x = 0; x < in->w; x++) { out->g[y][in->w - 1 - x] = in->g[y][x]; out->fl[y][in->w - 1 - x] = in->fl[y][x]; }
    for (int i = 0; i < in->nport; i++) { out->port[i].x = (int8_t)(in->w - 1 - in->port[i].x); out->port[i].d = flip_way(in->port[i].d); }
    for (int i = 0; i < in->nentry; i++) { out->entry[i].x = (int8_t)(in->w - 1 - in->entry[i].x); out->entry[i].d = (int8_t)flip_way((uint8_t)in->entry[i].d); }
    for (int i = 0; i < in->ndoor; i++) { out->door[i].x = (int8_t)(in->w - 1 - in->door[i].x); out->door[i].d = (int8_t)flip_way((uint8_t)in->door[i].d); }
    for (int i = 0; i < in->nmark; i++) out->mark[i].x = (int8_t)(in->w - 1 - in->mark[i].x);
}

static void turn_cw(const Stamp *in, Stamp *out) {                /* (x, y) in w x h goes to (h - 1 - y, x) in h x w */
    *out = *in;
    out->w = in->h; out->h = in->w;
    memset(out->g, 0, sizeof out->g); memset(out->fl, 0, sizeof out->fl);
    for (int y = 0; y < in->h; y++)
        for (int x = 0; x < in->w; x++) { out->g[x][in->h - 1 - y] = in->g[y][x]; out->fl[x][in->h - 1 - y] = in->fl[y][x]; }
    for (int i = 0; i < in->nport; i++) { out->port[i].x = (int8_t)(in->h - 1 - in->port[i].y); out->port[i].y = in->port[i].x; out->port[i].d = turn_way(in->port[i].d); }
    for (int i = 0; i < in->nentry; i++) { out->entry[i].x = (int8_t)(in->h - 1 - in->entry[i].y); out->entry[i].y = in->entry[i].x; out->entry[i].d = (int8_t)turn_way((uint8_t)in->entry[i].d); }
    for (int i = 0; i < in->ndoor; i++) { out->door[i].x = (int8_t)(in->h - 1 - in->door[i].y); out->door[i].y = in->door[i].x; out->door[i].d = (int8_t)turn_way((uint8_t)in->door[i].d); }
    for (int i = 0; i < in->nmark; i++) { out->mark[i].x = (int8_t)(in->h - 1 - in->mark[i].y); out->mark[i].y = in->mark[i].x; }
}

void stamp_xform(const Stamp *in, int t, Stamp *out) {
    Stamp a, b;
    t &= 7;
    a = *in;                                                       /* a copy, so out may be in */
    if (t & 1) { mirror_x(&a, &b); a = b; }
    for (int q = (t >> 1) & 3; q > 0; q--) { turn_cw(&a, &b); a = b; }
    a.xf = (uint8_t)t;                                             /* t is from the drawing: call this on a stamp as parsed */
    set_lane(&a);
    *out = a;
}

/* ---------------------------------------------------------------- judging */

int stamp_valid(const Stamp *s, char *why, size_t whylen) {
    int bad = 0;
#define BAD(...) do { if (!bad++) note(why, whylen, __VA_ARGS__); } while (0)
    if (why && whylen) why[0] = 0;
    if (s->nfloor < 1) BAD("%s: no floor", s->name);

    /* the floor is one region */
    {
        static uint8_t seen[STAMP_MAXH][STAMP_MAXW];
        static int qx[STAMP_MAXW * STAMP_MAXH], qy[STAMP_MAXW * STAMP_MAXH];
        int head = 0, tail = 0, reached = 0;
        memset(seen, 0, sizeof seen);
        for (int y = 0; y < s->h && !tail; y++)
            for (int x = 0; x < s->w && !tail; x++)
                if (is_floor(s, x, y)) { seen[y][x] = 1; qx[tail] = x; qy[tail++] = y; }
        while (head < tail) {
            int x = qx[head], y = qy[head++];
            reached++;
            for (int k = 0; k < 4; k++) {
                int nx = x + DXS[k], ny = y + DYS[k];
                if (is_floor(s, nx, ny) && !seen[ny][nx]) { seen[ny][nx] = 1; qx[tail] = nx; qy[tail++] = ny; }
            }
        }
        if (s->nfloor > 0 && reached != s->nfloor) BAD("%s: the floor is not one region (%d of %d cells reached from the first)", s->name, reached, s->nfloor);
    }

    /* what each kind has and has not */
    if (s->kind == ST_SECRET) {
        if (s->nport) BAD("%s: a secret room has no ports", s->name);
        if (!s->nentry) BAD("%s: a secret room needs an entrance (E)", s->name);
    } else {
        if (s->nentry || s->ndoor) BAD("%s: only a secret room has E or D", s->name);
        if ((s->w + 1) % 4 || (s->h + 1) % 4) BAD("%s: %dx%d is not on the lattice (3, 7, 11 or 15 cells a side)", s->name, s->w, s->h);
        if (s->kind == ST_JOINER && s->nport < 2) BAD("%s: a joiner needs two ports", s->name);
        if (s->kind == ST_PLACE && s->nport < 1) BAD("%s: a place needs a port", s->name);
    }

    /* ports: on one edge, facing out, in runs that lie in one block and take in its middle */
    {
        uint8_t pm[4][STAMP_MAXW > STAMP_MAXH ? STAMP_MAXW : STAMP_MAXH];
        memset(pm, 0, sizeof pm);
        for (int i = 0; i < s->nport; i++) {
            const StampPort *p = &s->port[i];
            if (p->d > 3) { BAD("%s: the port at (%d,%d) is not on exactly one edge", s->name, p->x, p->y); continue; }
            if (edge_way(s, p->x, p->y) != p->d) { BAD("%s: the port at (%d,%d) does not face its edge", s->name, p->x, p->y); continue; }
            pm[p->d][(p->d & 1) ? p->x : p->y] = (uint8_t)(p->must ? 2 : 1);
        }
        for (int d = 0; d < 4; d++) {
            int len = (d & 1) ? s->w : s->h;
            for (int a = 0; a < len; a++) {
                if (!pm[d][a] || (a > 0 && pm[d][a - 1])) continue;   /* the start of a run */
                int e = a, has_mid = 0;
                while (e + 1 < len && pm[d][e + 1]) e++;
                for (int k = a; k <= e; k++) { has_mid |= (k & 3) == 1; if (pm[d][k] != pm[d][a]) BAD("%s: a port's run on edge %d mixes P and p", s->name, d); }
                if (a / 4 != e / 4 || (e & 3) == 3) BAD("%s: the port run %d..%d on edge %d is not within one block (3 cells wide, 4 apart)", s->name, a, e, d);
                else if (!has_mid) BAD("%s: the port run %d..%d on edge %d misses its block's middle cell (1 mod 4)", s->name, a, e, d);
            }
        }
    }

    /* entrances and doorways: stone outside the room, touching its floor */
    for (int i = 0; i < s->nentry; i++) {
        const StampSpot *e = &s->entry[i];
        if (e->d < 0 || e->d > 3) { BAD("%s: the entrance at (%d,%d) touches no floor", s->name, e->x, e->y); continue; }
        if (in_box(s, e->x + DXS[e->d], e->y + DYS[e->d])) BAD("%s: the entrance at (%d,%d) has no open side outside the box", s->name, e->x, e->y);
    }
    for (int i = 0; i < s->ndoor; i++) {
        const StampSpot *e = &s->door[i];
        if (e->d < 0 || e->d > 3) { BAD("%s: the doorway at (%d,%d) touches no floor", s->name, e->x, e->y); continue; }
        int bx = e->x + DXS[e->d], by = e->y + DYS[e->d];
        if (in_box(s, bx, by) && s->g[by][bx] != '#') BAD("%s: the doorway at (%d,%d) has no solid rock behind it", s->name, e->x, e->y);
    }
#undef BAD
    return bad;
}

int stamp_same(const Stamp *a, const Stamp *b) {
    if (a->w != b->w || a->h != b->h) return 0;
    for (int y = 0; y < a->h; y++) if (memcmp(a->g[y], b->g[y], a->w)) return 0;
    return 1;
}

int stamp_set(int kind, Stamp *out, int max) {
    int n = 0;
    for (int i = 0; i < NSTAMP_DEFS; i++) {
        if (kind >= 0 && STAMP_DEFS[i].kind != kind) continue;
        Stamp base, t[8];
        if (stamp_parse(&STAMP_DEFS[i], i, &base, 0, 0)) continue;
        int ok = 1, kept[8], nk = 0;
        for (int x = 0; x < 8 && ok; x++) {
            stamp_xform(&base, x, &t[x]);
            t[x].xf = (uint8_t)x;
            if (stamp_valid(&t[x], 0, 0)) ok = 0;
        }
        if (!ok) continue;
        for (int x = 0; x < 8; x++) {
            int dup = 0;
            for (int k = 0; k < nk && !dup; k++) dup = stamp_same(&t[kept[k]], &t[x]);
            if (!dup) kept[nk++] = x;
        }
        if (n + nk > max) return -1;
        for (int k = 0; k < nk; k++) { out[n] = t[kept[k]]; out[n].norient = (uint8_t)nk; n++; }
    }
    return n;
}

int stamp_weight(const Stamp *s, int step, int run) {
    if (step < s->minstep || step > s->maxstep) return 0;
    if (s->runs && !((s->runs >> run) & 1)) return 0;
    return s->weight * 840 / (s->norient ? s->norient : 1);
}
