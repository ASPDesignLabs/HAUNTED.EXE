/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Ways to touch the world: things carried, set down and used (thing_use). */

/* ---- ways to touch the world (1.0.0b13, docs/PLAN-b13.md section 6) ----
 * What a puzzle lays out: things taken and set down, places that take them, what turns, levers, lights,
 * what is struck, flagstones that sink, carvings to read. All with E but the stepping, and nothing a spell
 * or an ability does ever works them. One thing is carried at a time, and you may cast while you carry it;
 * a death, a pocket or the level's end puts it back where it was taken from. */
static int thing_new(int role, float x, float y, int spr) {
    if (G.nthing >= MAXTHING) return -1;
    Thing *t = &G.thing[G.nthing];
    memset(t, 0, sizeof *t);
    t->role = (uint8_t)role; t->x = x; t->y = y; t->spr = (int16_t)spr; t->in = t->home = -1; t->wall = 0; t->nstate = 2;
    t->cx = (int)x; t->cy = (int)y; t->scale = 1.0f; t->t = 9.0f; t->ramp = -1;
    return G.nthing++;
}
static int thing_wall(int role, int cx, int cy, int d, int spr) {  /* against the wall to the d of (cx, cy), facing into it */
    int i = thing_new(role, cx + 0.5f + DX[d] * 0.4f, cy + 0.5f + DY[d] * 0.4f, spr);
    if (i >= 0) { G.thing[i].wall = (uint8_t)(d + 1); G.thing[i].cx = cx; G.thing[i].cy = cy; }
    return i;
}
static void thing_pile(int item, int place);
static void thing_put(int item, int place) {                       /* an item into a place: drawn where the place holds it */
    Thing *it = &G.thing[item], *p = &G.thing[place];
    if (p->cap > 1) { thing_pile(item, place); return; }
    it->in = (int8_t)place; p->in = (int8_t)item; it->below = -1;
    it->x = p->x; it->y = p->y; it->cx = p->cx; it->cy = p->cy; it->z = p->z + p->hold;
}
static void thing_drop_home(void) {                                /* what was carried goes back where it was taken from */
    if (!G.carry) return;
    int i = G.carry - 1;
    Thing *t = &G.thing[i];
    G.carry = 0;
    if (t->home >= 0 && G.thing[t->home].in < 0) thing_put(i, t->home);
    else { t->x = t->hx; t->y = t->hy; t->cx = (int)t->hx; t->cy = (int)t->hy; t->z = 0.0f; t->in = -1; }
}
/* what E would touch: a thing within reach, near the middle of the view, in sight */
static int thing_facing(void) {
    int best = -1;
    float bs = 1e9f;
    for (int i = 0; i < G.nthing; i++) {
        const Thing *t = &G.thing[i];
        if (t->role == TR_PLATE || G.carry == i + 1 || (t->role == TR_ITEM && t->in >= 0)) continue;   /* an item in its place: through the place */
        float d = hypotf(t->x - G.px, t->y - G.py);
        if (d > 1.7f) continue;
        float a = fabsf(wrap_pi(atan2f(t->y - G.py, t->x - G.px) - G.ang));
        if (a > 0.45f + 0.35f / fmaxf(d, 0.4f)) continue;
        float ex = t->x, ey = t->y;
        if (t->wall) { ex -= DX[t->wall - 1] * 0.15f; ey -= DY[t->wall - 1] * 0.15f; }
        if (!line_clear(G.px, G.py, ex, ey)) continue;
        float sc = d + a * 1.5f;
        if (sc < bs) { bs = sc; best = i; }
    }
    return best;
}
static void thing_say(const Thing *t, const char *fmt) {           /* a caption with the thing's name in it */
    if (t->name[0]) say(fmt, t->name);
}
/* E at a thing: by what it is. Returns 1 if anything happened */
static int thing_use(int i) {
    Thing *t = &G.thing[i];
    float x = t->x, y = t->y;
    if (t->slow && G.work_i != i + 1) {                            /* a second's work first (a wheel, a prop): update_work */
        G.work_i = i + 1; G.work_t = t->slow >= 5 ? (float)t->slow / 10.0f : 1.0f;   /* (in tenths of a second) */
        thing_say(t, "%s: A MOMENT'S WORK");
        sfx_at(SFX_PUSHWALL, x, y, 0.3f);
        return 1;
    }
    if (t->slow) G.work_i = 0;
    t->t = 0.0f;
    switch (t->role) {
    case TR_ITEM:
        if (G.carry) { say("YOUR HANDS ARE FULL"); snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f); return 1; }
        G.carry = i + 1; t->home = t->in; t->hx = x; t->hy = y; t->in = -1;
        thing_say(t, "YOU TAKE %s");
        sfx_at(SFX_BONES, x, y, 0.4f);
        pz_touched(i, TU_TAKE);
        return 1;
    case TR_PLACE:
        if (G.carry) {
            int it = G.carry - 1;
            if (t->in >= 0 && (t->cap <= 1 || pile_count(i) >= t->cap)) { say("THERE IS NO MORE ROOM THERE"); snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f); return 1; }
            if (!pz_takes(i, it)) { say("IT WILL NOT GO THERE"); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 1; }
            G.carry = 0;
            thing_put(it, i);
            sfx_at(SFX_KNOCK, x, y, 0.5f); sfx_at(SFX_BONES, x, y, 0.3f);
            pz_touched(it, TU_SET);
            return 1;
        }
        if (t->in >= 0) {                                          /* take back what it holds (a pile's top) */
            int it = t->in;
            Thing *o = &G.thing[it];
            t->in = o->below >= 0 && t->cap > 1 ? o->below : -1; o->below = -1; o->in = -1; o->home = (int8_t)i; o->hx = o->x; o->hy = o->y;
            G.carry = it + 1;
            thing_say(o, "YOU TAKE %s");
            sfx_at(SFX_BONES, x, y, 0.4f);
            pz_touched(it, TU_TAKE);
            return 1;
        }
        if (t->text[0]) { say("%s", t->text); doc_add(t->name, t->text, 0); }   /* a saint's line, a socket's shape */
        else thing_say(t, "%s, EMPTY");
        return 1;
    case TR_TURN:
        t->state = (uint8_t)((t->state + 1) % imax(1, t->nstate));
        sfx_at(SFX_PUSHWALL, x, y, 0.25f); sfx_at(SFX_KNOCK, x, y, 0.4f);
        pz_touched(i, TU_TURN);
        return 1;
    case TR_LEVER:
        t->state ^= 1;
        sfx_at(SFX_UNLOCK, x, y, 0.6f);
        pz_touched(i, TU_PULL);
        return 1;
    case TR_LIGHT:
        t->state ^= 1;
        sfx_at(t->state ? SFX_IGNITE : SFX_WHOOSH, x, y, 0.5f);
        pz_touched(i, TU_LIGHT);
        return 1;
    case TR_STRIKE:
        sfx_at(SFX_MIRRORBELL, x, y, 0.35f + 0.1f * (float)(t->val % 4));
        pz_touched(i, TU_STRIKE);
        return 1;
    case TR_READ:
        snprintf(G.read_text, sizeof G.read_text, "%s", t->text);
        G.read_t = 6.0f + 0.04f * (float)strlen(t->text);
        doc_add(t->name, t->text, 0);
        sfx_at(SFX_WHISPER, x, y, 0.15f);
        pz_touched(i, TU_READ);
        return 1;
    default: return 0;
    }
}
/* E with something in your hands and nothing to put it in: set down on the floor before you */
static int thing_set_down(void) {
    if (!G.carry) return 0;
    int i = G.carry - 1;
    float x = G.px + cosf(G.ang) * 0.45f, y = G.py + sinf(G.ang) * 0.45f;
    if (cell((int)x, (int)y)->kind != K_FLOOR) { x = G.px; y = G.py; }
    Thing *t = &G.thing[i];
    G.carry = 0;
    t->x = x; t->y = y; t->cx = (int)x; t->cy = (int)y; t->z = 0.0f; t->in = -1;
    sfx_at(SFX_BONES, x, y, 0.3f);
    pz_touched(i, TU_SET);
    return 1;
}
static void update_things(float dt) {
    if (G.read_t > 0.0f) G.read_t -= dt;
    if (!G.nthing || PK.depth || G.shop) return;
    int px = (int)G.px, py = (int)G.py;
    for (int i = 0; i < G.nthing; i++) {
        Thing *t = &G.thing[i];
        t->t += dt;
        if (G.carry == i + 1) { t->x = G.px; t->y = G.py; t->cx = px; t->cy = py; }
        if (t->role == TR_PLATE) {                                 /* a flagstone sinks under whoever stands on it */
            int on = !G.dead && px == t->cx && py == t->cy;
            if (on && !t->on) { t->on = 1; t->t = 0.0f; pz_touched(i, TU_STEP); }
            else if (!on) t->on = 0;
        }
    }
}
#ifdef CRAWL_DEBUG
/* testing: one of each, near the start, to touch by hand (CRAWL_THINGS=1) */
static void things_test(void) {
    int sx = G.cx, sy = G.cy, k = 0;
    static const int ROLES[] = { TR_ITEM, TR_PLACE, TR_TURN, TR_LEVER, TR_LIGHT, TR_STRIKE, TR_PLATE, TR_READ };
    for (int r = 1; r < 5 && k < 8; r++)
        for (int d = 0; d < 4 && k < 8; d++) {
            int x = sx + DX[d] * r, y = sy + DY[d] * r;
            if (!hz_floor(x, y)) continue;
            int i = thing_new(ROLES[k], x + 0.5f, y + 0.5f, -1);
            Thing *t = &G.thing[i];
            static const char *const NM[] = { "A SKULL", "A PLINTH", "A DIAL", "A LEVER", "A CANDLE", "A BELL", "A FLAGSTONE", "A CARVING" };
            snprintf(t->name, sizeof t->name, "%s", NM[k]);
            t->spr = (int16_t)(k == 0 ? SP_PZ_SKULL : k == 1 ? SP_PZ_PLINTH : k == 2 ? SP_PZ_DIAL0 : k == 3 ? SP_PZ_LEVER0 : k == 4 ? SP_PZ_CANDLE0 : k == 5 ? SP_BELL : k == 6 ? -1 : SP_PZ_TABLET);
            t->frames = (uint8_t)(k == 2 ? 8 : k == 3 || k == 4 ? 2 : 1);
            if (k == 0) t->mark = 1;
            if (k == 1) t->hold = 0.18f;
            if (k == 2) t->nstate = 8;
            if (k == 7) snprintf(t->text, sizeof t->text, "HERE LIE THE BROTHERS OF THE ORDER, WHO KEPT THE BELLS AND THE BONES");
            k++;
        }
}
#endif

static void ach_earn(int bit, int spr, int family, int rank, const char *title, const char *desc);
static void rec_puzzle(void) {                                     /* the record: a puzzle solved, and its kind */
    if (RUN.deed_pz < 65535) RUN.deed_pz++;
    RUN.pz_kinds |= 1u << G.pz;
    char d[32];
    for (int k = 0; k < 3; k++) if (RUN.deed_pz >= PZ_STEP[k]) { snprintf(d, sizeof d, "%d PUZZLE%s SOLVED", PZ_STEP[k], PZ_STEP[k] > 1 ? "S" : ""); ach_earn(ACH_DEED0 + k, SP_PZ_LEVER1, -1, 0, PZ_ACH[k], d); }
}

