/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The Black Altar: its wisp called home, update_altar, waking, sending, blessing, anointing; update_abilities. */

/* The Black Altar (docs/PLAN-b6.md): gold once every secret is found, every link of every chain. Its
 * wisp goes to one secret at a time and waits at its door, the cell before the cracked wall, while you
 * are inside; once you have been in and come out again, E at the wisp sends it home, and at the altar
 * it can be sent on to the next for ALTAR_SOULS and five more for every secret found on the level. */
static int nearest_shut(float x, float y) {
    int best = -1; float bd = 1e9f;
    for (int i = 0; i < G.nsecrets; i++) {
        if (G.secrets[i].state) continue;
        float d = hypotf(G.secrets[i].fx + 0.5f - x, G.secrets[i].fy + 0.5f - y);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
static int in_secret(int i) {                                      /* standing in that secret's room */
    const Secret *sc = &G.secrets[i];
    return G.cx >= sc->x0 && G.cx <= sc->x1 && G.cy >= sc->y0 && G.cy <= sc->y1;
}
static int altar_price(void) { return ALTAR_SOULS + 5 * G.links_found; }
static int altar_callable(void) {                                  /* been in, and out again: it may be sent home */
    return G.altar_wisp == AW_SEEK && G.altar_seek >= 0 && G.secrets[G.altar_seek].state && G.altar_inside && !in_secret(G.altar_seek);
}
static void altar_call_home(void) {
    G.altar_wisp = AW_HOME; G.wisps[1].retarget = 0.0f;
    sfx_at(SFX_SHIMMER, G.wisps[1].x, G.wisps[1].y, 0.8f);
    if (G.player) say("THE WISP GOES HOME TO ITS ALTAR");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar wisp home t=%.2f\n", G.t);
#endif
}
static void update_altar(void) {
    if (G.altar_x < 0 || G.altar_state >= ALTAR_READY) return;
    if (G.links_found >= G.nlinks) {
        G.altar_state = ALTAR_READY; G.altar_seek = -1;
        if (G.wisps[1].on && G.wisps[1].state != WS_GONE) wisp_out(&G.wisps[1]);   /* its work done */
        G.altar_wisp = AW_NONE;
        sfx_at(SFX_SHIMMER, G.altar_x + 0.5f, G.altar_y + 0.5f, 1.2f);
        if (G.player) say("THE BLACK ALTAR BURNS GOLD");
        return;
    }
    if (G.altar_wisp == AW_SEEK && G.altar_seek >= 0) {
        if (in_secret(G.altar_seek)) G.altar_inside = 1;
        if (!G.player && G.secrets[G.altar_seek].state && !in_secret(G.altar_seek)   /* the autopilot calls it home as it goes */
            && hypotf(G.wisps[1].x - G.px, G.wisps[1].y - G.py) < 4.0f) altar_call_home();
    }
    if (G.altar_wisp == AW_HOME && G.wisps[1].on && G.wisps[1].state == WS_HOVER) G.altar_wisp = AW_REST;
    if ((G.altar_wisp == AW_HOME || G.altar_wisp == AW_REST) && !G.wisps[1].on) summon_wisp(WK_ALTAR, 0, 0.0f);   /* lost: it comes again */
}

/* The altar's three gifts, chosen from its menu (E) or by the autopilot: its wisp for ten souls, its
 * blessing on your exit wisps for twenty-five, and when it burns gold, an anointing */
static int altar_wake(void) {
    if (G.altar_state != ALTAR_DORMANT) return 0;
    if (!spend_spirit((float)ALTAR_SOULS)) { if (G.player) say("THE ALTAR WANTS %d SOULS", ALTAR_SOULS); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    G.altar_state = ALTAR_AWAKE; G.altar_seek = nearest_shut(G.px, G.py); G.altar_wisp = AW_SEEK; G.altar_inside = 0;
    if (G.player) say("THE ALTAR'S WISP SEEKS WHAT IS HIDDEN");
    snd_sfx_ex(SFX_WHISPER, 0.9f, 0.5f); sfx_at(SFX_SHIMMER, G.altar_x + 0.5f, G.altar_y + 0.5f, 1.0f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar wakes t=%.2f\n", G.t);
#endif
    summon_wisp(WK_ALTAR, 0, 0.0f);                                /* the altar's own, free */
    return 1;
}
static int altar_send(void) {                                      /* its wisp, resting, sent on to the next secret */
    if (G.altar_state != ALTAR_AWAKE || G.altar_wisp != AW_REST || nearest_shut(G.px, G.py) < 0) return 0;
    if (!spend_spirit((float)altar_price())) { if (G.player) say("SENDING IT ON WANTS %d SOULS", altar_price()); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    G.altar_seek = nearest_shut(G.px, G.py); G.altar_wisp = AW_SEEK; G.altar_inside = 0; G.wisps[1].retarget = 0.0f;
    if (G.player) say("THE WISP GOES TO THE NEXT SECRET");
    snd_sfx_ex(SFX_WHISPER, 0.9f, 0.5f); sfx_at(SFX_SHIMMER, G.altar_x + 0.5f, G.altar_y + 0.5f, 1.0f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar sends price=%d t=%.2f\n", altar_price(), G.t);
#endif
    return 1;
}
static int altar_bless(void) {
    if (G.blessed) return 0;
    if (!spend_spirit((float)ALTAR_BLESS)) { if (G.player) say("THE BLESSING WANTS %d SOULS", ALTAR_BLESS); snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); return 0; }
    G.blessed = 1;
    if (G.player) say("YOUR EXIT WISPS WILL TURN THE DEAD");
    snd_sfx_ex(SFX_RUNE, 0.8f, 0.5f); snd_sfx_ex(SFX_MOAN, 0.7f, 0.5f); sfx_at(SFX_SHIMMER, G.altar_x + 0.5f, G.altar_y + 0.5f, 1.2f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar blesses t=%.2f\n", G.t);
#endif
    return 1;
}
static int altar_anoint(void) {
    if (G.altar_state != ALTAR_READY) return 0;
    int can[NSLOT], n = 0, worn = 0;                               /* one worn piece, at random, not anointed yet */
    for (int s = 0; s < NSLOT; s++) if (RUN.gear.worn[s].kind) { worn++; if (!RUN.gear.worn[s].anointed) can[n++] = s; }
    if (!n) { if (G.player) say(worn ? "ALL YOU WEAR IS ANOINTED ALREADY" : "YOU WEAR NOTHING FOR IT TO BLESS"); return 0; }
    Piece *p = &RUN.gear.worn[can[rnd(n)]];
    piece_anoint(p);
    G.altar_state = ALTAR_SPENT;
    G.max_hp = gear_max_hp(&RUN.gear);
    char a[48], b[48];
    piece_name(p, a, b, sizeof a);
    say("ANOINTED: %s", b);
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.9f, 0.5f); G.grin_t = 2.0f;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT altar anoints %s %s t=%.2f\n", a, b, G.t);
#endif
    return 1;
}

static void update_abilities(float dt) {
    const Gear *g = &RUN.gear;
    if (gear_ability(g, AB_TREASURE, 0) > 0.0f && (G.whisper_t -= dt) <= 0.0f) {
        G.whisper_t = 2.4f;
        float best = 3.5f; int bi = -1;
        for (int i = 0; i < G.nsecrets; i++) {
            if (G.secrets[i].state) continue;
            float d = hypotf(G.secrets[i].sx + 0.5f - G.px, G.secrets[i].sy + 0.5f - G.py);
            if (d < best) { best = d; bi = i; }
        }
        float wx = bi >= 0 ? G.secrets[bi].sx + 0.5f : 0.0f, wy = bi >= 0 ? G.secrets[bi].sy + 0.5f : 0.0f;
        if (G.msg_deco >= 0 && !G.msg_found && !G.shop && !PK.depth) {   /* and near the hidden message (1.0.0b13) */
            const Deco *dc = &G.decos[G.msg_deco];
            float d = hypotf(dc->x - G.px, dc->y - G.py);
            if (d < best) { best = d; bi = 0; wx = dc->x; wy = dc->y; }
        }
        if (bi >= 0) {
            sfx_at(SFX_WHISPER, wx, wy, 0.35f + 0.9f * (1.0f - best / 3.5f));
            G.whisper_t = 1.6f + best * 0.5f;                       /* quicker, the nearer */
        }
    }
    float reach = gear_ability(g, AB_COMMUNE, 0);
    G.commune_show = fmaxf(0.0f, G.commune_show - dt);
    if (reach > 0.0f && (G.commune_t += dt) >= 15.0f) {
        G.commune_t = 0.0f;
        G.commune_show = G.commune_len = gear_ability(g, AB_COMMUNE, 1) / 10.0f;
        G.commune_r = reach;
        snd_sfx_ex(SFX_HEART, 0.7f, 0.5f);
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT commune t=%.2f r=%.0f for %.1f\n", G.t, reach, G.commune_len);
#endif
    }
}

