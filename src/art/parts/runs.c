/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Each run's set made (crypts_run, catacombs_run, hollows_run, deep_run); the specials on the run's stone; the trials' doors and things; the places to mend; crawl_art_run, crawl_art_stone, crawl_art_init. */

static void treasure_plain(void) {                                 /* the treasure as the Crypts have it */
    spr_coins(SP_COINS_A, 0); spr_coins(SP_COINS_B, 1);
    spr_chalice(); spr_gems(); spr_crown(); spr_chest();
}

/* The hall's sign, the Repository and the Portal, and the arenas' gates, fonts and drains, are drawn on
 * plain stone; on a drawn run they stand in its ashlar instead, wherever the plain stone showed */
static void rebase(int t, uint32_t stone_seed, int base) {
    static uint8_t keep[TS * TS];
    memcpy(keep, crawl_tex[T_STONE], sizeof keep);
    gen_stone(T_STONE, R_STONE, stone_seed);
    for (int i = 0; i < TS * TS; i++) if (crawl_tex[t][i] == crawl_tex[T_STONE][i]) crawl_tex[t][i] = crawl_tex[base][i];
    memcpy(crawl_tex[T_STONE], keep, sizeof keep);
}
/* a puzzle's sealed wing: a great iron-banded door, a round seal carved in the stone over it */
static void gen_wingdoor(int t, uint32_t seed) {
    gen_stone(t, R_STONE, seed);
    for (int y = 14; y < 64; y++)
        for (int x = 10; x < 54; x++) {
            float dx = x + 0.5f - 32.0f, top = 24.0f - sqrtf(fmaxf(0.0f, 22.0f * 22.0f - dx * dx)) * 0.45f;
            if (y < top) continue;
            int frame = y < top + 2.0f || x < 12 || x > 51;
            uint8_t c = frame ? sh(R_STONE, 0.7f) : sh(R_WOOD, 0.2f + 0.12f * fbm(x / 3.0f, y / 9.0f, 16, seed) + 0.08f * ((x - 12) % 7 == 0 ? -1.0f : 0.0f));
            if (!frame && ((y - 14) % 14 < 2)) c = sh(R_IRON, 0.35f + 0.2f * ((x & 3) == 0));   /* the bands */
            tput(t, x, y, c);
        }
    for (int a = 0; a < 40; a++) {                                    /* the seal: a ring with a cross of bars through it */
        float an = a * 0.157f;
        tput(t, 32 + (int)lroundf(7.0f * cosf(an)), 11 + (int)lroundf(5.5f * sinf(an)), sh(R_BLOOD, 0.55f));
    }
    for (int k = -4; k <= 4; k++) { tput(t, 32 + k, 11, sh(R_BLOOD, 0.4f)); tput(t, 32, 11 + k * 5 / 7, sh(R_BLOOD, 0.4f)); }
    for (int y = 36; y < 44; y++) { tput(t, 31, y, sh(R_IRON, 0.6f)); tput(t, 33, y, sh(R_IRON, 0.6f)); }   /* a lockless ring */
}
static void hz_walls(void) {                                       /* the runs' hazards (1.0.0b13) */
    gen_hz_slab(T_HZ_SLAB, 109, 0); gen_hz_slab(T_HZ_SLAB_OPEN, 109, 1); gen_hz_cold(T_HZ_COLD, 113);
    gen_hz_sluice(T_HZ_SLUICE, 127, 0); gen_hz_sluice(T_HZ_SLUICE_OPEN, 127, 1);
    gen_hz_saint(T_HZ_SAINT, 131, 0); gen_hz_saint(T_HZ_SAINT_LIT, 131, 1);
    gen_hz_seam(T_HZ_SEAM, 137, 0); gen_hz_seam(T_HZ_SEAM_LIT, 137, 1);
    gen_hz_fissure(T_HZ_FISSURE0, 139, 0); gen_hz_fissure(T_HZ_FISSURE1, 139, 1);
    gen_hz_mirror(T_HZ_MIRROR, 149, 0); gen_hz_mirror(T_HZ_MIRROR_SEEN, 149, 1);
    gen_wingdoor(T_WINGDOOR, 151);
}
static void specials_plain(void) {
    gen_sign(T_SIGN, 91);
    gen_repository(T_REPOSITORY);
    for (int f = 0; f < 4; f++) gen_portal(T_PORTAL0 + f, f, 107);
    gen_gate(T_GATE, 97); gen_gate_seal(T_GATE_SEAL, 97);
    gen_font(T_FONT, 101, 1); gen_font(T_FONT_DRY, 101, 0);
    gen_grate(T_GRATE, 103);
    hz_walls();
}
static void specials_on(int base) {
    specials_plain();
    rebase(T_SIGN, 91, base); rebase(T_REPOSITORY, 233, base);
    for (int f = 0; f < 4; f++) rebase(T_PORTAL0 + f, 107, base);
    rebase(T_GATE, 97, base); rebase(T_GATE_SEAL, 97, base); rebase(T_FONT, 101, base); rebase(T_FONT_DRY, 101, base); rebase(T_GRATE, 103, base);
    static const uint32_t HZS[] = { 109, 109, 113, 127, 127, 131, 131, 137, 137, 139, 139, 149, 149, 151 };
    for (int i = 0; i < 14; i++) rebase(T_HZ_SLAB + i, HZS[i], base);
}

/* The run's own textures and keys, for the level's place in it: made from the level (so each level's
 * stones differ), with the rot of its step. Called after crawl_art_level, which sets the palette. */
static void crypts_run(float dk, uint32_t s, uint32_t field) {
    const RunLook *L = &crawl_looks[0];
    for (int v = 0; v < NVAR; v++) {
        uint32_t vs = v ? s + 1u : 0u;
        gen_ashlar(T_MATV(M1_ASHLAR, v), M1_ASHLAR, s, vs);
        gen_rubble_c(T_MATV(M1_RUBBLE, v), M1_RUBBLE, s + 10, vs);
        gen_plaster(T_MATV(M1_PLASTER, v), M1_PLASTER, s + 20, vs);
        gen_bedrock(T_MATV(M1_BEDROCK, v), M1_BEDROCK, s + 30, vs);
        gen_bonewall(T_MATV(M1_BONES, v), M1_BONES, s + 40, vs);
    }
    for (int v = 0; v < NVAR; v++) plaster_flake(T_MATV(M1_PLASTER, v), T_MATV(M1_RUBBLE, 0), dk, field + 30);
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) decay_wall(T_MATV(m, v), m == M1_BONES ? dk * 0.6f : dk, field, s + 60u + (uint32_t)(m * NVAR + v) * 3u);
    for (int k = 0; k < NBLEND; k++) gen_blend(T_BLEND + k, L->blend[k][0], L->blend[k][1], L->blend[k][0] == M1_PLASTER, s + 70u + (uint32_t)k);
    gen_feat_niche(T_FEAT + 0, s + 80);
    gen_feat_banner(T_FEAT + 1, s + 81, dk);
    gen_feat_slab(T_FEAT + 2, s + 82);
    gen_feat_shrine(T_FEAT + 3, s + 83);
    gen_feat_tablet(T_FEAT + 4, s + 84);
    for (int f = 0; f < 3; f++) gen_feat_torch(T_FEAT + 5 + f, f, 11);
    for (int i = 0; i < 5; i++) decay_wall(T_FEAT + i, dk * 0.7f, field, s + 90u + (uint32_t)i * 3u);
    gen_floor_flags(T_FLOORS + 0, s + 100, 0);
    gen_floor_chapter(T_FLOORS + 1, s + 101);
    gen_floor_setts(T_FLOORS + 2, s + 102);
    gen_floor_earth(T_FLOORS + 3, s + 103);
    gen_floor_flags(T_FLOORS + 4, s + 100, 1);
    gen_ceil_rib(T_CEILS + 0, s + 110);
    gen_ceil_vault(T_CEILS + 1, s + 111);
    gen_ceil_rock(T_CEILS + 2, s + 112);
    for (int i = 0; i < NFLOORS; i++) decay_flat(T_FLOORS + i, dk, s + 130u + (uint32_t)i);
    for (int i = 0; i < NCEILS; i++) decay_flat(T_CEILS + i, dk * 0.7f, s + 140u + (uint32_t)i);
    gen_rdoor(T_RDOOR, s + 120, -1);
    gen_rjamb(T_RJAMB, s);
    gen_rdoor(T_RLOCKED, s + 121, R_GOLD);
    gen_rdoor(T_RLOCKED_S, s + 121, R_IRON);
    gen_rdoor(T_RLOCKED_C, s + 121, R_BLOOD);
    decay_flat(T_RDOOR, dk * 0.5f, s + 150);
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key_trefoil(SP_KEY0 + f, KS[f], R_GOLD); spr_key_trefoil(SP_KEYS0 + f, KS[f], R_IRON); spr_key_trefoil(SP_KEYC0 + f, KS[f], R_BLOOD); }
    specials_on(T_MATV(M1_ASHLAR, 0));
}
static void hollows_run(float dk, uint32_t s, uint32_t field) {
    const RunLook *L = &crawl_looks[2];
    for (int v = 0; v < NVAR; v++) {
        uint32_t vs = v ? s + 1u : 0u;
        gen_granite(T_MATV(M3_GRANITE, v), M3_GRANITE, s, vs);
        gen_mineface(T_MATV(M3_FACE, v), M3_FACE, s + 10, vs);
        gen_timber(T_MATV(M3_TIMBER, v), M3_TIMBER, s + 20, vs, dk);
        gen_basalt(T_MATV(M3_BASALT, v), M3_BASALT, s + 30, vs, R_IRON);
        gen_backfill(T_MATV(M3_FILL, v), M3_FILL, s + 40, vs);
    }
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) decay_hollows(T_MATV(m, v), dk, field, s + 60u + (uint32_t)(m * NVAR + v) * 3u, v);
    for (int k = 0; k < NBLEND; k++) gen_blend(T_BLEND + k, L->blend[k][0], L->blend[k][1], 0, s + 70u + (uint32_t)k);
    gen_feat_ore(T_FEAT + F3_ORE, s + 80);
    gen_feat_fossil(T_FEAT + F3_FOSSIL, s + 81);
    gen_feat_frame(T_FEAT + F3_FRAME, s + 82);
    gen_feat_chute(T_FEAT + F3_CHUTE, s + 83);
    gen_feat_marks(T_FEAT + F3_MARKS, s + 84);
    for (int f = 0; f < 3; f++) gen_feat_lamp(T_FEAT + F3_LAMP + f, f, 11);
    for (int i = 0; i < 5; i++) decay_wall(T_FEAT + i, dk * 0.5f, field, s + 90u + (uint32_t)i * 3u);
    gen_floor_gravel(T_FLOORS + FL3_GRAVEL, s + 100, 0);
    gen_floor_rails(T_FLOORS + FL3_RAILS, s + 101);
    gen_floor_trodden(T_FLOORS + FL3_EARTH, s + 102);
    gen_floor_earth(T_FLOORS + FL3_CHARNEL, s + 103);
    gen_floor_gravel(T_FLOORS + FL3_SCREE, s + 100, 1);
    gen_ceil_lagging(T_CEILS + CE3_LAGGING, s + 110);
    gen_ceil_rock(T_CEILS + CE3_ROCK, s + 111);
    gen_ceil_roots(T_CEILS + CE3_ROOTS, s + 112);
    for (int i = 0; i < NFLOORS; i++) decay_flat(T_FLOORS + i, dk * 0.8f, s + 130u + (uint32_t)i);
    for (int i = 0; i < NCEILS; i++) decay_flat(T_CEILS + i, dk * 0.6f, s + 140u + (uint32_t)i);
    gen_minedoor(T_RDOOR, s + 120, -1);
    gen_timberjamb(T_RJAMB, s);
    gen_minedoor(T_RLOCKED, s + 121, R_GOLD);
    gen_minedoor(T_RLOCKED_S, s + 121, R_IRON);
    gen_minedoor(T_RLOCKED_C, s + 121, R_BLOOD);
    decay_flat(T_RDOOR, dk * 0.5f, s + 150);
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key_tag(SP_KEY0 + f, KS[f], R_GOLD); spr_key_tag(SP_KEYS0 + f, KS[f], R_IRON); spr_key_tag(SP_KEYC0 + f, KS[f], R_BLOOD); }
    spr_nuggets(SP_COINS_A, 0); spr_nuggets(SP_COINS_B, 1);
    spr_rawgems(); spr_strongbox();
    specials_on(T_MATV(M3_FACE, 0));
}
static void deep_run(float dk, uint32_t s, uint32_t field) {
    const RunLook *L = &crawl_looks[3];
    float heat = deep_heat(dk), dark = deep_dark(dk), glow = 1.0f - 0.8f * dark;
    for (int v = 0; v < NVAR; v++) {
        uint32_t vs = v ? s + 1u : 0u;
        gen_obsidian(T_MATV(M4_OBSIDIAN, v), M4_OBSIDIAN, s, vs);
        gen_basalt(T_MATV(M4_BASALT, v), M4_BASALT, s + 10, vs, R_STONE);
        gen_ashrock(T_MATV(M4_ASH, v), M4_ASH, s + 20, vs);
        gen_crystal(T_MATV(M4_CRYSTAL, v), M4_CRYSTAL, s + 30, vs, glow);
        gen_boneglass(T_MATV(M4_BONES, v), M4_BONES, s + 40, vs);
    }
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) decay_deep(T_MATV(m, v), dk, field, s + 60u + (uint32_t)(m * NVAR + v) * 3u, v);
    for (int k = 0; k < NBLEND; k++) gen_blend(T_BLEND + k, L->blend[k][0], L->blend[k][1], 0, s + 70u + (uint32_t)k);
    gen_feat_geode(T_FEAT + F4_GEODE, s + 80, glow);
    gen_feat_seam(T_FEAT + F4_SEAM, s + 81, glow);
    gen_feat_gold(T_FEAT + F4_GOLD, s + 82);
    gen_feat_fused(T_FEAT + F4_FUSED, s + 83);
    gen_feat_frieze(T_FEAT + F4_FRIEZE, s + 84);
    for (int f = 0; f < 3; f++) gen_feat_fissure(T_FEAT + F4_FISSURE + f, f, s + 85, glow);
    for (int i = 0; i < 5; i++) decay_deep(T_FEAT + i, dk * 0.5f, field, s + 90u + (uint32_t)i * 3u, 0);
    gen_floor_glass(T_FLOORS + FL4_GLASS, s + 100, 0, heat);
    gen_floor_throne(T_FLOORS + FL4_THRONE, s + 101);
    gen_floor_ash(T_FLOORS + FL4_ASH, s + 102);
    gen_floor_cinder(T_FLOORS + FL4_CINDER, s + 103, heat, dark);
    gen_floor_glass(T_FLOORS + FL4_SHARDS, s + 100, 1, heat);
    gen_ceil_crystal(T_CEILS + CE4_CRYSTAL, s + 110, glow);           /* after the blends: it borrows the crust's blocks */
    gen_ceil_glint(T_CEILS + CE4_ROCK, s + 111);
    gen_ceil_soot(T_CEILS + CE4_SOOT, s + 112, heat);
    for (int i = 0; i < NFLOORS; i++) decay_flat_deep(T_FLOORS + i, dk * 0.8f, s + 130u + (uint32_t)i);
    for (int i = 0; i < NCEILS; i++) decay_flat_deep(T_CEILS + i, dk * 0.6f, s + 140u + (uint32_t)i);
    gen_irondoor(T_RDOOR, s + 120, -1, glow);
    gen_glassjamb(T_RJAMB, s);
    gen_irondoor(T_RLOCKED, s + 121, R_GOLD, glow);
    gen_irondoor(T_RLOCKED_S, s + 121, R_IRON, glow);
    gen_irondoor(T_RLOCKED_C, s + 121, R_BLOOD, glow);
    decay_flat_deep(T_RDOOR, dk * 0.5f, s + 150);
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key_glass(SP_KEY0 + f, KS[f], R_GOLD); spr_key_glass(SP_KEYS0 + f, KS[f], R_IRON); spr_key_glass(SP_KEYC0 + f, KS[f], R_BLOOD); }
    spr_molten(SP_COINS_A, 0); spr_molten(SP_COINS_B, 1);
    spr_glowgems(); spr_idol();
    specials_on(T_MATV(M4_ASH, 0));
}
static void catacombs_run(float dk, uint32_t s, uint32_t field) {
    const RunLook *L = &crawl_looks[1];
    for (int v = 0; v < NVAR; v++) {
        uint32_t vs = v ? s + 1u : 0u;
        gen_slate(T_MATV(M2_SLATE, v), M2_SLATE, s, vs);
        gen_lining(T_MATV(M2_LINING, v), M2_LINING, s + 10, vs);
        gen_silt(T_MATV(M2_SILT, v), M2_SILT, s + 20, vs);
        gen_flow(T_MATV(M2_FLOW, v), M2_FLOW, s + 30, vs);
        gen_bonesilt(T_MATV(M2_BONES, v), M2_BONES, s + 40, vs);
    }
    for (int m = 0; m < NMAT; m++)
        for (int v = 0; v < NVAR; v++) decay_catacomb(T_MATV(m, v), dk, field, s + 60u + (uint32_t)(m * NVAR + v) * 3u);
    for (int k = 0; k < NBLEND; k++) gen_blend(T_BLEND + k, L->blend[k][0], L->blend[k][1], L->blend[k][0] == M2_FLOW, s + 70u + (uint32_t)k);
    gen_feat_fniche(T_FEAT + F2_NICHE, s + 80);
    gen_feat_saint(T_FEAT + F2_SAINT, s + 81);
    gen_feat_culvert(T_FEAT + F2_CULVERT, s + 82);
    gen_feat_hands(T_FEAT + F2_HANDS, s + 83);
    gen_feat_web(T_FEAT + F2_WEB, s + 84);
    for (int f = 0; f < 3; f++) gen_feat_lantern(T_FEAT + F2_LANTERN + f, f, 11);
    for (int i = 0; i < 4; i++) decay_catacomb(T_FEAT + i, dk * 0.7f, field, s + 90u + (uint32_t)i * 3u);
    gen_floor_wetflags(T_FLOORS + FL2_FLAGS, s + 100, 0);
    gen_floor_silt(T_FLOORS + FL2_SILT, s + 101);
    gen_floor_slateedge(T_FLOORS + FL2_SLATE, s + 102);
    gen_floor_water(T_FLOORS + FL2_WATER, s + 103);
    gen_floor_wetflags(T_FLOORS + FL2_PUDDLES, s + 100, 1);
    gen_ceil_slatevault(T_CEILS + CE2_VAULT, s + 110);
    gen_ceil_bedded(T_CEILS + CE2_ROCK, s + 111);
    gen_ceil_drip(T_CEILS + CE2_DRIP, s + 112);
    for (int i = 0; i < NFLOORS; i++) decay_flat2(T_FLOORS + i, dk, s + 130u + (uint32_t)i, i != FL2_WATER);
    for (int i = 0; i < NCEILS; i++) decay_flat2(T_CEILS + i, dk * 0.7f, s + 140u + (uint32_t)i, 0);
    gen_sluice(T_RDOOR, s + 120, -1);
    gen_slatejamb(T_RJAMB, s);
    gen_sluice(T_RLOCKED, s + 121, R_GOLD);
    gen_sluice(T_RLOCKED_S, s + 121, R_IRON);
    gen_sluice(T_RLOCKED_C, s + 121, R_BLOOD);
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key_ring(SP_KEY0 + f, KS[f], R_GOLD); spr_key_ring(SP_KEYS0 + f, KS[f], R_IRON); spr_key_ring(SP_KEYC0 + f, KS[f], R_BLOOD); }
    spr_coins_silt(SP_COINS_A, 0); spr_coins_silt(SP_COINS_B, 1);
    spr_reliquary(); spr_pearls();
    spr_crown(); spr_drown(SP_CROWN, s + 160); spr_chest(); spr_drown(SP_CHEST, s + 161);
    specials_on(T_MATV(M2_LINING, 0));
    for (int i = 0; i < TS * TS; i++) {                             /* the drains the bile comes from: culverts in the lining */
        uint8_t o = crawl_tex[T_FEAT + F2_CULVERT][i];
        crawl_tex[T_GRATE][i] = o != C_CLEAR ? o : crawl_tex[T_MATV(M2_LINING, 0)][i];
    }
}
/* ---- the trials (1.0.0b13, docs/PLAN-b13.md part D): each run's door, the lights by it, a bier, the hourglass ---- */
static const char *const TR_SKULL[9] = { "  #####  ", " ####### ", "#########", "##oo#oo##", "##oo#oo##", "#########", " ###o### ", "  #####  ", "  # # #  " };
/* a trial's door: the run's own, with its skull and crossed things on it (D1): the Crypts' iron plate and thighbones,
 * the Catacombs' verdigris plate and oars, the Hollows' red ochre and picks, the Deep's black glass skull, lit from
 * within, over gold bones */
static void gen_trdoor(int run) {
    memcpy(crawl_tex[T_TRDOOR], crawl_tex[T_RDOOR], sizeof crawl_tex[0]);
    int t = T_TRDOOR;
    if (run == 0 || run == 1)                                       /* the plate, riveted */
        for (int y = 12; y < 40; y++)
            for (int x = 18; x < 46; x++) {
                int rim = x == 18 || x == 45 || y == 12 || y == 39, rivet = (x == 20 || x == 43) && (y == 14 || y == 37);
                tput(t, x, y, run ? sh(R_MOSS, rim ? 0.3f : 0.45f + 0.12f * h01(x, y, 61)) : sh(R_IRON, rim ? 0.2f : 0.38f + 0.1f * h01(x, y, 61)));
                if (rivet) tput(t, x, y, run ? sh(R_MOSS, 0.75f) : sh(R_IRON, 0.75f));
            }
    for (int k = 0; k < 2; k++)                                     /* the crossed things, behind the skull */
        for (int i = 0; i <= 22; i++) {
            int x = k ? 43 - i : 21 + i, y = 15 + i * 21 / 22;
            uint8_t c = run == 0 ? sh(R_BONE, 0.65f) : run == 1 ? sh(R_WOOD, 0.4f) : run == 2 ? sh(R_BRICK, 0.55f) : sh(R_GOLD, 0.7f);
            tput(t, x, y, c); tput(t, x + (k ? -1 : 1), y, c);
            int end = i < 3 || i > 19;
            if (run == 0 || run == 3) { if (end) { tput(t, x, y - 1, c); tput(t, x, y + 1, c); tput(t, x + (k ? 1 : -1), y, c); } }   /* knobbed ends */
            else if (run == 1 && i > 16) { tput(t, x, y - 1, c); tput(t, x, y + 1, c); tput(t, x, y + 2, c); }   /* the oars' blades */
            else if (run == 2 && i < 4) for (int j = -3; j <= 3; j++) tput(t, x + j * (k ? 1 : -1) / 2 + j, y - abs(j) / 2, c);   /* the picks' heads */
        }
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++) {
            char ch = TR_SKULL[r][c];
            if (ch == ' ') continue;
            uint8_t v = ch == 'o' ? (run == 3 ? (uint8_t)(R_FLAME + 4) : sh(R_IRON, 0.02f))
                      : run == 0 ? sh(R_BONE, 0.7f - 0.04f * r) : run == 1 ? sh(R_BONE, 0.55f - 0.03f * r) : run == 2 ? sh(R_BRICK, 0.6f) : sh(R_IRON, 0.1f + 0.05f * (c < 3));
            tput(t, 28 + c, 20 + r, v);
        }
}
/* the lights by a trial's door, a run each: a red candle on an iron stand; a drowned lantern on a post, burning red;
 * a red lamp hung on a nail; a brazier of black glass, red embers in it. The flames are drawn full bright */
static void spr_trlight(int id, int run) {
    cv_begin(cv_sprite, TS, TS);
    if (run == 3) {
        cv_flat(32, 62, 9, 1.4f, R_WARM + 1);
        for (int y = 46; y < 62; y++) for (int x = 24; x < 41; x++) {
            float w = 8.0f - (float)(61 - y) * 0.15f;
            if (fabsf(x + 0.5f - 32.5f) > w) continue;
            cv_px(x, y, sh(R_IRON, 0.05f + 0.1f * (x < 27) + 0.05f * h01(x, y, 3)));
        }
        for (int x = 25; x < 40; x++) { cv_px(x, 45, (uint8_t)(R_FLAME + 3 + (int)(h01(x, 1, 7) * 4.0f))); if (h01(x, 2, 7) < 0.5f) cv_px(x, 44, (uint8_t)(R_FLAME + 2)); }
        cv_store(id);
        return;
    }
    for (int y = 18; y < 63; y++) { cv_px(31, y, sh(R_IRON, 0.25f)); cv_px(32, y, sh(R_IRON, 0.45f)); }   /* the stand, or post */
    cv_flat(32, 62, 6, 1.2f, sh(R_IRON, 0.2f));
    if (run == 0) {
        cv_flat(32, 19, 5, 1.5f, sh(R_IRON, 0.4f));
        for (int y = 10; y < 19; y++) for (int x = 30; x < 35; x++) cv_px(x, y, sh(R_BLOOD, 0.35f + 0.2f * (x == 30)));
        cv_flat(32, 7, 1.6f, 2.6f, R_FLAME + 3); cv_px(32, 8, R_FLAME + 8);
    } else {
        for (int x = 32; x < 40; x++) cv_px(x, 18, sh(R_IRON, 0.4f));   /* an arm, or a nail */
        int lx = 39;
        cv_line(lx, 18, lx, 22, sh(R_IRON, 0.3f));
        for (int y = 22; y < 33; y++) for (int x = lx - 4; x <= lx + 4; x++) {
            int frame = x == lx - 4 || x == lx + 4 || y == 22 || y == 32;
            cv_px(x, y, frame ? (run == 1 ? sh(R_MOSS, 0.45f) : sh(R_IRON, 0.3f)) : (uint8_t)(R_FLAME + 2 + ((x + y) & 1)));
        }
        if (run == 1) for (int x = lx - 5; x <= lx + 5; x++) cv_px(x, 21, sh(R_MOSS, 0.55f));   /* its cap, gone green */
    }
    cv_store(id);
}
static void spr_tr_prop(int id, int braced) {                      /* a pit prop: leaning loose, or upright and wedged under its cap */
    cv_begin(cv_sprite, TS, TS);
    float lean = braced ? 0.0f : 9.0f;
    cv_capsule(32 - lean * 0.5f, 62, 32 + lean * 0.5f, 10, 2.6f, R_WOOD, 0.2f, 0.55f);
    for (int x = 20; x < 45; x++) for (int y = 6; y < 10; y++) cv_px(x, y, sh(R_WOOD, 0.3f + 0.15f * (y == 6)));   /* its cap */
    if (braced) { float w[6] = { 27, 10, 37, 10, 34, 14 }; cv_poly(w, 3, R_WOOD, 0.6f, 0.4f, 10, 4); cv_line(30, 61, 35, 61, sh(R_STONE, 0.4f)); }
    else for (int k = 0; k < 6; k++) cv_px(24 + k * 3, 12 + (k & 1), sh(R_STONE, 0.4f));   /* grit sifting down */
    cv_outline(sh(R_WOOD, 0.02f));
    cv_store(id);
}
static void spr_tr_bier(void) {                                    /* a bier of grey stone, a pall over it */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 46; y < 64; y++)
        for (int x = 8; x < 56; x++) {
            int top = y < 50, leg = y >= 56 && (x < 13 || x > 50);
            if (y >= 56 && !leg) continue;
            cv_px(x, y, top ? sh(R_BLOOD, 0.2f + 0.1f * (y == 46)) : sh(R_STONE, 0.3f + 0.12f * (x < 11) + 0.05f * h01(x, y, 5)));
        }
    for (int x = 10; x < 54; x += 6) for (int y = 50; y < 54; y++) cv_px(x, y, sh(R_BLOOD, 0.15f));   /* the pall's folds */
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_TR_BIER);
}
void crawl_hourglass(unsigned char *pix, int stride, int x0, int y0, float sand, int state, float t) {
    #define HG(x, y, c) (pix[(y0 + (y)) * stride + x0 + (x)] = (unsigned char)(c))
    sand = state ? 0.0f : clampf(sand, 0.0f, 1.0f);
    for (int x = 0; x < 21; x++) { HG(x, 0, sh(R_WOOD, 0.55f)); HG(x, 1, sh(R_WOOD, 0.35f)); HG(x, 31, sh(R_WOOD, 0.55f)); HG(x, 32, sh(R_WOOD, 0.35f)); }
    for (int y = 2; y < 31; y++) { HG(1, y, sh(R_WOOD, 0.45f)); HG(19, y, sh(R_WOOD, 0.3f)); }   /* its posts */
    for (int r = 0; r < 29; r++) {                                  /* the glass: two bulbs and a neck */
        int y = 2 + r, top = r < 14, d = top ? 13 - r : r - 15;
        float hw = r == 14 ? 0.5f : 0.8f + (float)d * 0.55f;
        if (hw > 7.0f) hw = 7.0f - (float)(d > 11) * 1.5f;
        int w = (int)hw;
        for (int x = 10 - w; x <= 10 + w; x++) {
            int edge = x == 10 - w || x == 10 + w;
            if (state == 1 && edge && ((y * 7 + x) % 5 < 2)) continue;   /* broken: the glass gone in places */
            uint8_t c = edge ? sh(R_GHOST, 0.55f) : sh(R_STONE, 0.06f);
            if (!edge && top && r >= 14 - (int)(sand * 12.0f + 0.5f)) c = sh(R_GOLD, 0.55f + 0.1f * ((x + y) & 1));
            if (!edge && !top && r != 14) {
                float fill = (1.0f - sand) * 12.0f, mound = fill - (float)abs(x - 10) * 0.35f;
                if (state == 1) mound = 1.5f - (float)abs(x - 10) * 0.2f;   /* spilled: a little left in the bottom */
                if ((float)(28 - r) < mound) c = sh(R_GOLD, 0.5f + 0.1f * ((x + y) & 1));
            }
            HG(x, y, c);
        }
    }
    if (state == 0 && sand > 0.0f && sand < 1.0f)                   /* the stream, grains in it */
        for (int y = 16; y < 30; y++) if (((y + (int)(t * 24.0f)) & 3) != 0) HG(10, y, sh(R_GOLD, 0.75f));
    if (state == 2) { HG(7, 8, sh(R_GHOST, 0.9f)); HG(8, 9, sh(R_GHOST, 0.9f)); HG(8, 10, sh(R_GHOST, 0.9f)); HG(9, 11, sh(R_GHOST, 0.9f)); HG(11, 22, sh(R_GHOST, 0.9f)); HG(12, 23, sh(R_GHOST, 0.9f)); }   /* cracked */
    if (state == 1)                                                 /* shards below it */
        for (int k = 0; k < 7; k++) HG(3 + k * 2 + (k & 1), 30 - (k % 3), sh(R_GHOST, 0.7f));
    #undef HG
}

/* A place to mend (docs/PLAN-ai.md, section 28), set in the run's own stone: a votive niche with candles and a saint's
 * skull; a stoup fed by a clean seep; a miners' rest, a lamp on a hook over a cask and a cup; a basin of cold black
 * glass. The flames are drawn full bright. */
static void gen_rest(int run) {
    static const int BASE[4] = { T_MATV(M1_ASHLAR, 0), T_MATV(M2_LINING, 0), T_MATV(M3_FACE, 0), T_MATV(M4_ASH, 0) };
    int t = T_REST;
    run = run < 0 ? 0 : run > 3 ? 3 : run;
    memcpy(crawl_tex[t], crawl_tex[BASE[run]], sizeof crawl_tex[0]);
    if (run == 0) {
        for (int y = 8; y < 52; y++)                                /* the niche, arched */
            for (int x = 16; x < 48; x++) {
                float dx = (x + 0.5f - 32.0f) / 16.0f, dy = (y + 0.5f - 22.0f) / 14.0f;
                if (y < 22 && dx * dx + dy * dy > 1.0f) continue;
                int rim = (y < 22 && dx * dx + dy * dy > 0.82f) || x == 16 || x == 47;
                tput(t, x, y, rim ? sh(R_STONE, 0.42f) : sh(R_STONE, 0.04f + 0.07f * (float)(y - 8) / 44.0f + 0.03f * h01(x, y, 5)));
            }
        for (int x = 15; x < 49; x++) for (int y = 44; y < 48; y++) tput(t, x, y, sh(R_STONE, y == 44 ? 0.62f : 0.4f));   /* the ledge */
        static const char *SK[9] = { "..#####..", ".#######.", "#########", "##o###o##", "##o###o##", "####.####", ".#######.", "..#.#.#..", "..#####.." };
        for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) {
            char ch = SK[r][c];
            if (ch == '.') continue;
            tput(t, 28 + c, 35 + r, ch == 'o' ? sh(R_STONE, 0.02f) : sh(R_BONE, 0.72f - 0.03f * r - 0.04f * (c > 5)));
        }
        static const int CX[3] = { 21, 41, 45 }, CH[3] = { 9, 6, 4 };
        for (int k = 0; k < 3; k++) {                               /* candles, and their wax down the ledge */
            for (int y = 44 - CH[k]; y < 44; y++) { tput(t, CX[k], y, sh(R_BONE, 0.85f)); tput(t, CX[k] + 1, y, sh(R_BONE, 0.6f)); }
            tput(t, CX[k], 43 - CH[k], (uint8_t)(R_FLAME + 15)); tput(t, CX[k], 42 - CH[k], (uint8_t)(R_FLAME + 12));
            tput(t, CX[k] + 1, 43 - CH[k], (uint8_t)(R_FLAME + 11));
            for (int y = 48; y < 48 + 2 + k * 2; y++) tput(t, CX[k] + (k & 1), y, sh(R_BONE, 0.75f));
        }
    } else if (run == 1) {
        for (int y = 2; y < 40; y++) {                              /* the seep: a dark wet streak down from a crack */
            int w = 3 + (y > 20) + (int)(h01(y, 1, 9) * 2.0f);
            for (int x = 32 - w / 2; x < 32 + (w + 1) / 2; x++) {
                uint8_t c = crawl_tex[t][x * TS + y];
                tput(t, x, y, (h01(x, y, 11) > 0.8f) ? (uint8_t)(R_GLOW + 7) : (uint8_t)(c > 2 ? c - 2 : c));
            }
        }
        for (int x = 29; x < 36; x++) tput(t, x, 3, sh(R_STONE, 0.02f));
        for (int y = 38; y < 58; y++)                               /* the stoup, a half bowl out of the wall */
            for (int x = 12; x < 52; x++) {
                float dx = (x + 0.5f - 32.0f) / 20.0f, dy = (y + 0.5f - 38.0f) / 18.0f;
                if (dx * dx + dy * dy > 1.0f) continue;
                float v = 0.36f + 0.25f * (-dx * 0.5f) + 0.25f * (1.0f - dy) + 0.1f * h01(x / 3, y / 3, 13);
                tput(t, x, y, y < 40 ? sh(R_STONE, 0.78f) : sh(R_STONE, v));
            }
        for (int x = 15; x < 49; x++)                               /* clean water over its lip */
            for (int y = 34; y < 38; y++) {
                float dx = (x + 0.5f - 32.0f) / 17.0f, dy = (y + 0.5f - 37.5f) / 3.5f;
                if (dx * dx + dy * dy > 1.0f) continue;
                tput(t, x, y, h01(x, y, 17) > 0.85f ? (uint8_t)(R_GLOW + 12) : sh(R_GLOW, 0.45f + 0.2f * (float)(y - 34) / 4.0f));
            }
    } else if (run == 2) {
        for (int x = 0; x < 64; x++) for (int y = 10; y < 17; y++)  /* a timber across the face */
            tput(t, x, y, sh(R_WOOD, (y == 10 || y == 16) ? 0.18f : 0.32f + 0.12f * h01(x / 5, y, 19) + ((x + 3 * y) % 11 == 0 ? -0.08f : 0.0f)));
        for (int y = 17; y < 24; y++) tput(t, 31, y, sh(R_IRON, 0.55f));   /* the hook, and the lamp on it */
        for (int y = 24; y < 26; y++) for (int x = 28; x < 36; x++) tput(t, x, y, sh(R_IRON, 0.5f));
        for (int y = 26; y < 33; y++) for (int x = 28; x < 36; x++) {
            int frame = x == 28 || x == 35 || y == 32;
            tput(t, x, y, frame ? sh(R_IRON, 0.4f) : (uint8_t)(R_FLAME + 9 + (y > 28 && x > 30 && x < 33 ? 6 : (y > 27 ? 3 : 0))));
        }
        for (int y = 40; y < 60; y++)                               /* a cask on its side */
            for (int x = 14; x < 40; x++) {
                float dx = (x + 0.5f - 27.0f) / 13.0f, dy = (y + 0.5f - 50.0f) / 10.0f;
                if (dx * dx + dy * dy > 1.0f) continue;
                int hoop = x == 18 || x == 19 || x == 35 || x == 36;
                tput(t, x, y, hoop ? sh(R_IRON, 0.35f) : sh(R_WOOD, 0.3f + 0.25f * (1.0f - dy * dy) - 0.08f * ((y & 3) == 0)));
            }
        for (int y = 50; y < 58; y++) for (int x = 44; x < 50; x++)  /* a tin cup */
            tput(t, x, y, (x == 44 || y == 50) ? sh(R_IRON, 0.75f) : sh(R_IRON, 0.5f + 0.1f * (x == 45)));
        for (int y = 52; y < 56; y++) tput(t, 50, y, sh(R_IRON, 0.6f));
    } else {
        for (int y = 30; y < 58; y++)                               /* a basin of black glass */
            for (int x = 10; x < 54; x++) {
                float dx = (x + 0.5f - 32.0f) / 22.0f, dy = (y + 0.5f - 34.0f) / 22.0f;
                if (dx * dx + dy * dy > 1.0f || y < 34) continue;
                float g = h01(x / 2, y / 2, 23);
                uint8_t c = g > 0.93f ? (uint8_t)(R_PURPLE + 9) : sh(R_STONE, 0.03f + 0.05f * (1.0f - dy));
                if ((x + y) % 9 == 0 && y < 46) c = (uint8_t)(R_GLOW + 6);   /* a cold glint */
                tput(t, x, y, y < 36 ? (uint8_t)(R_GLOW + 11) : c);
            }
        for (int x = 14; x < 50; x++)                               /* what is in it: still and cold */
            for (int y = 31; y < 34; y++) tput(t, x, y, ((x * 7 + y) % 13 == 0) ? (uint8_t)(R_GLOW + 13) : sh(R_STONE, 0.02f));
        for (int k = 0; k < 18; k++) {                              /* the cold standing off it */
            int x = 14 + (int)(h01(k, 3, 29) * 36.0f), y = 18 + (int)(h01(k, 5, 29) * 12.0f);
            tput(t, x, y, (uint8_t)(R_GHOST + 9 + (int)(h01(k, 7, 29) * 4.0f)));
        }
    }
}
void crawl_art_run(int level) {
    float dk = crawl_decay(level);
    uint32_t s = 7001u + (uint32_t)level * 7u, field = s + 50u;
    if (crawl_set(level) == 1) catacombs_run(dk, s, field);
    else if (crawl_set(level) == 2) hollows_run(dk, s, field);
    else if (crawl_set(level) == 3) deep_run(dk, s, field);
    else crypts_run(dk, s, field);                                  /* the Crypts, and the runs not drawn yet */
    gen_trdoor(crawl_set(level));                                   /* a trial's door, on the run's own */
    gen_rest(crawl_set(level));                                     /* a place to mend, on the run's own (1.0.0b15) */
}

/* a run's main stone (its first look) and first floor at a level, as crawl_art_run lays them, into wall and floor, without the
 * rest of its art (docs/PLAN-portraits.md: the portraits keep each run's). It draws over those two slots: the caller draws its
 * own level's art again after it. */
void crawl_art_stone(int level, uint8_t *wall, uint8_t *floor) {
    float dk = crawl_decay(level);
    uint32_t s = 7001u + (uint32_t)level * 7u, field = s + 50u;
    g_decay = fmaxf(dk, crawl_depth(level));
    switch (crawl_set(level)) {
    case 1:
        gen_slate(T_MATV(M2_SLATE, 0), M2_SLATE, s, 0u);
        decay_catacomb(T_MATV(M2_SLATE, 0), dk, field, s + 60u + (uint32_t)(M2_SLATE * NVAR) * 3u);
        gen_floor_wetflags(T_FLOORS + FL2_FLAGS, s + 100, 0);
        decay_flat2(T_FLOORS + FL2_FLAGS, dk, s + 130u + FL2_FLAGS, 1);
        memcpy(wall, crawl_tex[T_MATV(M2_SLATE, 0)], TS * TS); memcpy(floor, crawl_tex[T_FLOORS + FL2_FLAGS], TS * TS);
        break;
    case 2:
        gen_granite(T_MATV(M3_GRANITE, 0), M3_GRANITE, s, 0u);
        decay_hollows(T_MATV(M3_GRANITE, 0), dk, field, s + 60u + (uint32_t)(M3_GRANITE * NVAR) * 3u, 0);
        gen_floor_gravel(T_FLOORS + FL3_GRAVEL, s + 100, 0);
        decay_flat(T_FLOORS + FL3_GRAVEL, dk * 0.8f, s + 130u + FL3_GRAVEL);
        memcpy(wall, crawl_tex[T_MATV(M3_GRANITE, 0)], TS * TS); memcpy(floor, crawl_tex[T_FLOORS + FL3_GRAVEL], TS * TS);
        break;
    case 3:
        gen_obsidian(T_MATV(M4_OBSIDIAN, 0), M4_OBSIDIAN, s, 0u);
        decay_deep(T_MATV(M4_OBSIDIAN, 0), dk, field, s + 60u + (uint32_t)(M4_OBSIDIAN * NVAR) * 3u, 0);
        gen_floor_glass(T_FLOORS + FL4_GLASS, s + 100, 0, deep_heat(dk));
        decay_flat_deep(T_FLOORS + FL4_GLASS, dk * 0.8f, s + 130u + FL4_GLASS);
        memcpy(wall, crawl_tex[T_MATV(M4_OBSIDIAN, 0)], TS * TS); memcpy(floor, crawl_tex[T_FLOORS + FL4_GLASS], TS * TS);
        break;
    default:
        gen_ashlar(T_MATV(M1_ASHLAR, 0), M1_ASHLAR, s, 0u);
        decay_wall(T_MATV(M1_ASHLAR, 0), dk, field, s + 60u + (uint32_t)(M1_ASHLAR * NVAR) * 3u);
        gen_floor_flags(T_FLOORS + 0, s + 100, 0);
        decay_flat(T_FLOORS + 0, dk, s + 130u);
        memcpy(wall, crawl_tex[T_MATV(M1_ASHLAR, 0)], TS * TS); memcpy(floor, crawl_tex[T_FLOORS + 0], TS * TS);
        break;
    }
}

void crawl_art_init(void) {
    static int done;
    if (done) return;
    done = 1;
    memset(crawl_dress_ix, 0xFF, sizeof crawl_dress_ix);           /* no dressing until a creature draws its own */
    build_palette();
    build_colormap();
    build_rank_luts();
    gen_stone(T_STONE, R_STONE, 11);
    gen_rubble(T_RUBBLE, 23);
    gen_skulls(T_SKULLS, 31);
    gen_brick(T_BRICK, 47);
    gen_moss(T_MOSS, 53);
    gen_niche(T_NICHE, 11);
    gen_banner(T_BANNER, 11);
    for (int f = 0; f < 3; f++) gen_torch(T_TORCH0 + f, f, 11);
    gen_door(T_DOOR, 71);
    gen_jamb(T_JAMB, 79);
    gen_locked(T_LOCKED, 83, R_GOLD);
    gen_locked(T_LOCKED_S, 85, R_IRON);
    gen_locked(T_LOCKED_C, 87, R_BLOOD);
    gen_sign(T_SIGN, 91);
    gen_gate(T_GATE, 97); gen_gate_seal(T_GATE_SEAL, 97); gen_ice(T_ICE, 829);
    gen_font(T_FONT, 101, 1); gen_font(T_FONT_DRY, 101, 0);
    gen_grate(T_GRATE, 103);
    hz_walls();
    for (int f = 0; f < 4; f++) gen_portal(T_PORTAL0 + f, f, 107);
    gen_sealed(T_SEALED);
    gen_repository(T_REPOSITORY);
    gen_exit(T_EXIT0, 0, 0);
    gen_exit(T_EXIT1, 1, 0);
    gen_exit(T_EXIT_OPEN, 1, 1);
    gen_passage(T_PASSAGE, 1);
    for (int k = 0; k < 3; k++) gen_paper(T_PAPER + k, k, 301);
    gen_bdoor(T_BDOOR, 301);
    for (int f = 0; f < 4; f++) gen_visportal(T_VISPORT0 + f, f, 301);
    gen_carpet(T_CARPET, 307);
    for (int k = 0; k < 4; k++) gen_ceiling(T_CEILTILE + k, k, 311);
    for (int k = 0; k < 3; k++) gen_white(T_WHITE + k, k, 401);
    gen_sdoor(T_SDOOR, 401);
    gen_scarpet(T_SCARPET, 407);
    gen_sceiling(T_SCEIL, 0, 411); gen_sceiling(T_SCEILLAMP, 1, 411);

    props_alts();
    spr_skeleton(SP_SKEL_A, 1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_B, -1.0f, 0.0f, 3, 0);
    spr_skeleton(SP_SKEL_D1, 0.0f, 0.35f, 3, 0);
    spr_skeleton(SP_SKEL_D2, 0.0f, 0.7f, 3, 0);
    spr_skeleton(SP_SKEL_D3, 0.0f, 1.0f, 3, 0);
    spr_spectre(SP_GHOST_A, 0.0f, 0);
    spr_spectre(SP_GHOST_B, PI_F, 0);
    spr_spectre(SP_GHOST_D1, 1.0f, 1);
    spr_spectre(SP_GHOST_D2, 2.5f, 1);
    spr_bat(SP_BAT_A, 0);
    spr_bat(SP_BAT_B, 1);
    spr_bat(SP_BAT_D, 2);
    bat_dress(SP_BAT_A, 1.0f); bat_dress(SP_BAT_B, -1.0f);
    spr_bat_roost(SP_BAT_ROOST, 0); spr_bat_roost(SP_BAT_ROOST2, 1);   /* 1.0.0b18 */
    spr_fire(SP_FIRE_A, 0);
    spr_fire(SP_FIRE_B, 1);
    for (int f = 0; f < 3; f++) spr_boom(SP_BOOM1 + f, f);
    spr_skeleton(SP_SKEL_T, 0.3f, 0.0f, 3, 1);
    spr_shieldskel(SP_SHIELD_A, 1.0f, 0, 1); spr_shieldskel(SP_SHIELD_B, -1.0f, 0, 1); spr_shieldskel(SP_SHIELD_S, 0.2f, 1, 1);
    spr_shieldskel(SP_SHIELD_XA, 1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XB, -1.0f, 0, 0); spr_shieldskel(SP_SHIELD_XS, 0.2f, 1, 0);
    for (int h = 0; h < 4; h++) { spr_wskull(SP_WSKULL0 + h, h); if (h < 3) wskull_dress(SP_WSKULL0 + h); }
    for (int f = 0; f < 3; f++) { spr_spider(SP_SPIDER_A + f, f); spider_dress(SP_SPIDER_A + f); }
    spr_spider(SP_SPIDER_D, 3);
    spr_ball(SP_SKULLFIRE_A, 10.0f, R_FLAME, R_BLOOD, 0, 1); spr_ball(SP_SKULLFIRE_B, 10.5f, R_FLAME, R_BLOOD, 1, 1);
    spr_ball(SP_HELL_A, 12.0f, R_FLAME, R_BLOOD, 0, 0); spr_ball(SP_HELL_B, 12.5f, R_FLAME, R_BLOOD, 1, 0);
    spr_frost(); spr_spear(); spr_wisp(); spr_spark(); spr_flask();
    spr_burn(SP_BURN_A, 0); spr_burn(SP_BURN_B, 1);
    spr_soulurn(); spr_ringcushion(); spr_pedestal(); spr_items(); spr_spearbone(); spr_altarfire(); spr_double();
    spr_shrine_body(0); spr_shrine_body(1); spr_shrine_glow(0); spr_shrine_glow(1);
    for (int k = 0; k <= 9; k++) spr_srune(k);
    spr_ccoin();
    spr_scrip(); spr_hextotem();
    spr_grit(SP_GRIT0, 0); spr_grit(SP_GRIT1, 1); spr_rock(); spr_rubble(); spr_signs();
    spr_erupt(SP_ERUPT0, 0); spr_erupt(SP_ERUPT1, 1); spr_voidcol(); spr_hands();
    spr_desks(); spr_cooler(SP_COOLER, 0); spr_cooler(SP_COOLER_LOW, 1);
    for (int f = 0; f < 4; f++) spr_bonefly(SP_BONEFLY0 + f, f * PI_F * 0.25f);
    spr_coins(SP_COINS_A, 0);
    spr_coins(SP_COINS_B, 1);
    spr_chalice();
    spr_gems();
    spr_crown();
    spr_chest();
    static const float KS[4] = { 1.0f, 0.5f, 0.12f, -0.5f };
    for (int f = 0; f < 4; f++) { spr_key(SP_KEY0 + f, KS[f], R_GOLD); spr_key(SP_KEYS0 + f, KS[f], R_IRON); spr_key(SP_KEYC0 + f, KS[f], R_BLOOD); }
    spr_phial();
    spr_tomb(SP_TOMB_FRESH, 4);                                     /* her plot's newest (the four kinds are props_level's) */
    spr_lectern(); spr_mapware(); spr_lampstand();
    for (int k = 0; k < 3; k++) spr_mk_rseal(SP_MK_RSEAL0 + k, k); spr_mapfloor();   /* the tutorial's (docs/PLAN-tutorial.md) */
    spr_mk_pricket(SP_MK_PRICKET0, 0); spr_mk_pricket(SP_MK_PRICKET1, 1); spr_mk_coffer(SP_MK_COFFER0, 0); spr_mk_coffer(SP_MK_COFFER1, 1);   /* 1.0.0b16 */
    spr_mk_cage(SP_MK_CAGE0, 0); spr_mk_cage(SP_MK_CAGE1, 1); spr_mk_rack(SP_MK_RACK0, 0); spr_mk_rack(SP_MK_RACK1, 1);
    spr_mk_urn(SP_MK_URN0, 0); spr_mk_urn(SP_MK_URN1, 1); spr_mk_coil(SP_MK_COIL0, 0); spr_mk_coil(SP_MK_COIL1, 1);
    spr_mk_vine(SP_MK_VINE0, 0); spr_mk_vine(SP_MK_VINE1, 1); spr_mk_lantern(SP_MK_LANTERN0, 0); spr_mk_lantern(SP_MK_LANTERN1, 1);
    spr_mk_barricade(SP_MK_BARRICADE0, 0); spr_mk_barricade(SP_MK_BARRICADE1, 1);
    spr_mk_corpselight(SP_MK_CLIGHT0, 0); spr_mk_corpselight(SP_MK_CLIGHT1, 1); spr_mk_rollskull(SP_MK_RSKULL0, 0); spr_mk_rollskull(SP_MK_RSKULL1, 1);
    for (int k = 0; k < 3; k++) spr_mk_heatseal(SP_MK_HSEAL0 + k, k);
    spr_mk_bell(SP_MK_BELL0, 0); spr_mk_bell(SP_MK_BELL1, 1); spr_mk_winch(SP_MK_WINCH0, 0); spr_mk_winch(SP_MK_WINCH1, 1);
    spr_mk_hearth(SP_MK_HEARTH0, 0); spr_mk_hearth(SP_MK_HEARTH1, 1); spr_mk_vent(SP_MK_VENT0, 0); spr_mk_vent(SP_MK_VENT1, 1);
    spr_mk_cart(SP_MK_CART0, 0); spr_mk_cart(SP_MK_CART1, 1); spr_mk_lodestone(SP_MK_LODE0, 0); spr_mk_lodestone(SP_MK_LODE1, 1); spr_mk_ice(SP_MK_ICE);
    spr_mk_tablet(SP_MK_TABLET); spr_mk_plinth(SP_MK_PLINTH0, 0); spr_mk_plinth(SP_MK_PLINTH1, 1);
    for (int k = 0; k < 3; k++) { spr_mk_reliq(SP_MK_RELIQ0 + k, k); spr_mk_relay(SP_MK_RELAY0 + k, k); }
    spr_mk_wardstone(SP_MK_WSTONE0, 0); spr_mk_wardstone(SP_MK_WSTONE1, 1); spr_mk_cask(SP_MK_CASK0, 0); spr_mk_cask(SP_MK_CASK1, 1);
    spr_mk_pillar(SP_MK_PILLAR0, 0); spr_mk_pillar(SP_MK_PILLAR1, 1); spr_mk_pfont(SP_MK_PFONT0, 0); spr_mk_pfont(SP_MK_PFONT1, 1);
    spr_mk_drum(SP_MK_DRUM0, 0); spr_mk_drum(SP_MK_DRUM1, 1);
    for (int k = 0; k < 4; k++) { spr_pbell(SP_PBELL0 + k, k, 0); spr_pbell(SP_PBELL0R + k, k, 1); }   /* 1.0.0b18.1 */
    spr_bell_dead();
    for (int r = 0; r < 4; r++) for (int s = 0; s < 3; s++) spr_wlamp(SP_WLAMP0 + 3 * r + s, r, s);
    spr_orb(SP_ORB_A, 0); spr_orb(SP_ORB_B, 1); spr_lance(); spr_ballista(); spr_fspirit(SP_FSPIRIT_A, 0); spr_fspirit(SP_FSPIRIT_B, 1); spr_rime(); spr_mote();
    spr_deadbell(SP_DBELL, 0); spr_deadbell(SP_DBELL_C, 1); spr_font(SP_FONT, 0); spr_font(SP_FONT_C, 1);
    spr_censer(); spr_puff(SP_SMOKE, R_STONE, 0.25f, 0.6f, 81); spr_puff(SP_VAPOUR, R_MOSS, 0.2f, 0.55f, 83); spr_puff(SP_GAS, R_WARM, 0.35f, 0.75f, 87);
    spr_votive(); spr_hz_reliq(SP_RELIQ, 0); spr_hz_reliq(SP_RELIQ_O, 1); spr_snare(); spr_spikes(); spr_ashfall();
    spr_pz_skull(); spr_pz_plinth(); spr_pz_tablet();             /* puzzles' things (1.0.0b13) */
    for (int k = 0; k < 8; k++) spr_pz_dial(SP_PZ_DIAL0 + k, k);
    spr_pz_lever(SP_PZ_LEVER0, 0); spr_pz_lever(SP_PZ_LEVER1, 1); spr_pz_candle(SP_PZ_CANDLE0, 0); spr_pz_candle(SP_PZ_CANDLE1, 1);
    for (int k = 0; k < 16; k++) spr_pz_mark(SP_MARK0 + k, k);
    for (int n = 1; n <= 9; n++) spr_pz_pips(SP_PIP1 + n - 1, n);
    spr_pz_niche(); for (int k = 0; k < 4; k++) spr_pz_effigy(SP_PZ_EFFIGY0 + k, k);
    spr_pz_pan(); for (int k = 0; k < 3; k++) spr_pz_scale(SP_PZ_SCALE0 + k, k);
    spr_pz_casket(); spr_pz_banner();
    for (int k = 0; k < 9; k++) spr_pz_cistern(SP_PZ_CISTERN0 + k, k);
    spr_pz_saint(); spr_pz_emblem(); spr_pz_spout(); spr_pz_mill(SP_PZ_MILL0, 0); spr_pz_mill(SP_PZ_MILL1, 1); spr_pz_idol(); spr_pz_lock(); spr_pz_wheel();
    spr_pz_board(); spr_pz_box(); spr_pz_sack(); spr_pz_hook(); spr_pz_tag(); spr_pz_socket(); spr_pz_fossil(); spr_pz_stone();
    spr_pz_lamp(SP_PZ_LAMP0, 0); spr_pz_lamp(SP_PZ_LAMP1, 1); for (int k = 0; k < 4; k++) spr_pz_prism(SP_PZ_PRISM0 + k, k);
    spr_pz_geode(SP_PZ_GEODE, 0); spr_pz_geode(SP_PZ_GEODE1, 1); spr_pz_throne(); spr_pz_statue(); spr_pz_vent(SP_PZ_VENT0, 0); spr_pz_vent(SP_PZ_VENT1, 1);
    spr_pz_slab(); spr_pz_numstone(); for (int k = 0; k < 26; k++) spr_pz_letter(SP_PZ_LETTER0 + k, k);
    for (int k = 0; k < 4; k++) spr_hide(SP_HIDE0 + k, k);
    spr_bshrine(); spr_bshrine_eyes(SP_BSHRINE_E0, 0); spr_bshrine_eyes(SP_BSHRINE_E1, 1);
    for (int k = 0; k < 4; k++) spr_trlight(SP_TRLIGHT0 + k, k);
    spr_tr_bier();
    spr_tr_prop(SP_TR_PROP0, 0); spr_tr_prop(SP_TR_PROP1, 1);
    spr_shambler(SP_SHAMB_A, 1.0f, 0); spr_shambler(SP_SHAMB_B, -1.0f, 0); spr_shambler(SP_SHAMB_GRAB, 0.0f, 1);
    shambler_dress(SP_SHAMB_A, 1.0f, 0); shambler_dress(SP_SHAMB_B, -1.0f, 0); shambler_dress(SP_SHAMB_GRAB, 0.0f, 1);
    g_back = 1; spr_shambler(SP_SHAMB_BKA, -1.0f, 0); spr_shambler(SP_SHAMB_BKB, 1.0f, 0); g_back = 0;   /* 1.0.0b18: its back */
    shambler_dress(SP_SHAMB_BKA, -1.0f, 0); shambler_dress(SP_SHAMB_BKB, 1.0f, 0); spr_turn(SP_SHAMB_BKA); spr_turn(SP_SHAMB_BKB);
    spr_shambler(SP_SHAMB_RISE1, 0.0f, 2); spr_shambler(SP_SHAMB_RISE2, 0.0f, 3); spr_shambler(SP_SHAMB_D1, 0.0f, 4); spr_shambler(SP_SHAMB_D2, 0.0f, 5);
    spr_drowned(SP_DROWN_A, 1.0f, 0); spr_drowned(SP_DROWN_B, -1.0f, 1); spr_drowned(SP_DROWN_GRAB, 0.0f, 2);
    drowned_dress(SP_DROWN_A, 0); drowned_dress(SP_DROWN_B, 0); drowned_dress(SP_DROWN_GRAB, 1);
    g_back = 1; spr_drowned(SP_DROWN_BKA, -1.0f, 0); spr_drowned(SP_DROWN_BKB, 1.0f, 1); g_back = 0;   /* 1.0.0b18: its back */
    drowned_dress(SP_DROWN_BKA, 0); drowned_dress(SP_DROWN_BKB, 0); spr_turn(SP_DROWN_BKA); spr_turn(SP_DROWN_BKB);
    spr_drowned(SP_DROWN_RISE, 0.0f, 3); spr_drowned(SP_DROWN_BURST, 0.0f, 4); spr_drowned(SP_DROWN_POOL, 0.0f, 5);
    spr_ghoul(SP_GHOUL_A, 1.0f, 0); spr_ghoul(SP_GHOUL_B, -1.0f, 1); spr_ghoul(SP_GHOUL_LEAP, 0.0f, 2); spr_ghoul(SP_GHOUL_FEED, 0.0f, 3); spr_ghoul(SP_GHOUL_D, 0.0f, 4);
    spr_ghoul_back(SP_GHOUL_BKA, 1.0f); spr_ghoul_back(SP_GHOUL_BKB, -1.0f);   /* 1.0.0b18: its back */
    g_back = 1; ghoul_dress(SP_GHOUL_BKA, 0.0f, 0.0f); ghoul_dress(SP_GHOUL_BKB, 0.0f, 0.0f); g_back = 0;
    ghoul_dress(SP_GHOUL_A, 0.0f, 0.0f); ghoul_dress(SP_GHOUL_B, 0.0f, 0.0f); ghoul_dress(SP_GHOUL_LEAP, -10.0f, 0.0f); ghoul_dress(SP_GHOUL_FEED, 0.0f, 1.0f);
    spr_bestiary_bones();
    for (int f = 0; f < 4; f++) { spr_hunter(SP_HUNTER_A + f, f); if (f < 3) hunter_dress(SP_HUNTER_A + f, f); spr_brood(SP_BROOD_A + f, f); if (f < 3) brood_dress(SP_BROOD_A + f, f); }
    g_back = 1;                                                     /* 1.0.0b18: their backs */
    spr_hunter(SP_HUNTER_BKA, 0); spr_hunter(SP_HUNTER_BKB, 1); spr_brood(SP_BROOD_BKA, 0); spr_brood(SP_BROOD_BKB, 1);
    g_back = 0;
    hunter_dress(SP_HUNTER_BKA, 0); hunter_dress(SP_HUNTER_BKB, 1); brood_dress(SP_BROOD_BKA, 0); brood_dress(SP_BROOD_BKB, 1);
    /* docs/PLAN-combat.md section 17: each track draws its new sprites only above its own end mark */
    spr_shield_charge(SP_SHIELD_RAISE, 0); spr_shield_charge(SP_SHIELD_RUSH, 1);   /* C4: the charge */
    spr_drowned(SP_DROWN_SWELL, 0.0f, 6); spr_drowned(SP_DROWN_SPEW, 0.0f, 7);
    drowned_dress(SP_DROWN_SWELL, 0); drowned_dress(SP_DROWN_SPEW, 0);
    spr_spew(SP_SPEW_A, 0); spr_spew(SP_SPEW_B, 1); spr_dust(SP_DUST_A, 0); spr_dust(SP_DUST_B, 1);
    /* the caster's track (C) ends */
    /* the dead's track (D): */
    /* the dead's track (D) ends */
    for (int f = 0; f < 3; f++) spr_bspider(SP_BSPIDER_A + f, f);   /* the Wandering Spider's own */
    spr_bspider(SP_BSPIDER_REAR, 3); spr_bspider(SP_BSPIDER_D, 4);
    for (int f = 0; f < 3; f++) spr_sac(SP_SAC_A + f, f);
    spr_ling(SP_LING_A, 0); spr_ling(SP_LING_B, 1);
    spr_wraith_big(SP_WRAITH_A, 0.0f, 0); spr_wraith_big(SP_WRAITH_B, PI_F, 1); spr_wraith_big(SP_WRAITH_REACH, 1.0f, 2); spr_wraith_big(SP_WRAITH_BACK, 2.0f, 3);
    spr_shade(SP_SHADE_A, 0.0f, 0); spr_shade(SP_SHADE_B, PI_F, 1); spr_shade(SP_SHADE_TOUCH, 1.0f, 2); spr_shade(SP_SHADE_D, 2.0f, 3);
    for (int f = 0; f < 4; f++) { spr_imp(SP_IMP_A + f, f); if (f < 3) imp_dress(SP_IMP_A + f); }
    g_back = 1; spr_imp(SP_IMP_BKA, 0); spr_imp(SP_IMP_BKB, 1); g_back = 0;   /* 1.0.0b18: its back */
    imp_dress(SP_IMP_BKA); imp_dress(SP_IMP_BKB); spr_turn(SP_IMP_BKA); spr_turn(SP_IMP_BKB);
    spr_ember(SP_EMBER_A, 0); spr_ember(SP_EMBER_B, 1);
    for (int f = 0; f < 5; f++) { spr_gargoyle(SP_GARG_PERCH + f, f); if (f >= 1 && f <= 3) garg_dress(SP_GARG_PERCH + f); }
    g_back = 1; spr_gargoyle(SP_GARG_BKA, 1); spr_gargoyle(SP_GARG_BKB, 2); g_back = 0;   /* 1.0.0b18: its back, in flight */
    garg_dress(SP_GARG_BKA); garg_dress(SP_GARG_BKB); spr_turn(SP_GARG_BKA); spr_turn(SP_GARG_BKB);
    for (int f = 0; f < 3; f++) spr_magma(SP_MAGMA_A + f, f);
    spr_carcass(); spr_armourstand(); spr_helm(); spr_timber();
    spr_minelamp(SP_MINELAMP_A, 0); spr_minelamp(SP_MINELAMP_B, 1); spr_cart(); spr_fungus();
    spr_crystal(SP_CRYSTAL_A, 0); spr_crystal(SP_CRYSTAL_B, 1); spr_obsidian();
    spr_altar(SP_ALTAR_A, 0); spr_altar(SP_ALTAR_B, 1);
    spr_sides();
    find_glints();
}
