/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The walls: the sliding secret's ray, render_walls. */

static int slide_ray(float rdx, float rdy, float *t, float *u, int *side) {
    float bx, by, tx0, tx1, ty0, ty1;
    slide_box(&bx, &by);
    if (fabsf(rdx) < 1e-6f) { if (G.px < bx || G.px > bx + 1) return 0; tx0 = -1e30f; tx1 = 1e30f; }
    else { tx0 = (bx - G.px) / rdx; tx1 = (bx + 1 - G.px) / rdx; if (tx0 > tx1) { float q = tx0; tx0 = tx1; tx1 = q; } }
    if (fabsf(rdy) < 1e-6f) { if (G.py < by || G.py > by + 1) return 0; ty0 = -1e30f; ty1 = 1e30f; }
    else { ty0 = (by - G.py) / rdy; ty1 = (by + 1 - G.py) / rdy; if (ty0 > ty1) { float q = ty0; ty0 = ty1; ty1 = q; } }
    float tn = fmaxf(tx0, ty0), tf = fminf(tx1, ty1);
    if (tn > tf || tn <= 0.0f) return 0;
    *side = tx0 > ty0 ? 0 : 1;
    *u = *side == 0 ? G.py + rdy * tn - by : G.px + rdx * tn - bx;
    *t = tn;
    return 1;
}

static void render_walls(Frame *f, float dx, float dy, float plx, float ply, float horizon) {
    int tframe = (int)(G.t * 8.0);
    static int cracks_built;
    if (!cracks_built) { build_cracks(); cracks_built = 1; }
    for (int x = 0; x < W; x++) {
        float camx = 2.0f * (x + 0.5f) / W - 1.0f;
        float rdx = dx + plx * camx, rdy = dy + ply * camx;
        int mx = (int)G.px, my = (int)G.py;
        float ddx = rdx == 0.0f ? 1e30f : fabsf(1.0f / rdx), ddy = rdy == 0.0f ? 1e30f : fabsf(1.0f / rdy);
        int sx = rdx < 0 ? -1 : 1, sy = rdy < 0 ? -1 : 1;
        float sdx = rdx < 0 ? (G.px - mx) * ddx : (mx + 1.0f - G.px) * ddx;
        float sdy = rdy < 0 ? (G.py - my) * ddy : (my + 1.0f - G.py) * ddy;
        int side = 0, tex = T_STONE, texx = 0, hit = 0, over = -1, flip = 0, por = -1, ptx = 0, ng = 0, gw[8];
        float perp = 30.0f, cracks = 0.0f, galong = 0.0f;
        for (int i = 0; i < 100 && !hit; i++) {
            if (sdx < sdy) { sdx += ddx; mx += sx; side = 0; } else { sdy += ddy; my += sy; side = 1; }
            Cell *c = cell(mx, my);
            if (G.slide >= 0 && slide_covers(mx, my)) {           /* the secret wall, on its way back */
                float t, u; int sd;
                if (slide_ray(rdx, rdy, &t, &u, &sd) && (int)floorf(G.px + rdx * t + rdx * 1e-3f) == mx && (int)floorf(G.py + rdy * t + rdy * 1e-3f) == my) {
                    perp = t; tex = G.secrets[G.slide].tex; side = sd;
                    texx = clampi((int)(u * TS), 0, TS - 1);
                    if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0)) texx = TS - 1 - texx;
                    cracks = 1.0f;
                    hit = 1;
                    continue;
                }
            }
            if (c->kind == K_WALL) {
                perp = side == 0 ? sdx - ddx : sdy - ddy;
                tex = c->tex;
                int pxc = side == 0 ? mx - sx : mx, pyc = side == 1 ? my - sy : my;
                int fd = side == 0 ? (sx > 0 ? 2 : 0) : (sy > 0 ? 3 : 1);   /* the face looked at */
                if (c->por && POR.at[c->por - 1].fd == fd) {              /* a portrait of the slain, over its face's own stone */
                    por = c->por - 1; flip = c->fflip >> fd & 1;
                    tex = c->face[fd] ? c->face[fd] : plain_tex(tex) ? tex : T_BRICK;
                } else if (cell(pxc, pyc)->kind == K_DOOR) tex = G.faced ? T_RJAMB : T_JAMB;
                else if (c->face[fd] && plain_tex(tex)) {
                    tex = c->face[fd]; flip = c->fflip >> fd & 1;
                    if (c->fover[fd]) over = c->fover[fd] == T_FEAT + F1_TORCH ? T_FEAT + F1_TORCH + (int)((tframe + hash2(mx, my)) % 3) : c->fover[fd];
                }
                if (tex == T_TORCH0) tex = T_TORCH0 + (int)((tframe + hash2(mx, my)) % 3);
                if (tex == T_EXIT0) tex = boss_sealed() ? T_GATE : G.exit_open > 0.3f ? T_EXIT_OPEN : T_EXIT0 + ((int)(G.t * 2.5) & 1);   /* barred while a boss lives */
                if (tex == T_VISPORT0) tex = T_VISPORT0 + (PK.vis_used[PK.chain] ? 0 : ((int)(G.t * 5.0) & 3));
                if (tex == T_PORTAL0) tex = T_PORTAL0 + ((int)(G.t * 3.0) & 3);   /* the Accursed Portal, turning */
                float wallx = side == 0 ? G.py + perp * rdy : G.px + perp * rdx;
                wallx -= floorf(wallx);
                texx = (int)(wallx * TS);
                if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0)) texx = TS - 1 - texx;
                if (por >= 0) {                                         /* the portrait's own column, never mirrored */
                    ptx = clampi((int)(wallx * POR_RES), 0, POR_RES - 1);
                    if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0)) ptx = POR_RES - 1 - ptx;
                }
                if (flip) texx = TS - 1 - texx;
                if (G.shop && POR.nwf) {                                /* the portraits' candles: their glow on the wall about them */
                    float plane = side == 0 ? (float)(mx + (fd == 0)) : (float)(my + (fd == 1));
                    galong = side == 0 ? G.py + perp * rdy : G.px + perp * rdx;
                    for (int k = 0; k < POR.nwf && ng < 8; k++) {
                        const PorFlame *w = &POR.wf[k];
                        if (w->fd == fd && fabsf(w->plane - plane) < 0.01f && fabsf(w->along - galong) < 0.8f) gw[ng++] = k;
                    }
                }
                if (c->secret == 2) cracks = 1.0f - smoothstepf(1.3f, 2.8f, perp);   /* only close up do the cracks show */
                hit = 1;
            } else if (c->kind == K_DOOR) {
                Door *d = &G.doors[c->door];
                float t, u;
                if (d->along_x) {
                    if (rdx == 0.0f) continue;
                    t = (mx + 0.5f - G.px) / rdx;
                    u = G.py + t * rdy - my;
                } else {
                    if (rdy == 0.0f) continue;
                    t = (my + 0.5f - G.py) / rdy;
                    u = G.px + t * rdx - mx;
                }
                if (t <= 0.0f || u < 0.0f || u >= 1.0f || u < d->open) continue;
                static const int LOCKTEX[NKEYS] = { T_LOCKED, T_LOCKED_S, T_LOCKED_C }, RLOCKTEX[NKEYS] = { T_RLOCKED, T_RLOCKED_S, T_RLOCKED_C };
                perp = t; tex = d->locked == WINGED ? T_WINGDOOR : g_drawn == 2 && d - G.doors == IC.gate_door ? (d->locked == GATED && G.py > d->y + 0.5f ? T_GATE_SEAL : T_GATE) : d->locked == GATED ? T_GATE : G.tri >= 0 && d - G.doors == G.tri_door ? T_TRDOOR : d->locked ? (G.faced ? RLOCKTEX[d->color] : LOCKTEX[d->color]) : (G.faced ? T_RDOOR : T_DOOR); side = d->along_x ? 0 : 1;
                texx = clampi((int)((u - d->open) * TS), 0, TS - 1);
                hit = 1;
            }
        }
        if (perp < 0.05f) perp = 0.05f;
        zbuf[x] = perp;
        if (!hit) continue;
        float lineh = FOCAL / perp;
        float top = horizon - (1.0f - G.camz) * lineh, bot = horizon + G.camz * lineh;
        float hx = G.px + perp * rdx - (side == 0 ? sx * 0.13f : 0.0f);
        float hy = G.py + perp * rdy - (side == 1 ? sy * 0.13f : 0.0f);
        float light = lm_at(hx, hy) * atten(perp) * (side ? 0.82f : 1.0f);
        float fog = fog_amount(hx, hy, 1.0f - expf(-perp * 0.28f));
        const uint8_t *col = &crawl_tex[tex][texx * TS];
        int y0 = clampi((int)ceilf(top - 0.5f), 0, H), y1 = clampi((int)ceilf(bot - 0.5f), 0, H);
        float tstep = TS / lineh, ty = (y0 + 0.5f - top) * tstep;
        if (over >= 0) {                                            /* what stands on the wall, over its stone */
            static uint8_t mix[TS];
            const uint8_t *oc = &crawl_tex[over][texx * TS];
            for (int k = 0; k < TS; k++) mix[k] = oc[k] != C_CLEAR ? oc[k] : col[k];
            col = mix;
        }
        int tn = TS;                                                /* the column's texels */
        if (por >= 0) {                                             /* a portrait, at twice the walls' texels, over its stone */
            static uint8_t pmix[POR_RES];
            const uint8_t *pc = &POR_OV[por][ptx * POR_RES];
            for (int k = 0; k < POR_RES; k++) pmix[k] = pc[k] != C_CLEAR ? pc[k] : col[k * TS / POR_RES];
            col = pmix; tn = POR_RES; tstep = POR_RES / lineh; ty = (y0 + 0.5f - top) * tstep;
            POR.seen[por] = POR.frame;                              /* in view: drawn again as it moves */
        }
        if (cracks > 0.0f) {
            const uint8_t *cm = &crack_mask[texx * TS];
            int thr = (int)(cracks * 16.0f);
            for (int y = y0; y < y1; y++, ty += tstep) {
                int tyi = clampi((int)ty, 0, TS - 1), m = cm[tyi];
                float L = m && BAYER[y & 3][x & 3] < thr ? (m == 1 ? light * 0.2f : light * 1.3f) : light;
                f->pix[y * W + x] = fogged(shade(col[tyi], L, x, y), fog, light, x, y);
            }
            continue;
        }
        if (ng) {                                                   /* a candle's glow, row by row (as the mock-ups light them), worked
                                                                       out only within its reach, every other row on a portrait's face */
            static float gl[TS];
            float lk = atten(perp) * (side ? 0.82f : 1.0f);
            for (int r = 0; r < TS; r++) gl[r] = 0.0f;
            for (int q = 0; q < ng; q++) {
                const PorFlame *w = &POR.wf[gw[q]];
                float fk = 1.5f * w->pw * torch_flicker(&G.lights[w->light]) * lk, dx = galong - w->along, reach = sqrtf(fmaxf(0.0f, 0.64f - dx * dx)) / 1.1f;
                int r0 = imax(0, (int)((w->z - reach) * TS)), r1 = imin(TS - 1, (int)((w->z + reach) * TS) + 1);
                for (int r = r0; r <= r1; r++) {
                    float dz = (((float)r + 0.5f) / (float)TS - w->z) * 1.1f, d3 = sqrtf(dx * dx + dz * dz);
                    if (d3 < 0.8f) { float e = 1.0f - d3 / 0.8f; gl[r] += fk * e * e; }
                }
            }
            int gs = tn / TS;                                       /* texels of the column to a row of the glow */
            for (int y = y0; y < y1; y++, ty += tstep) {
                int ti = clampi((int)ty, 0, tn - 1);
                f->pix[y * W + x] = fogged(shade(col[ti], light + gl[ti / gs], x, y), fog, light, x, y);
            }
            continue;
        }
        for (int y = y0; y < y1; y++, ty += tstep)
            f->pix[y * W + x] = fogged(shade(col[clampi((int)ty, 0, tn - 1)], light, x, y), fog, light, x, y);
    }
}

/* alpha < 1: dithered see-through. remap: the ramp R_PURPLE turns into. flags: how it is drawn.
 * lut: a creature's rank, recolouring its body (crawl_rank_lut) */
typedef struct { float x, y, z, depth; int spr; float alpha, twinkle, scale; int remap, flags; float sway; const uint8_t *lut; int glint; float web, mg; int mgw;
                 int eye, etype, erank, estate; const uint8_t *ov; float vap; int ecol, ward; } Draw;   /* ecol: 1 + EC_*, its eyes' colour whatever its rank */   /* ov: its rank's dressing (crawl_dress);
                                                                                                vap: its spirit vapour, 0 none, 1 full */   /* eye: a creature's eyes go on after it (render_eyes), as its kind, rank and state */   /* mg: the hidden
                                                                                                   message's glint, mgw through the walls; glint: a key's
                                                                                                   ramp, for Keymaker's sparkle; web: how much of yours holds it */
enum { DR_EYES = 1, DR_IMPLODE = 2, DR_BRIGHT = 4, DR_ICE = 8, DR_CAT = 16, DR_SWAY = 32, DR_FLIPV = 64, DR_COMMUNE = 128, DR_GLINT = 256, DR_EDGE = 512,
       DR_GLITCH = 1024, DR_CLIP = 2048, DR_LOOPED = 4096, DR_LIT = 8192, DR_SHINE = 16384, DR_MEND = 32768,
       DR_RIPPLE = 65536, DR_ALLY = 131072, DR_WEBBED = 262144, DR_VENOM = 524288, DR_POISONED = 1048576, DR_BURNING = 2097152, DR_PINNED = 4194304,
       DR_FLIPH = 8388608,                                           /* FLIPH: turned left to right (a side view, facing your left) */
       /* docs/PLAN-combat.md, two tracks on one branch (its section 17): each adds flags only above its own end mark; the
          caster's track takes bits from 24 upwards, the dead's from 30 downwards, so the two never take the same */
       DR_ELEMX = 1 << 24,                                           /* a missile carrying an element (C3): shimmering in its colour, the
                                                                      ramp in remap; a mote of it wholly */
       /* the caster's track (C) ends */
       /* the dead's track (D): */
       DR_CHILLA = 1 << 30, DR_CHILLB = 1 << 29,                     /* Chilled (section 5): its stacks, 1-3, in two bits; frost creeping
                                                                      over it, more with each */
       DR_BLEED = 1 << 28,                                           /* bleeding (D7, Barbed): dark drops running down it */
       /* the dead's track (D) ends */
};
_Static_assert(C4_EDGE == DR_EDGE, "C4's flash is DR_EDGE");
/* VENOM: fire's colours turned green (the flask's splash); POISONED: green bubbles rising off it; BURNING: flames licking up it */   /* ALLY: risen by your Grave Call, a violet edge; WEBBED: held in yours */                                         /* RIPPLE: the Wraith's Time Slip, grey, going out */   /* SHINE: polished plate, a glint
                                                                                                 sliding over it */
/* GLITCH: rows torn sideways and colours slipping, now and then (the Backrooms), more often when LOOPED; CLIP: cut at
 * the floor and ceiling; LIT: brighter than the light it stands in (Severance's creatures, under the panels) */
/* FLIPV: upside down, on the ceiling; COMMUNE: red through walls; GLINT: a key glinting through walls (Keymaker) */

/* The eyes (docs/InitialArtPass.md section 5). Every creature with eyes has them put on the screen again after its body,
 * after the fog, hidden only by walls (and by whatever stands nearer, drawn after it): never dimmed by the light or the fog,
 * never lost to a far-off sprite's sampling, at least a point each with a pixel between. The kind's own colour at rank I,
 * the rank's from II, and a form for each rank: II a glow beside, III a cold slit, IV a streak to the temple, V a white
 * flare, gilded a small gold cross. Alerted they gain a halo and stop blinking; striking they flare. The Shade has none. */
enum { EC_AMBER, EC_COLD, EC_SULPHUR, EC_EMBER, EC_WGOLD, EC_CRIMSON, EC_PALE, EC_ROT, EC_CHILL, EC_BLOOD, EC_HOT, EC_GILT, EC_ASH, EC_FIRE,
       EC_EL0, EC_EL_LAST = EC_EL0 + EL_PHYS - 1, NEC };          /* 1.0.0b19: born to an element (docs/PLAN-combat.md 7.4), by EL_*,
                                                                      in tones apart from the ranks' own (the rank keeps its form) */
static const uint8_t EYE_CORE[NEC] = { R_FLAME + 12, R_GLOW + 15, R_FLAME + 15, R_BLOOD + 15, C_WHITE, R_BLOOD + 15, C_WHITE,
                                       R_MOSS + 15, R_GLOW + 15, R_FLAME + 8, C_WHITE, R_GOLD + 12, R_FLAME + 5, R_FLAME + 15,
                                       R_FLAME + 15, C_WHITE, C_WHITE, R_GOLD + 11, R_PURPLE + 15 };   /* fire, frost (white in cyan, not the
                                                                      third rank's blue), lightning (white in gold), poison (bile over
                                                                      green), spirit */
static const uint8_t EYE_GLOW[NEC] = { R_FLAME + 7, R_GLOW + 10, R_MOSS + 13, R_BLOOD + 10, R_FLAME + 12, R_BLOOD + 11, R_GHOST + 11,
                                       R_MOSS + 11, R_GLOW + 11, R_BLOOD + 13, R_PURPLE + 14, R_GOLD + 8, R_FLAME + 1, R_FLAME + 9,
                                       R_FLAME + 10, R_GHOST + 11, R_GOLD + 12, R_MOSS + 10, R_PURPLE + 10 };
static const int8_t EYE_OWN[NKINDS] = {                            /* each kind's own, at rank I */
    [MOB_SKEL] = EC_AMBER, [MOB_GHOST] = EC_COLD, [MOB_BAT] = EC_EMBER, [MOB_WSKULL] = EC_AMBER, [MOB_SHIELD] = EC_AMBER,
    [MOB_SPIDER] = EC_EMBER, [MOB_SHAMBLER] = EC_SULPHUR, [MOB_DROWNED] = EC_COLD, [MOB_HOUND] = EC_AMBER, [MOB_WRAITH] = EC_COLD,
    [MOB_HUNTER] = EC_EMBER, [MOB_GHOUL] = EC_SULPHUR, [MOB_KNIGHT] = EC_CRIMSON, [MOB_BROOD] = EC_EMBER, [MOB_LING] = EC_EMBER,
    [MOB_SAC] = EC_EMBER, [MOB_SHADE] = EC_COLD, [MOB_IMP] = EC_WGOLD, [MOB_DEACON] = EC_PALE, [MOB_GARGOYLE] = EC_WGOLD,
    [MOB_WHIPPER] = EC_AMBER, [MOB_SEXTON] = EC_WGOLD };   /* the Lightless has none */
static const int8_t EYE_RANK[NRANK] = { -1, EC_ROT, EC_CHILL, EC_BLOOD, EC_HOT, EC_GILT };

