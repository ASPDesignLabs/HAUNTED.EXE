/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The pad into keys (pad_keys), touch into keys (touch_keys), crawl_input, crawl_control. */

/* ---- the scene's side of the bargain with the app ---- */

/* The controller, made into the keys each place reads. In a menu the D-pad and the left stick are the
 * arrows (repeating), A Enter, B and Menu Esc, Y Space, the bumpers the pages, View Tab. In the crypt
 * and the merchant's room the sticks walk and turn (human_walk), RT casts and LT uses the ability readied,
 * the bumpers turn the spellbook, A is E, B Swift Dodge the way the left stick leans, X Q, Y L, View Tab,
 * Menu Esc, and the D-pad the bestiary, the Codex, the status bar and the wisps. */
static void pad_keys(float dt) {
    int keys = IN.ntext > 0 || IN.wheel || fabsf(IN.mdx) + fabsf(IN.mdy) > 4.0f, pads = 0;
    PADX.dodge = 0;
    PADX.wheel_req = PADX.wheel_rel = PADX.wheel_cancel = PADX.rune_hit = PADX.abil_hit = 0;
    pl_read();
    for (int k = 0; k < KEY_COUNT && !keys; k++) keys = IN.hit[k];
    if (!IN.pad) { PADX.used = PADX.run = 0; PADX.look = 0; PADX.vm_p = PADX.vm_s = 0; return; }
    PADX.plx = IN.lx; PADX.ply = IN.ly; PADX.prx = IN.rx; PADX.pry = IN.ry;
    if (PADX.fresh) {                                              /* the button held to take over counts once let go */
        PADX.fresh = 0; PADX.held = 0;
        for (int b = 0; b < PAD_COUNT; b++) if (IN.pad_down[b]) PADX.held |= 1u << b;
        if (PADX.held) PADX.used = 1;                              /* taken by a button: the hints are the pad's from the start */
    }
    uint8_t dn[PAD_COUNT], hit[PAD_COUNT];
    for (int b = 0; b < PAD_COUNT; b++) {
        if (!IN.pad_down[b]) PADX.held &= ~(1u << b);
        int ok = !(PADX.held >> b & 1);
        dn[b] = (uint8_t)(ok && IN.pad_down[b]); hit[b] = (uint8_t)(ok && IN.pad_hit[b]);
        pads |= hit[b];
    }
    pads |= IN.lx * IN.lx + IN.ly * IN.ly > 0.09f || IN.rx * IN.rx + IN.ry * IN.ry > 0.09f;
    if (pads) PADX.used = 1; else if (keys) PADX.used = 0;
    {                                                              /* View and Menu (1.0.0b18.1): together, the look; alone, each */
        int vm[2] = { PAD_VIEW, PAD_MENU };                        /* acts as it is let go, unless the other came in the while */
        PADX.vm_t += dt;
        for (int i = 0; i < 2; i++) {
            int b = vm[i], bit = 1 << i, other = 1 << (1 - i);
            if (hit[b]) {
                if (PADX.vm_p & other && PADX.vm_t < 0.15f) {      /* the second of the two, in time */
                    PADX.vm_s |= 3;
                    if (G.player && !CON.open) { PADX.look = !PADX.look; PADX.look_t = 0.0f; snd_sfx_ex(PADX.look ? SFX_PAGE : SFX_KEY, 0.6f, 0.5f); }
#ifdef CRAWL_DEBUG
                    fprintf(stderr, "EVENT pad_look %s t=%.2f\n", PADX.look ? "on" : "off", G.t);
#endif
                } else if (PADX.look) PADX.vm_s |= bit;            /* Menu alone puts the look away, below; spent either way */
                if (!(PADX.vm_p & other)) PADX.vm_t = 0.0f;
                PADX.vm_p |= bit;
                hit[b] = 0;
            } else if (PADX.vm_p & bit && !IN.pad_down[b]) {       /* let go: its own job now, if the look didn't take it */
                hit[b] = !(PADX.vm_s & bit);
                PADX.vm_p &= ~bit; PADX.vm_s &= ~bit;
            } else hit[b] = 0;
        }
    }
    if (PADX.look) {                                               /* the look: B or Menu puts it away; nothing else gets through */
        if (hit[PAD_B] || hit[PAD_MENU] || (IN.pad_hit[PAD_MENU] && !(PADX.vm_p & 1))) { PADX.look = 0; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
        return;
    }
    if (PL.swap && G.ui == UI_NONE) {                              /* the sticks swapped, in play */
        float t = IN.lx; IN.lx = IN.rx; IN.rx = t; t = IN.ly; IN.ly = IN.ry; IN.ry = t;
    }
    if (IN.ly < 0.33f) PADX.run = 0;                               /* the sprint ends as the stick stops leaning forward */
#ifdef CRAWL_DEBUG
    static double ptr_t;
    if (getenv("CRAWL_PADTRACE") && G.t - ptr_t >= 0.5) { ptr_t = G.t; fprintf(stderr, "PAD trace t=%.1f at (%.2f,%.2f) ang %.3f sticks %.2f %.2f %.2f %.2f\n", G.t, G.px, G.py, G.ang, IN.lx, IN.ly, IN.rx, IN.ry); }
#endif
    if (CON.open) return;
#define PRESS(k) (IN.hit[k] = IN.down[k] = 1)
    if (G.ui != UI_NONE) {
        static const int DIRK[4] = { KEY_W, KEY_S, KEY_A, KEY_D };
        float sx = IN.lx, sy = IN.ly;                              /* the stick a person moves through menus with (1.0.0b18.1) */
        if (PL.mstick == 1 || (PL.mstick == 2 && IN.rx * IN.rx + IN.ry * IN.ry > sx * sx + sy * sy)) { sx = IN.rx; sy = IN.ry; }
        int want[4] = { dn[PAD_UP] || sy > 0.55f, dn[PAD_DOWN] || sy < -0.55f, dn[PAD_LEFT] || sx < -0.55f, dn[PAD_RIGHT] || sx > 0.55f };
        if (!PADX.menu)                                            /* just opened: what opened it, or a walk, isn't a turn of the page */
            for (int d = 0; d < 4; d++) { PADX.on[d] = (uint8_t)want[d]; PADX.rep[d] = 0.4f; }
        PADX.menu = 1;
        for (int d = 0; d < 4; d++) {
            if (!want[d]) { PADX.on[d] = 0; continue; }
            IN.down[DIRK[d]] = 1;
            if (!PADX.on[d]) { PADX.on[d] = 1; PADX.rep[d] = 0.4f; IN.hit[DIRK[d]] = 1; }
            else if ((PADX.rep[d] -= dt) <= 0.0f) { PADX.rep[d] = 0.12f; IN.hit[DIRK[d]] = 1; }
        }
        if (hit[PAD_A]) PRESS(KEY_ENTER);
        if (hit[PAD_B] || hit[PAD_MENU]) PRESS(KEY_ESC);
        if (hit[PAD_Y]) PRESS(KEY_SPACE);
        if (hit[PAD_X] && aug_on()) PRESS(KEY_BACK);               /* X: back, with the augments (unbinding a line, taking back: 1.0.0b21) */
        if (hit[PAD_RB]) PRESS(G.ui == UI_ALIGN ? KEY_PGDN : KEY_D);   /* at the Lampstand, the lines */
        if (hit[PAD_LB]) PRESS(G.ui == UI_ALIGN ? KEY_PGUP : KEY_A);
        if (hit[PAD_VIEW]) PRESS(KEY_TAB);
        PADX.up_t = PADX.dn_t = 0.0f; PADX.up_w = PADX.dn_w = 0;
        return;
    }
    PADX.menu = 0;
    if (G.map_on && G.player && !G.shop) {                         /* the map up: the D-pad pans it, B closes it, whatever the layout */
        if (hit[PAD_UP]) PRESS(KEY_UP);
        if (hit[PAD_DOWN]) PRESS(KEY_DOWN);
        if (hit[PAD_LEFT]) PRESS(KEY_LEFT);
        if (hit[PAD_RIGHT]) PRESS(KEY_RIGHT);
        if (hit[PAD_B]) PRESS(KEY_M);
        hit[PAD_UP] = hit[PAD_DOWN] = hit[PAD_LEFT] = hit[PAD_RIGHT] = hit[PAD_B] = 0;
        dn[PAD_UP] = dn[PAD_DOWN] = dn[PAD_LEFT] = dn[PAD_RIGHT] = dn[PAD_B] = 0;
    }
    {                                                              /* the layout: each button's state goes to its job (1.0.0b18.1); */
        uint8_t jd[PAD_COUNT], jh[PAD_COUNT];                      /* from here on a PAD_* names a job, by the button it began on */
        for (int b = 0; b < PAD_COUNT; b++) { jd[PL.job[b]] = dn[b]; jh[PL.job[b]] = hit[b]; }
        memcpy(dn, jd, sizeof jd); memcpy(hit, jh, sizeof jh);
    }
    {                                                              /* the D-pad's up and down: tapped, the map and the bar; held, the wheels */
        float *ht[2] = { &PADX.up_t, &PADX.dn_t };
        int *hw[2] = { &PADX.up_w, &PADX.dn_w };
        static const int DB[2] = { PAD_UP, PAD_DOWN }, TAPK[2] = { KEY_M, KEY_H }, WK[2] = { WH_GIFTS, WH_SPELLS };
        for (int i = 0; i < 2; i++) {
            if (G.map_on && G.player && !G.shop) { *ht[i] = 0.0f; *hw[i] = 0; continue; }   /* the map up: they pan it, above */
            if (dn[DB[i]]) {
                *ht[i] += dt;
                if (*ht[i] >= WHEEL_HOLD && !*hw[i]) { *hw[i] = 1; PADX.wheel_req = WK[i]; PADX.w_hand = pad_hand(pl_on(DB[i])); }
            } else if (*ht[i] > 0.0f) {
                if (!*hw[i]) PRESS(TAPK[i]); else PADX.wheel_rel = WK[i];
                *ht[i] = 0.0f; *hw[i] = 0;
            }
        }
    }
    if (dn[PAD_X]) { IN.down[KEY_Q] = 1; PADX.q_hand = pad_hand(pl_on(PAD_X)); }   /* X held, as Q held: the runes' wheel (1.0.0b18.1) */
    if (WH.open) {                                                 /* a wheel up: the stick points, B or Menu puts it away */
        if (hit[PAD_B] || hit[PAD_MENU]) PADX.wheel_cancel = 1;
        return;
    }
    if (hit[PAD_A]) PRESS(KEY_E);
    if (hit[PAD_X]) PRESS(KEY_Q);
    if (hit[PAD_VIEW]) PRESS(KEY_TAB);
    if (hit[PAD_MENU]) PRESS(KEY_ESC);
    if (hit[PAD_LEFT]) PRESS(KEY_B);
    if (hit[PAD_RIGHT]) PRESS(KEY_C);
    if (hit[PAD_Y]) IN.hit[KEY_L] = 1;
    if (dn[PAD_Y]) IN.down[KEY_L] = 1;                             /* held, for the list when L casts at a tap */
    if (dn[PAD_RT]) IN.down[KEY_MOUSE1] = 1;
    if (hit[PAD_RT]) IN.hit[KEY_MOUSE1] = 1;
    if (hit[PAD_LT]) PRESS(KEY_MOUSE2);
    if (hit[PAD_RB]) PADX.rune_hit = 1;                            /* the rune readied (1.0.0b16: the spells are on a wheel now) */
    if (hit[PAD_LB]) PADX.abil_hit = 1;                            /* the boss's gift readied */
    if (hit[PAD_LS]) PADX.run = !PADX.run;                         /* a toggle (docs/PLAN-ai.md, 24) */
    if (hit[PAD_B]) {                                              /* Swift Dodge: the way the stick leans, else back */
        PADX.dodge = 1;
        PADX.dodge_a = G.ang + (IN.lx * IN.lx + IN.ly * IN.ly > 0.09f ? atan2f(IN.lx, IN.ly) : PI_F);
    }
#undef PRESS
}

/* ---- touch (docs/PLAN-mobile.md, Part C): the stones' places on the stage, and the fingers into keys and sticks ---- */
static int title_at_prompt(void);
static int touch_ring(void) { return clampi(IN.stage_h / 9, 18, 30); }
static void touch_geo(TRect r[NTB]) {
    int sw = IN.stage_w, sh = IN.stage_h, l = IN.safe_l, t = IN.safe_t, rr = IN.safe_r, b = IN.safe_b;
    if (sw - l - rr < W) l = rr = 0;                                /* as app_stage_place has it, the drift aside */
    int col = clampi((sw - l - rr - W) / 2, 0, 112);               /* the black beside the picture: never over it */
    int cw = col - 6, ch = clampi(sh / 4, 38, 60), bh = clampi(sh / 7, 24, 34), gap = 3;
    int lx = l + 3, rx = sw - rr - 3 - cw, y2 = t + 3 + ch + gap, y3 = y2 + bh + gap, y4 = y3 + bh + gap;
    r[TB_CORNER_L] = (TRect){ lx, t + 3, cw, ch };
    r[TB_CORNER_R] = (TRect){ rx, t + 3, cw, ch };
    r[TB_WISPS] = (TRect){ lx, y2, cw, bh };                      /* WISPS over MAP on the left, WARDROBE on the right */
    r[TB_MAP] = (TRect){ lx, y3, cw, bh };
    r[TB_WARD] = (TRect){ rx, y2, cw, bh };
    r[TB_STRIP_L] = (TRect){ lx, y4, cw, imax(24, sh - b - 3 - y4) };
    r[TB_STRIP_R] = (TRect){ rx, y3, cw, imax(24, sh - b - 3 - y3) };
    if (cw < 20) for (int k = 0; k < NTB; k++) r[k].w = 0;         /* no room: no stones (the page leaves room) */
}
enum { TC_PLAY, TC_MENU, TC_TITLE, TC_WHEEL };
static int touch_ctx(void) { return title_up() ? TC_TITLE : WH.open ? TC_WHEEL : G.ui != UI_NONE ? TC_MENU : TC_PLAY; }
static int touch_shown(int k, int ctx) {                           /* which stones there are, by what is up */
    if (ctx == TC_WHEEL) return k == TB_STRIP_R;
    if (ctx == TC_TITLE) return k == TB_STRIP_L || k == TB_STRIP_R;
    if (ctx == TC_MENU) return k != TB_WISPS && k != TB_MAP;
    return 1;
}
static int in_rect(const TRect *r, float x, float y) { return x >= r->x && y >= r->y && x < r->x + r->w && y < r->y + r->h; }
static void touch_wheel_tap(float x, float y) {                    /* a tap with a wheel up: a slot, the hub, or away */
    float fx = x - (float)IN.pic_x, fy = y - (float)IN.pic_y, R = WH.open == WH_GIFTS ? 60.0f : 54.0f;
    float d = hypotf(fx - (float)(W / 2), fy - 88.0f), a = atan2f(fy - 88.0f, fx - (float)(W / 2));
    if (d < R * 0.45f) {                                           /* the hub: the other wheel */
        int other = WH.open == WH_GIFTS ? WH_SPELLS : WH_GIFTS;
        wheel_close(0); wheel_open(other, 2);
        return;
    }
    if (d > R + 24.0f || WH.n <= 0) { wheel_close(0); snd_sfx_ex(SFX_KEY, 0.5f, 0.5f); return; }
    int best = 0; float bd = 1e9f;
    for (int i = 0; i < WH.n; i++) { float e = fabsf(wrap_pi(a - WH.s[i].ang)); if (e < bd) { bd = e; best = i; } }
    WH.hi = best;
    wheel_close(1);
}
/* Turning by a finger slid on the play field: the view follows it, a whole picture's width 150 degrees at 100% (PL.tturn in
 * tenths). Smoothed by a filter whose lag falls off as the finger quickens (a 1-euro filter: steady when it creeps or rests,
 * close behind when it sweeps); lifted, what the smoothing still owed is turned at once, so a flick lands where it went. */
#define TOUCH_TURN (2.618f / (float)W)                             /* radians a stage pixel at 100% */
static void touch_follow(float *sx, float *sv, float x, float dt, int lifted) {
    float prev = *sx;
    if (lifted || PL.tsmooth <= 0 || dt <= 0.0f) *sx = x;
    else {
        float raw = (x - prev) / dt, fmin = 0.8f * powf(1.3f, (float)(10 - PL.tsmooth));   /* 0.8 Hz at 10, 3 at 5, 8.5 at 1 */
        *sv += (raw - *sv) * (1.0f - expf(-dt * TAU_F * 8.0f));
        float fc = fmin + 0.06f * fabsf(*sv);                      /* stage pixels a second: a sweep of 300 adds 18 Hz */
        *sx += (x - *sx) * (1.0f - expf(-dt * TAU_F * fc));
    }
    IN.mdx += (*sx - prev) * TOUCH_TURN * (float)PL.tturn * 0.1f / MOUSE_TURN;
}
static void touch_keys(float dt) {
    if (!IN.touch || !IN.stage_w) { TCH.nf = 0; TCH.used = 0; return; }
    pl_read();
    int keys = IN.ntext > 0 || IN.wheel || fabsf(IN.mdx) + fabsf(IN.mdy) > 4.0f;
    for (int k = 0; k < KEY_COUNT && !keys; k++) keys = IN.hit[k];
    if (keys || PADX.used) TCH.used = 0;
    if (IN.nfinger) TCH.used = 1;                                  /* the finger that took over counts too */
    TRect R[NTB];
    touch_geo(R);
    int ctx = touch_ctx(), ring = touch_ring();
    float rg = (float)ring;
    memset(TCH.press, 0, sizeof TCH.press);
    TCH.walk = 0;
    TCH.walk_up += dt;
    if (CON.open) { TCH.nf = 0; return; }
#define PRESS(k) (IN.hit[k] = IN.down[k] = 1)
    for (int i = 0; i < IN.nfinger; i++) {                         /* each finger the shell has, matched to what we keep of it */
        const Finger *fg = &IN.finger[i];
        int j = 0;
        while (j < TCH.nf && TCH.f[j].id != fg->id) j++;
        if (j == TCH.nf) {
            if (!fg->down || TCH.nf >= TOUCH_MAX) continue;
            TCH.nf++;
            memset(&TCH.f[j], 0, sizeof TCH.f[j]);
            TCH.f[j].id = fg->id; TCH.f[j].x0 = TCH.f[j].px = TCH.f[j].bx = TCH.f[j].sx = fg->x; TCH.f[j].y0 = TCH.f[j].py = TCH.f[j].by = fg->y;
            TCH.f[j].spent = TCH.fresh;
            TCH.used = 1;
            int role = TF_TAP, btn = -1;
            for (int k = 0; k < NTB && btn < 0; k++) if (k != TB_STRIP_L && k != TB_STRIP_R && touch_shown(k, ctx) && in_rect(&R[k], fg->x, fg->y)) btn = k;
            if (btn >= 0) role = TF_BTN;
            else if (ctx == TC_PLAY && in_rect(&R[TB_STRIP_R], fg->x, fg->y)) { role = TF_CAST; IN.hit[KEY_MOUSE1] = 1; }
            else if (ctx == TC_PLAY && in_rect(&R[TB_STRIP_L], fg->x, fg->y)) role = TF_PEND;
            else if (ctx == TC_PLAY) role = fg->x < IN.stage_w * 0.5f ? TF_WALK : TF_TURN;
            else if (in_rect(&R[TB_STRIP_L], fg->x, fg->y)) btn = TB_STRIP_L;
            else if (in_rect(&R[TB_STRIP_R], fg->x, fg->y)) btn = TB_STRIP_R;
            if (btn == TB_STRIP_L || btn == TB_STRIP_R) role = TF_BTN;
            TCH.f[j].role = role; TCH.f[j].btn = btn;
            if ((role == TF_WALK || role == TF_PEND) && TCH.walk_up < 0.3f) TCH.dodge_arm = 1;   /* touched twice: a dodge, once it leans */
        }
        typeof(TCH.f[0]) *f = &TCH.f[j];
        f->t += dt;
        float dx = fg->x - f->x0, dy = fg->y - f->y0;
        if (dx * dx + dy * dy > 36.0f) f->moved = 1;
        if (f->spent) { f->px = fg->x; f->py = fg->y; continue; }
        if (f->role == TF_PEND && (f->moved || f->t > 0.25f)) f->role = TF_WALK;   /* it stayed or slid: the walking stick */
        if (f->role == TF_BTN && f->btn >= 0) TCH.press[f->btn] = 1;
        if (f->role == TF_BTN && !f->held && f->t >= 0.35f && ctx == TC_PLAY) {   /* the corners held */
            if (f->btn == TB_CORNER_L) { f->held = 1; PRESS(KEY_ESC); }
            else if (f->btn == TB_CORNER_R) { f->held = 1; PADX.wheel_req = WH_SPELLS; TCH.wheel = 1; }
        }
        if (f->role == TF_BTN && f->btn == TB_WISPS && ctx == TC_PLAY) { IN.down[KEY_L] = 1; if (fg->down) IN.hit[KEY_L] = 1; }
        if (ctx == TC_PLAY && f->role == TF_CAST) {
            IN.down[KEY_MOUSE1] = 1;
            touch_follow(&f->sx, &f->sv, fg->x, dt, fg->up);       /* aim while casting */
            TCH.press[TB_STRIP_R] = 1;
        }
        if (f->role == TF_WALK || (f->role == TF_TURN && ctx != TC_PLAY) || (f->role == TF_TAP && ctx != TC_PLAY && f->moved)) {
            float ox = fg->x - f->bx, oy = fg->y - f->by, d = hypotf(ox, oy);
            if (d > rg * 1.8f && f->role == TF_WALK) { f->bx = fg->x - ox / d * rg * 1.8f; f->by = fg->y - oy / d * rg * 1.8f; ox = fg->x - f->bx; oy = fg->y - f->by; d = rg * 1.8f; }
            float lx = ox / rg, ly = -oy / rg, m = hypotf(lx, ly);
            if (m > 1.0f) { lx /= m; ly /= m; }
            if (m < 0.17f) lx = ly = 0.0f;
            if (ctx == TC_PLAY) {
                IN.lx = lx; IN.ly = ly;
                if (d > rg * 1.35f && -oy > d * 0.75f) IN.down[KEY_SHIFT] = 1;   /* pushed forward past the ring: the sprint */
                if (TCH.dodge_arm && m > 0.6f) { TCH.dodge_arm = 0; PADX.dodge = 1; PADX.dodge_a = G.ang + atan2f(lx, ly); }
                TCH.walk = 1; TCH.wbx = f->bx; TCH.wby = f->by; TCH.wx = fg->x; TCH.wy = fg->y;
            } else {                                               /* a menu: a direction, repeating as the pad's */
                static const int DIRK[4] = { KEY_W, KEY_S, KEY_A, KEY_D };
                int want[4] = { ly > 0.6f, ly < -0.6f, lx < -0.6f, lx > 0.6f };
                for (int k = 0; k < 4; k++) {
                    if (!want[k]) { TCH.on[k] = 0; continue; }
                    IN.down[DIRK[k]] = 1;
                    if (!TCH.on[k]) { TCH.on[k] = 1; TCH.rep[k] = 0.4f; IN.hit[DIRK[k]] = 1; }
                    else if ((TCH.rep[k] -= dt) <= 0.0f) { TCH.rep[k] = 0.12f; IN.hit[DIRK[k]] = 1; }
                }
            }
        }
        if (f->role == TF_TURN && ctx == TC_PLAY) touch_follow(&f->sx, &f->sv, fg->x, dt, fg->up);   /* the view follows it */
        if (ctx != TC_PLAY) { f->sx = fg->x; f->sv = 0.0f; }       /* a menu between: it follows on from here */
        f->px = fg->x; f->py = fg->y;
        if (!fg->up) continue;
        int tap = !f->moved && f->t < 0.25f;                       /* lifted: a tap does its stone's thing */
        if (f->role == TF_WALK || f->role == TF_PEND) TCH.walk_up = 0.0f;
        if (f->role == TF_PEND && tap) PRESS(KEY_E);
        if (ctx == TC_WHEEL && tap && f->role != TF_BTN) touch_wheel_tap(fg->x, fg->y);
        if (ctx == TC_TITLE && tap && f->role == TF_TAP && title_at_prompt()) PRESS(KEY_ENTER);
        if (ctx == TC_MENU && tap && f->role == TF_TAP && G.ui == UI_TUTOR) PRESS(KEY_ENTER);   /* a lesson's page: a tap goes on */
        if (ctx == TC_PLAY && tap && (f->role == TF_TURN || f->role == TF_WALK) && fg->y >= IN.pic_y + H - 36 && fg->y < IN.pic_y + H
            && fg->x >= IN.pic_x && fg->x < IN.pic_x + W) PRESS(KEY_H);   /* a tap on the bar's place: the bar */
        if (f->role == TF_BTN && !f->held && f->t < 0.6f) {
            int b = f->btn;
            if (ctx == TC_PLAY) {
                if (b == TB_CORNER_L) PADX.rune_hit = 1;
                else if (b == TB_CORNER_R) PRESS(KEY_MOUSE2);
                else if (b == TB_MAP && RUN.has_map) PRESS(KEY_M);
                else if (b == TB_WARD) PRESS(KEY_TAB);
            } else if (ctx == TC_WHEEL) { if (b == TB_STRIP_R) { wheel_close(0); snd_sfx_ex(SFX_KEY, 0.5f, 0.5f); } }
            else {                                                 /* a menu or the title: the stones are the keys they say */
                if (b == TB_STRIP_L) PRESS(KEY_ENTER);
                else if (b == TB_STRIP_R) PRESS(KEY_ESC);
                else if (b == TB_CORNER_L) PRESS(KEY_BACK);
                else if (b == TB_CORNER_R) PRESS(KEY_SPACE);
                else if (b == TB_WARD) PRESS(KEY_TAB);
            }
        }
    }
#undef PRESS
    int n = 0;                                                     /* what is kept of the fingers lifted, or gone, goes */
    for (int j = 0; j < TCH.nf; j++) {
        int alive = 0;
        for (int i = 0; i < IN.nfinger; i++) if (IN.finger[i].id == TCH.f[j].id && !IN.finger[i].up) alive = 1;
        if (alive) TCH.f[n++] = TCH.f[j];
    }
    TCH.nf = n;
    TCH.fresh = 0;
    if (ctx != TC_PLAY) for (int k = 0; k < 4; k++) if (!IN.down[k == 0 ? KEY_W : k == 1 ? KEY_S : k == 2 ? KEY_A : KEY_D]) TCH.on[k] = 0;
}

static void crawl_input(const Input *in, double dt) {
    IN = *in; IN_DT = dt;
    if (!title_up()) pad_keys((float)dt);                          /* the title reads its own (1.0.0b21.1) */
    touch_keys((float)dt);                                         /* and by touch, the fingers (docs/PLAN-mobile.md) */
}

static void crawl_control(int what) {
    if (what == CTRL_PAUSE) {
        if (title_up()) return;                                    /* the title waits as it is */
        if (CON.open) { CON.open = 0; snd_hold(0); }
        if (G.player && !G.ui && G.status_ret != PLAY_QUIT) open_ui(UI_QUIT);
        return;
    }
    if (what == CTRL_PEEK) { if (!G.player) { pop_readout(); G.peek = 0.15f; } return; }
    if (title_enabled()) { title_open(); return; }                 /* the title first (1.0.0b21.1); a choice there makes it yours */
    show_bar();
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE;
    PADX.fresh = 1; TCH.fresh = 1;                                 /* the button (or finger) that took over isn't a press */
    LEDGER.yours = 1;                                              /* the dream is over: from now on the run is yours */
    if (!G.quit_line) G.quit_line = 1 + rnd(97);                    /* the crypt's arguments, from somewhere new each time */
    if (G.shop) { G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f; old_spells_said(); old_lines_said(); }   /* no cast left half-made */
    else if (!G.exiting && !G.tally && !G.dead) human_begin();
    G.hint_t = TU.on && g_drawn ? 0.0f : 6.0f;                     /* the lessons' first page says what the card would */
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f); snd_sfx_ex(SFX_WHOOSH, 0.6f, 0.5f);
    say("THE CONTROLS ARE YOURS");
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_XP")) RUN.xp += atoi(getenv("CRAWL_XP"));   /* testing: experience banked, not yet claimed */
    if (getenv("CRAWL_TPPED") && G.shop && G.ped_n) {              /* testing: before her pedestal k, facing it (1.0.0b18.1) */
        int k = clampi(atoi(getenv("CRAWL_TPPED")) - 1, 0, G.ped_n - 1), bx, by;
        float best = -1.0f, tx = 0.0f, ty = 0.0f;
        for (int i = 0; i < 16; i++) {                             /* a step and a half off, on the side away from her */
            float a = (float)i * (TAU_F / 16.0f), x = G.ped_x[k] + cosf(a) * 1.4f, y = G.ped_y[k] + sinf(a) * 1.4f, s = hypotf(x - CAT_X, y - CAT_Y);
            if (!body_blocked(x, y, &bx, &by) && s > best) { best = s; tx = x; ty = y; }
        }
        if (best >= 0.0f) { G.px = tx; G.py = ty; G.cx = G.tx = (int)tx; G.cy = G.ty = (int)ty; G.ang = atan2f(G.ped_y[k] - ty, G.ped_x[k] - tx); }
        fprintf(stderr, "TPPED %d of %d at (%.2f,%.2f) ang %.3f\n", k + 1, G.ped_n, G.px, G.py, G.ang);
    }
    if (getenv("CRAWL_TPAT") && !G.shop) {                         /* testing: at x,y facing deg (1.0.0b18.1, for frames) */
        float x = 0.0f, y = 0.0f, a = 0.0f;
        if (sscanf(getenv("CRAWL_TPAT"), "%f,%f,%f", &x, &y, &a) >= 2) {
            G.px = x; G.py = y; G.cx = G.tx = (int)x; G.cy = G.ty = (int)y; G.ang = a * (float)M_PI / 180.0f;
            fprintf(stderr, "TPAT (%.2f,%.2f) ang %.3f\n", G.px, G.py, G.ang);
        }
    }
    if (getenv("CRAWL_TPWARD") && !G.shop && WD.n) {               /* testing: before soul-lamp k, facing it (1.0.0b18.1) */
        const WLamp *L = &WD.l[clampi(atoi(getenv("CRAWL_TPWARD")) - 1, 0, WD.n - 1)];
        G.px = L->cx + 0.5f - DX[L->d] * 0.1f; G.py = L->cy + 0.5f - DY[L->d] * 0.1f; G.cx = G.tx = L->cx; G.cy = G.ty = L->cy;
        G.ang = atan2f((float)DY[L->d], (float)DX[L->d]);
        fprintf(stderr, "TPWARD lamp %d at (%.2f,%.2f) ang %.3f\n", (int)(L - WD.l), G.px, G.py, G.ang);
    }
    if (getenv("CRAWL_TPEXIT") && !G.shop && G.exit_fx >= 0) tp_exit();    /* testing: straight to the exit, facing it */
    if (getenv("CRAWL_TPSECRET") && !G.shop && G.nsecrets) tp_secret(atoi(getenv("CRAWL_TPSECRET")) % G.nsecrets);
    if (getenv("CRAWL_TPALTAR") && !G.shop) tp_altar();
    if (getenv("CRAWL_TPHZ") && !G.shop) tp_hz();                  /* before the first fixed hazard (1.0.0b13) */
    if (getenv("CRAWL_TPPUZZLE") && !G.shop) tp_puzzle();          /* before the puzzle's first piece */
    if (getenv("CRAWL_TPMSG") && !G.shop && G.msg_deco >= 0) tp_near(G.decos[G.msg_deco].x, G.decos[G.msg_deco].y);   /* before what hides its message */
    if (getenv("CRAWL_TPBSHRINE") && !G.shop && G.bsh_x >= 0) tp_near(G.bsh_x + 0.5f, G.bsh_y + 0.5f);   /* before the Black Shrine */
    if (getenv("CRAWL_TPTRIAL") && !G.shop) tp_trial();            /* before a trial's door */
    if (getenv("CRAWL_HP")) G.hp = clampi(atoi(getenv("CRAWL_HP")), 1, G.max_hp);   /* hurt, for testing (1.0.0b15) */
    if (getenv("CRAWL_TPMEND") && !G.shop && G.nrest) {            /* before a place to mend, facing it (1.0.0b15) */
        G.px = G.rest_fx[0] + 0.5f; G.py = G.rest_fy[0] + 0.5f;
        G.ang = atan2f((float)(G.rest_y[0] - G.rest_fy[0]), (float)(G.rest_x[0] - G.rest_fx[0])); human_begin();
    }
    if (getenv("CRAWL_TPAMBUSH") && !G.shop && G.namb) {           /* outside an ambush's door, its sign at your feet (1.0.0b18) */
        const Ambush *a = &G.amb[0];
        int dx = a->ix - a->ox, dy = a->iy - a->oy, bx = a->ox, by = a->oy;
        for (int k = 1; k <= 2; k++) { int x = a->ox - dx * k, y = a->oy - dy * k; if (x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1 || G.map[y][x].kind != K_FLOOR) break; bx = x; by = y; }
        G.px = bx + 0.5f; G.py = by + 0.5f; G.ang = atan2f((float)dy, (float)dx); human_begin();
    }
    if (getenv("CRAWL_TPMARK") && !G.shop && G.nmk) {              /* a few steps from a thing that wants a spell, facing it (1.0.0b16) */
        const Marked *m = &G.mk[clampi(atoi(getenv("CRAWL_TPMARK")) - 1, 0, G.nmk - 1)];
        int best = -1, bx = 0, by = 0;
        for (int y = (int)m->y - 4; y <= (int)m->y + 4; y++)
            for (int x = (int)m->x - 4; x <= (int)m->x + 4; x++) {
                if (x < 0 || y < 0 || x >= MAPN || y >= MAPN || G.map[y][x].kind != K_FLOOR || !line_clear(x + 0.5f, y + 0.5f, m->x, m->y)) continue;
                int d = abs(x - (int)m->x) + abs(y - (int)m->y), sc = 10 - abs(d - 3);
                if (sc > best) { best = sc; bx = x; by = y; }
            }
        if (best >= 0) { G.px = bx + 0.5f; G.py = by + 0.5f; G.ang = atan2f(m->y - G.py, m->x - G.px); human_begin(); }
        if (RUN.gear.atk[LINE_ATK[m->el]] >= 0) pick_spell(LINE_ATK[m->el]);   /* its line readied */
    }
    if (getenv("CRAWL_THINGS") && !G.shop && G.nthing) {           /* among the test's things, facing the first */
        float tx = G.thing[0].x, ty = G.thing[0].y;
        for (int d = 0; d < 4; d++) {
            int x = (int)tx + DX[d] * 2, y = (int)ty + DY[d] * 2;
            if (!hz_floor(x, y)) continue;
            G.px = x + 0.5f; G.py = y + 0.5f; G.ang = atan2f(ty - G.py, tx - G.px); human_begin();
            break;
        }
    }
    if (getenv("CRAWL_TPDOOR") && !G.shop) tp_door();              /* before a secret's doorway, its room open */
    if (getenv("CRAWL_TPROOM") && !G.shop) tp_room(atoi(getenv("CRAWL_TPROOM")));   /* at the end of the first room of that type, RT_* */
    if (getenv("CRAWL_TPHOLD") && !G.shop) tp_hold(atoi(getenv("CRAWL_TPHOLD")));   /* holding a doorway (step 7) */
    if (getenv("CRAWL_TPPORTAL") && G.shop) { G.px = PORTAL_X + 0.5f; G.py = PORTAL_Y - 1.4f; G.ang = PI_F * 0.5f; G.cx = PORTAL_X; G.cy = PORTAL_Y - 2; }   /* before the Portal */
    if (getenv("CRAWL_TPSTONE") && G.shop) { G.px = STONE_AT[0][0] + 0.5f; G.py = STONE_AT[0][1] + 1.6f; G.ang = -PI_F * 0.5f; G.cx = STONE_AT[0][0]; G.cy = STONE_AT[0][1] + 1; }   /* before the newest stone */
    if (getenv("CRAWL_TOAST")) toast(SP_SKEL_A, PF_BONE, RK_II, 0, "SLAYER OF THE ORDER", "40 GREEN SKELETONS");   /* a pop-up, for the shots */
    if (getenv("CRAWL_CLAIM")) {                                    /* her card as it would read, nothing taken: for the shots */
        snprintf(G.claim_text, sizeof G.claim_text, "YOU TRIED TO CHEAT THE DEAD OF THEIR BOUNTY. WHATEVER YOU MEANT BY IT, SHE HAS TAKEN THEIR CLAIM: "
                 "1,250 GOLD, AND THE UNCOMMON TREASURE HUNTER BRACER OF SHADOWS. THE LEDGER IS SIGNED.");
        G.claim_card = 1; G.claim_t = 0.0f;
    }
    if (getenv("CRAWL_TPSHRINE") && G.shop) { G.px = SHRINE_X; G.py = SHRINE_Y - 1.6f; G.ang = PI_F * 0.5f; G.cx = (int)G.px; G.cy = (int)G.py; }   /* before the Rune Shrine */
    if (getenv("CRAWL_LETGO") && G.shop && RUN.gear.bag_size) {    /* the full bag's question, over the first ware for a worn slot */
        uint32_t rs = 0x2545f491u;
        for (int i = 0; i < RUN.gear.bag_size; i++) if (RUN.gear.bag[i].kind == IK_NONE) RUN.gear.bag[i] = piece_roll(IK_RING, 4, Q_RARE, &rs);
        for (int i = 0; i < G.ped_n; i++)
            if (G.offers[i].kind == GK_PIECE && piece_slot(&RUN.gear, G.offers[i].piece.kind) >= 0) { G.shop_letgo = i; G.ui = UI_LETGO; G.ui_sel = 0; break; }
    }
    if (getenv("CRAWL_TPPORTRAIT") && G.shop && POR.n) {          /* before portrait k (1 the first), facing it, d cells from its
                                                                      cell's middle (k,d; 2.1 if not given) */
        int k = 1; float d = 2.1f;
        sscanf(getenv("CRAWL_TPPORTRAIT"), "%d,%f", &k, &d);
        const PorAt *a = &POR.at[por_head(clampi(k - 1, 0, POR.n - 1))];
        float mx = a->boss ? (float)DY[a->fd] * 0.5f : 0.0f, my = a->boss ? -(float)DX[a->fd] * 0.5f : 0.0f;   /* a boss's: between its faces */
        G.px = (float)a->x + 0.5f + mx + (float)DX[a->fd] * d; G.py = (float)a->y + 0.5f + my + (float)DY[a->fd] * d;
        G.ang = heading_angle((a->fd + 2) & 3); G.cx = (int)G.px; G.cy = (int)G.py;
    }
    if (getenv("CRAWL_TPHALL") && G.shop) {                        /* testing: at x,y facing deg in her hall (the portraits' cost) */
        float x = 0, y = 0, a = 0;
        if (sscanf(getenv("CRAWL_TPHALL"), "%f,%f,%f", &x, &y, &a) == 3) { G.px = x; G.py = y; G.ang = a * PI_F / 180.0f; G.cx = (int)x; G.cy = (int)y; }
    }
    if (getenv("CRAWL_TPCAT") && G.shop) { G.px = CAT_X + 1.3f; G.py = CAT_Y + 0.4f; G.ang = face_of(CAT_X, CAT_Y); G.cx = (int)G.px; G.cy = (int)G.py; }   /* before the merchant */
    if (getenv("CRAWL_TPREPO") && G.shop) { G.px = 5.5f; G.py = 14.5f; G.ang = PI_F; G.cx = 5; G.cy = 14; }   /* before the Repository's niches */
    if (getenv("CRAWL_TPLECTERN") && G.shop) { G.px = LECTERN_X + 0.5f; G.py = LECTERN_Y - 1.6f; G.ang = PI_F * 0.5f; G.cx = LECTERN_X; G.cy = LECTERN_Y - 2; }   /* before the Compendium */
    if (getenv("CRAWL_TPTOTEM") && G.shop) { G.px = 6.9f; G.py = 14.6f; G.ang = face_of(TOTEM_X, TOTEM_Y); G.cx = 6; G.cy = 14; }   /* before the Totem */
    if (getenv("CRAWL_TPBELL") && G.shop) { G.px = BELL_X + 0.5f; G.py = BELL_Y + 2.3f; G.ang = face_of(BELL_X + 0.5f, BELL_Y + 0.5f); G.cx = BELL_X; G.cy = BELL_Y + 2; }
    if (getenv("CRAWL_TPFONT") && G.shop) { G.px = FONT_X + 0.5f; G.py = FONT_Y + 2.3f; G.ang = face_of(FONT_X + 0.5f, FONT_Y + 0.5f); G.cx = FONT_X; G.cy = FONT_Y + 2; }
    if (getenv("CRAWL_TPLAMP") && G.shop) { G.px = LAMP_X + 0.5f; G.py = LAMP_Y + 2.8f; G.ang = face_of(LAMP_X + 0.5f, LAMP_Y + 0.5f); G.cx = LAMP_X; G.cy = LAMP_Y + 2; }   /* before the Lampstand */
    if (getenv("CRAWL_TPBOSS") && !G.shop && !PK.depth && G.boss >= 0 && G.bstart_x >= 0) {   /* in the boss's antechamber, facing the arch */
        G.px = G.bstart_x + 0.5f; G.py = G.bstart_y + 0.5f; G.ang = 0.0f; human_begin();
    }
    fprintf(stderr, "EVENT control t=%.2f at (%.2f,%.2f) ang %.3f shop %d pocket %d\n", G.t, G.px, G.py, G.ang, G.shop, PK.depth);
#endif
}

static int crawl_status(void) { return G.player ? G.status_ret : PLAY_AUTO; }

/* What a save keeps: the level to start from, health, gold, souls and the gear, and since version 2
 * the run's record (kills, secrets, levels cleared, achievements). Not where you stood: you come back
 * to the start of the level, so its gold and souls are taken back out. Version 3 packs the gear field
 * by field (what is worn, the bag, the bones, a ware held at the merchant's) and keeps souls in
 * tenths. Version 4 adds four bytes after the record: the wisp L casts and the menu's toggle. Version
 * 5 keeps runes by quality, not tier, and adds 32 bytes for the Rune Shrine: the coins, its rank, its
 * attunements and its nine slots. Older saves' runes come by their tier (rune_from_tier), and a rune
 * held at the merchant's gives back what holding it cost. Version 6 (runs of ten) adds the rest of the
 * record: levels cleared to 40, and room for more achievements; older records' descents of twenty
 * become descents of forty (rec_from_old). Versions 1 and 2 still load: their spells and runes stay,
 * their robe, hood and best ring become Common pieces of their tier, and version 1 has an empty record. */
#define SAVE_MAGIC 0x31535648u              /* "HVS1" */
#define OLD_GEAR 46                         /* versions 1 and 2: the gear as it was, copied raw */
#define SAVE_V1 (28 + OLD_GEAR)
#define PIECE_BYTES (6 + 3 * PIECE_STATS)
#define GEAR_BYTES (NATK + NDEF + NSLOT * PIECE_BYTES + 2 + BAG_MAX * PIECE_BYTES + 2 + 1 + 8 + PIECE_BYTES)
#define SAVE_V3 (28 + GEAR_BYTES)
static void put32(uint8_t *p, int32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)((uint32_t)v >> (8 * i)); }
static int32_t get32(const uint8_t *p) { uint32_t v = 0; for (int i = 0; i < 4; i++) v |= (uint32_t)p[i] << (8 * i); return (int32_t)v; }
static void put16(uint8_t *p, int v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static int get16(const uint8_t *p) { return p[0] | p[1] << 8; }

