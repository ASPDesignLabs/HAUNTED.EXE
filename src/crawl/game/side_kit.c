/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The builder's kit (the owner, 27 November 2026; docs/PLAN-side.md, 2i): ten pieces for the World Editor's passages. Fixed: the
 * hanging chain, the bone springboard, wet slate (S), cobweb (W), cracked masonry. Moved by logic: bell stones and their bells, ghost
 * stone, the press, the sarcophagus, the bone-mill belt. Each answers the triggers as side_text.c says. The dream knows none of them
 * yet: the owner plays them first. */

static uint8_t KSET;                                               /* the bell stones: 0 A's up, 1 B's up */
static float KSET_T;                                               /* since they last swapped, for the drawing */
static uint8_t KUP[SD_THINGS];                                     /* a bell stone or ghost stone solid now (it waits for room to rise) */
static float KT[SD_THINGS];                                        /* each one's clock: a ghost's, a press's, a bell's, a spring's squash,
                                                                      masonry's rubble */
static uint8_t KPH[SD_THINGS];                                     /* a press: 0 up, 1 warning, 2 coming down, 3 down, 4 rising */
static float KY[SD_THINGS];                                        /* a press's head: how far below its slot's top, in pixels */
static uint8_t KBRK[SD_THINGS];                                    /* masonry broken */
static float KRING[SD_THINGS];                                     /* a bell: its swing after a ring, 1 to 0 */
static int8_t KDIR[SD_THINGS];                                     /* a belt's way now */
static float KRUN[SD_THINGS];                                      /* a belt's run so far, pixels, for the drawing */
static float KTX[SD_THINGS], KTY[SD_THINGS], KTVY[SD_THINGS];      /* a sarcophagus: its left, its foot, its fall */
static uint8_t KWEB[SD_H][SD_W];                                   /* a cobweb's tile: 0 whole, 1 burning, 2 gone */
static float KWEBT[SD_H][SD_W];
static float KPREVY, KGRAB_T, KBURN_SND;                           /* the feet before the step (the springs); no grab just after a jump */

#define KIT_PRESS_DOWN 520.0f                                      /* a press coming down, pixels a second; going back up */
#define KIT_PRESS_UP 44.0f
#define KIT_PRESS_WARN 0.7f
#define KIT_PRESS_HOLD 0.6f
#define KIT_TOMB_W 32.0f
#define KIT_TOMB_H 16.0f

static void kit_says(const char *what, int i) {
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_kit %s %d t=%.2f\n", what, i, SD.t);
#else
    (void)what; (void)i;
#endif
}
static int kit_box_caster(float x0, float y0, float x1, float y1) {   /* the caster's box overlaps this one */
    return !SD.dying && SD.x + SD_HW > x0 && SD.x - SD_HW < x1 && SD.y > y0 && SD.y - sd_box_h() < y1;
}
static int sd_body_at(float x0, float y0, float x1, float y1, int skip) {
    for (int k = 0; k < KB.n; k++) {
        if (k == skip) continue;
        const typeof(KB.b[0]) *b = &KB.b[k];
        if (x1 >= b->x && x0 < b->x + b->w && y1 >= b->y && y0 < b->y + b->h) return k;
    }
    return -1;
}
static int kit_body_of(int thing) { for (int k = 0; k < KB.n; k++) if (KB.b[k].thing == thing) return k; return -1; }
static void kit_place_bodies(int keep_prev) {                      /* each body's box from its thing's state */
    for (int k = 0; k < KB.n; k++) {
        typeof(KB.b[0]) *b = &KB.b[k];
        const SdThing *t = &SD.L->th[b->thing];
        if (!keep_prev) { b->px = b->x; b->py = b->y; }
        if (b->tomb) { b->x = KTX[b->thing]; b->y = KTY[b->thing] - KIT_TOMB_H; b->w = KIT_TOMB_W; b->h = KIT_TOMB_H; }
        else { b->x = (float)(t->x * SD_TILE); b->y = (float)(t->y * SD_TILE) + KY[b->thing]; b->w = (float)(t->w * SD_TILE); b->h = (float)SD_TILE; }
        if (keep_prev) { b->px = b->x; b->py = b->y; }
    }
}

/* ---- the stones that rise and sink, the masonry, the belts: KSOL ---- */
static int kit_room(const SdThing *t) {                            /* nothing stands in its box: it may rise */
    float x0 = (float)(t->x * SD_TILE), y0 = (float)(t->y * SD_TILE), x1 = x0 + (float)(t->w * SD_TILE), y1 = y0 + (float)(t->h * SD_TILE);
    return !kit_box_caster(x0, y0, x1, y1) && sd_body_at(x0, y0, x1 - 0.01f, y1 - 0.01f, -1) < 0;
}
static int kit_ghost_shown(int i) {                                /* a ghost stone wants to show now */
    const SdThing *t = &SD.L->th[i];
    if (t->f[0] <= 0.0f) return SD.st[i];
    float per = t->f[0] + t->f[1];
    return fmodf(KT[i], per) < t->f[0];
}
static void kit_solids(void) {
    const SdLevel *L = SD.L;
    for (int y = 0; y < L->h; y++) memset(KSOL[y], 0, (size_t)L->w);
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        int on;
        switch (t->kind) {
        case SDT_STONE: case SDT_GHOST: on = KUP[i]; break;
        case SDT_CRACK: on = !KBRK[i]; break;
        case SDT_BELT: on = 1; break;
        default: continue;
        }
        if (!on) continue;
        int h = t->kind == SDT_BELT ? 1 : t->h;
        for (int y = t->y; y < t->y + h && y < L->h; y++) for (int x = t->x; x < t->x + t->w && x < L->w; x++) KSOL[y][x] = 1;
    }
}
static void kit_set(int b_up) {                                    /* the bell stones: A's up, or B's */
    if (KSET == (uint8_t)b_up) return;
    KSET = (uint8_t)b_up; KSET_T = 0.0f;
    snd_sfx_ex(SFX_PUSHWALL, 0.6f, 0.5f);
    kit_says(b_up ? "stones B up" : "stones A up", -1);
}
static void kit_ring(int i) {
    if (KRING[i] > 0.8f) return;                                   /* (still swinging from the last) */
    KRING[i] = 1.0f;
    sd_sfx_th(SFX_DEEPCHIME, 0.9f, i);
    kit_set(!KSET);
    kit_says("bell rung", i);
}
static void kit_break(int i) {
    if (KBRK[i]) return;
    KBRK[i] = 1; KT[i] = 0.0f;
    sd_sfx_th(SFX_PUSHWALL, 0.8f, i); sd_sfx_th(SFX_DIRT, 0.7f, i);
    kit_says("masonry broken", i);
}
static void kit_drop(int i) { if (KPH[i] == 0) { KPH[i] = 1; KT[i] = 0.0f; sd_sfx_th(SFX_RUMBLE, 0.6f, i); kit_says("press warns", i); } }

/* ---- laid as the passage opens, and again after a life lost ---- */
static void sd_kit_build(void) {
    const SdLevel *L = SD.L;
    memset(KUP, 0, sizeof KUP); memset(KT, 0, sizeof KT); memset(KPH, 0, sizeof KPH); memset(KY, 0, sizeof KY); memset(KBRK, 0, sizeof KBRK);
    memset(KRING, 0, sizeof KRING); memset(KDIR, 0, sizeof KDIR); memset(KRUN, 0, sizeof KRUN); memset(KWEB, 0, sizeof KWEB); memset(KWEBT, 0, sizeof KWEBT);
    KSET = 0; KSET_T = 9.0f; KB.n = 0; KGRAB_T = 0.0f; KBURN_SND = 0.0f;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        switch (t->kind) {
        case SDT_STONE: KUP[i] = t->a == 0; break;
        case SDT_GHOST: KT[i] = t->f[2]; KUP[i] = (uint8_t)kit_ghost_shown(i); break;
        case SDT_BELL: KT[i] = t->f[0]; break;
        case SDT_BELT: KDIR[i] = (int8_t)t->a; break;
        case SDT_PRESS:                                            /* its first fall: where AT says it is in its clock */
            KT[i] = t->f[0] > 0.0f ? t->f[0] - fmodf(fmaxf(t->f[1], 0.0f), t->f[0]) : 0.0f;
            if (KB.n < SD_BODIES) KB.b[KB.n++].thing = i, KB.b[KB.n - 1].tomb = 0;
            break;
        case SDT_TOMB:
            KTX[i] = (float)(t->x * SD_TILE); KTY[i] = (float)((t->y + 1) * SD_TILE); KTVY[i] = 0.0f;
            if (KB.n < SD_BODIES) KB.b[KB.n++].thing = i, KB.b[KB.n - 1].tomb = 1;
            break;
        default: break;
        }
    }
    kit_place_bodies(1);
    kit_solids();
}

/* ---- what triggers do to them ---- */
static int sd_kit_act(int target, int verb, int back) {
    const SdThing *t = &SD.L->th[target];
    int on = verb == TRV_ON ? !back : verb == TRV_OFF ? back : -1;  /* -1: FLIP */
    switch (t->kind) {
    case SDT_STONE: kit_set(on < 0 ? !KSET : !on); return 1;     /* ON: A's up; OFF: B's; FLIP: the other */
    case SDT_BELL: if (!back || on < 0) kit_ring(target); return 1;
    case SDT_CRACK: if (on != 0) kit_break(target); return 1;
    case SDT_PRESS: if (t->f[0] > 0.0f) break; if (on != 0) kit_drop(target); return 1;   /* (a press on a clock: started and stopped) */
    case SDT_BELT: if (on < 0) { KDIR[target] = (int8_t)-KDIR[target]; sd_sfx_th(SFX_CREAK, 0.7f, target); kit_says("belt turned", target); return 1; } break;
    case SDT_CHAIN: case SDT_SPRING: case SDT_GHOST: case SDT_TOMB: break;
    default: return 0;
    }
    int was = SD.st[target], now = on < 0 ? !was : on;              /* the rest: on and off, as everything else */
    SD.st[target] = (uint8_t)now;
    if (was != now) {
        if (t->kind == SDT_CHAIN) sd_sfx_th(SFX_RATTLE, 0.8f, target);
        else if (t->kind == SDT_GHOST) sd_sfx_th(SFX_SHIMMER, 0.6f, target);
        else if (t->kind == SDT_BELT || t->kind == SDT_PRESS) sd_sfx_th(SFX_CREAK, 0.6f, target);
        else sd_sfx_th(SFX_KNOCK, 0.5f, target);
        if (t->kind == SDT_CHAIN && !now && SD.climb == target) { SD.climb = -1; SD.ground = 0; }
        kit_says(now ? "on" : "off", target);
    }
    return 1;
}
static int sd_kit_pressed(int i, int *is_kit) {                    /* masonry broken; a bell the moment it rings; a sarcophagus on a plate */
    const SdThing *t = &SD.L->th[i];
    *is_kit = 0;
    if (t->kind == SDT_CRACK) { *is_kit = 1; return KBRK[i]; }
    if (t->kind == SDT_BELL) { *is_kit = 1; return KRING[i] > 0.85f; }
    if (t->kind != SDT_PLATE) return 0;
    float x0 = (float)(t->x * SD_TILE), x1 = x0 + SD_TILE, top = (float)((t->y + 1) * SD_TILE);
    for (int k = 0; k < KB.n; k++) {
        const typeof(KB.b[0]) *b = &KB.b[k];
        if (b->tomb && b->x + b->w > x0 + 2.0f && b->x < x1 - 2.0f && fabsf(b->y + b->h - top) < 3.0f) return 1;
    }
    return 0;
}

/* ---- the caster against the bodies ---- */
static int kit_tomb_free(int i, float nx, float ny, int skip) {    /* the sarcophagus could stand at (nx, ny), its left and foot */
    float x0 = nx + 0.01f, x1 = nx + KIT_TOMB_W - 0.01f, y0 = ny - KIT_TOMB_H + 0.01f, y1 = ny - 0.01f;
    for (int ty = sd_floor_px(y0); ty <= sd_floor_px(y1); ty++)
        for (int tx = sd_floor_px(x0); tx <= sd_floor_px(x1); tx++) if (sd_solid(tx, ty)) return 0;
    (void)i;
    return sd_body_at(x0, y0, x1, y1, skip) < 0;
}
static float kit_tomb_move(int k, float dx) {                      /* along, as far as it can go: how far it went */
    typeof(KB.b[0]) *b = &KB.b[k];
    int i = b->thing;
    float step = dx > 0.0f ? 1.0f : -1.0f, went = 0.0f;
    while (fabsf(went) < fabsf(dx)) {
        float d = fabsf(dx - went) < 1.0f ? dx - went : step;
        if (!kit_tomb_free(i, KTX[i] + d, KTY[i], k)) break;
        KTX[i] += d; went += d;
    }
    b->x = KTX[i];
    return went;
}
static void sd_kit_block_x(float dx) {
    float h = sd_box_h();
    for (int k = 0; k < KB.n; k++) {
        typeof(KB.b[0]) *b = &KB.b[k];
        if (!(SD.x + SD_HW > b->x && SD.x - SD_HW < b->x + b->w && SD.y > b->y + 0.5f && SD.y - h < b->y + b->h)) continue;
        if (b->tomb && SD.ground && SD.on_body != k) {             /* a sarcophagus: walked into, it goes, slowly */
            float lim = SD_WALK * 0.45f;
            SD.vx = clampf(SD.vx, -lim, lim);
            float over = dx > 0.0f ? SD.x + SD_HW - b->x : SD.x - SD_HW - (b->x + b->w);
            if (kit_tomb_move(k, over) != 0.0f && (int)(SD.t * 5.0f) != (int)((SD.t - SD_STEP) * 5.0f)) sd_sfx_at(SFX_PUSHWALL, 0.35f, b->x + 16.0f, b->y + 8.0f);
        }
        if (dx > 0.0f) SD.x = b->x - SD_HW; else SD.x = b->x + b->w + SD_HW;
        SD.vx = b->tomb ? SD.vx : 0.0f;
    }
}
static void sd_kit_block_y(float prev, float dy) {
    float h = sd_box_h();
    for (int k = 0; k < KB.n; k++) {
        const typeof(KB.b[0]) *b = &KB.b[k];
        if (!(SD.x + SD_HW > b->x && SD.x - SD_HW < b->x + b->w)) continue;
        if (dy > 0.0f && prev <= b->py + 0.5f && SD.y >= b->y) { SD.y = b->y; SD.vy = 0.0f; SD.ground = 1; SD.on_plat = -1; SD.on_body = k; return; }
        if (dy < 0.0f && SD.y - h < b->y + b->h && prev - h >= b->y + b->h - 0.5f) { SD.y = b->y + b->h + h; SD.vy = 0.0f; }
    }
}
static int sd_kit_on_body(void) {
    if (SD.on_body < 0 || SD.on_body >= KB.n) return 0;
    const typeof(KB.b[0]) *b = &KB.b[SD.on_body];
    return SD.x + SD_HW > b->x && SD.x - SD_HW < b->x + b->w && fabsf(SD.y - b->y) < 1.5f;
}
static void sd_kit_ride(void) {
    if (SD.on_body < 0 || SD.on_body >= KB.n) return;
    const typeof(KB.b[0]) *b = &KB.b[SD.on_body];
    int keep = SD.on_body;
    if (b->x != b->px) sd_move_x(b->x - b->px);
    SD.on_body = keep;
    SD.y += b->y - b->py;
}

/* ---- the chain: up to it and up held, it is taken; up and down climb it; jump lets go ---- */
static int sd_kit_climb(float dt) {
    SdIn *in = &SD.in;
    const SdLevel *L = SD.L;
    if (KGRAB_T > 0.0f) KGRAB_T -= dt;
    if (SD.climb < 0) {
        if (!in->up || SD.crouch || KGRAB_T > 0.0f) return 0;
        for (int i = 0; i < L->nth; i++) {
            const SdThing *t = &L->th[i];
            if (t->kind != SDT_CHAIN || !SD.st[i]) continue;
            float cx = (float)(t->x * SD_TILE) + 8.0f, top = (float)(t->y * SD_TILE), bot = (float)((t->y + t->h) * SD_TILE);
            if (fabsf(SD.x - cx) < 7.0f && SD.y - SD_HT - 10.0f < bot && SD.y > top + 4.0f) {   /* (a hand up reaches its end) */
                SD.climb = i; SD.vx = SD.vy = 0.0f; SD.ground = 0; SD.on_plat = -1; SD.on_body = -1;
                snd_sfx_ex(SFX_RATTLE, 0.35f, 0.5f);
                kit_says("chain taken", i);
                break;
            }
        }
        if (SD.climb < 0) return 0;
    }
    const SdThing *t = &L->th[SD.climb];
    if (!SD.st[SD.climb]) { SD.climb = -1; return 0; }
    int dir = (in->right ? 1 : 0) - (in->left ? 1 : 0);
    if (dir) SD.face = dir;
    if (in->jump_hit) {                                            /* off it, with a jump the way pressed */
        SD.climb = -1; KGRAB_T = 0.3f;
        SD.vy = -SD_JUMP * 0.85f; SD.vx = (float)dir * SD_WALK; SD.ground = 0;
        sd_jump_sfx();
        return 1;
    }
    float cx = (float)(t->x * SD_TILE) + 8.0f, top = (float)(t->y * SD_TILE) + SD_HT + 2.0f, bot = (float)((t->y + t->h) * SD_TILE) + 6.0f;
    sd_move_x((cx - SD.x) * fminf(1.0f, dt * 20.0f));
    int v = (in->down ? 1 : 0) - (in->up ? 1 : 0);
    float ny = clampf(SD.y + (float)v * SDT[TU_CLIMB] * dt, top, bot + 1.0f);
    SD.vx = SD.vy = 0.0f; SD.ground = 0;
    if (v > 0) {                                                   /* down onto a floor, or off its end */
        int r = sd_floor_px(ny), c0 = sd_floor_px(SD.x - SD_HW + 0.01f), c1 = sd_floor_px(SD.x + SD_HW - 0.01f);
        for (int c = c0; c <= c1; c++) if (sd_solid(c, r)) { SD.y = (float)(r * SD_TILE); SD.ground = 1; SD.climb = -1; return 1; }
        if (ny > bot) { SD.y = ny; SD.climb = -1; KGRAB_T = 0.3f; return 1; }
    }
    if (v < 0) {                                                   /* the head against stone */
        int r = sd_floor_px(ny - SD_HT), c0 = sd_floor_px(SD.x - SD_HW + 0.01f), c1 = sd_floor_px(SD.x + SD_HW - 0.01f);
        for (int c = c0; c <= c1; c++) if (sd_solid(c, r)) ny = SD.y;
    }
    SD.y = ny;
    SD.anim += fabsf((float)v) * dt * 6.0f;
    return 1;
}

/* ---- what the feet are on, and what they are in ---- */
static float sd_kit_belt_under(float x, float y) {                 /* a running belt's carry under feet at (x, y), pixels a second */
    int r = sd_floor_px(y + 0.5f);
    if (fabsf(y - (float)(r * SD_TILE)) > 1.0f) return 0.0f;
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_BELT || !SD.st[i] || t->y != r) continue;
        if (x >= (float)(t->x * SD_TILE) && x < (float)((t->x + t->w) * SD_TILE)) return (float)KDIR[i] * t->f[0] * SD_TILE;
    }
    return 0.0f;
}
static int sd_kit_slate_under(void) {
    int r = sd_floor_px(SD.y + 0.5f), c0 = sd_floor_px(SD.x - SD_HW + 0.01f), c1 = sd_floor_px(SD.x + SD_HW - 0.01f);
    for (int c = c0; c <= c1; c++) if (sd_tile(c, r) == 'S') return 1;
    return 0;
}
static int sd_kit_web_at(float x, float y) {
    int tx = sd_floor_px(x), ty = sd_floor_px(y);
    if (tx < 0 || ty < 0 || tx >= SD.L->w || ty >= SD.L->h) return 0;
    return SD.L->t[ty][tx] == 'W' && KWEB[ty][tx] < 2;
}
static int sd_kit_web(float x, float y, float h) {
    for (float yy = y - h + 1.0f; yy <= y - 1.0f; yy += 4.0f) {
        if (sd_kit_web_at(x - SD_HW + 1.0f, yy) || sd_kit_web_at(x + SD_HW - 1.0f, yy)) return 1;
        if (yy + 4.0f > y - 1.0f && (sd_kit_web_at(x - SD_HW + 1.0f, y - 1.0f) || sd_kit_web_at(x + SD_HW - 1.0f, y - 1.0f))) return 1;
    }
    return 0;
}
static void sd_kit_burn(float x, float y) {                        /* fire on a cobweb: it catches, and the fire runs through it */
    int tx = sd_floor_px(x), ty = sd_floor_px(y);
    if (!sd_kit_web_at(x, y) || KWEB[ty][tx]) return;
    KWEB[ty][tx] = 1; KWEBT[ty][tx] = 0.0f;
    if (KBURN_SND <= 0.0f) { sd_sfx_at(SFX_IGNITE, 0.6f, x, y); KBURN_SND = 0.5f; }
    kit_says("web catches", tx);
}

/* ---- spells and flasks on the masonry and the bells ---- */
static int kit_at(int kind, int tx, int ty) {                      /* the thing of this kind whose box holds the tile, -1 none */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != kind) continue;
        int w = kind == SDT_BELL ? 1 : t->w, h = kind == SDT_BELL ? 1 : t->h;
        if (tx >= t->x && tx < t->x + w && ty >= t->y && ty < t->y + h) return i;
    }
    return -1;
}
static void sd_kit_struck(int tx, int ty) { int i = kit_at(SDT_CRACK, tx, ty); if (i >= 0 && !KBRK[i]) kit_break(i); }
static int sd_kit_bell_hit(float x, float y) {
    int i = kit_at(SDT_BELL, sd_floor_px(x), sd_floor_px(y));
    if (i < 0) return 0;
    float bx = (float)(SD.L->th[i].x * SD_TILE) + 8.0f, by = (float)(SD.L->th[i].y * SD_TILE) + 9.0f;
    if (fabsf(x - bx) > 5.0f || fabsf(y - by) > 6.0f) return 0;   /* the bell's own shape, near enough: not its bracket */
    kit_ring(i);
    return 1;
}
static void sd_kit_burst(float x, float y) {                       /* a flask's splash: masonry within it broken, a bell in it rung */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_CRACK && t->kind != SDT_BELL) continue;
        int w = t->kind == SDT_BELL ? 1 : t->w, h = t->kind == SDT_BELL ? 1 : t->h;
        float nx = clampf(x, (float)(t->x * SD_TILE), (float)((t->x + w) * SD_TILE)), ny = clampf(y, (float)(t->y * SD_TILE), (float)((t->y + h) * SD_TILE));
        if (hypotf(x - nx, y - ny) > 26.0f) continue;
        if (t->kind == SDT_CRACK) kit_break(i); else kit_ring(i);
    }
}
static void sd_kit_beam(float x0, float y, float x1) {             /* lightning: the bells on its way, the masonry where it stops */
    float s = x1 > x0 ? 1.0f : -1.0f;
    for (float x = x0; (x1 - x) * s > 0.0f; x += s * 2.0f) if (sd_kit_bell_hit(x, y)) break;
    sd_kit_struck(sd_floor_px(x1 + s * 2.0f), sd_floor_px(y));
}
static int sd_kit_use(void) {                                      /* up beside a bell: rung by hand */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_BELL) continue;
        float bx = (float)(t->x * SD_TILE) + 8.0f, by = (float)((t->y + 1) * SD_TILE);
        if (fabsf(SD.x - bx) < 14.0f && SD.y > by - 4.0f && SD.y - sd_box_h() < by + 8.0f) { kit_ring(i); return 1; }
    }
    return 0;
}

/* ---- each step: the bells' clocks, the stones, the ghosts, the presses, the sarcophagi, the belts, the webs burning ---- */
static void kit_crush(int i, const typeof(KB.b[0]) *b) {           /* a press down on the caster: thrown clear if there is room, else killed */
    float h = sd_box_h(), nl = b->x - SD_HW - 0.5f, nr = b->x + b->w + SD_HW + 0.5f;
    float pick[2] = { SD.x < b->x + b->w * 0.5f ? nl : nr, SD.x < b->x + b->w * 0.5f ? nr : nl };
    int moved = 0;
    for (int k = 0; k < 2 && !moved; k++) {
        float x = pick[k];
        int clear = sd_body_at(x - SD_HW, SD.y - h, x + SD_HW, SD.y - 0.5f, -1) < 0;
        for (int r = sd_floor_px(SD.y - h); r <= sd_floor_px(SD.y - 0.5f) && clear; r++)
            for (int c = sd_floor_px(x - SD_HW + 0.01f); c <= sd_floor_px(x + SD_HW - 0.01f); c++) if (sd_solid(c, r)) clear = 0;
        if (clear) { SD.x = x; moved = 1; }
    }
    G.blame = (Blame){ BL_HAZARD, 0, 0 };
    kit_says(moved ? "press strikes" : "press crushes", i);
    if (!moved) {                                                  /* no room either side: all of it */
        SD.inv_t = 0.0f; sd_hurt(99999.0f, EL_PHYS, b->x + b->w * 0.5f);
        return;
    }
    sd_hurt(SDT[TU_PRESS] * (float)G.max_hp, EL_PHYS, b->x + b->w * 0.5f);
}
static void sd_kit(float dt) {
    const SdLevel *L = SD.L;
    KPREVY = SD.y;
    KSET_T += dt; KBURN_SND = fmaxf(0.0f, KBURN_SND - dt);
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        switch (t->kind) {
        case SDT_BELL:
            KRING[i] = fmaxf(0.0f, KRING[i] - dt / 1.6f);
            if (t->f[0] > 0.0f && SD.st[i] && (KT[i] -= dt) <= 0.0f) { KT[i] += t->f[0]; kit_ring(i); }
            break;
        case SDT_GHOST:
            if (t->f[0] > 0.0f && SD.st[i]) KT[i] += dt;
            /* fall through */
        case SDT_STONE: {
            int want = t->kind == SDT_STONE ? t->a == KSET : kit_ghost_shown(i);
            if (want && !KUP[i]) { if (kit_room(t)) KUP[i] = 1; }  /* rising: not through the caster or a body */
            else if (!want) KUP[i] = 0;
            break;
        }
        case SDT_CRACK: if (KBRK[i]) KT[i] += dt; break;
        case SDT_SPRING: KT[i] = fmaxf(0.0f, KT[i] - dt); break;
        case SDT_BELT: if (SD.st[i]) KRUN[i] += (float)KDIR[i] * t->f[0] * SD_TILE * dt; break;
        case SDT_PRESS: {
            float travel = (float)((t->h - 1) * SD_TILE);
            switch (KPH[i]) {
            case 0: if (t->f[0] > 0.0f && SD.st[i] && (KT[i] -= dt) <= 0.0f) { KT[i] += t->f[0]; kit_drop(i); } break;
            case 1: if ((KT[i] += dt) >= KIT_PRESS_WARN) { KPH[i] = 2; KT[i] = 0.0f; } break;
            case 2: {                                              /* down: to its foot, or onto what is under it */
                float ny = KY[i] + KIT_PRESS_DOWN * dt, stop = travel;
                float x0 = (float)(t->x * SD_TILE), x1 = x0 + (float)(t->w * SD_TILE);
                for (int k = 0; k < KB.n; k++) {
                    const typeof(KB.b[0]) *o = &KB.b[k];
                    if (o->thing == i || !(o->x + o->w > x0 && o->x < x1)) continue;
                    float rest = o->y - (float)((t->y + 1) * SD_TILE);   /* its head's foot on the other's top */
                    if (rest >= KY[i] - 0.5f && rest < stop) stop = rest;
                }
                if (ny >= stop) { ny = stop; KPH[i] = 3; KT[i] = 0.0f; sd_sfx_th(SFX_BOOM, 0.9f, i); rumble(0.4f, 0.4f, 0.25f); kit_says("press lands", i); }
                KY[i] = ny;
                break;
            }
            case 3: if ((KT[i] += dt) >= KIT_PRESS_HOLD) { KPH[i] = 4; KT[i] = 0.0f; } break;
            case 4: {                                              /* back up, slowly: not while a rider would meet the stone over it */
                int k = kit_body_of(i);
                if (k >= 0 && SD.on_body == k && SD.ground) {
                    float head = SD.y - sd_box_h() - KIT_PRESS_UP * dt;
                    int blocked = 0;
                    for (int c = sd_floor_px(SD.x - SD_HW + 0.01f); c <= sd_floor_px(SD.x + SD_HW - 0.01f); c++) if (sd_solid(c, sd_floor_px(head))) blocked = 1;
                    if (blocked) break;
                }
                KY[i] = fmaxf(0.0f, KY[i] - KIT_PRESS_UP * dt);
                if (KY[i] <= 0.0f) KPH[i] = 0;
                break;
            }
            }
            break;
        }
        default: break;
        }
    }
    /* the sarcophagi: they fall, rest on what is solid, and go where water and belts take them */
    for (int k = 0; k < KB.n; k++) {
        if (!KB.b[k].tomb) continue;
        int i = KB.b[k].thing;
        float cx = KTX[i] + KIT_TOMB_W * 0.5f;
        int cur = sd_current_at(cx, KTY[i] - 4.0f);
        if (cur >= 0) {                                            /* carried, slowly: it is heavy */
            const SdThing *w = &L->th[cur];
            float n = hypotf((float)w->a, (float)w->b), sp = w->f[0] * SD_TILE * 0.5f;
            kit_tomb_move(k, (float)w->a / n * sp * dt);
            if (w->b < 0) KTVY[i] = fminf(KTVY[i], (float)w->b / n * sp);
        }
        float carry = sd_kit_belt_under(cx, KTY[i]);
        if (carry != 0.0f) kit_tomb_move(k, carry * dt);
        KTVY[i] = fminf(SD_FALL_MAX, KTVY[i] + SD_G * dt);
        float ny = KTY[i] + KTVY[i] * dt;
        if (KTVY[i] > 0.0f) {
            int landed = !kit_tomb_free(i, KTX[i], ny, k);
            for (int c = sd_floor_px(KTX[i] + 0.01f); c <= sd_floor_px(KTX[i] + KIT_TOMB_W - 0.01f) && !landed; c++)   /* a ledge, from above */
                if (sd_tile(c, sd_floor_px(ny - 0.01f)) == '=' && KTY[i] <= (float)(sd_floor_px(ny - 0.01f) * SD_TILE) + 0.5f) landed = 1;
            if (!landed && kit_box_caster(KTX[i], ny - KIT_TOMB_H, KTX[i] + KIT_TOMB_W, ny)) landed = 1;   /* on a head: it stops there */
            if (landed) {                                          /* down to what it lands on, a pixel at a time */
                if (KTVY[i] > 120.0f) sd_sfx_at(SFX_BOOM, 0.5f, cx, KTY[i]);
                float y = floorf(KTY[i] + 0.5f);
                while (y + 1.0f <= ny + 1.0f && kit_tomb_free(i, KTX[i], y + 1.0f, k) && !kit_box_caster(KTX[i], y + 1.0f - KIT_TOMB_H, KTX[i] + KIT_TOMB_W, y + 1.0f)) {
                    int lr = sd_floor_px(y + 1.0f - 0.01f), ledge = 0;
                    for (int c = sd_floor_px(KTX[i] + 0.01f); c <= sd_floor_px(KTX[i] + KIT_TOMB_W - 0.01f); c++) if (sd_tile(c, lr) == '=' && y <= (float)(lr * SD_TILE) + 0.5f) ledge = 1;
                    if (ledge) break;
                    y += 1.0f;
                }
                KTY[i] = y; KTVY[i] = 0.0f;
            }
            else KTY[i] = ny;
        } else if (KTVY[i] < 0.0f) { if (kit_tomb_free(i, KTX[i], ny, k)) KTY[i] = ny; else KTVY[i] = 0.0f; }
        if (KTY[i] > (float)(L->h * SD_TILE + 64)) { KTY[i] = (float)(L->h * SD_TILE + 64); KTVY[i] = 0.0f; }
    }
    kit_place_bodies(0);
    /* a press coming down: on the caster, and on the dead under it */
    for (int k = 0; k < KB.n; k++) {
        const typeof(KB.b[0]) *b = &KB.b[k];
        if (b->tomb || KPH[b->thing] != 2) continue;
        if (SD.on_body != k && kit_box_caster(b->x, b->y, b->x + b->w, b->y + b->h)) kit_crush(b->thing, b);
        for (int m = 0; m < SD_MOBS; m++) {
            SdMob *d = &SD.mob[m];
            if (!d->alive || d->state == SDM_DIE || d->fly || d->x < b->x || d->x > b->x + b->w || d->y - 12.0f > b->y + b->h || d->y < b->y) continue;
            d->state = SDM_DIE; d->t = 0.0f;
            if (d->thing >= 0) SDP.slain[SD.side][d->thing] = 1;
            sd_sfx_at(KINDS[d->kind].sfx_die, 0.8f, d->x, d->y - 8.0f);
            kit_says("press takes one of the dead", m);
        }
    }
    /* the belts carry the dead that walk, and what lies loose */
    for (int m = 0; m < SD_MOBS; m++) {
        SdMob *d = &SD.mob[m];
        if (!d->alive || d->state == SDM_DIE || d->fly) continue;
        float carry = sd_kit_belt_under(d->x, d->y);
        if (carry != 0.0f && !sd_solid(sd_floor_px(d->x + carry * dt + (carry > 0.0f ? 6.0f : -6.0f)), sd_floor_px(d->y - 4.0f))) d->x += carry * dt;
    }
    for (int i = 0; i < L->nth; i++) {
        if (!sd_loose(&L->th[i]) || SD.taken[i] || SD.ivy[i] != 0.0f) continue;
        float carry = sd_kit_belt_under(SD.ix[i], SD.iy[i] + 5.0f);
        if (carry != 0.0f && !sd_solid(sd_floor_px(SD.ix[i] + carry * dt + (carry > 0.0f ? 5.0f : -5.0f)), sd_floor_px(SD.iy[i]))) SD.ix[i] += carry * dt;
    }
    /* the webs burning: each burning tile catches its neighbours and is gone */
    for (int y = 0; y < L->h; y++)
        for (int x = 0; x < L->w; x++) {
            if (KWEB[y][x] != 1) continue;
            if ((KWEBT[y][x] += dt) < 0.14f) continue;
            KWEB[y][x] = 2;
            static const int D4[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
            for (int d = 0; d < 4; d++) {
                int nx = x + D4[d][0], ny = y + D4[d][1];
                if (nx >= 0 && ny >= 0 && nx < L->w && ny < L->h && L->t[ny][nx] == 'W' && !KWEB[ny][nx]) { KWEB[ny][nx] = 1; KWEBT[ny][nx] = -0.02f * (float)d; }
            }
        }
    kit_solids();
}
static void sd_kit_after(float dt) {                               /* the springs: landed or stepped on, they throw */
    (void)dt;
    if (SD.dying || SD.climb >= 0) return;
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_SPRING || !SD.st[i] || KT[i] > 0.12f) continue;
        float x0 = (float)(t->x * SD_TILE) + 3.0f, x1 = x0 + 10.0f, foot = (float)((t->y + 1) * SD_TILE), top = foot - 6.0f;
        if (!(SD.x + SD_HW > x0 && SD.x - SD_HW < x1) || SD.y < top || SD.y > foot + 0.5f || SD.vy < 0.0f) continue;
        if (KPREVY > top + 0.5f && !SD.ground) continue;
        SD.y = top; SD.vy = -(SD.in.jump ? SDT[TU_SPRING] : SDT[TU_SPRING] * 0.7f);
        SD.ground = 0; SD.on_plat = -1; SD.on_body = -1; SD.coyote = 0.0f;
        KT[i] = 0.3f;
        sd_sfx_th(SFX_THRUM, 0.8f, i);
        kit_says("spring throws", i);
    }
}
