/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * update_mob: one creature's step, every kind's branch. */

static void update_mob(Mob *m, float dt) {
    if (!m->wset && m->state == MS_WALK) ward_give(m);            /* its rank's strengths (1.0.0b19; warded, 1.0.0b16) */
    dt = mob_gate(m, dt);                                          /* its statuses (docs/PLAN-combat.md section 5, 1.0.0b19) */
#ifdef CRAWL_DEBUG
    if (g_stage >= 0 && m->state == MS_WALK) { m->t += dt; m->anim += dt * 1.5f; m->sub_t += dt; if (m->wring > 0.0f) m->wring -= dt; return; }   /* stood still for frames */
#endif
    if (m->state == MS_DIE && m->sub == 77) { if ((m->t += dt) >= 0.9f) m->alive = 0; return; }   /* gone into the BFS9000 */
    if (m->ally && m->state == MS_WALK) { update_ally(m, dt); return; }   /* raised by your Grave Call: it fights for you */
    elem_tick(m, dt);                                              /* an attack's element held through its wind-up (C3) */
    if (m->wring > 0.0f) m->wring -= dt;
    if (G.player && m->ward[0] && !elem_mode() && m->state == MS_WALK && !(G.ward_said >> (m->ward[0] - 1) & 1) && hypotf(m->x - G.px, m->y - G.py) < 6.0f
        && line_clear(G.px, G.py, m->x, m->y)) { G.ward_said |= 1 << (m->ward[0] - 1); say("WARDED AGAINST %s", ELEM_NAME[m->ward[0] - 1]); }   /* the first
                                                                      seen on a level, named */
    if (m->boss && m->state == MS_WALK && (G.bstate != BS_FIGHT || G.bpose_t > 0.0f)) {   /* waiting, holding while its wave
                                                                                            fights, or casting */
        m->t += dt;
        if (G.bstate != BS_WAIT) m->face += wrap_pi(atan2f(G.py - m->y, G.px - m->x) - m->face) * fminf(1.0f, dt * 2.0f);
        return;
    }
    if (G.ptheme == PT_SEVERANCE && m->cool > 0.0f) m->cool -= dt * 0.25f;   /* Severance's strike and throw a fifth sooner */
    if (m->type == MOB_GHOST) { update_ghost(m, dt); return; }
    if (m->type == MOB_WSKULL) { update_wskull(m, dt); return; }
    if (m->type == MOB_SPIDER) { update_spider(m, dt); return; }
    if (m->type == MOB_WRAITH) { if (m->boss) wraith_boss(m, dt); else update_wraith(m, dt); return; }
    if (m->type == MOB_SAC) { update_sac(m, dt); return; }
    if (update_riser(m, dt)) return;
    if (m->sleep && m->state == MS_WALK) { roost_doze(m, dt); return; }   /* a roost on the ceiling */
    if (m->amb && !m->bait_st && m->state == MS_WALK && m->amb <= G.namb && !G.amb[m->amb - 1].sprung) {   /* a tooth, waiting */
        m->t += dt; m->face = m->pf;
        if (m->type == MOB_HUNTER) m->z = 1.0f - KINDS[MOB_HUNTER].scale * RANK_SIZE[m->rank];
        if (m->type == MOB_GARGOYLE) m->z = 0.0f;
        return;
    }
    m->t += dt;
    if (m->state == MS_DIE) {
        float dur = m->type == MOB_BAT ? 0.9f : 0.75f;
        if (m->type == MOB_BAT || m->type == MOB_HUNTER || m->type == MOB_GARGOYLE) m->z = fmaxf(0.0f, m->z - dt * (m->z > 0.3f ? 1.6f : 0.6f));   /* down it falls */
        if (m->t >= dur) {
            int di = G.ndecos;
            if (KINDS[m->type].remains >= 0) add_deco(m->x, m->y, KINDS[m->type].remains);
            if (G.ndecos > di && !m->ally && !m->boss) note_remains(m, di);   /* for Grave Call */
            m->alive = 0;
        }
        return;
    }
    if (m->type == MOB_SKEL && m->sub == 5) { if ((m->sub_t += dt) >= 0.7f) m->sub = 0; return; }   /* called up, it gets to its feet */
    dt = mob_time(m, dt);
    if (dt <= 0.0f) return;
    if (m->webbed > 0.0f) { web_claw(m, dt); return; }            /* held in your web: it claws at that, not at you */
    m->anim += dt * m->speed * 3.0f;
    m->z = m->type == MOB_BAT ? 0.20f + 0.07f * sinf(m->t * 7.0f) : 0.0f;
    float pdx0 = G.px - m->x, pdy0 = G.py - m->y, pd0 = sqrtf(pdx0 * pdx0 + pdy0 * pdy0);
    int undead = KIND_IS(m, KF_UNDEAD) != 0, mind = minded(m);
    int sees = mind ? mind_sees(m, pd0) : !G.dead && G.wwalk_t <= 0.0f && pd0 < 9.0f && line_clear(m->x, m->y, G.px, G.py);   /* wraithwalking, you are not there */
    if (mind) {                                                    /* how the dead think (docs/PLAN-ai.md, Part A) */
        if (sees && G.level_t > 1.5 && m->lost > 1.0f && m->type == MOB_SKEL) m->cool = fmaxf(m->cool, 0.6f * throw_interval());
        if (sees) { m->lsx = G.px; m->lsy = G.py; m->lst = (float)G.level_t; }   /* where it saw you last (step 7) */
        if (m->door_t < 0.0f) m->door_t = fminf(0.0f, m->door_t + dt);
        else if (m->dwx) {                                         /* by a doorway you held: its patience runs, open or shut */
            if (m->alert > 0.0f && abs(m->dwx - m->cx) + abs(m->dwy - m->cy) <= 3) { if ((m->door_t += dt) > 8.0f) { m->door_t = -6.0f; m->dwx = 0; } }
            else m->dwx = 0;
        } else m->door_t = fmaxf(0.0f, m->door_t - 0.2f * dt);
        if (m->dodge_cd > 0.0f) m->dodge_cd -= dt;
        mind_tick(m, dt, sees);
        mind_face(m, dt, sees, m->tx != m->cx || m->ty != m->cy, m->tx + 0.5f - m->x, m->ty + 0.5f - m->y);
        if (m->bait_st == B_LURE) m->face = atan2f(G.py - m->y, G.px - m->x);   /* a bait backs away, looking back */
    } else if (sees && G.level_t > 1.5) {                          /* it has you: it will come after you a while */
        m->alert = m->type == MOB_SKEL ? 7.0f : 12.0f;            /* a thrower gives up sooner */
        if (m->lost > 1.0f && m->type == MOB_SKEL) m->cool = fmaxf(m->cool, 0.6f * throw_interval());   /* finding its range again */
        m->lost = 0.0f;
    } else {
        if (m->alert > 0.0f) m->alert -= dt;
        m->lost += dt;
    }
    if (m->feared > 0.0f) m->feared -= dt; else m->dreaded = 0;
    if (m->flee > 0.0f && (m->flee -= dt) <= 0.0f) {               /* a broken roost's bat, scattered: a wanderer now */
        m->flee = 0.0f; m->alert = 0.0f; m->aware = MA_CALM; m->pack = 0; m->home = -1; m->role = RL_ROAM; roam_next(m);
    }
    if (m->stagger > 0.0f) { m->stagger -= dt; return; }
    if (m->dodge_t > 0.0f) {                                       /* stepping aside from a spell (step 7) */
        float gx, gy; cell_pass(m->tx, m->ty, &gx, &gy);
        float dx = gx - m->x, dy = gy - m->y, d = hypotf(dx, dy), st = 3.4f * dt;
        m->dodge_t -= dt;
        if (d > st && m->dodge_t > 0.0f) { m->x += dx / d * st; m->y += dy / d * st; return; }
        if (d <= st) { m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty; }
        else { m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y; }
        m->dodge_t = 0.0f;
        return;
    }
    if (mind && !G.dead && cunning(m) >= RK_III && m->dodge_cd <= 0.0f && m->swing <= 0.0f && m->throw_t <= 0.0f && m->alert > 0.0f && mind_dodge(m)) return;
    int keeps_off = (m->type == MOB_IMP || m->type == MOB_DEACON || m->type == MOB_WHIPPER || m->type == MOB_SEXTON) && !m->boss && !m->add;   /* they fight from further off, and back
                                                                                    away (not in a boss's fight: they hold the floor) */
    int cowed = m->feared > 0.0f && (undead || m->dreaded);       /* Dread */
    int lured = !cowed && G.decoy_t > 0.0f && hypotf(m->x - G.decoy_x, m->y - G.decoy_y) < 6.0f;   /* Blink's double draws it */
    float lure_x = G.decoy_x, lure_y = G.decoy_y;
    Mob *foe = lured || cowed || m->boss ? NULL : ally_near(m, 6.0f);   /* and so does one you have raised, to fight it */
    if (foe) { lured = 1; lure_x = foe->x; lure_y = foe->y; }
    else if (m->lure && !cowed) { lured = 1; lure_x = m->lx; lure_y = m->ly; }   /* a trial's: after its things */
    int hold = mind ? m->hold : 0;                                 /* holding off: the fight is full, or it waits its pack's turn */
    if (mind && m->type == MOB_SHADE && !G.dead) {                 /* the shade stalks (section 11): four to six cells behind, */
        if (m->shy > 0.0f) m->shy -= dt;
        if (m->alert > 0.0f && m->swing <= 0.0f) {
            if (!hold && !shade_may_strike(m)) hold = 3;           /* till you fight something else or stand still */
            if (!shade_hidden(m) && pd0 < 9.0f && m->shy <= 0.0f && fabsf(wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang)) < 0.6f) {
                m->shy = 1.5f + rndf(); DBG_COUNT(shade_fades);    /* lit and looked at, it fades back */
            }
        }
    }
    int falling = 0;                                               /* green: alone and hurt, back to its pack (step 7) */
    const Mob *fall_to = NULL;
    if (mind && m->fell > 0) {
        const Mob *o = &G.mobs[m->fell - 1];
        if (!o->alive || o->state != MS_WALK || hypotf(o->x - m->x, o->y - m->y) < 2.0f || (m->fall_t += dt) > 10.0f || pd0 < 1.4f) m->fell = -1;
        else { falling = 1; fall_to = o; }
    } else if (mind && !m->fell && m->pack && m->alert > 0.0f && m->hp < 0.25f * m->maxhp && cunning(m) >= RK_II && m->swing <= 0.0f && m->throw_t <= 0.0f) {
        const Mob *o = fall_mate(m);
        if (o) { m->fell = 1 + (int)(o - G.mobs); m->fall_t = 0.0f; m->engage = 0; falling = 1; fall_to = o; DBG_COUNT(fell_back); }
    }
    if (mind && !falling && m->pack && m->swing <= 0.0f && m->throw_t <= 0.0f) {   /* the brood's guard (step 8): never far from her, and */
        const Pack *bp = pack_of(m);                               /* at her side once she is hurt */
        const Mob *L = bp && bp->kind == PACK_BAND && !bp->broken && bp->leader >= 0 && bp->leader != (int)(m - G.mobs) ? &G.mobs[bp->leader] : NULL;
        if (L && L->type == MOB_BROOD && L->alive && L->state == MS_WALK && pd0 > 1.4f
            && hypotf(L->x - m->x, L->y - m->y) > (L->hp < 0.75f * L->maxhp ? 2.5f : 6.0f)) {
            static double leash_said;
            if (G.t - leash_said > 3.0) { leash_said = G.t; DBG_COUNT(leashed); }
            falling = 1; fall_to = L;
        }
    }
    if (mind && !hold && !falling && charge_read(m, pd0)) {          /* black: out of a held charge's reach */
        static double read_at = -1.0;
        if (G.t - read_at > 2.0) DBG_COUNT(charge_reads);
        read_at = G.t;
        hold = 4;
    }
    int fights = !G.dead && !cowed && !lured && G.wwalk_t <= 0.0f && !hold && m->flee <= 0.0f && !falling
              && !(m->bait_st >= B_WAIT && m->bait_st <= B_LURE);   /* it may go for the caster now (a bait only leads) */
    int afraid = cowed || (keeps_off && sees && pd0 < 2.8f) || m->flee > 0.0f || m->shy > 0.0f;
    if (foe && hypotf(foe->x - m->x, foe->y - m->y) < 1.35f) {    /* at it: blows, as it would give you */
        m->face = atan2f(foe->y - m->y, foe->x - m->x);
        if ((m->acool -= dt) <= 0.0f) {
            float every, b = mob_blow(m, &every);
            m->acool = every;
            sfx_at(m->type == MOB_SHIELD || m->type == MOB_KNIGHT ? SFX_SWORD : SFX_BONES, m->x, m->y, 0.7f);
            ally_hurt(foe, b);
        }
        return;
    }
    if (m->type == MOB_SKEL && fights && !afraid) {               /* once it has seen you, it keeps throwing */
        int los = sees && G.level_t > 1.5;
        float lx, ly;
        if (los && !m->seen) { m->seen = 1; m->cool = (0.45f + 0.35f * rndf()) * throw_interval() / 1.5f; }
        if (c4_thrower(m, dt, los, pd0)) return;                    /* C4: the Bone Thrower */
        if (m->throw_t > 0.0f) {
            m->throw_t -= dt;
            if (m->throw_t <= 0.0f) {
                if (feint_held(m)) return;                         /* black: the arm goes back, and no bone */
                if (!los && mind && cunning(m) >= RK_II && corner_aim(m, &lx, &ly)) { throw_to(m, MIS_BONE, lx, ly, 1); DBG_COUNT(corner_throws); }
                else throw_bone(m);
                m->cool = fa_every(FA_BONE) * (1.0f + (rndf() - 0.5f) * 0.27f); DBG_COUNT(throws);
                if (mind && cunning(m) >= RK_II && m->cool > 0.5f && cover_near(m)) { m->cover = 1; DBG_COUNT(covers); }   /* green: behind
                                                                      the corner a step off, out again for the next */
            }
            return;                                                /* stands still to throw */
        }
        if (m->seen && los && m->cover != 1 && (m->cool -= dt) <= 0.0f) { m->throw_t = ATTACKS[FA_BONE].wind; feint_roll(m); elem_arm(m, m->throw_t); return; }
        if (m->seen && !los && !m->cover && mind && cunning(m) >= RK_II && (m->cool -= dt) <= 0.0f && corner_aim(m, &lx, &ly)) {   /* over the
                                                                      corner, to where it saw you go */
            m->throw_t = ATTACKS[FA_BONE].wind; return;
        }
    }
    if (m->type == MOB_SHIELD && fights && !afraid) {             /* closes in behind its shield and strikes */
        int los = sees && G.level_t > 1.5;
        if (los) { m->seen = 1; m->face += wrap_pi(atan2f(pdy0, pdx0) - m->face) * fminf(1.0f, dt * 6.0f); }
        if (c4_charge(m, dt, los, pd0)) return;                     /* C4: the charge */
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f;
                if (feint_held(m)) return;
                m->cool = ATTACKS[FA_SWORD].cool;
                sfx_at(SFX_SWORD, m->x, m->y, 1.2f);
                if (pd0 < ATTACKS[FA_SWORD].reach && foe_hit(m, FA_SWORD, 1.0f) > 0) { thorns(m); DBG_COUNT(hits_sword); }
            }
            return;
        }
        if (los && pd0 < ATTACKS[FA_SWORD].go && m->cool <= 0.0f) { m->swing = ATTACKS[FA_SWORD].wind; feint_roll(m); elem_arm(m, m->swing); return; }
    }
    if ((m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) && fights && !afraid) {   /* it lurches in and takes hold */
        if (m->type == MOB_DROWNED && c4_spew(m, dt, sees, pd0)) return;   /* C4: the spew */
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f;
                if (feint_held(m)) return;
                const FoeAtk *ga = &ATTACKS[m->type == MOB_DROWNED ? FA_DGRAB : FA_GRAB];
                m->cool = ga->cool;
                if (pd0 < ga->reach && foe_hit(m, (int)(ga - ATTACKS), 1.0f) > 0) {
                    G.slow_t = fmaxf(G.slow_t, ga->slow * (1.0f - gear_slow_resist(&RUN.gear)));   /* it holds on */
                    c4_grapple(m);                                 /* C4: or holds you fast */
                    thorns(m); DBG_COUNT(grabs);
                }
            }
            return;
        }
        if (sees && pd0 < ATTACKS[FA_GRAB].go && m->cool <= 0.0f) { m->swing = ATTACKS[FA_GRAB].wind; feint_roll(m); elem_arm(m, m->swing); sfx_at(SFX_GROAN, m->x, m->y, 1.0f); return; }
        if (pd0 < 7.0f && rnd(500) == 0) sfx_at(SFX_GROAN, m->x, m->y, 0.7f);
    }
    if (m->type == MOB_HOUND && fights && !afraid) {               /* in, a bite, and away */
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = ATTACKS[FA_HBITE].cool; m->feared = 0.8f; m->wfeared = 0;
                sfx_at(SFX_BONES, m->x, m->y, 0.7f);
                if (pd0 < ATTACKS[FA_HBITE].reach && foe_hit(m, FA_HBITE, 1.0f) > 0) { thorns(m); DBG_COUNT(hound_bites); }
            }
            return;
        }
        if (sees && pd0 < ATTACKS[FA_HBITE].go && m->cool <= 0.0f) { m->swing = ATTACKS[FA_HBITE].wind; elem_arm(m, m->swing); return; }
    }
    if (m->type == MOB_RAT && fights && !afraid) {                 /* the charnel rat: in low, a bite, a scuttle back (docs/PLAN-mice.md) */
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = ATTACKS[FA_RBITE].cool + ATTACKS[FA_RBITE].cool_rnd * rndf(); m->feared = 0.45f; m->wfeared = 0;
                sfx_at(SFX_RAT_SQUEAL, m->x, m->y, 0.45f);
                if (pd0 < ATTACKS[FA_RBITE].reach && foe_hit(m, FA_RBITE, 1.0f) > 0) thorns(m);
            }
            return;
        }
        if (sees && pd0 < ATTACKS[FA_RBITE].go && m->cool <= 0.0f) { m->swing = ATTACKS[FA_RBITE].wind; elem_arm(m, m->swing); return; }
    }
    if (m->boss == 1 + BOSS_SPIDER && spider_boss(m, dt, pd0, sees)) return;
    if (m->type == MOB_HUNTER && !G.dead) {                        /* along the ceiling, down a thread, and a leap */
        if (update_leap(m, dt)) return;
        m->cool -= dt;
        if (m->sub == 0) {                                         /* overhead, upside down */
            m->z = 1.0f - KINDS[MOB_HUNTER].scale * RANK_SIZE[m->rank];
            if (fights && sees && pd0 < 2.6f && G.level_t > 1.5) { m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SKITTER, m->x, m->y, 1.3f); }
        } else if (m->sub == 1) {                                  /* it drops */
            m->sub_t += dt;
            m->z = lerpf(1.0f - KINDS[MOB_HUNTER].scale, 0.0f, fminf(1.0f, m->sub_t / 0.4f));
            if (m->sub_t >= 0.4f) { m->sub = 2; m->z = 0.0f; m->alert = 12.0f; }
            return;
        } else if (fights && sees && pd0 < ATTACKS[FA_HLEAP].go2 && pd0 > ATTACKS[FA_HLEAP].go && m->cool <= 0.0f) { start_leap(m); return; }
    }
    if (m->type == MOB_GHOUL && !G.dead) {                          /* it leaps the last of the way; it feeds on the dead */
        if (update_leap(m, dt)) return;
        m->cool -= dt;
        if (m->sub == 4) {                                         /* feeding: it mends, and is open */
            m->hp = fminf(m->maxhp, m->hp + m->maxhp * 0.06f * dt);
            if ((m->sub_t -= dt) <= 0.0f || (sees && pd0 < 6.0f)) m->sub = 0;
            if (rnd(100) == 0) sfx_at(SFX_CHEW, m->x, m->y, 0.7f);   /* heard at its meal */
            return;
        }
        if (fights && !afraid && sees && pd0 < ATTACKS[FA_GLEAP].go2 && pd0 > ATTACKS[FA_GLEAP].go && m->cool <= 0.0f) { start_leap(m); return; }
        if (m->alert <= 0.0f && m->hp < m->maxhp && (m->contact -= dt) <= 0.0f) {   /* bones near? it stops to eat */
            m->contact = 2.0f;
            for (int i = 0; i < G.ndecos; i++) {
                int sp = G.decos[i].spr;
                if ((sp == SP_BONES1 || sp == SP_SIGN_RAKE || sp == SP_BONES2 || sp == SP_SKEL_D3 || sp == SP_SKULLPILE || sp == SP_CARCASS)
                    && fabsf(G.decos[i].x - m->x) < 1.2f && fabsf(G.decos[i].y - m->y) < 1.2f) { m->sub = 4; m->sub_t = 4.0f; break; }
            }
        }
    }
    if (m->boss == 1 + BOSS_KNIGHT && knight_boss(m, dt, pd0, sees)) return;
    if (m->type == MOB_KNIGHT && fights && !afraid) {              /* a long wind-up, and a blow that tells */
        int st = knight_stage(m);
        if (st != m->sub) { m->sub = st; sfx_at(SFX_SHATTER, m->x, m->y, 1.1f); m->speed = knight_pace(m, st); }
        m->cool -= dt;
        if (m->throw_t > 0.0f) m->throw_t -= dt;                   /* the blow, still showing */
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f;
                if (feint_held(m)) { m->cool = 0.5f; return; }      /* the blade held high, and down a moment later */
                m->cool = ATTACKS[FA_KNIGHT].cool; m->throw_t = 0.3f;
                sfx_at(SFX_SWORD, m->x, m->y, 1.4f);
                if (pd0 < ATTACKS[FA_KNIGHT].reach && foe_hit(m, FA_KNIGHT, 1.0f) > 0) { thorns(m); DBG_COUNT(knight_blows); }
            }
            return;
        }
        if (sees && pd0 < ATTACKS[FA_KNIGHT].go && m->cool <= 0.0f) { m->swing = ATTACKS[FA_KNIGHT].wind; feint_roll(m); elem_arm(m, m->swing); sfx_at(SFX_SWORD, m->x, m->y, 0.4f); return; }
    }
    if (m->type == MOB_BROOD && !G.dead) {                          /* she lays as she goes, and bites what comes close */
        m->cool -= dt;
        if (m->swing > 0.0f) { if ((m->swing -= dt) <= 0.0f) m->swing = 0.0f; return; }   /* laying */
        if (fights && sees && pd0 < ATTACKS[FA_BROOD].go && m->cool <= 0.0f) {
            m->cool = ATTACKS[FA_BROOD].cool;
            if (foe_hit(m, FA_BROOD, 1.0f) > 0) { thorns(m); DBG_COUNT(brood_bites); }
            sfx_at(SFX_SKITTER, m->x, m->y, 1.2f);
            return;
        }
        int sacs = 0;
        for (int i = 0; i < MAXMOBS; i++) sacs += G.mobs[i].alive && G.mobs[i].type == MOB_SAC && G.mobs[i].state == MS_WALK;
        if (m->alert > 0.0f && !hold && sacs < 4 && (m->contact -= dt) <= 0.0f) {   /* held at the fight's edge, she waits too */
            m->contact = 7.0f; m->swing = 1.0f;
            Mob *e = spawn_mob(m->cx, m->cy, MOB_SAC);
            if (e) { e->rank = m->rank; DBG_COUNT(sacs_laid); }
            return;
        }
    }
    if (m->type == MOB_LING && !G.dead) {                           /* a swarm of small bites */
        m->cool -= dt; m->alert = fmaxf(m->alert, 5.0f);
        if (fights && pd0 < ATTACKS[FA_LING].go && m->cool <= 0.0f) {
            m->cool = ATTACKS[FA_LING].cool;
            if (foe_hit(m, FA_LING, 1.0f) > 0) { thorns(m); DBG_COUNT(ling_bites); }
        }
    }
    if (m->type == MOB_SHADE && !G.dead) {                         /* unseen in the dark; its touch takes souls */
        m->cool -= dt;
        if (m->sub_t > 0.0f) m->sub_t -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f;
                if (feint_held(m)) return;
                m->cool = ATTACKS[FA_SHADE].cool; m->sub_t = 1.5f;
                if (pd0 < ATTACKS[FA_SHADE].reach) {
                    int take = RUN.souls < 2 + m->rank ? RUN.souls : 2 + m->rank;
                    RUN.souls -= take;
                    if (foe_hit(m, FA_SHADE, take ? 0.5f : 1.0f) > 0) thorns(m);   /* nothing to take: it takes more of you */
                    snd_sfx_ex(SFX_WHISPER, 0.9f, 0.5f); pop_readout();
                    DBG_COUNT(shade_touches);
                }
            }
            return;
        }
        if (fights && pd0 < ATTACKS[FA_SHADE].go && m->cool <= 0.0f) { m->swing = ATTACKS[FA_SHADE].wind; feint_roll(m); sfx_at(SFX_GHOST_TICK, m->x, m->y, 0.8f); if (mind) DBG_COUNT(shade_strikes); return; }
        if (pd0 < 6.0f && shade_hidden(m) && rnd(100) == 0) sfx_at(SFX_STEP, m->x, m->y, 0.45f);   /* footsteps, and no one there */
    }
    if (m->type == MOB_IMP && !G.dead) {                           /* it keeps off, and lobs embers */
        float lx, ly;
        if (sees) m->seen = 1;
        if (m->throw_t > 0.0f) {
            if ((m->throw_t -= dt) <= 0.0f) {
                if (feint_held(m)) return;
                if (!sees && mind && cunning(m) >= RK_II && corner_aim(m, &lx, &ly)) { throw_to(m, MIS_EMBER, lx, ly, 1); DBG_COUNT(corner_throws); }
                else throw_missile(m, MIS_EMBER);
                m->cool = ATTACKS[FA_EMBER].cool + ATTACKS[FA_EMBER].cool_rnd * rndf(); DBG_COUNT(embers);
                if (mind && cunning(m) >= RK_II && cover_near(m)) { m->cover = 1; DBG_COUNT(covers); }
            }
            return;
        }
        m->cool -= dt;
        if (fights && sees && pd0 > ATTACKS[FA_EMBER].go && pd0 < ATTACKS[FA_EMBER].go2 && m->cool <= 0.0f && G.level_t > 1.5 && m->cover != 1) { m->throw_t = ATTACKS[FA_EMBER].wind; feint_roll(m); return; }
        if (fights && !sees && !m->cover && mind && cunning(m) >= RK_II && m->cool <= 0.0f && corner_aim(m, &lx, &ly)) { m->throw_t = ATTACKS[FA_EMBER].wind; return; }
    }
    if (m->type == MOB_DEACON && !G.dead) {                        /* it keeps back, and calls the fallen up again */
        if (mind && !m->called) {                                  /* the first time he sees you he calls the level's dead */
            if (m->call_t > 0.0f) { if ((m->call_t -= dt) <= 0.0f) alarm_at(m, 14); return; }
            if (sees && G.level_t > 1.5) { m->call_t = 0.8f; sfx_at(SFX_MOAN, m->x, m->y, 1.3f); sfx_at(SFX_WHISPER, m->x, m->y, 1.1f); return; }
        }
        if (m->swing > 0.0f) {                                     /* chanting over the bones */
            if ((m->swing -= dt) > 0.0f) return;
            m->swing = 0.0f; m->contact = m->boss ? 12.0f : 6.0f;      /* the Black Deacon calls one at a time, with a pause */
            int di = m->deco;
            m->deco = -1;
            if (di >= 0 && (G.decos[di].spr == SP_SKEL_D3 || G.decos[di].spr == SP_BONES2)) {
                int x = (int)G.decos[di].x, y = (int)G.decos[di].y;
                Mob *r = occupied(x, y, NULL) ? NULL : spawn_mob(x, y, MOB_SKEL);
                if (r) {
                    G.decos[di].spr = -1;                          /* the heap gets up */
                    set_rank(r, m->rank > RK_I && m->rank < RK_GILDED ? m->rank - 1 : RK_I);
                    r->sub = 5; r->sub_t = 0.0f; r->alert = 12.0f; r->seen = 1; r->cool = throw_interval();
                    m->raised++;
                    if (m->boss) { r->add = 3; r->home = -1; r->alert = 30.0f; }   /* the Black Deacon's: they come for you */
                    sfx_at(SFX_BONES, r->x, r->y, 1.1f);
                    DBG_COUNT(raised);
                }
            }
            return;
        }
        int called = 0;
        for (int i = 0; i < MAXMOBS && m->boss; i++) called += G.mobs[i].alive && G.mobs[i].add == 3 && G.mobs[i].state != MS_DIE;
        if (fights && m->alert > 0.0f && (m->boss ? called < 3 : m->raised < 4) && (m->contact -= dt) <= 0.0f) {
            m->contact = 1.0f;                                     /* it looks again in a moment */
            for (int i = 0; i < G.ndecos; i++) {
                const Deco *dc = &G.decos[i];
                if (dc->spr != SP_SKEL_D3 && dc->spr != SP_BONES2) continue;
                if (hypotf(dc->x - m->x, dc->y - m->y) > 6.0f || !line_clear(m->x, m->y, dc->x, dc->y) || occupied((int)dc->x, (int)dc->y, NULL)) continue;
                m->swing = DEACON_CHANT; m->deco = i; m->ax = dc->x; m->ay = dc->y;
                G.bchant_dmg = 0.0f;
                sfx_at(SFX_WHISPER, m->x, m->y, 1.0f);
                return;
            }
        }
    }
    if (m->type == MOB_WHIPPER && !G.dead) {                       /* the huntsman: behind his hounds, his horn and his whip */
        if (m->call_t > 0.0f) { m->call_t -= dt; m->face = atan2f(pdy0, pdx0); return; }   /* winding his horn: his hounds wait on it */
        if (m->throw_t > 0.0f) m->throw_t -= dt;                   /* the crack, still showing */
        m->cool -= dt;
        if (m->swing > 0.0f) {                                     /* the lash, come too close */
            if ((m->swing -= dt) > 0.0f) return;
            m->swing = 0.0f;
            if (feint_held(m)) return;
            m->cool = ATTACKS[FA_LASH].cool; m->throw_t = 0.3f;
            sfx_at(SFX_WHIPCRACK, m->x, m->y, 1.2f);
            if (pd0 < ATTACKS[FA_LASH].reach && foe_hit(m, FA_LASH, 1.0f) > 0) { thorns(m); DBG_COUNT(whip_lashes); }
            return;
        }
        if (fights && sees && pd0 < ATTACKS[FA_LASH].go && m->cool <= 0.0f && G.level_t > 1.5) { m->swing = ATTACKS[FA_LASH].wind; feint_roll(m); elem_arm(m, m->swing); return; }
        if (mind && !m->called && !pack_of(m) && sees && G.level_t > 1.5) {   /* his hounds gone: the horn, for whatever hears it */
            m->call_t = 1.1f; sfx_at(SFX_HORN, m->x, m->y, 1.2f); alarm_at(m, 10); m->feat |= FT_HORN; DBG_COUNT(horns);
            return;
        }
    }
    if (m->type == MOB_SEXTON && !G.dead) {                        /* he doesn't fight: he rings */
        if (m->ice_t > 0.0f) { m->ice_t -= dt; if (m->ring_t > 0.0f) { m->ring_t = 0.0f; m->rings = 0; } }   /* frozen: it can't */
        if (m->ring_cd > 0.0f) m->ring_cd -= dt;
        if (m->ring_t > 0.0f) {
            m->ring_t -= dt;
            if (m->rings == 1 && m->ring_t <= 1.2f) {              /* raised, and rung: the widest call on the level, to his place */
                m->rings = 2; m->ring_cd = 8.0f; m->contact = 0.0f; m->feat |= FT_RUNG;
                noise(m->x, m->y, 14, NZ_BELL, m->type);
                way_met(WY_BELL);
                DBG_COUNT(bell_alarms);
            }
            if (m->rings == 2 && (m->contact -= dt) <= 0.0f) { m->contact = 0.16f; sfx_at(SFX_HANDBELL, m->x, m->y, 1.25f); }
            if (m->ring_t <= 0.0f) m->rings = 0;
            if (m->rings) { m->face = atan2f(pdy0, pdx0); return; }   /* he stands to ring it */
        }
        if (mind && sees && G.level_t > 1.5 && m->ring_cd <= 0.0f && m->ice_t <= 0.0f && m->stagger <= 0.0f) { sexton_ring(m, 1); return; }
    }
    if (m->type == MOB_LIGHTLESS && !G.dead) {                     /* quick to the light, without a sound; it strikes what is lit */
        m->speed = KINDS[MOB_LIGHTLESS].speed * RANK_SPEED[m->rank] * (m->wild ? 1.3f : 1.0f)
                 * (m->alert > 0.0f || m->aware == MA_TRACK ? 1.4f : 0.6f);
        m->cool -= dt;
        if (m->swing > 0.0f) {
            if ((m->swing -= dt) > 0.0f) return;
            m->swing = 0.0f;
            if (feint_held(m)) return;
            m->cool = ATTACKS[FA_GLASS].cool;
            sfx_at(SFX_GLASS, m->x, m->y, 1.0f);
            if (pd0 < ATTACKS[FA_GLASS].reach && foe_hit(m, FA_GLASS, 1.0f) > 0) { thorns(m); DBG_COUNT(lightless_hits); }
            return;
        }
        Wisp *w = &G.wisps[0];
        if (w->on && w->state != WS_GONE && hypotf(w->x - m->x, w->y - m->y) < 1.3f) {   /* at a wisp: it puts it out */
            m->swing = 0.4f; wisp_out(w); sfx_at(SFX_GLASS, m->x, m->y, 1.0f);
            DBG_COUNT(wisps_struck);
            return;
        }
        if (fights && !afraid && sees && pd0 < ATTACKS[FA_GLASS].go && m->cool <= 0.0f) { m->swing = ATTACKS[FA_GLASS].wind; feint_roll(m); return; }
        if ((m->tx != m->cx || m->ty != m->cy) && pd0 < 9.0f && rnd(110) == 0) sfx_at(SFX_GLASS, m->x, m->y, 0.4f);   /* its glass, as it goes */
    }
    if (m->type == MOB_GARGOYLE) {                                 /* stone till you pass; then wings */
        if (m->sub == 0) {
            if (m->pack && !G.dead && pd0 < 8.0f && rnd(500) == 0) sfx_at(SFX_DIRT, m->x, m->y, 0.3f);   /* grit falling from a statue */
            if (!G.dead && sees && pd0 < 3.0f && G.level_t > 1.5) garg_wake(m);
            return;
        }
        if (m->sub == 1) {                                         /* it shakes itself loose */
            m->sub_t += dt; m->z = GARG_Z * smoothstepf(0.2f, 0.8f, m->sub_t);
            if (m->sub_t >= 0.8f) {
                m->sub = 2; m->alert = 12.0f;
                if (minded(m)) { sfx_at(SFX_SHRIEK, m->x, m->y, 0.8f); alarm_at(m, 8); }   /* loose, it shrieks */
            }
            return;
        }
        if (update_leap(m, dt)) return;
        m->z = GARG_Z + 0.03f * sinf(m->t * 5.0f);
        m->cool -= dt;
        if (m->swing > 0.0f) {                                     /* its claws */
            if ((m->swing -= dt) <= 0.0f) {
                m->swing = 0.0f;
                if (feint_held(m)) return;
                m->cool = ATTACKS[FA_CLAW].cool;
                if (pd0 < ATTACKS[FA_CLAW].reach && foe_hit(m, FA_CLAW, 1.0f) > 0) thorns(m);
            }
            return;
        }
        if (fights && sees && pd0 < ATTACKS[FA_CLAW].go && m->cool <= 0.0f) { m->swing = ATTACKS[FA_CLAW].wind; feint_roll(m); elem_arm(m, m->swing); return; }
        if (fights && sees && pd0 > ATTACKS[FA_SWOOP].go && pd0 < ATTACKS[FA_SWOOP].go2 && m->cool <= 0.0f) { start_leap(m); return; }
    }
    if (m->type == MOB_BAT && !G.dead) {                           /* bats come for your face */
        int los = pd0 < 5.0f && sees;
        if (los && fights) m->attacking = 1;
        if (m->attacking && (pd0 > 7.0f || !fights)) { m->attacking = 0; m->tx = m->cx; m->ty = m->cy; }
        if (G.latch == (int)(m - G.mobs) + 1) return;              /* on your face: update_latch keeps it there */
        if (m->attacking) {
            int waits = G.latch != 0;                              /* one latched at a time: the rest keep off your face */
            m->orbit += dt * (1.3f + 0.4f * sinf(m->t * 0.7f));
            float a = G.ang + sinf(m->orbit) * 1.5f, r = (waits ? 2.2f : 0.95f) + 0.18f * sinf(m->orbit * 2.3f);
            float gx = G.px + cosf(a) * r, gy = G.py + sinf(a) * r;
            float vx = gx - m->x, vy = gy - m->y, vd = sqrtf(vx * vx + vy * vy) + 1e-4f, step = fminf(vd, 2.6f * dt);
            float nx = m->x + vx / vd * step, ny = m->y + vy / vd * step;
            if (!blocks_move((int)nx, (int)m->y)) m->x = nx;
            if (!blocks_move((int)m->x, (int)ny)) m->y = ny;
            m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
            if (!waits && hypotf(G.px - m->x, G.py - m->y) < ATTACKS[FA_BAT].reach) {   /* touching: after 3 s it bites, */
                if (m->contact <= 0.0f) elem_arm(m, ATTACKS[FA_BAT].wind);   /* its element decided as it settles on you (C3) */
                m->contact += dt;
                if (latch_begin(m)) return;                        /* or from rank II, sooner, it latches on */
                if (m->contact >= ATTACKS[FA_BAT].wind) {
                    m->contact = 0.0f;
                    if (foe_hit(m, FA_BAT, 1.0f) > 0) thorns(m);
                    sfx_at(SFX_BAT, m->x, m->y, 1.0f); DBG_COUNT(hits_bat);
                }
            } else m->contact = 0.0f;
            if (rnd(60) == 0) sfx_at(SFX_BAT, m->x, m->y, 0.6f);
            return;
        }
    }
    float gx, gy; cell_pass(m->tx, m->ty, &gx, &gy);               /* round a prop's part (1.0.0b18.1) */
    float dx = gx - m->x, dy = gy - m->y, d = sqrtf(dx * dx + dy * dy);
    float step = m->speed * dt * (m->type == MOB_BAT ? 0.8f + 0.4f * sinf(m->t * 3.0f) : 1.0f) * (afraid ? 1.4f : 1.0f);
    if (m->bait_st == B_LURE && m->rank != RK_GILDED) step = fminf(step, 1.8f * dt);   /* at your pace, not its own */
    if (mind && !afraid && m->alert <= 0.0f) step *= m->hunter == 2 ? 1.1f : m->aware == MA_WARY ? 0.5f : (m->aware == MA_SEARCH ? 0.8f : 1.0f);   /* to
                                                                      look: slower; on the trail, quicker than you walked */
    if (d > step) { m->x += dx / d * step; m->y += dy / d * step; if (!mind && m->type == MOB_SHIELD && !m->seen) m->face = atan2f(dy, dx); return; }
    m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty;
    if (m->cover == 1) {                                           /* green's thrower behind its corner, till the next throw is near */
        if (m->type == MOB_SKEL) m->cool -= dt;
        if (m->cool <= 1.0f / fmaxf(0.5f, m->speed) || m->alert <= 0.0f || afraid || lured || hold) { m->cover = 0; m->lost = 0.0f; }
        else if (m->cx == m->cvx && m->cy == m->cvy) return;
        else if (abs(m->cvx - m->cx) + abs(m->cvy - m->cy) == 1 && open_step(m, m->cvx, m->cvy)) { m->tx = m->cvx; m->ty = m->cvy; return; }
        else m->cover = 0;
    }
    /* next cell: close in on the caster when it is near and visible, otherwise wander; flee if afraid */
    float pdx = G.px - m->x, pdy = G.py - m->y, pd = sqrtf(pdx * pdx + pdy * pdy);
    float keep = KIND_IS(m, KF_MELEE) ? 1.05f : (keeps_off ? 4.0f : 2.2f);   /* those that fight hand to hand come right up close */
    if (hold) keep = fmaxf(keep, hold == 4 ? 4.5f : hold == 3 ? 5.0f : hold == 2 ? (m->type == MOB_HOUND ? 4.0f : 3.0f) : 3.5f);   /* just out of
                                                                      reach (4: a charge's) */
    else if (mind && m->type == MOB_SKEL && m->pack) {             /* a file's throwers keep behind its shields */
        const Pack *p = pack_of(m);
        for (int i = 0; p && p->kind == PACK_FILE && !p->broken && i < MAXMOBS; i++)
            if ((p->mem >> i & 1) && G.mobs[i].type == MOB_SHIELD && G.mobs[i].alive && G.mobs[i].state == MS_WALK)
                keep = fmaxf(keep, fminf(6.0f, hypotf(G.mobs[i].x - G.px, G.mobs[i].y - G.py) + 1.0f));
    }
    int chase = !lured && pd < (KIND_IS(m, KF_MELEE) ? 9.0f : 7.0f) && pd > keep && line_clear(m->x, m->y, G.px, G.py);
    if (mind) chase = chase && m->alert > 0.0f;                    /* only once it knows you are there */
    int here = hunt[m->cy][m->cx], opens = KIND_IS(m, KF_DOORS) != 0;   /* the dead open doors; bats and hounds wait behind them */
    int hunting = !afraid && !lured && !chase && !G.dead && m->alert > 0.0f && m->hunts && here > 0 && pd > keep;
    int spr = spreads(m), hz;                                      /* red, against a splash in hand: no two within a cell and a half */
    static double spread_said;
    if (falling) chase = hunting = 0;
    if (mind && !afraid && !lured && !falling && door_beside(m)) return;   /* blue: beside a doorway you hold */
    if (hold && !falling && !afraid && !lured && !G.dead && pd <= keep + 0.6f && line_clear(m->x, m->y, G.px, G.py)) {   /* holding off: the
                                                                      hand-to-hand circle at the edge; the rest keep their distance */
        int melee = KIND_IS(m, KF_MELEE) != 0;
        if (!melee && pd > keep - 0.7f && hold != 4 && !(spr && crowded(m, m->cx, m->cy))) return;
        float side = ((m - G.mobs) & 1) ? 1.0f : -1.0f;
        int best = -1; float bs = melee && hold != 4 ? -1e9f : -fabsf(pd - keep) * 2.0f - (spr && crowded(m, m->cx, m->cy) ? 2.0f : 0.0f)
                                                         + (hold == 4 ? fabsf(pdx * sinf(G.ang) - pdy * cosf(G.ang)) / (pd + 1e-4f) * 0.8f : 0.0f);   /* standing
                                                                      is a choice too */
        for (int k = 0; k < 4; k++) {
            int nx = m->cx + DX[k], ny = m->cy + DY[k];
            const Cell *c = cell(nx, ny);
            if (blocks_move(nx, ny) || occupied(nx, ny, m) || (c->kind == K_DOOR && G.doors[c->door].open < 0.9f)) continue;
            if ((hz = mind ? hz_shuns(m, nx, ny) : 0) == 1) continue;
            float ex = nx + 0.5f - G.px, ey = ny + 0.5f - G.py, dn = sqrtf(ex * ex + ey * ey);
            if (dn < 1.4f) continue;
            float tg = hold == 3 ? -((nx + 0.5f - G.px) * cosf(G.ang) + (ny + 0.5f - G.py) * sinf(G.ang)) / (dn + 1e-4f)   /* behind you */
                     : hold == 4 ? fabsf(ex * sinf(G.ang) - ey * cosf(G.ang)) / (dn + 1e-4f)   /* off the way you face */
                     : melee ? ((float)DX[k] * -pdy + (float)DY[k] * pdx) / (pd + 1e-4f) * side : 0.0f;
            float sc = -fabsf(dn - keep) * 2.0f + tg * 0.8f + rndf() * 0.3f - (hz ? 2.5f : 0.0f) - (spr && crowded(m, nx, ny) ? 2.0f : 0.0f);
            if (sc > bs) { bs = sc; best = k; }
        }
        if (best >= 0) { m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best]; }
        return;
    }
    int going = 0, gcx = 0, gcy = 0, ghere = -1;                   /* to look somewhere: a sound, where you were, a search */
    int16_t (*gf)[MAPN] = NULL;
    if (falling) {                                                 /* back to the one of its pack it makes for */
        gf = mind_field(fall_to->cx, fall_to->cy);
        ghere = gf[m->cy][m->cx];
        if (ghere > 0) going = 1; else if (m->fell > 0) m->fell = -1;
    } else if (mind && !hunting && !chase && !afraid && !lured && !G.dead && m->alert <= 0.0f && mind_goal(m, &gcx, &gcy)) {
        gf = mind_field(clampi(gcx, 0, MAPN - 1), clampi(gcy, 0, MAPN - 1));
        ghere = gf[m->cy][m->cx];
        if (ghere == 0 || (m->scav && ghere == 1)) mind_arrived(m);
        else if (ghere > 0) going = 1;
        else if (m->scav) m->scav = 0;                             /* no way to the meal */
        else if (m->aware == MA_SEARCH) m->wn = 0;                 /* no way there: somewhere else */
        else if (m->aware == MA_TRACK) mind_arrived(m);            /* no way after you: it searches from here */
        else if (m->aware == MA_WARY) m->aw_t = 8.0f;              /* nor to the sound: it lets it be */
    }
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_KINDTRACE") && m->type == atoi(getenv("CRAWL_KINDTRACE")) && rnd(30) == 0)
    {   fprintf(stderr, "KIND move going %d ghere %d goal (%d,%d) hunting %d chase %d afraid %d lured %d hold %d falling %d at (%d,%d):", going, ghere, gcx, gcy, hunting, chase, afraid, lured, hold, falling, m->cx, m->cy);
        for (int k = 0; k < 4 && gf; k++) { int nx = m->cx + DX[k], ny = m->cy + DY[k]; fprintf(stderr, " [h %d occ %d hz %d kind %d]", gf[ny][nx], occupied(nx, ny, m), hz_shuns(m, nx, ny), cell(nx, ny)->kind); }
        fprintf(stderr, "\n"); }
#endif
    int trailing = mind && !going && !hunting && !chase && !afraid && !lured && !G.dead && m->alert <= 0.0f && m->hunter == 2 && on_trail(m);
    float trail_here = G.scent[m->cy][m->cx];
    int trail_wait = 0;                                            /* a fresher cell ahead, but a shut door or one of its own in it */
    if (mind && !going && !hunting && !chase && !afraid && !lured && !trailing && m->alert <= 0.0f
        && ((m->aware == MA_WARY && (m->aw_t < 0.8f || ghere == 0)) || (m->aware == MA_SEARCH && m->wait > 0.0f)
            || m->bait_st == B_SHOW
            || (m->aware == MA_CALM && (m->wait > 0.0f || m->role == RL_FOLLOW || (m->role == RL_POST && m->cx == m->hx && m->cy == m->hy))))) return;   /* it stands and listens, looks about, or keeps its post */
    int16_t (*hf)[MAPN] = hunt;                                    /* red, the second and later of a pack: its other way to you */
    if (hunting && m->flank && here > 3) { int16_t (*f)[MAPN] = flank_field(m->flank - 1); if (f && f[m->cy][m->cx] > 0) hf = f; }
    int hhere = hf[m->cy][m->cx], cr = spr && crowded(m, m->cx, m->cy), spread_skip = 0;
    int best = -1; float bs = -1e9f;
    for (int k = 0; k < 4; k++) {
        int nx = m->cx + DX[k], ny = m->cy + DY[k];
        const Cell *c = cell(nx, ny);
        if ((hz = mind ? hz_shuns(m, nx, ny) : 0) == 1) continue;   /* blue: a hazard warning or harming there */
        if (spr && !cr && !going && !afraid && crowded(m, nx, ny)) { spread_skip = 1; continue; }
        if (going) {                                               /* towards the place it means to look */
            int h = gf[ny][nx];
            if (h < 0 || h >= ghere || occupied(nx, ny, m)) continue;
            if (c->kind == K_DOOR && !opens && G.doors[c->door].open < 0.9f) continue;
            float s = rndf() * 0.5f - (float)h - (hz ? 2.5f : 0.0f);
            if (s > bs) { bs = s; best = k; }
            continue;
        }
        if (trailing) {                                            /* the way you came: the freshest scent on */
            float sc = G.scent[ny][nx];
            if (sc <= trail_here || (float)G.level_t - sc > SCENT_LIFE) continue;
            if (occupied(nx, ny, m) || (c->kind == K_DOOR && !opens && G.doors[c->door].open < 0.9f)) { trail_wait = 1; continue; }
            if (c->kind != K_DOOR && (m->type == MOB_HUNTER && m->sub == 0 ? blocks_flight(nx, ny) : blocks_move(nx, ny))) continue;
            if (sc > bs) { bs = sc; best = k; }
            continue;
        }
        if (hunting) {                                             /* down the field, towards the caster */
            int h = hf[ny][nx];
            if (h < 0 || h >= hhere || occupied(nx, ny, m)) continue;
            if (c->kind == K_DOOR && !opens && G.doors[c->door].open < 0.9f) continue;
            float s = rndf() * 0.5f - (float)h - (hz ? 2.5f : 0.0f);
            if (s > bs) { bs = s; best = k; }
            continue;
        }
        if ((m->type == MOB_GARGOYLE ? blocks_flight(nx, ny) : blocks_move(nx, ny)) || occupied(nx, ny, m)) continue;
        if (!chase && !afraid && !lured && m->home >= 0 && c->region != m->home) continue;   /* keeps to its own room till it knows */
        float s = rndf() - (hz ? 2.5f : 0.0f);
        int brood = mind && m->type == MOB_IMP && m->pack && pack_of(m) && pack_of(m)->kind == PACK_IMPS && m->role == RL_POST;
        if (afraid) s += hypotf(nx + 0.5f - (m->wfeared ? m->wfx : G.px), ny + 0.5f - (m->wfeared ? m->wfy : G.py)) * 2.0f
                       - (brood ? hypotf(nx - m->hx, ny - m->hy) * 1.5f : 0.0f);   /* a brood falls back into the vent's heat */
        else if (lured) s -= hypotf(nx + 0.5f - lure_x, ny + 0.5f - lure_y);
        else if (chase) s -= hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py)
                           - (brood ? ((float)DX[k] * -pdy + (float)DY[k] * pdx) / (pd + 1e-4f) * (((m - G.mobs) & 1) ? 0.9f : -0.9f) : 0.0f);   /* and
                                                                      splits to either side, to lob from both */
        if (s > bs) { bs = s; best = k; }
    }
    if (trailing && best < 0 && !trail_wait) hunter_lost(m);       /* the trail ends here: water, or it went cold */
    if (going && best < 0 && !falling) {                           /* its way stood in (one on its post in a narrow hall, a statue): */
        if ((m->blocked += dt) > 3.0f) { m->blocked = 0.0f; mind_arrived(m); DBG_COUNT(ways_blocked); }   /* it gives the place up */
    } else m->blocked = 0.0f;
    if (spread_skip && G.t - spread_said > 1.0) { spread_said = G.t; DBG_COUNT(spread); }
    if (best >= 0 && (hunting || going || trailing)) {
        int nx = m->cx + DX[best], ny = m->cy + DY[best];
        const Cell *c = cell(nx, ny);
        if (hunting && !going && door_wait(m, nx, ny)) return;       /* blue: not into a doorway you hold */
        if (c->kind == K_DOOR && G.doors[c->door].open < 0.9f) {    /* a door in the way: it pulls it open, and waits */
            Door *dr = &G.doors[c->door];
            if (!dr->opening) dr->opening = 1;
            return;
        }
        m->tx = nx; m->ty = ny;
    } else if (best >= 0 && (afraid || lured || !(pd <= keep))) {
        if (chase && !afraid && !lured && door_wait(m, m->cx + DX[best], m->cy + DY[best])) return;
        m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best];
    }
    if (m->type == MOB_BAT && pd < 6.0f && rnd(40) == 0) sfx_at(SFX_BAT, m->x, m->y, 0.6f);
}

