/* A part of scene_crawl.c (docs/PLAN-side.md): it is #included there in its place and never compiled on its own.
 * The works of a side passage (2d, the owner's puzzles, 26 November 2026): gates, plates and zones, the triggers that join them to
 * what they move; water running in a current, and what is loose in it; flows on their own clocks; the crypt's traps in side view,
 * each kind in the shape of its own: things falling, a line that bursts, a spread that creeps and stays, a cloud. */

#ifdef CRAWL_DEBUG
static const char *sd_hz_name(int k) { for (unsigned i = 0; i < sizeof DXT_NAMES / sizeof DXT_NAMES[0]; i++) if (DXT_NAMES[i].v == k && !strncmp(DXT_NAMES[i].n, "HZ_", 3)) return DXT_NAMES[i].n; return "HZ_?"; }
#endif

/* ---- gates: bars across their box, rising from the foot as they open ---- */
static int sd_gate_at(int tx, int ty) {
    const SdLevel *L = SD.L;
    if (!L) return 0;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_GATE || tx < t->x || tx >= t->x + t->w || ty < t->y || ty >= t->y + t->h) continue;
        int shut_rows = (int)ceilf((float)t->h * (1.0f - SD.gk[i]) - 0.05f);   /* the bars cover this many rows from the top */
        if (ty < t->y + shut_rows) return 1;
    }
    return 0;
}

/* ---- loose things: they fall, rest on what is solid, and go where water takes them ---- */
static int sd_loose(const SdThing *t) { return (t->kind == SDT_GOLD || t->kind == SDT_KEY || t->kind == SDT_LIFE) && (t->flags & SDF_LOOSE); }
static int sd_current_at(float x, float y) {                       /* the running current over a point, -1 none */
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_CURRENT || !SD.st[i]) continue;
        if (x >= (float)(t->x * SD_TILE) && x < (float)((t->x + t->w) * SD_TILE) && y >= (float)(t->y * SD_TILE) && y < (float)((t->y + t->h) * SD_TILE)) return i;
    }
    return -1;
}
static void sd_items(float dt) {
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (!sd_loose(t) || SD.taken[i]) continue;
        float *x = &SD.ix[i], *y = &SD.iy[i], *vx = &SD.ivx[i], *vy = &SD.ivy[i];   /* the thing's middle, in pixels */
        int c = sd_current_at(*x, *y);
        if (c >= 0) {                                              /* carried: to the water's way and pace */
            const SdThing *w = &L->th[c];
            float n = hypotf((float)w->a, (float)w->b), sp = w->f[0] * SD_TILE;
            *vx += ((float)w->a / n * sp - *vx) * fminf(1.0f, dt * 4.0f);
            *vy += ((float)w->b / n * sp - *vy) * fminf(1.0f, dt * 4.0f);
        } else {
            *vy = fminf(SD_FALL_MAX, *vy + SD_G * dt);
            *vx *= expf(-dt * 3.0f);
        }
        float nx = *x + *vx * dt;
        if (sd_solid(sd_floor_px(nx + (*vx > 0.0f ? 5.0f : -5.0f)), sd_floor_px(*y))) *vx = 0.0f; else *x = nx;
        float ny = *y + *vy * dt;
        if (*vy > 0.0f) {                                          /* landing: stone, a ledge, a shut gate, a platform */
            int ty = sd_floor_px(ny + 5.0f), tx = sd_floor_px(*x);
            if (sd_solid(tx, ty) || (sd_tile(tx, ty) == '=' && *y + 5.0f <= (float)(ty * SD_TILE) + 0.5f)) { *y = (float)(ty * SD_TILE) - 5.0f; *vy = 0.0f; }
            else {
                int on = 0;
                for (int p = 0; p < L->nth && !on; p++) {
                    const SdThing *q = &L->th[p];
                    if (q->kind != SDT_PLAT) continue;
                    const SdPlat *pl = &SD.plat[p];
                    if (*x >= pl->x && *x < pl->x + (float)(q->w * SD_TILE) && *y + 5.0f <= pl->py + 0.5f && ny + 5.0f >= pl->y) { *y = pl->y - 5.0f; *vy = 0.0f; on = 1; *x += pl->x - pl->px; }
                }
                if (!on) *y = ny;
            }
        } else if (*vy < 0.0f) { if (sd_solid(sd_floor_px(*x), sd_floor_px(ny - 5.0f))) *vy = 0.0f; else *y = ny; }
        if (*y > (float)(L->h * SD_TILE + 64)) { *y = (float)(L->h * SD_TILE + 64); *vy = 0.0f; }   /* gone down the drop */
    }
}

/* ---- what presses: a plate stood on (the caster, or a loose thing on it), a zone the caster is in, a switch thrown ---- */
static int sd_pressed(int i) {
    const SdThing *t = &SD.L->th[i];
    if (t->kind == SDT_SWITCH) return SD.thrown[i];
    if (t->kind == SDT_ZONE) {
        float x0 = (float)(t->x * SD_TILE), x1 = (float)((t->x + t->w) * SD_TILE), y0 = (float)(t->y * SD_TILE), y1 = (float)((t->y + t->h) * SD_TILE);
        return !SD.dying && SD.x + SD_HW > x0 && SD.x - SD_HW < x1 && SD.y > y0 && SD.y - sd_box_h() < y1;
    }
    if (t->kind != SDT_PLATE) return 0;
    float x0 = (float)(t->x * SD_TILE), x1 = x0 + SD_TILE, top = (float)((t->y + 1) * SD_TILE);   /* it lies in its tile's foot */
    if (!SD.dying && SD.ground && SD.x + SD_HW > x0 + 2.0f && SD.x - SD_HW < x1 - 2.0f && fabsf(SD.y - top) < 3.0f) return 1;
    const SdLevel *L = SD.L;
    for (int k = 0; k < L->nth; k++)
        if (sd_loose(&L->th[k]) && !SD.taken[k] && SD.ivy[k] == 0.0f && SD.ix[k] > x0 && SD.ix[k] < x1 && fabsf(SD.iy[k] + 5.0f - top) < 3.0f) return 1;
    return 0;
}

/* ---- the triggers: what each does, and its way back ---- */
static void sd_act(int target, int verb, int arg, int back) {
    if (verb == TRV_CRYPT) {                                       /* the crypt's own triggers on a thing of its level */
        if (!back && arg >= 0 && ev_live() && arg < drawn()->nthing) ev_touch(TG_USE, arg);
        return;
    }
    if (target < 0 || target >= SD.L->nth) return;
    int was = SD.st[target], now = verb == TRV_ON ? !back : verb == TRV_OFF ? back : !was;
    SD.st[target] = (uint8_t)now;
    if (was != now) {
        int k = SD.L->th[target].kind;
        if (k == SDT_GATE) snd_sfx_ex(SFX_CREAK, 0.7f, 0.5f);
        else if (k == SDT_CURRENT || k == SDT_FLOW) snd_sfx_ex(SFX_SPLASH, now ? 0.7f : 0.4f, 0.5f);
        else if (k == SDT_HAZ && !now && SD.hz_ph[SD.L->th[target].a] == 3) SD.hz_ph[SD.L->th[target].a] = 4;   /* a spread drains */
        else if (k == SDT_HAZ && now) SD.hz_t[SD.L->th[target].a] = 0.0f;   /* a trap woken goes off at once if nothing else sets it off */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT side_works %s %d %s t=%.2f\n", k == SDT_GATE ? "gate" : k == SDT_CURRENT ? "current" : k == SDT_FLOW ? "flow" : k == SDT_PLAT ? "platform" : k == SDT_HAZ ? "trap" : "thing",
                target, now ? "on" : "off", SD.t);
#endif
    }
}
static void sd_trig_do(const SdTrig *g, int back) { for (int a = 0; a < g->nact; a++) sd_act(g->target[a], g->verb[a], g->arg[a], back); }
static void sd_trigs(float dt) {
    const SdLevel *L = SD.L;
    for (int k = 0; k < L->ntr; k++) {
        const SdTrig *g = &L->tr[k];
        int p = sd_pressed(g->src), rose = p && !SD.tr_was[k];
        if (rose && L->th[g->src].kind == SDT_PLATE) snd_sfx_ex(SFX_KNOCK, 0.4f, 0.5f);
        switch (g->mode) {
        case TRM_ONCE: if (rose && !SD.tr_on[k]) { SD.tr_on[k] = 1; sd_trig_do(g, 0); } break;
        case TRM_TOGGLE: if (rose) { sd_trig_do(g, SD.tr_on[k]); SD.tr_on[k] = !SD.tr_on[k]; } break;
        case TRM_HOLD: if (p != SD.tr_on[k]) { sd_trig_do(g, !p); SD.tr_on[k] = (uint8_t)p; } break;
        case TRM_TIMED:
            if (p) { SD.tr_t[k] = g->secs; if (!SD.tr_on[k]) { SD.tr_on[k] = 1; sd_trig_do(g, 0); } }
            else if (SD.tr_on[k] && (SD.tr_t[k] -= dt) <= 0.0f) { SD.tr_on[k] = 0; sd_trig_do(g, 1); }
            break;
        }
        SD.tr_was[k] = (uint8_t)p;
    }
}

/* ---- gates, flows and currents, as their states have them ---- */
static void sd_waters(float dt) {
    const SdLevel *L = SD.L;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind == SDT_GATE) {
            float want = SD.st[i] ? 1.0f : 0.0f, step = dt / SDT[TU_GATE_TIME];
            if (want < SD.gk[i]) {                                 /* shutting: not on the caster's head */
                int shut_rows = (int)ceilf((float)t->h * (1.0f - (SD.gk[i] - step)) - 0.05f);
                float x0 = (float)(t->x * SD_TILE), x1 = (float)((t->x + t->w) * SD_TILE), yb = (float)((t->y + shut_rows) * SD_TILE);
                if (SD.x + SD_HW > x0 && SD.x - SD_HW < x1 && SD.y - sd_box_h() < yb && SD.y > (float)(t->y * SD_TILE)) continue;
            }
            SD.gk[i] = want > SD.gk[i] ? fminf(want, SD.gk[i] + step) : fmaxf(want, SD.gk[i] - step);
        } else if (t->kind == SDT_FLOW) {                          /* full for on seconds, dry for off; drained and still while switched off */
            float target = 0.0f;
            if (SD.st[i]) {
                SD.fc[i] += dt;
                float per = t->f[0] + t->f[1], c = fmodf(SD.fc[i], per);
                target = c < t->f[0] ? 1.0f : 0.0f;
            }
            SD.fk[i] = target > SD.fk[i] ? fminf(target, SD.fk[i] + dt / 0.6f) : fmaxf(target, SD.fk[i] - dt / 0.6f);
        }
    }
    /* the caster in a current: pushed along, slowed, and bitten if it is poison */
    int c = sd_current_at(SD.x, SD.y - sd_box_h() * 0.5f);
    if (c >= 0 && !SD.dying) {
        const SdThing *w = &L->th[c];
        float n = hypotf((float)w->a, (float)w->b), push = w->f[0] * SD_TILE * SDT[TU_CURRENT];
        sd_move_x((float)w->a / n * push * dt);
        if (w->b) { SD.y += (float)w->b / n * push * dt * 0.5f; if (w->b < 0) SD.vy = fminf(SD.vy, 0.0f); }
    }
}
static int sd_in_current_poison(float x, float y) { int c = sd_current_at(x, y); return c >= 0 && (SD.L->th[c].flags & SDF_POISON); }

/* ---- the crypt's traps in side view ---- */
static float sd_hz_warn(const SdHaz *z) { return z->warn >= 0.0f ? fmaxf(0.9f, z->warn) : z->fam == HZF_FALL ? 1.2f : z->fam == HZF_LINE ? 1.4f : z->fam == HZF_CLOUD ? 1.0f : 0.0f; }
static float sd_hz_harm(const SdHaz *z) {                          /* a share of health, a strike (or a bite, for what stays) */
    if (z->harm >= 0) return (float)z->harm / 100.0f;
    return z->fam == HZF_FALL ? 0.30f : z->fam == HZF_LINE ? 0.25f : 0.08f;
}
static int sd_hz_cell(const SdHaz *z, int c, int *x, int *y) {     /* its c-th cell of those it covers this turn */
    const SdLevel *L = SD.L;
    if (c < 0 || c >= z->nc) return 0;
    *x = L->hzc[z->c0 + c][0]; *y = L->hzc[z->c0 + c][1];
    return 1;
}
static int sd_hz_first(int h) { return SD.hz_at[h]; }               /* the turn's first painted cell, and how many */
static void sd_hz_go(int i, int h) {                                /* a turn begins: for a zone, a place near the caster */
    const SdHaz *z = &SD.L->hz[h];
    const SdThing *t = &SD.L->th[i];
    SD.hz_at[h] = 0; SD.hz_n[h] = z->nc;
    if ((t->flags & SDF_ZONE) && z->nc > 1 && z->fam != HZF_CREEP && z->fam != HZF_CLOUD) {
        int best = 0; float bd = 1e9f;
        for (int c = 0; c < z->nc; c++) { int x, y; sd_hz_cell(z, c, &x, &y); float d = fabsf((float)(x * SD_TILE + 8) - SD.x) + 0.5f * fabsf((float)(y * SD_TILE + 8) - SD.y) + sd_rndf() * 24.0f; if (d < bd) { bd = d; best = c; } }
        SD.hz_n[h] = z->fam == HZF_FALL ? 1 : imin(4, z->nc - best);
        SD.hz_at[h] = z->fam == HZF_FALL ? best : best;
    }
    SD.hz_ph[h] = z->fam == HZF_CREEP && !sd_fx_kind(z->kind) ? 3 : 1; SD.hz_k[h] = 0.0f;
    snd_sfx_ex(z->fam == HZF_FALL ? SFX_RUMBLE : z->fam == HZF_LINE ? SFX_CRACKLE : z->fam == HZF_CLOUD ? SFX_HISS : SFX_DRIP, 0.6f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT side_trap %s goes, %d cells from %d t=%.2f\n", sd_hz_name(z->kind), SD.hz_n[h], SD.hz_at[h], SD.t);
#endif
}
static int sd_hz_covers(int h, float x0, float y0, float x1, float y1, float k) {   /* the turn's cells (the first k of them, a share) overlap the box */
    const SdHaz *z = &SD.L->hz[h];
    int n = (int)ceilf((float)SD.hz_n[h] * k - 0.001f);
    for (int c = 0; c < n; c++) {
        int x, y;
        if (!sd_hz_cell(z, sd_hz_first(h) + c, &x, &y)) continue;
        if (x1 > (float)(x * SD_TILE) && x0 < (float)((x + 1) * SD_TILE) && y1 > (float)(y * SD_TILE) && y0 < (float)((y + 1) * SD_TILE)) return 1;
    }
    return 0;
}
static void sd_hz_strike(int i, int h, float share) {               /* its harm on the caster, by its element and what it does */
    const SdHaz *z = &SD.L->hz[h];
    float dmg = share * (float)G.max_hp;
    if (SD.L->th[i].flags & SDF_SAFE) dmg = fminf(dmg, fmaxf(0.0f, (float)(G.hp - G.max_hp / 4)));
    if (z->kind == HZ_VOID) SD.silence_t = 1.0f;                    /* the void: no spells while in it */
    if (z->kind == HZ_SNARE && sd_rndf() * 100.0f < (z->chance >= 0 ? (float)z->chance : 60.0f)) SD.hold_t = 1.0f;   /* the snare holds */
    if (dmg < 1.0f) return;
    G.blame = (Blame){ BL_HAZARD, z->kind, 0 };
    float keep = SD.inv_t;
    if (z->fam == HZF_CREEP || z->fam == HZF_CLOUD) SD.inv_t = 0.0f;   /* what stays bites through the grace after a hurt */
    sd_hurt(dmg, z->el < 0 ? EL_SPIRIT : z->el, SD.x - (float)SD.face);
    if (z->fam == HZF_CREEP || z->fam == HZF_CLOUD) { SD.inv_t = fmaxf(keep, 0.1f); SD.vx *= 0.3f; SD.vy = fmaxf(SD.vy, -40.0f); }
}
static void sd_traps(float dt) {
    const SdLevel *L = SD.L;
    float bx0 = SD.x - SD_HW, bx1 = SD.x + SD_HW, by0 = SD.y - sd_box_h(), by1 = SD.y;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        if (t->kind != SDT_HAZ) continue;
        int h = t->a;
        const SdHaz *z = &L->hz[h];
        if (z->kind == HZ_CENSER || (!z->nc && !sd_fx_kind(z->kind))) continue;   /* the censer swings on its own (side_fx.c) */
        if (z->kind == HZ_SNARE) { if (SD.st[i]) sd_fx_snare(i, h, dt); continue; }   /* the web, walked into */
        /* what sets it off: its clock, its box walked into, a trigger's waking (neither of the first two: at once) */
        if (SD.st[i] && SD.hz_ph[h] == 0) {
            int go = 0;
            if (z->how & HH_CLOCK) { if ((SD.hz_t[h] -= dt) <= 0.0f) { go = 1; SD.hz_t[h] = z->every + (sd_rndf() * 2.0f - 1.0f) * z->give; } }
            if (z->how & HH_ENTER) {
                int in = SD.x >= (float)(z->x0 * SD_TILE) && SD.x < (float)((z->x1 + 1) * SD_TILE) && SD.y > (float)(z->y0 * SD_TILE) && SD.y - 1.0f < (float)((z->y1 + 1) * SD_TILE);
                if (in && !SD.hz_in[h]) go = 1;
                SD.hz_in[h] = in;
            }
            if (!z->how && SD.hz_t[h] <= 0.0f) { go = 1; SD.hz_t[h] = 1e9f; }   /* woken by a trigger, or live from the start */
            if (go) sd_hz_go(i, h);
        }
        if (sd_fx_kind(z->kind)) { sd_fx_trap(i, h, dt); continue; }   /* run I's that pour and drift (2e) */
        int ph = SD.hz_ph[h];
        if (!ph) continue;
        float w = sd_hz_warn(z);
        SD.hz_k[h] += dt;
        switch (z->fam) {
        case HZF_FALL:                                             /* dust and a shadow; then it comes down: rubble a while */
            if (ph == 1) sd_fx_grit(h, dt);                         /* grit from the crack first */
            if (ph == 1 && SD.hz_k[h] >= w) { SD.hz_ph[h] = 2; SD.hz_k[h] = 0.0f; snd_sfx_ex(SFX_BOOM, 0.7f, 0.5f); sd_fx_rubble(h); if (sd_hz_covers(h, bx0, by0, bx1, by1, 1.0f)) sd_hz_strike(i, h, sd_hz_harm(z)); }
            else if (ph == 2 && SD.hz_k[h] >= 2.5f) SD.hz_ph[h] = 0;
            break;
        case HZF_LINE:                                             /* the cells glow one by one in their order; then all burst a second; embers */
            if (ph == 1 && SD.hz_k[h] >= w) { SD.hz_ph[h] = 2; SD.hz_k[h] = 0.0f; snd_sfx_ex(z->el == EL_FIRE ? SFX_IGNITE : z->el == EL_LIGHT ? SFX_ZAP : SFX_BOOM, 0.8f, 0.5f); SD.hz_bite[h] = 0.0f; }
            else if (ph == 2) {
                if (SD.hz_bite[h] <= 0.0f && sd_hz_covers(h, bx0, by0, bx1, by1, 1.0f)) { sd_hz_strike(i, h, sd_hz_harm(z)); SD.hz_bite[h] = 1.0f; }
                if (SD.hz_k[h] >= 1.0f) { SD.hz_ph[h] = 5; SD.hz_k[h] = 0.0f; }
            } else if (ph == 5 && SD.hz_k[h] >= 1.5f) SD.hz_ph[h] = 0;
            break;
        case HZF_CREEP: {                                          /* it spreads cell by cell from the first, stays, then drains back */
            float per = 0.22f, k = fminf(1.0f, SD.hz_k[h] / (per * (float)imax(1, SD.hz_n[h])));
            if (ph == 4) k = fmaxf(0.0f, 1.0f - SD.hz_k[h] / (per * (float)imax(1, SD.hz_n[h])));
            if ((SD.hz_bite[h] -= dt) <= 0.0f && sd_hz_covers(h, bx0, by0, bx1, by1, k)) { sd_hz_strike(i, h, sd_hz_harm(z)); SD.hz_bite[h] = SDT[TU_BITE_EVERY]; }
            if (ph == 3 && (z->how & HH_CLOCK) && SD.hz_k[h] > per * (float)SD.hz_n[h] + z->hold) { SD.hz_ph[h] = 4; SD.hz_k[h] = 0.0f; }
            if (ph == 3 && !SD.st[i]) { SD.hz_ph[h] = 4; SD.hz_k[h] = 0.0f; }
            if (ph == 4 && k <= 0.0f) SD.hz_ph[h] = 0;
            break;
        }
        case HZF_CLOUD:                                            /* it gathers, hangs over its cells a while, and lifts */
            if (ph == 1 && SD.hz_k[h] >= w) { SD.hz_ph[h] = 2; SD.hz_k[h] = 0.0f; SD.hz_bite[h] = 0.0f; }
            else if (ph == 2) {
                if ((SD.hz_bite[h] -= dt) <= 0.0f && sd_hz_covers(h, bx0, by0, bx1, by1, 1.0f)) { sd_hz_strike(i, h, sd_hz_harm(z)); SD.hz_bite[h] = SDT[TU_BITE_EVERY]; }
                if (SD.hz_k[h] >= fminf(z->hold, 4.0f)) { SD.hz_ph[h] = 5; SD.hz_k[h] = 0.0f; }
            } else if (ph == 5 && SD.hz_k[h] >= 0.8f) SD.hz_ph[h] = 0;
            break;
        }
    }
    SD.silence_t = fmaxf(0.0f, SD.silence_t - dt); SD.hold_t = fmaxf(0.0f, SD.hold_t - dt);
    sd_fx_step(dt);                                                /* the particles and the censers (side_fx.c) */
}

/* ---- laid as the passage opens, and again after a life lost ---- */
static void sd_works_build(void) {
    const SdLevel *L = SD.L;
    memset(SD.st, 0, sizeof SD.st); memset(SD.gk, 0, sizeof SD.gk); memset(SD.fc, 0, sizeof SD.fc); memset(SD.fk, 0, sizeof SD.fk);
    memset(SD.tr_on, 0, sizeof SD.tr_on); memset(SD.tr_was, 0, sizeof SD.tr_was); memset(SD.tr_t, 0, sizeof SD.tr_t);
    memset(SD.hz_ph, 0, sizeof SD.hz_ph); memset(SD.hz_in, 0, sizeof SD.hz_in); memset(SD.hz_k, 0, sizeof SD.hz_k); memset(SD.hz_bite, 0, sizeof SD.hz_bite);
    SD.silence_t = SD.hold_t = 0.0f;
    for (int i = 0; i < L->nth; i++) {
        const SdThing *t = &L->th[i];
        SD.st[i] = !(t->flags & SDF_OFF);
        if (t->kind == SDT_GATE) { SD.st[i] = (t->flags & SDF_OPEN) != 0; SD.gk[i] = SD.st[i] ? 1.0f : 0.0f; }
        if (t->kind == SDT_FLOW) { SD.fc[i] = t->f[2]; float per = t->f[0] + t->f[1], c = fmodf(SD.fc[i], per); SD.fk[i] = SD.st[i] && c < t->f[0] ? 1.0f : 0.0f; }
        if (t->kind == SDT_HAZ) { const SdHaz *z = &L->hz[t->a]; SD.hz_t[t->a] = (z->how & HH_CLOCK) ? z->first : 0.0f; }
        SD.ix[i] = (float)(t->x * SD_TILE) + 8.0f; SD.iy[i] = (float)((t->y + 1) * SD_TILE) - 5.0f; SD.ivx[i] = SD.ivy[i] = 0.0f;
    }
    sd_fx_build();
}
