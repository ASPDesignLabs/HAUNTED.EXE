/* The new hanging lights and the events' things, as sheets for the owner (docs/PLAN-lights.md, step 2; docs/PLAN-events.md). The owner
 * approved both; the drawings are the art's now (src/art/parts/lights_events.c: SP_CORONA0 ... SP_EV_LEVER1, T_PLATE ... T_UNBARRED,
 * made each level by crawl_art_lights), and this draws the sheets from the game's own slots, byte for byte as they were approved. The
 * art is #included whole, as before, for its palette and slots.
 *
 *   build/lightmock lights media/sheets/lights/hanging_lights.png
 *   build/lightmock things media/sheets/events/event_things.png
 *   build/lightmock relics media/sheets/events/relics.png
 *
 * The twelve relics' looks were approved too, and are the art's in the same way (lights_events.c: relic_draw, RELIC_CUT, SP_RL1 ...
 * SP_RLB11); their sheet is drawn from those slots, byte for byte as approved.
 */
#include "crawl_art.c"
#include <stdio.h>
#include <zlib.h>

/* an event's plate, the art's overlay laid over a floor (the cell's own, in the game) */
static void plate_on(uint8_t *t, const uint8_t *floor, int pressed) {
    const uint8_t *o = crawl_tex[pressed ? T_PLATE_DOWN : T_PLATE];
    for (int i = 0; i < TS * TS; i++) t[i] = o[i] != C_CLEAR ? o[i] : floor[i];
}

/* what the game draws today for a plate (draw/floor.c, FM_PLATE): the floor, its edge cut deep; sunk, darker */
static void today_plate(uint8_t *t, const uint8_t *floor, int pressed) {
    for (int x = 0; x < TS; x++)
        for (int y = 0; y < TS; y++) {
            float fx = ((float)x + 0.5f) / TS, fy = ((float)y + 0.5f) / TS, e = fminf(fminf(fx, 1.0f - fx), fminf(fy, 1.0f - fy));
            uint8_t c = floor[x * TS + y];
            if (e < 0.06f) c = R_STONE + 1;
            else if (e < 0.1f) c = (uint8_t)(R_STONE + (pressed ? 4 : 10));
            else if (pressed) { int ramp = (c - 1) / 16 * 16 + 1; c = (uint8_t)(ramp + clampi((int)((float)(c - ramp) * 0.65f), 1, 15)); }
            t[x * TS + y] = c;
        }
}

/* a relic's look from the game's slots (2d): lying, and held in its socket. The bone key's are the events' things' own, but for one
 * cell: the sheet drew the key held as it drew the other eleven (spr_socket_k: the panel filled again, the relic outlined over the
 * hands), 76 pixels from the game's SP_EV_SOCKET1, which stays as it was; so that cell is drawn by the art's own hand, not its slot */
static void relic_from(uint8_t *out, int k, int held) {
    if (held && k == RL_BONEKEY) { spr_socket_k(out, RL_BONEKEY); return; }
    memcpy(out, crawl_spr[held ? (k ? SP_RLH1 + k - 1 : SP_EV_SOCKET1) : (k ? SP_RL1 + k - 1 : SP_EV_RELIC)], TS * TS);
}

/* ================================================================== */
/* The sheet                                                           */
/* ================================================================== */

static int iw, ih;
static uint8_t *img;
static void put(int x, int y, int r, int g, int b) { if (x < 0 || y < 0 || x >= iw || y >= ih) return; uint8_t *p = img + ((size_t)y * iw + x) * 3; p[0] = r; p[1] = g; p[2] = b; }
static void rect(int x, int y, int w, int h, int r, int g, int b) { for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) put(xx, yy, r, g, b); }
static void text(int x, int y, int sc, const char *s, int r, int g, int b) {
    for (; *s; s++, x += 8 * sc)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (font_bios8[(uint8_t)*s][row] >> (7 - col) & 1) rect(x + col * sc, y + row * sc, sc, sc, r, g, b);
}
#define INK 216, 208, 189
#define GOLDINK 233, 194, 76
#define DIMINK 150, 140, 128
static void pal_put(int x, int y, uint8_t c) { put(x, y, crawl_pal[c][0] * 255 / 63, crawl_pal[c][1] * 255 / 63, crawl_pal[c][2] * 255 / 63); }

static const uint8_t BAYER4[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static uint8_t shadeb(uint8_t c, float light, int x, int y) {   /* as draw/floor.c's shade */
    if (IS_FULLBRIGHT(c)) return c;
    int lv = (int)(light * LIGHT_UNIT + BAYER4[y & 3][x & 3] * (1.0f / 16.0f));
    return crawl_cmap[clampi(lv, 0, NLIGHT - 1)][c];
}

/* a cell enlarged: a backdrop (the run's wall over its floor), and over it a sprite or a face, at a light */
static void cell(int ox, int oy, int sc, const uint8_t *wall, const uint8_t *floor, const uint8_t *spr, const uint8_t *face, float light) {
    int n = TS * sc;
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            int tx = x / sc, ty = y / sc;
            uint8_t c;
            if (face) c = face[tx * TS + ty];
            else {
                c = ty < 52 ? wall[tx * TS + ty] : floor[tx * TS + ((ty - 52) * 5) % TS];
                float l = light * (ty < 52 ? 0.8f : 0.9f);
                c = crawl_cmap[clampi((int)(l * LIGHT_UNIT), 0, NLIGHT - 1)][c];
                if (spr && spr[tx * TS + ty] != C_CLEAR) c = spr[tx * TS + ty];
                else { pal_put(ox + x, oy + y, c); continue; }
            }
            pal_put(ox + x, oy + y, IS_FULLBRIGHT(c) ? c : crawl_cmap[clampi((int)(light * LIGHT_UNIT), 0, NLIGHT - 1)][c]);
        }
}

/* the game's view at its own size (320x200's pixels, FOCAL as the game's): a back wall at wd cells, the floor and vault
 * between; a sprite on the floor at sd cells, or a face in the wall's middle cell, or a texture on the floor's cell under the
 * sprite's place. The light: amb everywhere and a light at (lx, ly) of pw falling off over lr cells; dapple throws it
 * through a pierced lantern's holes. */
typedef struct {
    const uint8_t *wall, *floor, *ceil, *spr, *face, *flat;
    float wd, sd, amb, lx, ly, lz, pw, lr; int dapple;
} View;
static float view_light(const View *v, float x, float y, float z) {
    float dx = x - v->lx, dy = y - v->ly, dz = z - v->lz, d2 = dx * dx + dy * dy + dz * dz * 0.5f;
    float l = v->pw * expf(-d2 / (v->lr * v->lr));
    if (v->dapple) {                                                 /* the holes' light: spots on what it falls on */
        float a = atan2f(dy, dx), e = atan2f(dz, sqrtf(dx * dx + dy * dy));
        float s = 0.5f + 0.5f * sinf(a * 14.0f) * sinf(e * 12.0f + 1.0f);
        l *= 0.35f + 1.1f * (s > 0.62f ? 1.0f : s * 0.5f);
    }
    return v->amb + l;
}
static void view(int ox, int oy, int w, int h, const View *v) {
    const float F = (320 / 2) / 0.75f, camz = 0.5f, hz = (float)h * 0.5f;
    float top = hz - (1.0f - camz) * F / v->wd, bot = hz + camz * F / v->wd;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float cxp = (float)x + 0.5f - (float)w * 0.5f;
            uint8_t c; float l;
            if ((float)y >= top && (float)y < bot) {                 /* the back wall */
                float wx = cxp * v->wd / F + 0.5f, wy = ((float)y - top) / (bot - top);
                int tx = (int)floorf((wx - floorf(wx)) * TS), ty = clampi((int)(wy * TS), 0, TS - 1), mid = wx >= 0.0f && wx < 1.0f;
                c = (mid && v->face ? v->face : v->wall)[tx * TS + ty];
                l = view_light(v, wx - 0.5f, v->wd, 1.0f - wy) * (1.0f / (1.0f + 0.01f * v->wd * v->wd));
            } else {
                int fl = (float)y >= bot;
                float dist = (fl ? camz : 1.0f - camz) * F / fabsf((float)y + 0.5f - hz);
                float wx = cxp * dist / F + 0.5f, wy = dist;
                int tx = (int)floorf((wx - floorf(wx)) * TS) & 63, ty = (int)floorf((wy - floorf(wy)) * TS) & 63;
                const uint8_t *t = fl ? v->floor : v->ceil;
                c = t[tx * TS + ty];
                if (fl && v->flat && wx >= 0.0f && wx < 1.0f && wy >= v->sd - 0.5f && wy < v->sd + 0.5f) {
                    uint8_t k = v->flat[tx * TS + ((int)floorf((v->sd + 0.5f - wy) * TS) & 63)];
                    if (k != C_CLEAR) c = k;
                }
                l = view_light(v, wx - 0.5f, wy, fl ? 0.0f : 1.0f) * (1.0f / (1.0f + 0.01f * dist * dist)) * (fl ? 1.0f : 0.85f);
            }
            pal_put(ox + x, oy + y, shadeb(c, l, x, y));
        }
    if (v->spr) {                                                    /* the sprite, a cell high, on the floor at sd */
        float size = F / v->sd, sx0 = (float)w * 0.5f - size * 0.5f, sy1 = hz + camz * F / v->sd, sy0 = sy1 - size;
        float l = view_light(v, 0.0f, v->sd, 0.5f) * (1.0f / (1.0f + 0.01f * v->sd * v->sd));
        for (int y = (int)ceilf(sy0); y < (int)ceilf(sy1); y++)
            for (int x = (int)ceilf(sx0); x < (int)ceilf(sx0 + size); x++) {
                if (x < 0 || y < 0 || x >= w || y >= h) continue;
                int tx = clampi((int)(((float)x + 0.5f - sx0) / size * TS), 0, TS - 1), ty = clampi((int)(((float)y + 0.5f - sy0) / size * TS), 0, TS - 1);
                uint8_t c = v->spr[tx * TS + ty];
                if (c != C_CLEAR) pal_put(ox + x, oy + y, shadeb(c, l, x, y));
            }
    }
}

static void put32(FILE *f, uint32_t v) { uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v }; fwrite(b, 1, 4, f); }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    put32(f, len); fwrite(type, 1, 4, f); if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)type, 4); if (len) crc = crc32(crc, data, len); put32(f, crc);
}
static int write_png(const char *path) {
    size_t rowlen = 1 + (size_t)iw * 3;
    uint8_t *raw = malloc(rowlen * ih);
    for (int y = 0; y < ih; y++) { raw[rowlen * y] = 0; memcpy(raw + rowlen * y + 1, img + (size_t)y * iw * 3, (size_t)iw * 3); }
    uLongf clen = compressBound((uLong)(rowlen * ih)); uint8_t *comp = malloc(clen);
    compress2(comp, &clen, raw, (uLong)(rowlen * ih), 9);
    FILE *f = fopen(path, "wb"); if (!f) { free(raw); free(comp); return 0; }
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 }; fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = { iw >> 24, iw >> 16, iw >> 8, iw, ih >> 24, ih >> 16, ih >> 8, ih, 8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13); chunk(f, "IDAT", comp, (uint32_t)clen); chunk(f, "IEND", NULL, 0);
    fclose(f); free(raw); free(comp); return 1;
}

static const int RUNLEVEL[4] = { 3, 13, 23, 33 };
static const char *const RUNA[4] = { "THE", "THE", "THE", "THE" };
static const char *const RUNB[4] = { "CRYPTS", "CATACOMBS", "HOLLOWS", "DEEP" };
static void run_art(int r) { crawl_art_level(RUNLEVEL[r]); }
static void text_lines(int x, int y, int sc, const char *const *l, int n) { for (int i = 0; i < n; i++) text(x, y + i * 10 * sc, sc, l[i], INK); }

/* ---- sheet 1: the four hanging lights ---- */
#define SC 2
#define CW (TS * SC)
#define PAD 8
#define LAB 168
#define VW 200
#define VH 128
#define VH2 160
#define LIT 1.15f
#define DARK 0.55f
static uint8_t LT[4][4][3][TS * TS];                     /* [light][run][frame] */
static const int NFR[4] = { 3, 2, 3, 3 };
static void make_lights(int r) {
    run_art(r);
    for (int f = 0; f < 3; f++) {
        memcpy(LT[0][r][f], crawl_spr[SP_CORONA0 + f], TS * TS);       /* the game's own, made for the run's level */
        if (f < 2) memcpy(LT[1][r][f], crawl_spr[SP_SANCT0 + f], TS * TS);
        memcpy(LT[2][r][f], crawl_spr[SP_BONELAB0 + f], TS * TS);
        memcpy(LT[3][r][f], crawl_spr[SP_CAGE0 + f], TS * TS);
    }
}
static int sheet_lights(const char *path) {
    static const char *const NAME[4] = { "THE IRON CORONA", "THE SANCTUARY LAMP", "THE BONE CANDELABRUM", "THE PIERCED CAGE LANTERN" };
    static const char *const WHERE[4] = {
        "A GREAT WHEEL OF WROUGHT IRON ON FOUR CHAINS: TWO BANDS BARRED BETWEEN, EIGHT BARBED SPIKES, EIGHT CANDLES, SIX",
        "RED GLASS IN A BRASS CUP ON THREE FINE CHAINS FROM A SMOKE BELL, A SMALL FLAME IN IT: A LOW, STEADY GLOW.",
        "A GREAT SKULL WITH A STAG'S ANTLERS BOUND TO IT, A SKULL HUNG UNDER EACH END, RIBS ROUND A SPINE, A JAW AT",
        "TALL, OF BLACK IRON: A CROCKETED SPIRE, A LANCET OVER A QUATREFOIL IN FRONT, HOLES ROUND THE SIDES; ITS",
    };
    static const char *const WHERE2[4] = {
        "SPOKES TO A BOSS AND ITS DROP. FOR HALLS AND NAVES. ITS LIGHT AS THE CHANDELIER'S, OR A LITTLE WIDER.",
        "FOR CHAPELS AND BURIAL VAULTS. ITS LIGHT SMALL AND RED (A COLOUR TO SETTLE IN THE GAME'S STEP).",
        "THE FOOT; A CANDLE IN EACH EYE OF EACH SKULL. FOR OSSUARIES AND CHARNEL PITS.",
        "LIGHT COMES THROUGH THE HOLES. FOR PASSAGES AND SMALL ROOMS.",
    };
    int bandh = 62 + 4 * (CW + PAD + 4);
    iw = PAD + LAB + 4 * (CW + PAD) + VW + PAD * 2; ih = 176 + (30 + CW + 26) + 4 * bandh + 8;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 12, 2, "FOUR HANGING LIGHTS, FOR THE OWNER'S LOOK", GOLDINK);
    text(PAD + 42 * 16, 16, 1, "(PLAN-LIGHTS, STEP 2)", DIMINK);
    static const char *const HEAD[] = {
        "NOTHING HERE IS IN THE GAME. EACH IS DRAWN IN EACH RUN'S PALETTE: THE CATACOMBS' WATER RUSTS AND DRIPS ON IT AS",
        "ON THE CHANDELIER, THE HOLLOWS' SOOT DARKENS IT, THE DEEP BURNS BLACK CANDLES ON IRON GONE GLASSY. THE FRAMES ARE",
        "THE FLICKER; THE SWAY IS THE GAME'S, AS THE CHANDELIER'S. EACH FRAME TWICE ITS SIZE IN ITS OWN LIGHT; THEN THE FIRST",
        "DARKER, AS FROM A HALL'S FAR END; THEN AT THE GAME'S OWN SIZE, A CROP OF ITS 320X200 VIEW: THE LIGHT HUNG TWO AND",
        "A HALF CELLS OFF, A WALL FOUR AWAY. THE CAGE'S VIEW THROWS ITS HOLES' LIGHT IN SPOTS, AS IT MIGHT (NOT BUILT).",
    };
    text_lines(PAD, 44, 1, HEAD, 5);
    for (int r = 0; r < 4; r++) make_lights(r);
    /* for scale: the chandelier and the lantern the game hangs today, and the four new, the Crypts' */
    int y = 112;
    text(PAD, y, 2, "FOR SCALE, IN THE CRYPTS", GOLDINK);
    text(PAD + 25 * 16, y + 4, 1, "TODAY'S CHANDELIER AND LANTERN, THEN THE FOUR", INK);
    y += 30;
    run_art(0);
    const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
    static const char *const SCALE[6] = { "CHANDELIER", "LANTERN", "CORONA", "SANCTUARY", "BONES", "CAGE" };
    for (int k = 0; k < 6; k++) {
        int ox = PAD + k * (CW + PAD) + (k >= 2 ? 16 : 0);
        const uint8_t *s = k == 0 ? crawl_spr[SP_CHANDELIER_A] : k == 1 ? crawl_spr[SP_LANTERN_A] : LT[k - 2][0][0];
        cell(ox, y, SC, wall, floor, s, NULL, LIT);
        if (k < 2) text(ox, y + CW + 6, 1, SCALE[k], DIMINK); else text(ox, y + CW + 6, 1, SCALE[k], INK);
    }
    y += CW + 30;
    for (int L = 0; L < 4; L++) {
        text(PAD, y + 4, 2, NAME[L], GOLDINK);
        text(PAD, y + 24, 1, WHERE[L], INK); text(PAD, y + 34, 1, WHERE2[L], INK);
        y += 62;
        for (int c = 0; c < 4; c++) {
            const char *h = c < 3 ? (c < NFR[L] ? (c == 0 ? "FRAME 1" : c == 1 ? "FRAME 2" : "FRAME 3") : "") : "DARKER";
            if (c < 3) text(PAD + LAB + c * (CW + PAD), y - 12, 1, h, INK); else text(PAD + LAB + c * (CW + PAD), y - 12, 1, h, DIMINK);
        }
        text(PAD + LAB + 4 * (CW + PAD), y - 12, 1, "AT 1X, IN THE GAME'S VIEW", INK);
        for (int r = 0; r < 4; r++) {
            run_art(r);
            wall = crawl_tex[T_MATV(0, 0)]; floor = crawl_tex[T_FLOORS];
            text(PAD, y + CW / 2 - 18, 2, RUNA[r], INK); text(PAD, y + CW / 2 + 2, 2, RUNB[r], INK);
            for (int c = 0; c < 4; c++) {
                if (c < 3 && c >= NFR[L]) continue;
                cell(PAD + LAB + c * (CW + PAD), y, SC, wall, floor, LT[L][r][c < 3 ? c : 0], NULL, c < 3 ? LIT : DARK);
            }
            View v = { wall, floor, crawl_tex[T_CEILS], LT[L][r][0], NULL, NULL, 4.0f, 2.5f, 0.32f, 0.0f, 2.5f, 0.55f, 1.0f, 2.2f, L == 3 };
            view(PAD + LAB + 4 * (CW + PAD), y, VW, VH, &v);
            y += CW + PAD + 4;
        }
    }
    if (!write_png(path)) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    printf("%s: %dx%d\n", path, iw, ih);
    return 0;
}

/* ---- sheet 2: the events' things ---- */
#define NTH 9
static int sheet_things(const char *path) {
    static const char *const COLA[NTH] = { "BARRED", "UNBARRED", "RELIC", "SOCKET", "SOCKET", "LEVER", "LEVER", "PLATE", "PLATE" };
    static const char *const COLB[NTH] = { "DOOR", "DOOR", "", "EMPTY", "HOLDING", "UP", "DOWN", "UP", "PRESSED" };
    iw = PAD + LAB + NTH * (CW + PAD) + PAD; ih = 150 + 6 * (CW + PAD + 6) + 40 + 2 * (VH2 + 22) + 12;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 12, 2, "THE EVENTS' THINGS, FOR THE OWNER'S LOOK (PLAN-EVENTS; PLAN-LIGHTS, STEP 2)", GOLDINK);
    static const char *const HEAD[] = {
        "NOTHING HERE IS IN THE GAME. TODAY THESE BORROW THE PUZZLES' LOOKS (THE TOP ROW). THE BARRED DOOR IS THE RUN'S OWN DOOR WITH A BEAM ACROSS IT",
        "IN TWO STAPLES AND A CHAIN CROSSED OVER IT TO AN IRON PLATE, A HAND CUT IN THE PLATE: NO KEYHOLE, SO NOTHING SAYS A KEY OPENS IT. UNBARRED (AN",
        "EVENT'S UNLOCK, BEFORE IT SLIDES): THE BEAM AND PLATE GONE, A CHAIN HUNG LOOSE. THE RELIC IS A BONE BOUND IN GOLD, A KEY'S TEETH AT ITS END, A RED",
        "SEAL ON A CORD. THE SOCKET IS A PLINTH WITH TWO STONE HANDS HELD UP ON IT AND THE RELIC'S SHAPE CUT IN ITS FACE; HOLDING, THE RELIC LIES ACROSS",
        "THE HANDS AND THE SHAPE IS ALIGHT. THE LEVER: A STONE POST, A TOOTHED IRON QUADRANT, A TWISTED ARM. THE PLATE IS A FLOOR TEXTURE: A FLAGSTONE",
        "IN A DEEP JOINT, A ROSE CUT IN A ROUNDEL, IRON STUDS; PRESSED, SUNK AND DARKER, THE PIT'S SIDE SHOWING, DUST PUFFED FROM THE JOINT.",
    };
    text_lines(PAD, 40, 1, HEAD, 6);
    int y = 116;
    for (int c = 0; c < NTH; c++) { text(PAD + LAB + c * (CW + PAD), y, 1, COLA[c], INK); text(PAD + LAB + c * (CW + PAD), y + 10, 1, COLB[c], INK); }
    y += 26;
    static uint8_t T[NTH][TS * TS], P[2][TS * TS], tmp[TS * TS];
    for (int row = 0; row < 6; row++) {                              /* today; the four runs; the Crypts darker */
        int r = row == 0 || row == 5 ? 0 : row - 1;
        run_art(r);
        const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
        if (row == 0) {
            text(PAD, y + CW / 2 - 18, 2, "TODAY", DIMINK); text(PAD, y + CW / 2 + 2, 1, "THE CRYPTS", DIMINK);
            cell(PAD + LAB, y, SC, wall, floor, NULL, crawl_tex[T_GATE], LIT);
            cell(PAD + LAB + 2 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_FOSSIL], NULL, LIT);
            cell(PAD + LAB + 3 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_PLINTH], NULL, LIT);
            memcpy(tmp, crawl_spr[SP_PZ_PLINTH], TS * TS);
            for (int i = 0; i < TS * TS; i++) { int x = i / TS, yy = i % TS; if (yy + 17 < TS) { uint8_t c = crawl_spr[SP_PZ_FOSSIL][x * TS + yy + 17]; if (c != C_CLEAR && c != R_WARM + 1) tmp[i] = c; } }
            cell(PAD + LAB + 4 * (CW + PAD), y, SC, wall, floor, tmp, NULL, LIT);
            cell(PAD + LAB + 5 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_LEVER0], NULL, LIT);
            cell(PAD + LAB + 6 * (CW + PAD), y, SC, wall, floor, crawl_spr[SP_PZ_LEVER1], NULL, LIT);
            today_plate(P[0], floor, 0); today_plate(P[1], floor, 1);
            cell(PAD + LAB + 7 * (CW + PAD), y, SC, NULL, NULL, NULL, P[0], LIT);
            cell(PAD + LAB + 8 * (CW + PAD), y, SC, NULL, NULL, NULL, P[1], LIT);
            text(PAD + LAB + 1 * (CW + PAD) + 8, y + CW / 2 - 4, 1, "(IT SLIDES)", DIMINK);
            y += CW + PAD + 18;
            continue;
        }
        static const int16_t ID[NTH] = { T_BARRED, T_UNBARRED, SP_EV_RELIC, SP_EV_SOCKET0, SP_EV_SOCKET1, SP_EV_LEVER0, SP_EV_LEVER1, -1, -1 };
        for (int c = 0; c < 7; c++) memcpy(T[c], c < 2 ? crawl_tex[ID[c]] : crawl_spr[ID[c]], TS * TS);   /* the game's own */
        plate_on(T[7], floor, 0); plate_on(T[8], floor, 1);
        if (row == 5) { text(PAD, y + CW / 2 - 18, 2, "DARKER", DIMINK); text(PAD, y + CW / 2 + 2, 1, "THE CRYPTS", DIMINK); }
        else { text(PAD, y + CW / 2 - 18, 2, RUNA[r], INK); text(PAD, y + CW / 2 + 2, 2, RUNB[r], INK); }
        for (int c = 0; c < NTH; c++) {
            int face = c < 2 || c > 6;
            cell(PAD + LAB + c * (CW + PAD), y, SC, wall, floor, face ? NULL : T[c], face ? T[c] : NULL, row == 5 ? DARK : LIT);
        }
        y += CW + PAD + 6;
    }
    y += 12;
    text(PAD, y, 2, "AT 1X, IN THE GAME'S VIEW (A CROP OF 320X200)", GOLDINK);
    y += 40;
    static const char *const VL[6] = { "THE DOOR, 3 CELLS OFF", "UNBARRED, 3 OFF", "THE RELIC, 2 OFF", "THE SOCKET HOLDING, 2.5", "THE LEVER DOWN, 2.5", "THE PLATE, 2 OFF" };
    for (int row = 0; row < 2; row++) {
        int r = row ? 3 : 0;
        run_art(r);
        const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
        memcpy(T[0], crawl_tex[T_BARRED], TS * TS); memcpy(T[1], crawl_tex[T_UNBARRED], TS * TS);
        memcpy(T[2], crawl_spr[SP_EV_RELIC], TS * TS); memcpy(T[4], crawl_spr[SP_EV_SOCKET1], TS * TS); memcpy(T[6], crawl_spr[SP_EV_LEVER1], TS * TS);
        plate_on(T[8], floor, 0);
        text(PAD, y + VH2 / 2 - 18, 2, RUNA[r], INK); text(PAD, y + VH2 / 2 + 2, 2, RUNB[r], INK);
        for (int k = 0; k < 6; k++) {
            View v = { wall, floor, crawl_tex[T_CEILS], NULL, NULL, NULL, 4.0f, 2.0f, 0.45f, 0.6f, 1.2f, 0.6f, 0.75f, 2.6f, 0 };
            if (k < 2) { v.face = T[k]; v.wd = 3.0f; }
            else if (k == 5) { v.flat = T[8]; v.sd = 2.0f; }
            else { v.spr = T[k == 2 ? 2 : k == 3 ? 4 : 6]; v.sd = k == 2 ? 2.0f : 2.5f; }
            int ox = PAD + LAB + k * (VW + 4);
            view(ox, y, VW, VH2, &v);
            if (row == 0) text(ox, y - 12, 1, VL[k], INK);
        }
        y += VH2 + 22;
    }
    if (!write_png(path)) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    printf("%s: %dx%d\n", path, iw, ih);
    return 0;
}

/* ---- sheet 3: the twelve relics ---- */
#define RLAB 200
#define RV 128
static int sheet_relics(const char *path) {
    static const char *const NAME[NRLOOK] = { "THE BONE KEY", "THE SAINT'S FINGER", "THE HEART", "THE CROWNED SKULL", "THE ROSARY", "THE AMPULLA",
                                              "THE NAIL", "THE BROKEN HILT", "THE ABBOT'S SEAL", "THE PSALTER", "THE MASK", "THE WEEPING EYE" };
    static const char *const SAY[NRLOOK][3] = {
        { "A BONE BOUND IN GOLD", "WIRE, A KEY'S TEETH,", "A RED SEAL. TODAY'S." },
        { "IN A CRYSTAL TUBE ON A", "GILT FOOT, A RING ON IT,", "A SPIRE AND A CROSS." },
        { "DRIED, IN A SILVER", "CASKET, ITS LID BACK,", "A BAND OF GILT THORNS." },
        { "A GOLD CROWN OF FIVE", "POINTS, A RUBY AND TWO", "AMETHYSTS IN ITS BAND." },
        { "BONE BEADS IN A LOOP,", "A BIGGER ONE EACH", "DECADE, A BONE CROSS." },
        { "A PILGRIM'S FLASK OF", "LEAD, A SCALLOP SHELL", "ON IT, LOOPS, A CORD." },
        { "SQUARE IRON, RUSTED,", "BOUND IN FADED VELVET", "AND GILT THREAD." },
        { "WHEEL POMMEL, WIRED", "GRIP, KNOBBED GUARD, A", "HAND OF SNAPPED BLADE." },
        { "A POINTED OVAL OF", "BRONZE, THE ABBOT CUT", "IN IT, ITS LUG ON TOP." },
        { "RED LEATHER, BRASS", "CORNERS AND CLASP, A", "CHAIN FROM ITS BOARD." },
        { "FUNERARY, OF BRONZE,", "EYES SHUT, VERDIGRIS", "IN ITS HOLLOWS." },
        { "CRYSTAL, LIT WITHIN, IN", "A RAYED GILT MOUNT; A", "RED TEAR DOWN ITS STEM." },
    };
    static const char *const COL[10] = { "THE CRYPTS", "THE CATACOMBS", "THE HOLLOWS", "THE DEEP", "DARKER", "IN THE SOCKET",
                                         "1X: FLOOR", "1X: SOCKET", "1X: FLOOR", "1X: SOCKET" };
    int rowh = CW + 10, x1x = PAD + RLAB + 6 * (CW + PAD);
    iw = x1x + 4 * (RV + 6) + PAD; ih = 132 + NRLOOK * rowh + 8;
    img = calloc((size_t)iw * ih * 3, 1);
    rect(0, 0, iw, ih, 16, 14, 20);
    text(PAD, 12, 2, "TWELVE RELICS, FOR THE OWNER'S LOOK (PLAN-EVENTS)", GOLDINK);
    static const char *const HEAD[] = {
        "NOTHING HERE IS IN THE GAME. A LOOK FOR EACH RELIC, TO BE CHOSEN FOR IT IN THE EDITOR ONE DAY; THE FIRST IS TODAY'S. EACH LIES ON THE FLOOR IN EACH",
        "RUN'S PALETTE, TWICE ITS SIZE IN A TORCH'S LIGHT, THEN DARKER; THEN HELD IN THE SOCKET, WHOSE PANEL CUTS EACH ONE'S OWN SHAPE AND LIGHTS IT; THEN",
        "AT THE GAME'S OWN SIZE, A CROP OF ITS 320X200 VIEW: ON THE FLOOR TWO CELLS OFF AND IN THE SOCKET TWO AND A HALF, IN THE CRYPTS AND IN THE DEEP.",
    };
    text_lines(PAD, 40, 1, HEAD, 3);
    int y = 84;
    for (int c = 0; c < 10; c++) {
        int ox = c < 6 ? PAD + RLAB + c * (CW + PAD) : x1x + (c - 6) * (RV + 6);
        text(ox, y, 1, COL[c], c == 4 ? 150 : 216, c == 4 ? 140 : 208, c == 4 ? 128 : 189);
        if (c >= 6) text(ox, y + 10, 1, c < 8 ? "THE CRYPTS" : "THE DEEP", 150, 140, 128);
    }
    y += 32;
    static uint8_t R[TS * TS], S[TS * TS];
    for (int k = 0; k < NRLOOK; k++, y += rowh) {
        char num[4]; snprintf(num, sizeof num, "%d", k + 1);
        text(PAD, y + 6, 2, num, GOLDINK);
        text(PAD + 36, y + 10, 1, NAME[k], GOLDINK);
        for (int l = 0; l < 3; l++) text(PAD, y + 36 + l * 12, 1, SAY[k][l], INK);
        for (int c = 0; c < 6; c++) {
            int r = c < 4 ? c : 0;
            run_art(r);
            relic_from(R, k, 0);
            const uint8_t *wall = crawl_tex[T_MATV(0, 0)], *floor = crawl_tex[T_FLOORS];
            if (c == 5) relic_from(S, k, 1);
            cell(PAD + RLAB + c * (CW + PAD), y, SC, wall, floor, c == 5 ? S : R, NULL, c == 4 ? DARK : LIT);
        }
        for (int c = 0; c < 4; c++) {
            run_art(c < 2 ? 0 : 3);
            relic_from(R, k, 0); relic_from(S, k, 1);
            View v = { crawl_tex[T_MATV(0, 0)], crawl_tex[T_FLOORS], crawl_tex[T_CEILS], (c & 1) ? S : R, NULL, NULL, 4.0f, (c & 1) ? 2.5f : 2.0f, 0.45f, 0.6f, 1.2f, 0.6f, 0.75f, 2.6f, 0 };
            view(x1x + c * (RV + 6), y, RV, RV, &v);
        }
    }
    if (!write_png(path)) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    printf("%s: %dx%d\n", path, iw, ih);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s lights|things|relics out.png\n", argv[0]); return 1; }
    crawl_art_init();
    if (!strcmp(argv[1], "lights")) return sheet_lights(argv[2]);
    if (!strcmp(argv[1], "things")) return sheet_things(argv[2]);
    if (!strcmp(argv[1], "relics")) return sheet_relics(argv[2]);
    fprintf(stderr, "lights, things or relics\n");
    return 1;
}
