/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A drawn level's events (docs/PLAN-events.md): triggers heard where the game already acts, their actions done in order and undone;
 * the torches' light and look; the dream at them. The level's own data and its laying are drawn.c's (EV). */

static int ev_live(void) { return g_events && g_drawn && !PK.depth && !G.shop; }
static int ev_ntrig(void) { return imin(drawn()->ntrig, EV_TRIGS); }
static int ev_ready(int k) {                                       /* every trigger it waits on has fired */
    unsigned need = drawn()->trig[k].needs;
    for (int j = 0; j < ev_ntrig(); j++) if ((need >> j & 1) && !EV.tr[j].n) return 0;
    return 1;
}
static int ev_trigs_ready(int kind, int thing) {                   /* every trigger of this kind on the thing could fire (or there is none) */
    const Drawn *D = drawn();
    for (int k = 0; k < ev_ntrig(); k++) if (D->trig[k].kind == kind && D->trig[k].thing == thing && !ev_ready(k)) return 0;
    return 1;
}
static int ev_undo_of(int kind) {                                  /* an action's way back, as the editor's UNDO; -1 none */
    switch (kind) {
    case TA_FIELD_ON: return TA_FIELD_OFF;
    case TA_FIELD_OFF: return TA_FIELD_ON;
    case TA_FLICKER_ON: return TA_FLICKER_OFF;
    case TA_FLICKER_OFF: return TA_FLICKER_ON;
    case TA_LIGHT_ON: return TA_LIGHT_OFF;
    case TA_LIGHT_OFF: return TA_LIGHT_ON;
    case TA_DOOR_OPEN: case TA_DOOR_UNLOCK: return TA_DOOR_SHUT;
    case TA_DOOR_SHUT: return TA_DOOR_OPEN;
    default: return -1;
    }
}
#ifdef CRAWL_DEBUG
static const char *const TA_NAME[] = { "flicker_on", "flicker_off", "light_on", "light_off", "flicker_all", "field_on", "field_off",
                                       "door_open", "door_unlock", "door_shut", "wake", "secret", "line", "sound" };
static const char *const TG_NAME[] = { "pickup", "place", "strike", "enter", "use", "step", "all" };
#endif

/* ---- the torches: lit, flickering to life (cold, gutter, cold, gutter, lit, over about half a second, the group's one after
 * another), flickering out (lit, gutter, lit, gutter, then snuffed a second and cold), or cold; every light stuttering a moment ---- */
enum { TL_LIT, TL_GUTTER, TL_SNUFFED, TL_COLD };
#define EV_STEP 0.12f                       /* a beat of the flicker */
#define EV_NEXT 0.15f                       /* the next torch of the group after this long */
static int ev_torch_look(int i) {
    const EvTorch *T = &EV.torch[i];
    float t = T->t;
    if (T->anim == EA_IN) return t < 0.0f ? TL_COLD : t < 4.0f * EV_STEP ? (((int)(t / EV_STEP) & 1) ? TL_GUTTER : TL_COLD) : TL_LIT;
    if (T->anim == EA_OUT) return t < 0.0f ? TL_LIT : t < 4.0f * EV_STEP ? (((int)(t / EV_STEP) & 1) ? TL_GUTTER : TL_LIT) : t < 4.0f * EV_STEP + 1.0f ? TL_SNUFFED : TL_COLD;
    return T->lit ? TL_LIT : TL_COLD;
}
static float ev_all_k(void) {                                      /* every light, stuttering: full, or low a beat at a time */
    if (EV.all_t <= 0.0f) return 1.0f;
    uint32_t h = (uint32_t)(int)(EV.now * 14.0f) * 2654435761u;
    int b = (int)(h >> 13 & 3);
    return b == 0 ? 0.12f : b == 1 ? 0.45f : 1.0f;
}
static float ev_light(int l) {                                     /* build_lightmap: a light's share this frame */
    float k = ev_all_k();
    int t = l >= 0 && l < MAXLIGHTS ? EV.light_of[l] : 0;
    if (t) { static const float SHARE[4] = { 1.0f, 0.33f, 0.08f, 0.0f }; k *= SHARE[ev_torch_look(t - 1)]; }
    return k;
}
static int ev_torch_tex(int mx, int my, int tframe) {              /* render_walls: what hangs on an event's torch's face */
    int i = EV.at_torch[my][mx] - 1, look = ev_torch_look(i);
    if (look == TL_LIT && ev_all_k() < 0.5f) look = TL_GUTTER;     /* the stutter shows in the flame too */
    switch (look) {
    case TL_LIT: return T_FEAT + F1_TORCH + torch_frame(mx, my, tframe);
    case TL_GUTTER: return T_UNLIT + UL_GUTTER0 + (int)(((uint32_t)(int)(G.t * 15.0) + hash2(mx, my)) & 1);
    case TL_SNUFFED: return T_UNLIT + UL_SNUFFED;
    default: return T_UNLIT + UL_COLD;
    }
}
static void ev_torches(int group, int kind) {
    int nth = 0, first = -1;
    for (int i = 0; i < EV.nt; i++) {
        EvTorch *T = &EV.torch[i];
        if (T->group != group) continue;
        switch (kind) {
        case TA_FLICKER_ON: if (T->lit) continue; T->lit = 1; T->anim = EA_IN; T->t = -EV_NEXT * (float)nth++; break;
        case TA_FLICKER_OFF: if (!T->lit) continue; T->lit = 0; T->anim = EA_OUT; T->t = -EV_NEXT * (float)nth++; break;
        case TA_LIGHT_ON: T->lit = 1; T->anim = EA_NONE; break;
        default: T->lit = 0; T->anim = EA_NONE; break;
        }
        if (first < 0) first = i;
    }
    if (first >= 0) {                                              /* heard at the first of them */
        const EvTorch *T = &EV.torch[first];
        float x = T->wx + 0.5f + DX[T->d] * 0.6f, y = T->wy + 0.5f + DY[T->d] * 0.6f;
        sfx_at(kind == TA_FLICKER_ON || kind == TA_LIGHT_ON ? SFX_IGNITE : SFX_WHOOSH, x, y, 0.6f);
    }
}

/* ---- the fields on and off (drawn.c ticks them) and the doors ---- */
static void ev_field(int f, int on) {
    if (f < 0 || f >= EV.nf || EV.f[f].on == on) return;
    EvField *F = &EV.f[f];
    F->on = on; F->t = F->kind == DF_ICE ? 0.0f : 99.0f;
    if (!F->n) return;
    float x = EV.fx[F->first + F->n / 2] + 0.5f, y = EV.fy[F->first + F->n / 2] + 0.5f;
    if (F->kind == DF_FIRE) sfx_at(on ? SFX_IGNITE : SFX_HISS, x, y, 1.0f);
    else if (on) sfx_at(SFX_FROST, x, y, 1.1f);
    else { sfx_at(SFX_HISS, x, y, 1.1f); sfx_at(SFX_SHATTER, x, y, 0.5f); }
}
static void ev_door(int x, int y, int kind) {
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN || G.map[y][x].kind != K_DOOR) return;
    int i = G.map[y][x].door;
    Door *d = &G.doors[i];
    if (kind == TA_DOOR_SHUT) {                                    /* shut and barred: a portcullis now, whatever it was */
        d->locked = GATED; d->opening = 0; d->idle = 99.0f; g_gate[i] = 1;
        sfx_at(SFX_RUMBLE, x + 0.5f, y + 0.5f, 1.0f);
        return;
    }
    d->locked = d->locked == 1 || d->locked == 2 ? 2 : 0;          /* a vault's, unlocked; anything else, free */
    if (kind == TA_DOOR_OPEN) { d->opening = 1; sfx_at(g_gate[i] ? SFX_RUMBLE : SFX_UNLOCK, x + 0.5f, y + 0.5f, 1.0f); }
    else sfx_at(SFX_UNLOCK, x + 0.5f, y + 0.5f, 0.8f);
}

/* ---- the runner: actions wait out their delays in a queue, in the order they were given ---- */
static void ev_queue(int a, int trig, int kind, float after, int back);
static void ev_do(int a, int kind) {
    const Drawn *D = drawn();
    const DrawnAct *A = &D->act[a];
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ev_act %d %s arg %d (%d,%d) t=%.2f\n", a, kind >= 0 && kind <= TA_SOUND ? TA_NAME[kind] : "?", A->arg, A->x, A->y, G.t);
#endif
    switch (kind) {
    case TA_FLICKER_ON: case TA_FLICKER_OFF: case TA_LIGHT_ON: case TA_LIGHT_OFF: ev_torches(A->arg, kind); break;
    case TA_FLICKER_ALL: EV.all_t = 1.1f; snd_sfx_ex(SFX_WHOOSH, 0.5f, 0.5f); break;
    case TA_FIELD_ON: case TA_FIELD_OFF: ev_field(A->arg, kind == TA_FIELD_ON); break;
    case TA_DOOR_OPEN: case TA_DOOR_UNLOCK: case TA_DOOR_SHUT: ev_door(D->ox + A->x, D->oy + A->y, kind); break;
    case TA_WAKE: drawn_fill(A->arg + 1); break;
    case TA_SECRET:                                                /* the first not yet found slides, as if pressed */
        for (int i = 0; i < G.nsecrets; i++) {
            if (G.secrets[i].state) continue;
            if (G.slide >= 0) ev_queue(a, -1, kind, 0.5f, 1);      /* another still moving: in a moment */
            else open_secret(i);
            break;
        }
        break;
    case TA_LINE: if (A->text && A->text[0]) say("%s", A->text); break;
    case TA_SOUND: if (A->arg >= 0 && A->arg < SFX_COUNT) snd_sfx_ex(A->arg, 1.0f, 0.5f); break;
    default: break;
    }
}
static void ev_run(void) {                                         /* what is due, in order */
    for (int i = 0; i < EV.nq; ) {
        if (EV.q[i].due > EV.now) { i++; continue; }
        int a = EV.q[i].act, kind = EV.q[i].kind;
        memmove(&EV.q[i], &EV.q[i + 1], (size_t)(EV.nq - i - 1) * sizeof EV.q[0]); EV.nq--;
        ev_do(a, kind);
    }
}
static void ev_queue(int a, int trig, int kind, float after, int back) {
    if (a < 0 || a >= drawn()->nact) return;
    if (EV.nq >= EV_QUEUE) { ev_do(a, kind); return; }             /* (full: at once) */
    EV.q[EV.nq].act = a; EV.q[EV.nq].trig = trig; EV.q[EV.nq].kind = kind; EV.q[EV.nq].back = back; EV.q[EV.nq].due = EV.now + fmaxf(0.0f, after);
    EV.nq++;
}
static void ev_back(int k) {                                       /* a switch or a timed trigger goes back */
    const Drawn *D = drawn();
    const DrawnTrig *T = &D->trig[k];
    EV.tr[k].on = 0; EV.tr[k].back_t = -1.0f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ev_back %d %s by %s t=%.2f\n", k, TG_NAME[T->kind], G.player ? "player" : "autopilot", G.t);
#endif
    for (int i = 0; i < EV.nq; ) {                                 /* what it had still to do, not done */
        if (EV.q[i].trig == k && !EV.q[i].back) { memmove(&EV.q[i], &EV.q[i + 1], (size_t)(EV.nq - i - 1) * sizeof EV.q[0]); EV.nq--; }
        else i++;
    }
    if (T->undo)                                                   /* its torches, fields and doors as they were, last first */
        for (int a = T->act + T->nact - 1; a >= T->act; a--) {
            int u = a >= 0 && a < D->nact ? ev_undo_of(D->act[a].kind) : -1;
            if (u >= 0) ev_queue(a, k, u, 0.0f, 1);
        }
    for (int a = T->back; a < T->back + T->nback; a++) if (a >= 0 && a < D->nact) ev_queue(a, k, D->act[a].kind, D->act[a].after, 1);
    ev_run();
}
static void ev_fire(int k) {
    const Drawn *D = drawn();
    if (k < 0 || k >= ev_ntrig() || !ev_ready(k)) return;          /* never while what it waits on has not */
    const DrawnTrig *T = &D->trig[k];
    int n = EV.tr[k].n;
    if (T->type == TT_ONCE && n) return;
    if (T->type == TT_TOGGLE && EV.tr[k].on) { ev_back(k); return; }
    if (T->type == TT_TIMED && EV.tr[k].on) return;
    EV.tr[k].n++;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ev_fire %d %s n=%d by %s t=%.2f\n", k, TG_NAME[T->kind], EV.tr[k].n, G.player ? "player" : "autopilot", G.t);
#endif
    for (int a = T->act; a < T->act + T->nact; a++) if (a >= 0 && a < D->nact) ev_queue(a, k, D->act[a].kind, D->act[a].after, 0);
    if (T->type == TT_TOGGLE || T->type == TT_TIMED) { EV.tr[k].on = 1; EV.tr[k].back_t = T->hold > 0.0f ? EV.now + T->hold : -1.0f; }
    ev_run();
    for (int j = 0; j < ev_ntrig(); j++)                           /* and what waits on every one of a list */
        if (D->trig[j].kind == TG_ALL && !EV.tr[j].n && D->trig[j].needs && ev_ready(j)) ev_fire(j);
}

/* ---- where the game acts: a thing taken, set, pulled, stood on, struck; a box walked into ---- */
static void ev_touch(int kind, int thing) {
    if (!ev_live()) return;
    const Drawn *D = drawn();
    for (int k = 0; k < ev_ntrig(); k++) if (D->trig[k].kind == kind && D->trig[k].thing == thing) ev_fire(k);
}
static void ev_pickup(int thing) { ev_touch(TG_PICKUP, thing); }   /* collect: an item the drawing laid */
static void ev_thing(int i, int how) {                             /* pz_touched: one of the events' things */
    const Thing *t = &G.thing[i];
    if (!t->ev) return;
    switch (how) {
    case TU_TAKE: if (t->role == TR_ITEM) ev_touch(TG_PICKUP, t->ev - 1); break;
    case TU_SET: if (t->in >= 0 && G.thing[t->in].ev) ev_touch(TG_PLACE, G.thing[t->in].ev - 1); break;
    case TU_PULL: ev_touch(TG_USE, t->ev - 1); break;
    case TU_STEP: ev_touch(TG_STEP, t->ev - 1); break;
    default: break;
    }
}
static int ev_place_ok(int socket) { return ev_trigs_ready(TG_PLACE, socket); }   /* pz_takes: a socket takes nothing early */
static void ev_struck(int k) {                                     /* mk_finish */
    if (!ev_live() || k < 0 || k >= MAXMK || !EV.mk[k]) return;
    const Drawn *D = drawn();
    int th = EV.mk[k] - 1, again = 0;
    for (int j = 0; j < ev_ntrig(); j++) {
        if (D->trig[j].kind != TG_STRIKE || D->trig[j].thing != th) continue;
        if (!ev_ready(j) || D->trig[j].type != TT_ONCE) again = 1; /* too soon, or one that fires again: it may be struck again */
        ev_fire(j);
    }
    if (again) EV.rearm[k] = 2.0f;
}
static int ev_item_ok(const Item *it) {                            /* the dream leaves a thing a trigger watches till it counts */
    return !it->ev || !ev_live() || ev_trigs_ready(TG_PICKUP, it->ev - 1);
}
static int ev_thing_of(int thing) {                                /* the puzzles' thing a drawing's thing is; -1 none */
    for (int i = 0; i < G.nthing; i++) if (G.thing[i].ev == thing + 1) return i;
    return -1;
}
static void ev_tick(float dt) {                                    /* every frame, whoever plays */
    if (!ev_live()) return;
    const Drawn *D = drawn();
    EV.now += dt;
    if (EV.all_t > 0.0f) EV.all_t -= dt;
    for (int i = 0; i < EV.nt; i++) {
        EvTorch *T = &EV.torch[i];
        if (T->anim == EA_NONE) continue;
        float was = T->t;
        T->t += dt;
        if (was < 0.0f && T->t >= 0.0f && T->anim == EA_IN) sfx_at(SFX_IGNITE, T->wx + 0.5f + DX[T->d] * 0.6f, T->wy + 0.5f + DY[T->d] * 0.6f, 0.35f);
        if (T->t >= 4.0f * EV_STEP + (T->anim == EA_OUT ? 1.0f : 0.0f)) T->anim = EA_NONE;
    }
    for (int k = 0; k < G.nmk && k < MAXMK; k++)                   /* struck for a trigger that fires again: whole once more */
        if (EV.rearm[k] > 0.0f && (EV.rearm[k] -= dt) <= 0.0f) {
            Marked *m = &G.mk[k];
            m->done = 0; m->tried = 0;
            if (m->deco >= 0) mk_deco(m, MKK[m->kind].s0, 1);
        }
    int cx = (int)floorf(G.px) - D->ox, cy = (int)floorf(G.py) - D->oy;
    for (int k = 0; k < ev_ntrig(); k++) {
        const DrawnTrig *T = &D->trig[k];
        if (T->kind == TG_ENTER) {                                 /* the box: the first time in; every time, each time in again */
            int in = !G.dead && cx >= T->x0 && cx <= T->x1 && cy >= T->y0 && cy <= T->y1;
            if (in && (!EV.tr[k].n || (T->type == TT_EVERY && !EV.tr[k].inside))) ev_fire(k);
            EV.tr[k].inside = in;
        }
        if (T->kind == TG_STEP && T->type == TT_TIMED && EV.tr[k].on && T->hold > 0.0f) {   /* a timed plate holds while stood on */
            int p = ev_thing_of(T->thing);
            if (p >= 0 && G.thing[p].on) EV.tr[k].back_t = EV.now + T->hold;
        }
        if (EV.tr[k].on && EV.tr[k].back_t >= 0.0f && EV.now >= EV.tr[k].back_t) ev_back(k);
    }
    ev_run();
}

/* ---- the dream at the events (the map editor's dream walk is the reference): a relic fetched to the socket that takes it, a thing
 * a trigger waits on taken, levers used, plates stepped on, things struck, boxes walked into, and a door held open for a time run
 * for. It reads what to do from the level, as it does a puzzle's answer. ---- */
static int ev_wants(int kind, int thing, int again) {              /* a trigger of this kind on the thing it would fire now */
    const Drawn *D = drawn();
    for (int k = 0; k < ev_ntrig(); k++) {
        const DrawnTrig *T = &D->trig[k];
        if (T->kind != kind || T->thing != thing || !ev_ready(k)) continue;
        if (!EV.tr[k].n) return 1;
        if (again && T->type != TT_ONCE && !EV.tr[k].on && EV.tr[k].n < 4) return 1;
    }
    return 0;
}
static int ev_mk_watched(int k) { return ev_live() && k >= 0 && k < MAXMK && EV.mk[k] && ev_wants(TG_STRIKE, EV.mk[k] - 1, 1); }
static int ev_mk_can(const Marked *m) {                            /* the dream holds the spell it wants, ready */
    int sp = LINE_ATK[m->el];
    return RUN.gear.atk[sp] >= 0 && !(sp == AT_SPEAR && !spear_ready());
}
static int ev_strike_k = -1, ev_strike_x, ev_strike_y;             /* the thing to strike the dream is going for, and where from */
static int ev_at_strike(void) {                                    /* there: face it and cast, as at a thing that wants a spell (dream_looks) */
    if (G.player || !ev_live() || ev_strike_k < 0 || ev_strike_k >= G.nmk || G.cx != ev_strike_x || G.cy != ev_strike_y) return 0;
    Marked *m = &G.mk[ev_strike_k];
    if (m->done || no_cast() || in_combat() || !ev_mk_can(m) || !line_clear(G.px, G.py, m->x, m->y)) return 0;
    m->tried++;
    G.probe_mk = ev_strike_k + 1; G.probe_sp = LINE_ATK[m->el]; G.probe_n = 0; G.probe_last = 0.0f;
    begin_turn(atan2f(m->y - G.py, m->x - G.px));
    G.state = PS_PROBE; G.st = 0.0f; G.spd = 0.0f; G.u = 1.0f; G.fired = 0;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mk_dream %s at %.1f for a trigger t=%.2f\n", MKK[m->kind].name, hypotf(m->x - G.px, m->y - G.py), G.t);
#endif
    return 1;
}
static int ev_takes(const Thing *s, const Thing *r) { return s->role == TR_PLACE && s->ev && s->in < 0 && (s->want < 0 || s->want == r->val) && ev_place_ok(s->ev - 1); }
static int ev_goal_at(int x, int y, int *gx, int *gy, int again) {   /* again: what fires again too */
    const Drawn *D = drawn();
    bfs_from(x, y);
    int bd = 1 << 30, bx = -1, by = -1, use = -1, step = -1, why = 0, sx, sy;
#define EV_TRY(cx_, cy_, use_, step_, why_, slack_) do { int d_ = bfs_dist[cy_][cx_]; \
        if (d_ >= 0 && (d_ > 0 || (use_) >= 0 || (step_) >= 0 || (why_) == 5) && d_ - (slack_) < bd) { bd = d_ - (slack_); bx = cx_; by = cy_; use = use_; step = step_; why = why_; } } while (0)
    const Thing *held = G.carry ? &G.thing[G.carry - 1] : NULL;
    for (int i = 0; i < G.nthing; i++) {
        const Thing *t = &G.thing[i];
        if (!t->ev || G.carry == i + 1) continue;
        if (t->role == TR_PLACE && held && held->ev && ev_takes(t, held)) { if (use_spot(t->cx, t->cy, &sx, &sy)) EV_TRY(sx, sy, i, -1, 1, 0); }
        else if (t->role == TR_ITEM && !held && t->in < 0 && ev_trigs_ready(TG_PICKUP, t->ev - 1)) {
            int takes = 0;
            for (int s = 0; s < G.nthing && !takes; s++) takes = ev_takes(&G.thing[s], t);
            if (takes && use_spot(t->cx, t->cy, &sx, &sy)) EV_TRY(sx, sy, i, -1, 2, 0);
        } else if (t->role == TR_LEVER && ev_wants(TG_USE, t->ev - 1, again)) { if (use_spot(t->cx, t->cy, &sx, &sy)) EV_TRY(sx, sy, i, -1, 3, 0); }
        else if (t->role == TR_PLATE && !t->on && ev_wants(TG_STEP, t->ev - 1, again)) {
            if (apass(t->cx, t->cy)) EV_TRY(t->cx, t->cy, -1, -1, 4, 0);
            else for (int d = 0; d < 4; d++) if (apass(t->cx + DX[d], t->cy + DY[d])) EV_TRY(t->cx + DX[d], t->cy + DY[d], -1, i, 4, 0);
        }
    }
    int sk = -1;
    for (int k = 0; k < G.nmk && k < MAXMK; k++) {                 /* a thing to strike: the lane cell in reach of it, in sight, */
        const Marked *m = &G.mk[k];                                /* nearest it (the same each time, so it goes there and stops) */
        if (!EV.mk[k] || m->done || m->tried >= 8 || !ev_mk_can(m) || !ev_wants(TG_STRIKE, EV.mk[k] - 1, again)) continue;
        int mx = (int)m->x, my = (int)m->y, cx = -1, cy = -1;
        float cd = 1e9f;
        for (int yy = imax(1, my - 5); yy <= imin(MAPN - 2, my + 5); yy++)
            for (int xx = imax(1, mx - 5); xx <= imin(MAPN - 2, mx + 5); xx++) {
                float dd = hypotf(xx + 0.5f - m->x, yy + 0.5f - m->y);
                if (dd < 1.5f || dd > 5.5f || dd >= cd || !apass(xx, yy) || bfs_dist[yy][xx] < 0 || !line_clear(xx + 0.5f, yy + 0.5f, m->x, m->y)) continue;
                cd = dd; cx = xx; cy = yy;
            }
        int bd0 = bd;
        if (cx >= 0) EV_TRY(cx, cy, -1, -1, 5, 0);
        if (bd < bd0) sk = k;
    }
    for (int i = 0; i < G.nitems; i++) {                           /* a thing a trigger waits on, taken */
        const Item *it = &G.items[i];
        if (it->alive && it->ev && ev_wants(TG_PICKUP, it->ev - 1, 0)) EV_TRY(it->lx, it->ly, -1, -1, 6, 0);
    }
    for (int k = 0; k < ev_ntrig(); k++) {
        const DrawnTrig *T = &D->trig[k];
        if (T->kind == TG_ENTER && !EV.tr[k].n && ev_ready(k))     /* a box walked into */
            for (int yy = T->y0; yy <= T->y1; yy++)
                for (int xx = T->x0; xx <= T->x1; xx++) {
                    int ax = D->ox + xx, ay = D->oy + yy;
                    if (ax > 0 && ay > 0 && ax < MAPN - 1 && ay < MAPN - 1 && apass(ax, ay)) EV_TRY(ax, ay, -1, -1, 7, 0);
                }
        if (EV.tr[k].on && EV.tr[k].back_t >= 0.0f)                /* a door held open for a while: through it while it holds */
            for (int a = T->act; a < T->act + T->nact; a++) {
                if (a < 0 || a >= D->nact || (D->act[a].kind != TA_DOOR_OPEN && D->act[a].kind != TA_DOOR_UNLOCK)) continue;
                int dx = D->ox + D->act[a].x, dy = D->oy + D->act[a].y, far = -1, fd = -1;
                if (dx < 1 || dy < 1 || dx >= MAPN - 1 || dy >= MAPN - 1 || G.map[dy][dx].kind != K_DOOR || !passable(dx, dy)) continue;
                for (int d = 0; d < 4; d++) {                      /* the side away from the dream */
                    int nx = dx + DX[d], ny = dy + DY[d];
                    if (apass(nx, ny) && bfs_dist[ny][nx] > fd) { fd = bfs_dist[ny][nx]; far = d; }
                }
                if (far < 0) continue;
                int fx = dx + DX[far], fy = dy + DY[far];
                if (G.map[fy][fx].visits == 0 || G.leaving) EV_TRY(fx, fy, -1, -1, 8, 1000);
            }
    }
#undef EV_TRY
#ifdef CRAWL_DEBUG
    static int last_why = -1, last_x = -1, last_y = -1;
    if (bx >= 0 && (why != last_why || bx != last_x || by != last_y))
        fprintf(stderr, "EVENT ev_dream goal %d at (%d,%d) dist %d t=%.2f\n", why, bx - D->ox, by - D->oy, bd < 0 ? bd + 1000 : bd, G.t);
    last_why = bx >= 0 ? why : -1; last_x = bx; last_y = by;
#endif
    if (bx < 0) return 0;
    if (why == 5) { ev_strike_k = sk; ev_strike_x = bx; ev_strike_y = by; }
    if (use >= 0) { G.use_x = G.thing[use].cx; G.use_y = G.thing[use].cy; G.use_kind = USE_THING; G.use_thing = use; }
    if (step >= 0) { G.pz_stepx = G.thing[step].cx; G.pz_stepy = G.thing[step].cy; }
    *gx = bx; *gy = by;
    return 1;
}
static int ev_goal(int x, int y, int *gx, int *gy) {
    ev_strike_k = -1;
    if (G.player || !ev_live()) return 0;
    if (ev_goal_at(x, y, gx, gy, 0)) return 1;
    bfs_from(x, y);                                                /* nothing new, and the way out shut: what fires again, again */
    return G.exit_fx >= 0 && bfs_dist[G.exit_fy][G.exit_fx] < 0 && ev_goal_at(x, y, gx, gy, 1);
}
