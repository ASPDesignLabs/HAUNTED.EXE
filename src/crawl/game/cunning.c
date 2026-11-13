/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Cunning by rank (dodges, cover, doorways held, feints), and the new attacks (the Bone Thrower, the charge, the spew, the grapple). */

/* ---- Cunning by rank (docs/PLAN-ai.md, sections 13-15 and 30; step 7): what update_mob asks of each rank ---- */
static int hz_soft(int x, int y);
static int hz_danger(int x, int y);
static int spell_charges(int sp);
static int hz_shuns(const Mob *m, int x, int y) {                 /* blue and up keep out of what warns or harms (1) and walk round
                                                                      what is always there (2); green and white blunder in */
    if (!minded(m) || cunning(m) < RK_III || hz_danger(m->cx, m->cy) || hz_soft(m->cx, m->cy)) return 0;   /* in it: any way out */
    return hz_danger(x, y) ? 1 : (hz_soft(x, y) ? 2 : 0);
}
static int open_step(const Mob *m, int x, int y) {
    const Cell *c = cell(x, y);
    return !blocks_move(x, y) && !occupied(x, y, m) && !(c->kind == K_DOOR && G.doors[c->door].open < 0.9f);
}
/* blue: a slow spell seen coming at it (the fireball, Hellfire, the flask, the orb, a spear from beyond five cells): a step
 * aside, across its way, or from where a flask will land; once in four seconds */
static int mind_dodge(Mob *m) {
    if (m->type == MOB_BAT || m->sub == 3 || m->sub == 4 || (m->type == MOB_HUNTER && m->sub < 2) || (m->type == MOB_GARGOYLE && m->sub < 2)) return 0;
    for (int i = 0; i < MAXSHOTS; i++) {
        const Shot *s = &G.shots[i];
        if (!s->alive || s->tier < 0) continue;
        int fl = s->kind == AT_FLASK;
        if (!(s->kind == AT_FIRE || s->kind == AT_HELL || fl || s->kind == SHOT_ORB
              || (s->kind == AT_SPEAR && hypotf(m->x - G.lay_x[i], m->y - G.lay_y[i]) > 5.0f))) continue;
        float v2 = s->vx * s->vx + s->vy * s->vy, ax, ay;
        if (v2 < 1e-4f) continue;
        if (fl) {                                                  /* from where it will come down */
            float lx = s->x + s->vx * s->life, ly = s->y + s->vy * s->life;
            if (s->life < 0.2f || hypotf(m->x - lx, m->y - ly) > 1.6f) continue;
            ax = m->x - lx; ay = m->y - ly;
            if (ax * ax + ay * ay < 1e-4f) { ax = -s->vy; ay = s->vx; }
        } else {                                                   /* across its way, to the side it is on already */
            float rx = m->x - s->x, ry = m->y - s->y, tc = (rx * s->vx + ry * s->vy) / v2;
            if (tc < 0.15f || tc > 0.8f) continue;
            float mx = s->x + s->vx * tc - m->x, my = s->y + s->vy * tc - m->y;
            if (hypotf(mx, my) > KINDS[m->type].rad + 0.45f) continue;
            ax = -s->vy; ay = s->vx;
            if (ax * -mx + ay * -my < 0.0f) { ax = -ax; ay = -ay; }
        }
        if (!in_view(m, s->x, s->y) || !line_clear(m->x, m->y, s->x, s->y)) continue;
        int bx = (int)m->x, by = (int)m->y, best = -1;
        float bs = 0.3f, al = hypotf(ax, ay);
        for (int k = 0; k < 4; k++) {
            int nx = bx + DX[k], ny = by + DY[k];
            if (!open_step(m, nx, ny) || hz_shuns(m, nx, ny) == 1) continue;
            float sc = ((float)DX[k] * ax + (float)DY[k] * ay) / al;
            if (sc > bs) { bs = sc; best = k; }
        }
        if (best < 0) continue;
        m->cx = bx; m->cy = by; m->tx = bx + DX[best]; m->ty = by + DY[best];
        m->dodge_t = 0.5f; m->dodge_cd = 4.0f;
        DBG_COUNT(dodged);
        return 1;
    }
    return 0;
}
/* green: alone and hurt to a quarter (none of its pack within three cells), it falls back to the nearest of them, once */
static Mob *fall_mate(const Mob *m) {
    const Pack *p = pack_of(m);
    if (!p || p->broken) return NULL;
    Mob *best = NULL;
    float bd = 1e9f;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (o == m || !(p->mem >> i & 1) || !o->alive || o->state != MS_WALK || o->sleep || buried(o) || o->fell > 0) continue;
        float d = hypotf(o->x - m->x, o->y - m->y);
        if (d < 3.0f) return NULL;
        if (d < bd) { bd = d; best = o; }
    }
    return best;
}
/* green's throwers: between throws, a step behind a corner if one is a step away */
static int cover_near(Mob *m) {
    int bx = (int)m->x, by = (int)m->y;
    if ((float)G.level_t - m->cov_t > 15.0f) m->cov_n = 0;          /* three covers running, then it stays out a while: no endless */
    if (m->cov_n >= 3) return 0;                                   /* peeping, which held the dream in a corner for minutes */
    if (!line_clear(G.px, G.py, bx + 0.5f, by + 0.5f)) return 0;  /* out of your sight already */
    for (int k = 0; k < 4; k++) {
        int nx = bx + DX[k], ny = by + DY[k];
        if (!open_step(m, nx, ny) || cell(nx, ny)->kind == K_DOOR || hz_shuns(m, nx, ny)) continue;
        if (line_clear(G.px, G.py, nx + 0.5f, ny + 0.5f)) continue;
        m->cvx = nx; m->cvy = ny; m->cov_n++; m->cov_t = (float)G.level_t;
        return 1;
    }
    return 0;
}
/* the throw over a corner (section 30): where it saw you last, within six cells and three seconds, and a little on the way you
 * went while it can see there; the place it comes down must be open floor */
static int corner_aim(const Mob *m, float *ox, float *oy) {
    if (!m->seen || m->lst <= 0.0f || (float)G.level_t - m->lst > 3.0f || hypotf(m->lsx - m->x, m->lsy - m->y) > 6.0f) return 0;
    float x = m->lsx, y = m->lsy, hx = cosf(m->kh), hy = sinf(m->kh);
    for (int k = 1; k <= 3; k++) {
        float nx = m->lsx + hx * 0.5f * (float)k, ny = m->lsy + hy * 0.5f * (float)k;
        if (blocks_move((int)nx, (int)ny) || !line_clear(m->x, m->y, nx, ny)) break;
        x = nx; y = ny;
    }
    if (!line_clear(m->x, m->y, x, y) || cell((int)x, (int)y)->kind != K_FLOOR || hypotf(x - m->x, y - m->y) < 1.5f) return 0;
    *ox = x; *oy = y;
    return 1;
}
/* blue: a doorway (a door, or a gap a cell wide where one place gives onto another) that you are holding: you stand
 * looking at it within seven cells, not on the move, with a spell ready */
static int doorway(int cx, int cy, int nx, int ny) {
    const Cell *c = cell(nx, ny);
    if (c->kind == K_DOOR) return 1;
    int dx = nx - cx, dy = ny - cy;
    if (!blocks_move(nx + dy, ny + dx) || !blocks_move(nx - dy, ny - dx)) return 0;
    int r0 = cell(cx, cy)->region, r2 = cell(nx + dx, ny + dy)->region;
    return r0 != c->region || c->region != r2;
}
static int held_door(int nx, int ny) {
    if (G.dead || no_cast() || G.wwalk_t > 0.0f || hypotf(G.pvx, G.pvy) > 1.2f) return 0;
    if (G.player && (G.cooldown > 0.35f || G.pc_t >= 0.0f)) return 0;   /* a person mid-cast: nothing ready yet */
    float dx = nx + 0.5f - G.px, dy = ny + 0.5f - G.py, d = hypotf(dx, dy);
    if (d < 1.6f || d > 7.0f || fabsf(wrap_pi(atan2f(dy, dx) - G.ang)) > 0.45f) return 0;
    return line_clear(G.px, G.py, nx + 0.5f, ny + 0.5f);
}
static int door_held(const Mob *m, int nx, int ny) {
    return minded(m) && cunning(m) >= RK_III && m->door_t >= 0.0f && doorway(m->cx, m->cy, nx, ny) && held_door(nx, ny);
}
static int door_wait(Mob *m, int nx, int ny) {                     /* 1: it waits beside it, out of your sight; eight seconds by it at most */
    if (!door_held(m, nx, ny)) return 0;
    if (!m->dwx) DBG_COUNT(door_waits);
    m->dwx = nx; m->dwy = ny;
    m->alert = fmaxf(m->alert, 1.0f);                              /* it knows where you are: just there */
    if (line_clear(G.px, G.py, m->cx + 0.5f, m->cy + 0.5f))
        for (int k = 0; k < 4; k++) {
            int ax = m->cx + DX[k], ay = m->cy + DY[k];
            if ((ax == nx && ay == ny) || !open_step(m, ax, ay) || cell(ax, ay)->kind == K_DOOR || hz_shuns(m, ax, ay)) continue;
            if (!line_clear(G.px, G.py, ax + 0.5f, ay + 0.5f)) { m->tx = ax; m->ty = ay; break; }
        }
    return 1;
}
static int door_beside(Mob *m) {                                   /* stood aside, out of your sight: it waits while you hold it */
    if (!m->dwx || m->door_t < 0.0f || !held_door(m->dwx, m->dwy) || abs(m->dwx - m->cx) + abs(m->dwy - m->cy) > 3
        || line_clear(G.px, G.py, m->cx + 0.5f, m->cy + 0.5f)) return 0;
    m->alert = fmaxf(m->alert, 1.0f);
    return 1;
}
/* red: against a splash in hand (Hellfire, the flask, Shatter, Arc's leaps), no two within a cell and a half */
static int splash_in_hand(void) {
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SPLASH")) return 1;                          /* a test: as if one were always in hand */
#endif
    return G.spell == AT_HELL || G.spell == AT_FLASK || (G.spell == AT_FROST && align_has(EL_FROST, ND_BULT)) || (G.spell == AT_CHAIN && align_has(EL_LIGHT, ND_AULT));
}
static int crowded(const Mob *m, int x, int y) {
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *o = &G.mobs[i];
        if (o == m || !o->alive || o->state != MS_WALK || o->ally || buried(o) || o->sleep || KIND_IS(o, KF_SPIRIT)) continue;
        if (hypotf(o->x - (x + 0.5f), o->y - (y + 0.5f)) < 1.5f) return 1;
    }
    return 0;
}
static int spreads(const Mob *m) { return minded(m) && cunning(m) >= RK_IV && splash_in_hand() && m->alert > 0.0f; }
/* black: a feint, one wind-up in three (never two running): it holds the blow, and the real one comes close behind */
static void feint_roll(Mob *m) {
    if (m->feint == 2) { m->feint = 0; return; }
    if (minded(m) && cunning(m) >= RK_V && rnd(3) == 0) { m->feint = 1; DBG_COUNT(feints); }
}
static int feint_held(Mob *m) {
    if (m->feint != 1) return 0;
    m->feint = 2; m->cool = 0.35f;
    return 1;
}
/* ---- C4 (docs/PLAN-combat.md, section 8; track C): new attacks, their sheet approved (CRAWL_C4=0 in the debug build takes them
 * out; CRAWL_C4SOON=1 every few seconds, for frames). The Bone Thrower: a skeleton of
 * rank II up flashes 0.6 s, then lobs three bones half a second apart over twenty degrees. The shield-bearer's charge: a stamp
 * and the shield up 0.5 s, then up to five cells along a clear lane; it knocks you down 1.2 s, unless it is dodged, the Bone Ward
 * takes it all, or your armour is twice its harm. The Drowned's spew: swelling 0.5 s, then a gout two cells in a thirty-degree
 * cone, eight tenths of its blow and Poisoned. Stunned or Unbound, none of them begins (E3, E5) ---- */
enum { C4_NONE, C4_FLASH, C4_VOLLEY, C4_STAMP, C4_RUSH, C4_REST, C4_SWELL, C4_SPEW };
#define C4_EDGE 512                                                /* DR_EDGE, which is declared further on (checked there) */
static int c4_on(void) {                                           /* in play (the owner's word on its sheet); CRAWL_C4=0 or CRAWL_AF=0 takes them out */
#ifdef CRAWL_DEBUG
    static int v = -1;
    if (v < 0) v = getenv("CRAWL_C4") && !atoi(getenv("CRAWL_C4")) ? 0 : 1;
    return v && !af_off();
#else
    return 1;
#endif
}
static float c4_wait(int fa) {                                     /* the wait before the next (-1: the first, a while after it could) */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_C4SOON")) return fa < 0 ? 0.3f : 2.5f;
#endif
    return fa < 0 ? 2.0f + 5.0f * rndf() : ATTACKS[fa].cool + ATTACKS[fa].cool_rnd * rndf();
}
static int c4_ready(Mob *m, float dt) {                            /* its wait run down; the first set a while after it first could */
    if (!c4_on()) return 0;                                        /* off: nothing drawn from the dice */
    if (m->c4_cd == 0.0f) { m->c4_cd = c4_wait(-1); return 0; }
    if (m->c4_cd > 0.0f) m->c4_cd -= dt;
    return m->c4_cd <= 0.0f && !mob_stunned(m) && !mob_unbound(m) && !m->boss && !m->add && !m->ally && c4_on() && !G.dead;
}
static void c4_stop(Mob *m, int fa) { m->c4 = C4_NONE; m->c4_t = 0.0f; m->c_flags = 0; m->c4_cd = fmaxf(0.01f, c4_wait(fa)); }
static int c4_thrower(Mob *m, float dt, int los, float pd) {
    if (m->c4 == C4_NONE) {
        if (!c4_ready(m, dt) || m->rank < RK_II || m->rank == RK_GILDED || !m->seen || !los || m->throw_t > 0.0f || m->cover == 1
            || pd < ATTACKS[FA_VOLLEY].go || pd > ATTACKS[FA_VOLLEY].go2) return 0;
        m->c4 = C4_FLASH; m->c4_t = 0.0f; m->c_flags = C4_EDGE; m->c4n = 0;
        elem_arm(m, ATTACKS[FA_VOLLEY].wind + 1.1f);               /* its element, if it carries one this time, on the first of them */
        sfx_at(SFX_RATTLE, m->x, m->y, 1.2f); DBG_COUNT(c4_volleys);
    }
    if (m->c4 != C4_FLASH && m->c4 != C4_VOLLEY) return 0;
    if (mob_stunned(m) || mob_unbound(m) || G.dead) { c4_stop(m, FA_VOLLEY); m->throw_t = 0.0f; return 0; }
    m->c4_t += dt;
    m->face = atan2f(G.py - m->y, G.px - m->x);
    if (m->c4 == C4_FLASH) {
        m->c_flags = ((int)(m->c4_t * 12.0f) & 1) ? C4_EDGE : 0;   /* the edge flashing */
        if (m->c4_t >= ATTACKS[FA_VOLLEY].wind) { m->c4 = C4_VOLLEY; m->c4_t = 0.5f; m->c_flags = 0; }
    }
    if (m->c4 == C4_VOLLEY && m->c4_t >= 0.5f) {
        static const float OFF[3] = { 0.0f, -0.1745f, 0.1745f };    /* where you stand, then ten degrees either side */
        m->c4_t -= 0.5f;
        float a = atan2f(G.py - m->y, G.px - m->x) + OFF[m->c4n], d = hypotf(G.px - m->x, G.py - m->y);
        throw_to(m, MIS_BONE, m->x + cosf(a) * d, m->y + sinf(a) * d, 1);
        if (++m->c4n >= 3) { c4_stop(m, FA_VOLLEY); m->cool = fa_every(FA_BONE); m->throw_t = 0.0f; return 1; }
    }
    m->throw_t = 0.1f;                                             /* the throwing pose, held */
    return 1;
}
static float c4_lane(const Mob *m, float a, float len) {           /* how far a straight run goes before something stops it */
    float c = cosf(a), s = sinf(a);
    for (float t = 0.25f; t <= len; t += 0.25f) {
        float x = m->x + c * t, y = m->y + s * t;
        int cx = (int)x, cy = (int)y;
        if (blocks_move(cx, cy) || G.map[cy][cx].kind == K_DOOR) return t - 0.25f;
        if ((cx != m->cx || cy != m->cy) && occupied(cx, cy, m)) return t - 0.25f;
    }
    return len;
}
static int c4_charge(Mob *m, float dt, int los, float pd) {
    if (m->c4 == C4_NONE) {
        if (!c4_ready(m, dt) || m->shield <= 0.0f || !los || !m->seen || m->swing > 0.0f || pd < ATTACKS[FA_CHARGE].go || pd > ATTACKS[FA_CHARGE].go2) return 0;
        float a = atan2f(G.py - m->y, G.px - m->x);
        if (c4_lane(m, a, pd - 0.6f) < pd - 0.6f) return 0;        /* not along a clear lane */
        m->c4 = C4_STAMP; m->c4_t = 0.0f; m->c4a = a; m->c4x = m->x; m->c4y = m->y;
        sfx_at(SFX_STEP, m->x, m->y, 1.6f); sfx_at(SFX_DRUM, m->x, m->y, 0.6f); DBG_COUNT(c4_charges);
    }
    if (m->c4 != C4_STAMP && m->c4 != C4_RUSH && m->c4 != C4_REST) return 0;
    m->c4_t += dt;
    if (m->c4 == C4_STAMP) {
        m->face = m->c4a;
        if (mob_stunned(m) || mob_unbound(m)) { c4_stop(m, FA_CHARGE); return 0; }
        if (m->c4_t >= ATTACKS[FA_CHARGE].wind) { m->c4 = C4_RUSH; m->c4_t = 0.0f; sfx_at(SFX_WHOOSH, m->x, m->y, 1.0f); }
        return 1;
    }
    if (m->c4 == C4_RUSH) {
        float step = 7.0f * dt, ran = hypotf(m->x - m->c4x, m->y - m->c4y);
        float go = fminf(step, c4_lane(m, m->c4a, step + 0.25f));
        m->x += cosf(m->c4a) * go; m->y += sinf(m->c4a) * go;
        m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
        int stop = go < step * 0.5f || ran + go >= 5.0f;
        if (!G.dead && hypotf(G.px - m->x, G.py - m->y) < ATTACKS[FA_CHARGE].reach + BODY) {   /* it strikes you */
            float raw = mob_damage(m) * ATTACKS[FA_CHARGE].k;
            int d = foe_hit(m, FA_CHARGE, 1.0f);
            sfx_at(SFX_SWORD, m->x, m->y, 1.4f); DBG_COUNT(c4_charge_hits);
            if (d > 0 && (float)gear_armor(&RUN.gear) < 2.0f * raw && af_can(AF_DOWN)) { af_hold(AF_DOWN, 1.2f, 1 + (int)(m - G.mobs)); DBG_COUNT(c4_downs); }   /* M4 */
            if (d > 0) thorns(m);
            stop = 1;
        }
        if (stop) { m->c4 = C4_REST; m->c4_t = 0.0f; }
        return 1;
    }
    if (m->c4_t >= 0.6f) { c4_stop(m, FA_CHARGE); m->cool = 0.5f; }   /* getting its feet under it */
    return 1;
}
static int c4_spew(Mob *m, float dt, int sees, float pd) {
    if (m->c4 == C4_NONE) {
        if (!c4_ready(m, dt) || !sees || m->swing > 0.0f || pd < ATTACKS[FA_SPEW].go || pd > ATTACKS[FA_SPEW].go2) return 0;
        m->c4 = C4_SWELL; m->c4_t = 0.0f; m->c4n = 0;
        sfx_at(SFX_AF_SICK, m->x, m->y, 1.0f); sfx_at(SFX_GROAN, m->x, m->y, 0.6f); DBG_COUNT(c4_spews);
    }
    if (m->c4 != C4_SWELL && m->c4 != C4_SPEW) return 0;
    m->c4_t += dt;
    if (m->c4 == C4_SWELL) {
        m->face = atan2f(G.py - m->y, G.px - m->x);
        if (mob_stunned(m)) { c4_stop(m, FA_SPEW); return 0; }
        if (m->c4_t >= ATTACKS[FA_SPEW].wind) { m->c4 = C4_SPEW; m->c4_t = 0.0f; m->c4a = m->face; sfx_at(SFX_SPLASH, m->x, m->y, 1.6f); sfx_at(SFX_HISS, m->x, m->y, 0.8f); }
        return 1;
    }
    if (!m->c4n && m->c4_t >= 0.15f) {                             /* the gout arrives */
        m->c4n = 1;
        float d = hypotf(G.px - m->x, G.py - m->y), off = fabsf(wrap_pi(atan2f(G.py - m->y, G.px - m->x) - m->c4a));
        if (!G.dead && d < ATTACKS[FA_SPEW].reach && off < 0.2618f && line_clear(m->x, m->y, G.px, G.py)) {
            harm_next = EL_POISON;
            int h = hurt_by(m, mob_damage(m) * ATTACKS[FA_SPEW].k, HURT_SPELL);
            if (h > 0) { af_poison(6.0f * (1.0f - gear_poison_resist(&RUN.gear))); DBG_COUNT(c4_spew_hits); }
        }
    }
    if (m->c4_t >= 0.45f) { c4_stop(m, FA_SPEW); m->cool = fmaxf(m->cool, 1.0f); }
    return 1;
}
/* the dream's counters (8.6): a Bone Thrower flashing in sight, its bones about to come, counts as a missile on its way (the
 * runes and Swift Dodge answer it); the charge's lane while it stamps and runs, and the spew's cone while the Drowned swells and
 * spews, are warnings on the dream's map, so it steps off them, or dodges if it can */
static void hz_dput(int x, int y, int c);
static int c4_tells(float within) {
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK) continue;
        if ((m->c4 == C4_FLASH && m->c4_t > ATTACKS[FA_VOLLEY].wind - within) || (m->c4 == C4_STAMP && m->c4_t > ATTACKS[FA_CHARGE].wind - within))
            if (line_clear(m->x, m->y, G.px, G.py)) n++;
    }
    return n;
}
static void c4_dmarks(void) {
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK) continue;
        if (m->c4 == C4_STAMP || m->c4 == C4_RUSH) {
            float c = cosf(m->c4a), s = sinf(m->c4a);
            for (float t = 0.0f; t <= 5.2f; t += 0.3f) {
                int x = (int)(m->c4x + c * t), y = (int)(m->c4y + s * t);
                if (x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1 || blocks_move(x, y)) break;
                hz_dput(x, y, 1);
            }
        } else if (m->c4 == C4_SWELL || m->c4 == C4_SPEW) {
            float a = m->c4 == C4_SPEW ? m->c4a : m->face;
            for (int y = imax(1, (int)m->y - 3); y <= imin(MAPN - 2, (int)m->y + 3); y++)
                for (int x = imax(1, (int)m->x - 3); x <= imin(MAPN - 2, (int)m->x + 3); x++) {
                    float dx = x + 0.5f - m->x, dy = y + 0.5f - m->y, d = hypotf(dx, dy);
                    if (d < 2.6f && (d < 0.9f || fabsf(wrap_pi(atan2f(dy, dx) - a)) < 0.45f) && !blocks_move(x, y)) hz_dput(x, y, 1);
                }
        }
    }
}
static void c4_grapple(Mob *m) {                                   /* a shambler's grip with no element in it: held fast (8.3, M8) */
    if (!c4_on() || m->type != MOB_SHAMBLER || foe_el != EL_PHYS || !af_can(AF_DOWN) || rndf() >= af_rank_chance(m->rank) * 2.5f) return;
    af_hold(AF_DOWN, 1.2f, 1 + (int)(m - G.mobs));
    G.af_shake = 1.0f; G.af_k = 0.0f;                              /* no fall: the view stays up and the hand shoves at it */
    DBG_COUNT(c4_grapples);
}

/* black: a charged ultimate held: out of its reach until you let go */
static int charge_read(const Mob *m, float pd) {
    return G.chg_on && G.chg_t > 0.25f && spell_charges(G.chg_sp) && minded(m) && cunning(m) >= RK_V && pd < 6.0f;
}

