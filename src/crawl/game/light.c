/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The light map: dynamic lights, the torch's flicker, build_lightmap. */

/* ================================================================== */
/* Light map                                                           */
/* ================================================================== */

/* flat: an even pool of light that fades only towards its edge, rather than a flame's bright heart */
static void light_dynamic_ex(float lx, float ly, float power, float radius, int flat) {
    int x0 = clampi((int)((lx - radius) * LMR), 0, LMN - 1), x1 = clampi((int)((lx + radius) * LMR), 0, LMN - 1);
    int y0 = clampi((int)((ly - radius) * LMR), 0, LMN - 1), y1 = clampi((int)((ly + radius) * LMR), 0, LMN - 1);
    int lcx = (int)lx, lcy = (int)ly;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            float wx = (tx + 0.5f) / LMR, wy = (ty + 0.5f) / LMR;
            float d = sqrtf((wx - lx) * (wx - lx) + (wy - ly) * (wy - ly));
            if (d >= radius) continue;
            int cx = tx / LMR, cy = ty / LMR;
            if (abs(cx - lcx) + abs(cy - lcy) > 1) {               /* cheap occlusion: step towards the light */
                int blocked = 0;
                float sx = wx, sy = wy, vx = (lx - wx) / d * 0.35f, vy = (ly - wy) / d * 0.35f;
                for (float s = 0.35f; s < d - 0.35f; s += 0.35f) {
                    sx += vx; sy += vy;
                    if (blocks_light((int)sx, (int)sy)) { blocked = 1; break; }
                }
                if (blocked) continue;
            }
            float q = 1.0f - d * d / (radius * radius);
            G.lm[ty * LMN + tx] += flat ? power * q : power / (1.0f + 0.9f * d * d) * (1.0f - smoothstepf(radius * 0.55f, radius, d));
        }
}
static void light_dynamic(float lx, float ly, float power, float radius) { light_dynamic_ex(lx, ly, power, radius, 0); }
static void hz_lights(void);

static float torch_flicker(const Light *L) {
    if (L->phase < -100.0f) return 1.0f;                          /* a steady glow, not a flame */
    if (L->phase < 0.0f) {                                         /* a failing tube: steady, then a burst of blinking */
        float t = (float)G.t - L->phase * 13.0f;
        if (fmodf(t, 5.3f) > 0.9f) return 1.0f;
        uint32_t h = (uint32_t)(int)(t * 14.0f) * 2654435761u;
        return (h >> 13 & 3) ? 0.12f : 1.0f;
    }
    float t = (float)G.t + L->phase;
    return 0.80f + 0.12f * sinf(t * 9.1f) * sinf(t * 5.3f + 1.0f) + 0.08f * sinf(t * 23.0f + L->phase * 3.0f);
}

static void tri_dark(void);
static float lm_at(float x, float y);
static int gift_due(void);
static void ward_lights(void);
static void build_lightmap(void) {
    float ambient = G.shop ? 0.06f : (PK.depth ? (G.ptheme == PT_SEVERANCE ? 0.16f : 0.08f) : 0.06f - 0.012f * depth());   /* the dark itself deepens, set by set; not in a pocket */
    for (int i = 0; i < LMN * LMN; i++) G.lm[i] = ambient;
    const Mob *wr = NULL;                                          /* torches gutter where the Wraith goes */
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_WRAITH) wr = &G.mobs[i];
    for (int l = 0; l < G.nlights; l++) {
        const Light *L = &G.lights[l];
        float f = torch_flicker(L);
        if (wr && fabsf(L->x - wr->x) + fabsf(L->y - wr->y) < 5.0f) f *= 0.35f + 0.15f * sinf((float)G.t * 13.0f + l);
        for (int k = 0; k < G.namb; k++) if (G.amb[k].snuff == l + 1) f *= 0.1f;   /* snuffed by a kill room's door */
        for (int k = L->first; k < L->first + L->count; k++) G.lm[G.lc_idx[k]] += G.lc_w[k] * f;
    }
    tri_dark();                                                    /* a trial's room gone dark (1.0.0b13) */
    {
        float pre = lm_at(G.px, G.py);
        light_dynamic(G.px, G.py, 0.62f, 2.8f);                    /* the caster's own faint glow */
        G.glow_here = lm_at(G.px, G.py) - pre;                     /* what the dead see of it is apart (S2) */
    }
    if (lantern_lit() && !G.shop) light_dynamic_ex(G.px, G.py, 0.9f + 0.06f * sinf((float)G.t * 3.1f), 2.6f, 1);   /* the Lantern of the Dead */
    for (int b = 0; b < G.bbz_n && G.boss == BOSS_WRAITH && !PK.depth && !G.shop; b++)   /* the Wraith's braziers, those still lit */
        if (G.bbz_lit[b]) light_dynamic(G.bbz_x[b] + 0.5f, G.bbz_y[b] + 0.5f, 1.5f * (0.92f + 0.08f * sinf((float)G.t * 9.0f + b)), 5.2f);
    if (G.shop) light_dynamic(PORTAL_X + 0.5f, PORTAL_Y - 0.3f, 1.0f + 0.15f * sinf((float)G.t * 2.3f), 4.6f);   /* the Portal's glow */
    if (G.shop && RUN.shrine > 0)                                  /* the cursed shrine, brighter as it works */
        light_dynamic(SHRINE_X, SHRINE_Y - 0.4f, 0.7f + 0.8f * fminf(1.0f, G.sh_glow) + 0.08f * sinf((float)G.t * 5.0f), 3.6f);
    for (int i = 0; i < MAXWISPS && !G.shop; i++) {                /* and the wisps', four cells round them */
        const Wisp *w = &G.wisps[i];
        if (w->on && w->a > 0.0f) light_dynamic_ex(w->x, w->y, 0.6f * w->a * (0.93f + 0.07f * sinf((float)G.t * 7.0f + i)), 4.0f, 1);
    }
    if (G.chg_on) {                                                /* a charge: light in its colour, growing with it */
        float k = clampf(G.chg_t, 0.0f, 1.0f);
        light_dynamic_ex(G.px + cosf(G.ang) * 0.4f, G.py + sinf(G.ang) * 0.4f, 0.3f + 1.1f * k, 2.4f + 1.6f * k, 1);
    }
    for (int i = 0; i < MAXMOTES; i++)
        if (G.motes[i].alive) light_dynamic_ex(G.motes[i].x, G.motes[i].y, 0.35f * (1.0f - smoothstepf(MOTE_LIFE - 2.0f, MOTE_LIFE, G.motes[i].t)), 1.6f, 1);
    for (int i = 0; i < G.nitems && !G.shop; i++)                  /* a won trial's hoard: its own light, a flare as it comes (1.0.0b18.1) */
        if (G.items[i].alive && G.items[i].type == IT_CHEST && G.items[i].color == TRI_REWARD)
            light_dynamic_ex(G.items[i].x, G.items[i].y, 0.75f + 0.2f * sinf((float)G.t * 2.4f) + 1.4f * (1.0f - smoothstepf(0.0f, 1.2f, G.tri_end)), 2.6f, 1);
    ward_lights();                                                 /* a lit soul-lamp's (1.0.0b18.1) */
    for (int k = 0; k < 3 && G.fwall_t > 0.0f; k++) {
        float an = (float)G.t * 2.2f + (float)k * TAU_F / 3.0f;
        light_dynamic(G.px + cosf(an) * 1.2f, G.py + sinf(an) * 1.2f, 0.7f, 2.6f);
    }
    for (int i = 0; i < MAXSHOTS; i++)
        if (G.shots[i].alive) {
            static const float POW[NSHOTKIND] = { 1.7f, 1.1f, 1.2f, 0.4f, 1.0f, 2.3f, 0.8f, 0.7f, 0.0f, 1.4f, 0.0f, 2.4f, 1.3f, 0.4f, 0.2f };
            light_dynamic(G.shots[i].x, G.shots[i].y, POW[G.shots[i].kind], G.shots[i].kind == AT_HELL ? 4.4f : 3.4f);
        }
    for (int i = 0; i < MAXBONES; i++)
        if (G.bones[i].alive && G.bones[i].kind != MIS_BONE) light_dynamic(G.bones[i].x, G.bones[i].y, G.bones[i].kind == MIS_FIRE ? 1.6f : 0.9f, G.bones[i].kind == MIS_FIRE ? 3.6f : 2.6f);
    for (int i = 0; i < MAXBOOMS; i++)
        if (G.booms[i].alive) light_dynamic(G.booms[i].x, G.booms[i].y, (G.booms[i].big ? 3.6f : 2.8f) * (1.0f - G.booms[i].t / 0.6f), G.booms[i].big ? 5.5f : 4.2f);
    for (int i = 0; i < G.nmk; i++) {                              /* a pricket or a rack lit, a coil woken (1.0.0b16) */
        const Marked *m = &G.mk[i];
        if (m->kind == MKK_CLIGHT && !m->done) { light_dynamic(m->x, m->y, 0.45f, 2.2f); continue; }   /* a corpse-light's own pale glow */
        if (m->kind == MKK_HSEAL && m->st == 1 && !m->done) { light_dynamic(m->x, m->y, 0.5f * m->t / HSEAL_HOT + 0.2f, 2.4f); continue; }
        if (m->kind == MKK_HEARTH && (m->got || m->done)) { light_dynamic(m->x, m->y, (m->done ? 1.0f : 0.3f + 0.6f * (float)m->got / (float)m->pips) * (0.9f + 0.1f * sinf((float)G.t * 9.0f + (float)i)), 3.6f); continue; }
        if (!m->done) continue;
        if (m->kind == MKK_PRICKET || m->kind == MKK_RACK) light_dynamic(m->x, m->y, 0.85f * (0.92f + 0.08f * sinf((float)G.t * 7.0f + (float)i)), 3.2f);
        else if (m->kind == MKK_COIL || m->kind == MKK_LODE) light_dynamic(m->x, m->y, 0.65f + 0.1f * sinf((float)G.t * 23.0f), 2.8f);
        else if (m->kind == MKK_PRISM) light_dynamic(m->x, m->y, 0.9f, 3.4f);
    }
    if (G.shop) {                                                  /* the Lampstand's bowls light the room north of hers */
        float lit = 0.0f;
        for (int e = 0; e < NELEM; e++) lit += align_rank(e) ? 0.35f + 0.02f * (float)imin(align_rank(e), 20) : 0.0f;
        if (gift_due()) lit += 0.9f + 0.5f * sinf((float)G.t * 2.4f);   /* a gift waits: it breathes */
        lit += G.gift_glow;
        if (lit > 0.0f) light_dynamic(LAMP_X + 0.5f, LAMP_Y + 0.9f, lit * (0.94f + 0.06f * sinf((float)G.t * 6.0f)), 3.4f + 0.5f * fminf(lit, 3.0f));
    }
    for (int i = 0; i < MAXBURNS; i++)
        if (G.burns[i].alive) light_dynamic(G.burns[i].x, G.burns[i].y, (G.burns[i].foe == 2 ? 0.8f : 1.5f) * (1.0f - smoothstepf(G.burns[i].life - 0.7f, G.burns[i].life, G.burns[i].t)) * (0.85f + 0.15f * sinf((float)G.t * 17.0f + i)), 3.2f);
    drawn_fire_lights();                                           /* a drawn level's burning floor */
    for (int i = 0; i < MAXBEAMS; i++)
        if (G.beams[i].alive) light_dynamic((G.beams[i].x0 + G.beams[i].x1) * 0.5f, (G.beams[i].y0 + G.beams[i].y1) * 0.5f, 1.6f * (1.0f - G.beams[i].t / 0.3f), 3.5f);
    for (int i = 0; i < MAXMOBS; i++) {                            /* a wall skull gathering its fire; the bones a deacon calls */
        const Mob *m = &G.mobs[i];
        if (m->alive && m->type == MOB_WSKULL && m->state == MS_WALK && m->sub == 1)
            light_dynamic(m->x, m->y, 0.3f + 1.2f * fminf(1.0f, m->sub_t / 3.0f), 3.0f);
        if (m->alive && m->type == MOB_DEACON && m->state == MS_WALK && m->swing > 0.0f)
            light_dynamic(m->ax, m->ay, 0.4f + 0.8f * (1.0f - m->swing / DEACON_CHANT), 2.6f);
    }
    for (int i = 0; i < G.nitems; i++)                             /* keys glow, the urns faintly, a closet's loot a little */
        if (G.items[i].alive && G.items[i].secret && !G.map[G.items[i].cy][G.items[i].cx].secret)
            light_dynamic(G.items[i].x, G.items[i].y, 0.35f + 0.1f * sinf((float)G.t * 2.0f + i), 1.8f);
    for (int i = 0; i < G.nitems; i++)
        if (G.items[i].alive && (G.items[i].type == IT_KEY || G.items[i].type == IT_URN))
            light_dynamic(G.items[i].x, G.items[i].y, (G.items[i].type == IT_KEY ? 0.75f : 0.35f) + 0.2f * sinf((float)G.t * 3.0f + i), 2.2f);
    if (G.state == PS_CAST && G.hand > 0.3f)
        light_dynamic(G.px + cosf(G.ang) * 0.3f, G.py + sinf(G.ang) * 0.3f, 0.9f * G.hand, 2.4f);
    if (!G.shop && G.altar_x >= 0 && (G.altar_state == ALTAR_AWAKE || G.altar_state == ALTAR_READY || G.blessed))   /* the altar's fire */
        light_dynamic(G.altar_x + 0.5f, G.altar_y + 0.5f, 1.1f + 0.15f * sinf((float)G.t * 9.0f), 3.6f);
    hz_lights();
    if (G.rest_on && G.rest_on <= G.nrest)                         /* a place to mend, in use: it glows */
        light_dynamic(G.rest_fx[G.rest_on - 1] + 0.5f, G.rest_fy[G.rest_on - 1] + 0.5f, 0.9f + 0.25f * sinf((float)G.t * 2.4f), 3.0f);
    G.lit_here = lm_at(G.px, G.py) - G.glow_here;
}

#ifdef CRAWL_DEBUG
/* CRAWL_STAGE=k: one kind stood still down the longest straight hall, at set distances, the caster still and looking
 * at them; CRAWL_STAGEDARK=f scales the light (0 none), CRAWL_STAGEFOG=1 thickens the fog to its most, CRAWL_STAGESUB=s
 * puts them in that state, CRAWL_STAGEFACE=deg turns them from facing the caster, CRAWL_STAGEDIST=d just one at d cells, CRAWL_STAGEBOSS=b each drawn as that boss (BOSS_*, of its
 * kind), CRAWL_STAGEREAR=1 the Wandering Spider rearing to spit, CRAWL_STAGECHANT=1 a deacon chanting; with CRAWL_PROPROOM=k
 * no creature at all, and the caster in a corner of the first room of type k (RT_*), for the props, or with CRAWL_TPPROP=spr
 * before the first prop drawn as that sprite (tp_prop). For frames of the art pass (docs/InitialArtPass.md). */
static int g_stage = -1, g_stage_fog;
static float g_stage_lm = 1.0f;
#endif
static float lm_at(float x, float y) {
#ifdef CRAWL_DEBUG
    if (g_stage_lm < 1.0f) {
        float f0 = x * LMR - 0.5f, f1 = y * LMR - 0.5f; int i0 = (int)floorf(f0), i1 = (int)floorf(f1);
        if (i0 < 0 || i1 < 0 || i0 >= LMN - 1 || i1 >= LMN - 1) return 0.0f;
        return G.lm[i1 * LMN + i0] * g_stage_lm;
    }
#endif
    float fx = x * LMR - 0.5f, fy = y * LMR - 0.5f;
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    float ax = fx - ix, ay = fy - iy;
    if (ix < 0 || iy < 0 || ix >= LMN - 1 || iy >= LMN - 1) return 0.0f;
    const float *p = &G.lm[iy * LMN + ix];
    return (p[0] * (1 - ax) + p[1] * ax) * (1 - ay) + (p[LMN] * (1 - ax) + p[LMN + 1] * ax) * ay;
}

static inline float atten(float d) { return 1.0f / (1.0f + 0.010f * d * d); }

