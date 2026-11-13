/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The lessons (TU): their moments, the view turned, the page; update_human. */

/* ---- The lessons (docs/PLAN-tutorial.md, section 8): on the drawn levels, while a person's first run here is on them (or
 * they asked for them again under the title's HELP), pages that stop the crypt at moments (beats), triggers in the floor, and
 * the view turned a moment where a beat says. TU lasts while the program runs; a death on the level keeps what was shown. ---- */
enum { TB_ARRIVE, TB_DOOR, TB_THROUGH, TB_MENDED, TB_BLOOD, TB_TRIG1, TB_KEY, TB_TRIG2, TB_HOARD, TB_TRIG3, NTBEAT };
enum { TB2_ARRIVE, TB2_THING, TB2_TRIG1, TB2_MAP, TB2_GOLD, TB2_CRIMSON };   /* level 2's (B11, B12, B13, B16, B18, B19) */
enum { MV_TURN, MV_HOLD, MV_LEAD };                                /* what the lessons do with the body, in turn */
#define TU_Q 8
#define TU_MV 6
#define TU_STEER 0.5f                                              /* the view eased round in half a second */
#define TU_LEAD 2.4f                                               /* led at a walk, cells a second */
static struct {
    int on, level;                                                 /* this level's lessons run; the level they are for */
    uint32_t done;                                                 /* beats shown on it, a bit each (kept through a death on it) */
    int q[TU_Q], qa[TU_Q], nq;                                     /* lessons waiting, and their shorter words */
    int lesson, page, alt;                                         /* the one up */
    float fresh;                                                   /* no key for its first moment */
    int steer, back; float st_t, st_a0, st_a1, st_x0, st_y0, st_x1, st_y1;   /* the view taken */
    struct { int kind, back; float a, x, y, t; } mv[TU_MV]; int nmv;   /* the turnings, holds and leads to come, the first under way */
    float prev_y;                                                  /* level 2: last frame's row in the drawing's cells (trigger 2's line) */
    int crimson;                                                   /* level 2: the crimson key taken */
    int trig_in;                                                   /* the trigger the body stood in last frame (0 none) */
    float lx, ly;                                                  /* the last place it stood out of every trigger */
    int key_taken, gold_open, vault_been, mend_on, mend_hp, wisp_due, again[4];
} TU;
static int tu_due(int level) {                                     /* a drawn level's lessons, for a person, if not yet given */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TUT")) return atoi(getenv("CRAWL_TUT")) != 0 && drawn_level(level);
#endif
    pl_read();
    return drawn_level(level) && !(PL.tut & (level == 1 ? TUT_L1 : TUT_L2));
}
static void tu_level_begin(void) {                                 /* from start_level */
    int on = G.player && g_drawn && tu_due(G.level);
    if (G.level != TU.level || !on) { memset(&TU, 0, sizeof TU); TU.level = G.level; }
    else { TU.nq = 0; TU.steer = 0; TU.nmv = 0; TU.trig_in = 0; TU.mend_on = 0; TU.wisp_due = 0; TU.key_taken = TU.gold_open = TU.vault_been = 0; TU.crimson = 0; }
    TU.prev_y = -1.0f;
    TU.on = on;
    TU.lx = G.px; TU.ly = G.py;
#ifdef CRAWL_DEBUG
    if (on && getenv("CRAWL_TUTBEAT")) TU.done |= (1u << clampi(atoi(getenv("CRAWL_TUTBEAT")), 0, 31)) - 1u;   /* testing: the beats before k shown */
    if (on && getenv("CRAWL_TUTKEY")) {                            /* testing: the gold key held; 2, and the dead gone */
        G.has_key[0] = 1; TU.key_taken = 1; TU.done |= 1u << TB_KEY;
        if (atoi(getenv("CRAWL_TUTKEY")) >= 2) for (int i = 0; i < MAXMOBS; i++) G.mobs[i].alive = 0;
    }
#endif
    if (on) G.hint_t = 0.0f;                                       /* the first page says what the controls card would */
}
static int tu_mercy(void) { return g_drawn && TU.on; }             /* a death while they run costs nothing (T4 a) */
static int tu_cls(void) { int e = cls_el(); return e < 0 ? 0 : e; }
/* a key put in the voice the person plays in: the keyboard's as written, a pad's buttons by their layout, touch's stones */
static void tu_voice(const char *in, char *out, size_t n) {
    int pad = PADX.used, touch = !pad && TCH.used;
    size_t o = 0;
    while (*in && o + 1 < n) {
        if (*in != '[' || (!pad && !touch)) { out[o++] = *in++; continue; }
        const char *e = strchr(in, ']');
        if (!e) { out[o++] = *in++; continue; }
        char key[24];
        snprintf(key, sizeof key, "%.*s", (int)(e - in - 1), in + 1);
        const char *w = key;
        static char b[24];
        if (pad) {
            if (!strcmp(key, "WASD")) w = PL.swap ? "R-STICK" : "L-STICK";
            else if (!strcmp(key, "THE MOUSE")) w = PL.swap ? "L-STICK" : "R-STICK";
            else if (!strcmp(key, "SHIFT")) { snprintf(b, sizeof b, "%s", PAD_WORD[pad_sprint()]); w = b; }
            else if (!strcmp(key, "E")) w = PAD_WORD[pl_on(PAD_A)];
            else if (!strcmp(key, "H")) w = PAD_WORD[pl_on(PAD_DOWN)];
            else if (!strcmp(key, "CLICK")) w = PAD_WORD[pl_on(PAD_RT)];
            else if (!strcmp(key, "L")) w = PAD_WORD[pl_on(PAD_Y)];
            else if (!strcmp(key, "M")) w = PAD_WORD[pl_on(PAD_UP)];
            else if (!strcmp(key, "ARROWS")) w = "D-PAD";
        } else {
            if (!strcmp(key, "WASD")) w = "THE WALKING STICK";
            else if (!strcmp(key, "THE MOUSE")) w = "A FINGER SLID ON THE RIGHT";
            else if (!strcmp(key, "SHIFT")) w = "THE STICK PUSHED PAST ITS RING";
            else if (!strcmp(key, "E")) w = "USE";
            else if (!strcmp(key, "H")) w = "A TAP AT THE PICTURE'S FOOT";
            else if (!strcmp(key, "CLICK")) w = "CAST";
            else if (!strcmp(key, "L")) w = "WISPS";
            else if (!strcmp(key, "M")) w = "MAP";
            else if (!strcmp(key, "ARROWS")) w = "A DRAG";
        }
        o += (size_t)snprintf(out + o, n - o, "[%s]", w);
        if (o >= n) o = n - 1;
        in = e + 1;
    }
    out[o] = 0;
}
static void tu_show(void) {                                        /* the next lesson waiting, if nothing else is up */
    if (!TU.nq || G.ui != UI_NONE || TU.steer || G.dead || G.tally) return;
    TU.lesson = TU.q[0]; TU.alt = TU.qa[0]; TU.page = 0;
    TU.nq--; memmove(TU.q, TU.q + 1, sizeof TU.q[0] * (size_t)TU.nq); memmove(TU.qa, TU.qa + 1, sizeof TU.qa[0] * (size_t)TU.nq);
    open_ui(UI_TUTOR);
    TU.fresh = 0.4f;
    snd_sfx_ex(SFX_PAGE, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT tut page %d alt %d t=%.2f\n", TU.lesson, TU.alt, G.t);
#endif
}
static void tu_queue(int lesson, int alt) { if (TU.nq < TU_Q) { TU.q[TU.nq] = lesson; TU.qa[TU.nq++] = alt; } }
static int tu_hunted(void) {                                       /* something hunting within six cells: a person isn't turned from a fight */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TUTCALM")) return 0;                         /* testing: as if nothing hunted */
#endif
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m->alive && m->state == MS_WALK && m->alert > 0.0f && !m->ally && hypotf(m->x - G.px, m->y - G.py) < 6.0f) return 1;
    }
    return 0;
}
static void tu_move_begin(void) {                                  /* the first move to come, from where the body is now */
    TU.st_t = 0.0f; TU.st_a0 = G.ang; TU.st_x0 = G.px; TU.st_y0 = G.py;
    TU.back = TU.mv[0].back; TU.st_a1 = G.ang + wrap_pi(TU.mv[0].a - G.ang);
    TU.st_x1 = TU.mv[0].kind == MV_LEAD ? TU.mv[0].x : TU.back ? TU.lx : G.px;
    TU.st_y1 = TU.mv[0].kind == MV_LEAD ? TU.mv[0].y : TU.back ? TU.ly : G.py;
    if (TU.mv[0].kind == MV_LEAD) TU.mv[0].t = fmaxf(0.1f, hypotf(TU.st_x1 - G.px, TU.st_y1 - G.py) / TU_LEAD);
}
static void tu_move(int kind, float a, float x, float y, float t, int back) {
    if (TU.nmv >= TU_MV) return;
    TU.mv[TU.nmv].kind = kind; TU.mv[TU.nmv].a = a; TU.mv[TU.nmv].x = x; TU.mv[TU.nmv].y = y; TU.mv[TU.nmv].t = t; TU.mv[TU.nmv].back = back;
    if (TU.nmv++ == 0) tu_move_begin();
    TU.steer = 1; PADX.run = 0; G.chg_on = 0;
    if (G.rest_on) rest_end("turned");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT tut %s %.2f (%.2f,%.2f) %.1f back %d t=%.2f\n", kind == MV_TURN ? "steer" : kind == MV_HOLD ? "hold" : "lead", a, x, y, t, back, G.t);
#endif
}
static void tu_steer(float ang, int back) { tu_move(MV_TURN, ang, 0.0f, 0.0f, TU_STEER, back); }   /* the view eased round, and with back the
                                                                      body set back out of a trigger */
static void tu_hold(float t) { tu_move(MV_HOLD, 0.0f, 0.0f, 0.0f, t, 0); }   /* kept still, looking */
static void tu_lead(float x, float y) { tu_move(MV_LEAD, 0.0f, x, y, 0.0f, 0); }   /* walked there, the view as it is */
static int tu_taken(void) { return TU.steer; }
static int tu_beat(int b, int lesson, int alt) {                   /* a beat, once: its lesson queued */
    if (TU.done >> b & 1) return 0;
    TU.done |= 1u << b;
    tu_queue(lesson, alt);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT tut beat %d t=%.2f\n", b, G.t);
#endif
    return 1;
}
static int tu_trig(int cx, int cy) {                               /* the trigger a cell is, on the level's drawing (0 none) */
    if (!g_drawn) return 0;
    const Drawn *D = drawn();
    int x = cx - D->ox, y = cy - D->oy;
    if (g_drawn == 2) return y == 9 && (x == 12 || x == 13) ? 1 : y == 12 && x >= 7 && x <= 9 ? 4 : 0;   /* level 2's 1 and x (T20 a) */
    if (y == 23 && (x == 4 || x == 5)) return 1;
    if ((x == 14 && y == 30) || (x == 13 && y == 31) || (x == 14 && y == 31)) return 2;
    if (x == 19 && (y == 30 || y == 31)) return 3;
    return 0;
}
static int tu_steering(float dt) {                                 /* from update_human: 1 while the view is taken */
    if (!TU.steer) return 0;
    if (!TU.nmv) { TU.steer = 0; return 0; }
    TU.st_t += dt;
    int kind = TU.mv[0].kind;
    float dur = TU.mv[0].t, k = kind == MV_LEAD ? clampf(TU.st_t / dur, 0.0f, 1.0f) : smoothstepf(0.0f, dur, TU.st_t);
    if (kind == MV_TURN) G.ang = TU.st_a0 + (TU.st_a1 - TU.st_a0) * k;
    if (kind == MV_LEAD || (kind == MV_TURN && TU.back)) {
        G.px = TU.st_x0 + (TU.st_x1 - TU.st_x0) * k; G.py = TU.st_y0 + (TU.st_y1 - TU.st_y0) * k; G.cx = (int)G.px; G.cy = (int)G.py;
        if (kind == MV_LEAD) G.bobph += dt * 6.0f;                 /* a walk's sway */
    }
    G.spd = 0.0f; G.pvx = G.pvy = 0.0f;
    if (TU.st_t < dur) return 1;
    memmove(TU.mv, TU.mv + 1, sizeof TU.mv[0] * (size_t)(TU.nmv - 1));
    if (--TU.nmv > 0) { tu_move_begin(); return 1; }
    TU.steer = 0; TU.trig_in = tu_trig((int)floorf(G.px), (int)floorf(G.py)); TU.prev_y = -1.0f;
    tu_show();
    return 1;
}
static void tu_map_taken(void) {                                   /* B16: the ice comes back, watched, then the gate, then the pages */
    if (!TU.on || g_drawn != 2 || !tu_beat(TB2_MAP, TL_MAP, 0)) return;
    tu_steer(PI_F * 0.5f, 0); tu_hold(2.0f);
    const Marked *g = IC.gate >= 0 && IC.gate < G.nmk ? &G.mk[IC.gate] : NULL;
    tu_steer(g ? atan2f(g->y - G.py, g->x - G.px) : -PI_F * 0.5f, 0);
}
static int tu_exit_barred(void) {                                  /* B20 at the door itself: not without the map */
    if (!TU.on || g_drawn != 2 || RUN.has_map) return 0;
    if (!TU.steer && !TU.nq && G.ui == UI_NONE) { tu_queue(TL_NOMAP, TU.again[1]++ > 0); tu_steer(PI_F * 0.5f, 0); }
    return 1;
}
static void tu_after(int lesson) {                                 /* a lesson read: what follows it */
    if (lesson == TL_THROUGH && G.nrest > 0) summon_wisp(WK_MEND, 0, 0.0f);   /* free: the way to the place to mend */
}
static void tu_tick(float dt) {                                    /* from update_human, while no menu is up: the beats */
    if (!TU.on || !g_drawn || G.dead || G.tally || G.exiting) return;
    const Drawn *D = drawn();
    int cx = (int)floorf(G.px), cy = (int)floorf(G.py), dx = cx - D->ox, dy = cy - D->oy, trig = tu_trig(cx, cy);
    (void)dx; (void)dy;
    if (g_drawn == 1) {
        if (G.card > 3.6f && G.level_t > 1.0) tu_beat(TB_ARRIVE, TL_WALK, 0);
        for (int i = 0; i < G.ndoors; i++)                         /* the first door, open */
            if (G.doors[i].x == D->ox + 4 && G.doors[i].y == D->oy + 1 && G.doors[i].open > 0.5f && (TU.done >> TB_ARRIVE & 1)) tu_beat(TB_DOOR, TL_FIRE, 0);
        if (!(TU.done >> TB_THROUGH & 1) && G.py >= (float)D->oy + 7.0f && !tu_hunted()) {   /* through the fire: to the place to mend */
            tu_beat(TB_THROUGH, TL_THROUGH, 0); tu_steer(PI_F * 0.5f, 0);
        }
        if (G.rest_on && !TU.mend_on) { TU.mend_on = 1; TU.mend_hp = G.hp; }
        int mended = !G.rest_on && TU.mend_on && G.hp > TU.mend_hp;
        if (!G.rest_on) TU.mend_on = 0;
        int walked_on = G.px >= (float)D->ox + 9.0f && G.py >= (float)D->oy + 9.0f && G.py < (float)D->oy + 12.0f;
        if (!(TU.done >> TB_MENDED & 1) && (TU.done >> TB_THROUGH & 1) && (mended || walked_on) && !tu_hunted()) {
            tu_beat(TB_MENDED, TL_CLASS, !mended); tu_steer(0.0f, 0);   /* the hall on the left: east */
        }
        if (G.kills > 0) tu_beat(TB_BLOOD, TL_SOULS, 0);
        if (G.has_key[0] && !TU.key_taken) { TU.key_taken = 1; tu_beat(TB_KEY, TL_KEY, 0); }
        for (int i = 0; i < G.ndoors; i++) if (G.doors[i].color == 0 && G.doors[i].locked == 2) TU.gold_open = 1;
        int in_vault = G.nvaults > 0 && G.vault_room[0] >= 0 && in_room(&G.rooms[G.vault_room[0]], cx, cy);
        if (in_vault && TU.gold_open) TU.vault_been = 1;
        if (TU.vault_been && !in_vault) tu_beat(TB_HOARD, TL_HOARD, 0);   /* out of the vault: the way out (T12 b) */
        if (trig && trig != TU.trig_in) {                          /* into a trigger's cells */
            if (trig == 1) tu_beat(TB_TRIG1, TL_EVERY, 0);
            else if (trig == 2 && !TU.key_taken && !tu_hunted()) {  /* not yet: turned to the key room's door, again each time */
                tu_queue(TL_NOTYET, TU.again[2]++ > 0); tu_steer(atan2f((float)D->oy + 26.5f - G.py, (float)D->ox + 11.5f - G.px), 1);
            } else if (trig == 3 && !TU.gold_open && !tu_hunted()) {   /* the gold door unused: turned back west, again each time */
                tu_queue(TL_GOLDDOOR, TU.again[3]++ > 0); tu_steer(PI_F, 1);
            }
        }
    } else if (g_drawn == 2) {
        float fx = G.px - (float)D->ox, fy = G.py - (float)D->oy;   /* in the drawing's cells */
        if (G.card > 3.6f && G.level_t > 1.0) tu_beat(TB2_ARRIVE, TL_L2, 0);
        if (!(TU.done >> TB2_THING & 1) && (TU.done >> TB2_ARRIVE & 1))   /* B12: the first thing that wants a spell, near and in sight */
            for (int i = 0; i < G.nmk; i++) {
                const Marked *m = &G.mk[i];
                if (m->done || m->kind == MKK_RSEAL || m->kind == MKK_GSEAL || hypotf(m->x - G.px, m->y - G.py) > 4.0f || !line_clear(G.px, G.py, m->x, m->y)) continue;
                tu_beat(TB2_THING, TL_CRYPT, 0);
                break;
            }
        if (G.has_key[0] && !TU.key_taken) {                       /* B18: the gold key, and the cracks two cells off */
            TU.key_taken = 1;
            if (tu_beat(TB2_GOLD, TL_CRACKS, 0) && G.nsecrets && !G.secrets[0].state) tu_steer(atan2f((float)G.secrets[0].sy + 0.5f - G.py, (float)G.secrets[0].sx + 0.5f - G.px), 0);
        }
        if (G.has_key[2] && !TU.crimson) { TU.crimson = 1; tu_beat(TB2_CRIMSON, TL_CRIMSON, 0); }   /* B19 */
        if (TU.prev_y > 0.0f && fx >= 11.0f && fx < 15.0f && IC.live && !TU.steer && (TU.prev_y >= 18.0f) != (fy >= 18.0f)) {   /* trigger 2's line */
            int from_south = TU.prev_y >= 18.0f;
            if (!RUN.has_map || !from_south) {                     /* B14 without the map, B17 with it going south: led back */
                if (!RUN.has_map) tu_queue(TL_ICE, TU.again[0]++ > 0); else tu_queue(TL_WAYON, 0);
                tu_steer(from_south ? PI_F * 0.5f : -PI_F * 0.5f, 0);
                if (from_south) { float tx = (float)D->ox + clampf(fx, 12.3f, 13.7f); tu_lead(tx, G.py); tu_lead(tx, (float)D->oy + 23.5f); }
                else tu_lead(G.px, (float)D->oy + 15.5f);
            }
        }
        if (!TU.steer) TU.prev_y = fy;
        if (trig && trig != TU.trig_in && !TU.steer) {
            if (trig == 1 && !RUN.has_map) tu_beat(TB2_TRIG1, TL_GATE, 0);   /* B13: the map beyond the gate, the way in from the south */
            else if (trig == 4 && !RUN.has_map && !tu_hunted()) { tu_queue(TL_NOMAP, TU.again[1]++ > 0); tu_steer(PI_F * 0.5f, 1); }   /* B20 */
        }
    }
    if (!TU.steer) { TU.trig_in = trig; if (!trig) { TU.lx = G.px; TU.ly = G.py; } }
    tu_show();
}
static void tu_close(void) {
    int l = TU.lesson;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT tut closed %d page %d t=%.2f\n", l, TU.page, G.t);
#endif
    G.ui = UI_NONE; G.click_spent = 1;
    snd_sfx_ex(SFX_KEY, 0.6f, 0.5f);
    tu_after(l);
    tu_show();
}
static void update_tutor(void) {
    if ((TU.fresh -= (float)IN_DT) > 0.0f) return;
#ifdef CRAWL_DEBUG
    static float auto_t;                                           /* testing: CRAWL_TUTAUTO=s, each page turned after s seconds */
    if (getenv("CRAWL_TUTAUTO") && (auto_t += (float)IN_DT) >= (float)atof(getenv("CRAWL_TUTAUTO"))) { auto_t = 0.0f; IN.hit[KEY_E] = 1; }
#endif
    int n = TU.alt && (TU.lesson == TL_NOTYET || TU.lesson == TL_GOLDDOOR || TU.lesson == TL_ICE || TU.lesson == TL_NOMAP) ? 1 : tut_pages(TU.lesson);
    if ((IN.hit[KEY_A] || IN.hit[KEY_LEFT]) && TU.page > 0) { TU.page--; snd_sfx_ex(SFX_PAGE, 0.5f, 0.5f); }
    if ((IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) && TU.page + 1 < n) { TU.page++; snd_sfx_ex(SFX_PAGE, 0.5f, 0.5f); }
    if (IN.hit[KEY_E] || IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1]) {
        if (TU.page + 1 < n) { TU.page++; snd_sfx_ex(SFX_PAGE, 0.5f, 0.5f); }
        else tu_close();
        return;
    }
    if (IN.hit[KEY_ESC] || IN.hit[KEY_BACK]) tu_close();
}
static void render_tutor(Frame *f) {
    dim_view(f);
    const char *in[TUT_LINES], *vp[TUT_LINES];
    char v[TUT_LINES][96], wr[TUT_LINES][96];
    int cls = tu_cls(), n = tut_lines(TU.lesson, TU.page, cls, TU.alt, in);
    for (int i = 0; i < n; i++) { tu_voice(in[i], v[i], sizeof v[i]); vp[i] = v[i]; }
    n = tut_wrap(vp, n, wr, 34);
    UiTutor u;
    memset(&u, 0, sizeof u);
    u.head = tut_head(TU.lesson, TU.page, cls);
    for (int i = 0; i < n; i++) u.line[i] = wr[i];
    int pages = TU.alt && (TU.lesson == TL_NOTYET || TU.lesson == TL_GOLDDOOR || TU.lesson == TL_ICE || TU.lesson == TL_NOMAP) ? 1 : tut_pages(TU.lesson);
    u.nline = n; u.page = TU.page; u.npage = pages; u.t = G.ui_t;
    int last = TU.page + 1 >= pages, pad = PADX.used, touch = !pad && TCH.used;
    u.keys = pages < 2 ? (pad ? "\1A DONE" : touch ? "USE DONE" : "E DONE")
           : pad ? (last ? "D-PAD PAGES  \1A DONE" : "D-PAD PAGES  \1A ON") : touch ? (last ? "SLIDE PAGES  USE DONE" : "SLIDE PAGES  USE ON")
           : last ? "A/D PAGES  E DONE" : "A/D PAGES  E ON";
    ui_tutor(f, &u);
}
static void tu_cleared(void) {                                     /* a drawn level cleared with its lessons: given, for good */
    if (!TU.on || !g_drawn) return;
    pl_read(); PL.tut |= G.level == 1 ? TUT_L1 : TUT_L2; pl_write();
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT tut done level %d prefs %d\n", G.level, PL.tut);
#endif
}
static void tu_shop_begin(void) {                                  /* her room after level 1, the first time: her wares (T13 a) */
    g_map_kept = G.level == 2 && LEDGER.yours && tu_due(2);        /* level 2's lessons find it there for you */
#ifdef CRAWL_DEBUG
    if (g_map_kept) fprintf(stderr, "EVENT tut map_kept level %d t=%.2f\n", G.level, G.t);
#endif
    pl_read();
    int due = G.player && G.level == 2 && (PL.tut & TUT_L1) && !(PL.tut & TUT_HER);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TUT")) due = G.player && G.level == 2 && atoi(getenv("CRAWL_TUT")) != 0;
#endif
    memset(&TU, 0, sizeof TU); TU.level = -1;
    if (!due) return;
    tu_queue(TL_WARES, 0);
    PL.tut |= TUT_HER; pl_write();
}

static void update_human(float dt) {
    G.calm += dt;
#ifdef CRAWL_DEBUG
    { static double wt; if (getenv("CRAWL_WHERE") && G.t - wt >= 0.5) { wt = G.t; fprintf(stderr, "WHERE t=%.1f at (%.2f,%.2f) ang %.0f hp %d\n", G.t, G.px, G.py, G.ang * 57.2958f, G.hp); } }   /* a person, twice a second */
#endif
    if (tu_steering(dt)) return;                                   /* the lessons turn the view: a moment, nothing of yours moves it */
    tu_tick(dt);
    if (G.ui) return;                                              /* a lesson's page came up */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_FACEFOE")) {                                 /* for frames: the view turned to the nearest creature within eight cells, */
        G.hint_t = 0.0f;                                           /* and the controls card put away */
        float best = 8.0f; const Mob *t = NULL;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            float d = hypotf(m->x - G.px, m->y - G.py);
            if (m->alive && m->state == MS_WALK && !buried(m) && d < best) { best = d; t = m; }
        }
        if (t) G.ang += wrap_pi(atan2f(t->y - G.py, t->x - G.px) - G.ang) * fminf(1.0f, dt * 6.0f);
    }
#endif
    if (G.pilgrim == 1) {                                          /* a key touched: the feet are the caster's again */
        int touched = IN.ntext > 0;
        for (int k = 0; k < KEY_COUNT && !touched; k++) touched = IN.hit[k];
        if (touched) { G.pilgrim = 0; say("YOUR FEET ARE YOUR OWN AGAIN"); }
    }
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_FOLLOWWISP")) G.pilgrim = 2;                 /* testing: go the way the wisp went, to its end */
#endif
    if (G.pilgrim) pilgrim_step(dt);
    look_update(dt);
    spell_keys();
    if (G.map_on) map_keys(dt);                                    /* its own keys; the rest play on */
    if (IN.hit[KEY_TAB]) { open_ui(UI_INVENTORY); return; }
    if (IN.hit[KEY_B]) { open_ui(UI_BESTIARY); return; }
    if (IN.hit[KEY_C]) { open_ui(UI_CODEX); return; }
    if (IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    if (IN.hit[KEY_M]) {
        if (RUN.has_map) map_up();
        else say("YOU HAVE NO MAP. THE MERCHANT SELLS ONE");
    }
    if (lantern_held()) {                                          /* a hold shutters or lights it; a tap is the wisps' (S6 a) */
        if (IN.hit[KEY_L]) { G.l_down = 1; G.l_t = 0.0f; }
        if (G.l_down && IN.down[KEY_L]) {
            if ((G.l_t += dt) >= 0.6f) { G.l_down = 0; lantern_toggle(); }
        } else if (G.l_down) {                                     /* let go: a tap */
            G.l_down = 0;
            if (!RUN.wisp_auto) { open_ui(UI_WISPS); return; }
            if (G.l_tap > 0.0f) { G.l_tap = 0.0f; open_ui(UI_WISPS); return; }   /* tapped twice: the list */
            G.l_tap = 0.3f;                                        /* once: it casts, unless a second tap comes */
        }
        if (G.l_tap > 0.0f && (G.l_tap -= dt) <= 0.0f) {
            WispChoice c[WISP_CHOICES];
            int n = wisp_choices(c), i = wisp_choice_at(c, n, RUN.wisp_kind, RUN.wisp_color);
            G.l_tap = 0.0f;
            if (i < 0) { open_ui(UI_WISPS); return; }
            cast_choice(&c[i]);
        }
    } else {
        if (IN.hit[KEY_L]) {
            if (RUN.wisp_auto) { G.l_down = 1; G.l_t = 0.0f; }
            else { open_ui(UI_WISPS); return; }
        }
        if (G.l_down) {                                            /* the toggle is on: a tap casts, a hold opens the menu */
            WispChoice c[WISP_CHOICES];
            int n = wisp_choices(c), i = wisp_choice_at(c, n, RUN.wisp_kind, RUN.wisp_color);
            if (IN.down[KEY_L] && (G.l_t += dt) < 0.6f) { }
            else if (IN.down[KEY_L] || i < 0) { G.l_down = 0; open_ui(UI_WISPS); return; }
            else { G.l_down = 0; cast_choice(&c[i]); }
        }
    }
    if (IN.hit[KEY_H]) show_bar();                                 /* a look at where things stand */
    if (IN.hit[KEY_MOUSE2]) use_right();                           /* the right mouse (LT): what Q or a wheel readied last; Q itself in wheel_input */
    if (PADX.rune_hit) rune_hand(rune_readied());                  /* RB: the rune readied */
    if (PADX.abil_hit) use_readied();                              /* LB: the boss's gift readied */
    G.tap_t += dt;
    static const int TAP[4] = { KEY_W, KEY_D, KEY_S, KEY_A };      /* ahead, right, back, left */
    for (int k = 0; k < 4 && G.dash_t <= 0.0f; k++) {              /* and a direction tapped twice darts that way */
        if (!IN.hit[TAP[k]]) continue;
        if (G.tap_key == k && G.tap_t < 0.25f && swift_dodge(G.ang + (float)k * PI_F * 0.5f)) G.tap_key = -1;
        else G.tap_key = k;
        G.tap_t = 0.0f;
    }
    if (PADX.dodge) { PADX.dodge = 0; if (G.dash_t <= 0.0f) swift_dodge(PADX.dodge_a); }   /* or B on a controller */
    if (G.dash_t > 0.0f) dash_step(dt); else human_walk(dt);
    if (IN.hit[KEY_E] || (!WEB && IN.hit[KEY_SPACE])) {
        if (af_on(AF_UNBOUND) || af_rooted()) hush_say();          /* unbound, frozen or down: the hands take hold of nothing (S8) */
        else human_use();
    }
    if (G.exiting) return;
    if (CAST_UP) G.click_spent = 0;
    human_cast(dt, !G.click_spent);
}

/* in the merchant's room: walk up to a ware to see it, E, Space or a click to buy it */
