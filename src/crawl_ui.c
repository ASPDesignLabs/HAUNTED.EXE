/* The carved stone (docs/PLAN-ui.md): the marble, the slabs and wells cut from it, its rules and ornaments, text cut
 * or inlaid, the sockets and the Alignment's stone. Only gfx and the palette's ramps; the game and the sheet both draw
 * with it. */
#include "crawl_ui.h"
#include "gfx.h"
#include <stdlib.h>
#include <stdio.h>

#define MW 320
#define MH 200

/* ---- the marble ---- */

static uint8_t g_mshade[MW * MH];                  /* the slab's shade at each pixel of the screen, 0..15 */

static void ramp3(uint8_t pal[256][3], int first, const float a[3], const float b[3], float gamma) {
    for (int i = 0; i < 16; i++) {
        float k = powf((float)i / 15.0f, gamma);
        for (int c = 0; c < 3; c++) {
            float v = a[c] + (b[c] - a[c]) * k;
            pal[first + i][c] = (uint8_t)(v < 0 ? 0 : v > 63 ? 63 : v + 0.5f);
        }
    }
}

void ui_marble_pal(uint8_t pal[256][3], const uint8_t stone_mid[3]) {
    float lean[3] = { 1, 1, 1 };                   /* a tenth towards the run's stone, as bright as ever */
    if (stone_mid) {
        float m = (stone_mid[0] + stone_mid[1] + stone_mid[2]) / 3.0f;
        if (m > 0.5f) for (int c = 0; c < 3; c++) lean[c] = 1.0f + 0.1f * ((float)stone_mid[c] / m - 1.0f);
    }
    const float sa[3] = { 4.0f * lean[0], 4.0f * lean[1], 5.0f * lean[2] }, sb[3] = { 56.0f * lean[0], 56.0f * lean[1], 57.0f * lean[2] };
    const float wa[3] = { 4.0f * lean[0], 4.0f * lean[1], 5.0f * lean[2] }, wb[3] = { 38.0f * lean[0], 38.0f * lean[1], 42.0f * lean[2] };
    ramp3(pal, R_CEIL, sa, sb, 1.15f);             /* the slab: weathered grey, groove to vein */
    ramp3(pal, R_FLOOR, wa, wb, 1.05f);            /* the wells: a cooler, darker stone, never black */
}

/* While a menu is up the floor's and ceiling's ramps are the marble's. What of the crypt is already drawn in them goes to
 * the run's stone entry nearest it in brightness first, so the dimmed crypt behind looks as it did. */
void ui_lend(Frame *f) {
    uint8_t to[32];
    for (int i = 0; i < 32; i++) {
        const uint8_t *p = f->pal[R_FLOOR + i];
        int y = p[0] * 3 + p[1] * 6 + p[2], best = 0, bd = 1 << 30;
        for (int k = 0; k < 16; k++) {
            const uint8_t *q = f->pal[R_STONE + k];
            int d = abs(q[0] * 3 + q[1] * 6 + q[2] - y);
            if (d < bd) { bd = d; best = k; }
        }
        to[i] = (uint8_t)(R_STONE + best);
    }
    int n = f->w * f->h;
    for (int i = 0; i < n; i++) { uint8_t c = f->pix[i]; if (c >= R_FLOOR && c < R_CEIL + 16) f->pix[i] = to[c - R_FLOOR]; }
    ui_marble_pal(f->pal, f->pal[R_STONE + 8]);
}

static uint32_t hash2(int x, int y, unsigned s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float vnoise(float x, float y, unsigned s) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - xi, fy = y - yi;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    float a = (hash2(xi, yi, s) & 0xFFFF) / 65535.0f, b = (hash2(xi + 1, yi, s) & 0xFFFF) / 65535.0f;
    float c = (hash2(xi, yi + 1, s) & 0xFFFF) / 65535.0f, d = (hash2(xi + 1, yi + 1, s) & 0xFFFF) / 65535.0f;
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
static float fbm(float x, float y, unsigned s, int oct) {
    float v = 0, amp = 0.5f, sum = 0;
    for (int i = 0; i < oct; i++) { v += amp * vnoise(x, y, s + (unsigned)i * 101u); sum += amp; x *= 2.03f; y *= 2.03f; amp *= 0.5f; }
    return v / sum;
}

void ui_marble_make(unsigned seed) {
    static const float BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
    float ang = (float)(hash2(1, 2, seed) % 628) / 100.0f;
    float ca = cosf(ang) * 0.030f, sa = sinf(ang) * 0.030f;
    for (int y = 0; y < MH; y++)
        for (int x = 0; x < MW; x++) {
            float m = fbm(x * 0.045f, y * 0.045f, seed, 4);                    /* the body's clouding */
            float tu = fbm(x * 0.012f, y * 0.012f, seed + 7u, 4) * 9.0f;       /* the veins wander */
            float v1 = 1.0f - fabsf(sinf(x * ca + y * sa + tu));
            float v2 = 1.0f - fabsf(sinf(x * sa * 1.7f - y * ca * 1.3f + tu * 0.7f + 2.0f));
            v1 = powf(v1, 7.0f); v2 = powf(v2, 11.0f) * 0.45f;        /* soft, tonal veins, not a crackle */
            float sh = 4.9f + (m - 0.5f) * 2.6f + (v1 + v2) * 2.3f;
            if ((hash2(x, y, seed + 3u) % 97u) == 0) sh -= 1.3f;              /* pits */
            sh += (BAYER[y & 3][x & 3] / 16.0f) - 0.47f;
            int k = (int)floorf(sh);
            g_mshade[y * MW + x] = (uint8_t)(k < 1 ? 1 : k > 11 ? 11 : k);
        }
    for (int c = 0; c < 5; c++) {                                               /* hairline cracks */
        uint32_t h = hash2(c, 99, seed);
        float x = (float)(h % MW), y = (float)((h >> 9) % MH), a = (float)((h >> 17) % 628) / 100.0f;
        int len = 30 + (int)((h >> 3) % 70);
        for (int i = 0; i < len; i++) {
            a += ((float)(hash2(c, i, seed) % 100) / 100.0f - 0.5f) * 0.9f;
            x += cosf(a); y += sinf(a);
            int xi = (int)x, yi = (int)y;
            if (xi < 0 || yi < 0 || xi >= MW - 1 || yi >= MH - 1) break;
            g_mshade[yi * MW + xi] = 1;
            if (g_mshade[(yi + 1) * MW + xi + 1] > 1 && g_mshade[(yi + 1) * MW + xi + 1] < 9) g_mshade[(yi + 1) * MW + xi + 1] += 2;
        }
    }
}

static int mshade(int x, int y) { return (unsigned)x < MW && (unsigned)y < MH ? g_mshade[y * MW + x] : 5; }

/* ---- slabs, wells, rules ---- */

static int in_shape(int kind, int x, int y, int w, int h) {   /* a headstone's arch; a chip off each corner */
    if (kind == SLAB_HEAD) {
        int r = w / 2, ah = w / 5 > 22 ? 22 : w / 5;           /* the arch: an ellipse r across, ah high */
        if (y < ah) { float dx = (x - (w - 1) * 0.5f) / r, dy = (ah - y) / (float)ah; if (dx * dx + dy * dy > 1.0f) return 0; }
        return 1;
    }
    int c = kind == SLAB_TABLET ? 1 : 2;
    if ((x < c && y < c - x) || (x >= w - c && y < x - (w - 1 - c)) ) return 0;
    if ((x < c && h - 1 - y < c - x) || (x >= w - c && h - 1 - y < x - (w - 1 - c))) return 0;
    return 1;
}

void ui_slab(Frame *f, int x, int y, int w, int h, int kind, int orn, float t) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            if (!in_shape(kind, i, j, w, h)) continue;
            int s = mshade(x + i, y + j);
            int edge = 0;                                      /* how far in from the shape's edge, up to 5 */
            for (int d = 1; d <= 5 && !edge; d++)
                if (!in_shape(kind, i - d, j, w, h) || !in_shape(kind, i + d, j, w, h) || !in_shape(kind, i, j - d, w, h) || !in_shape(kind, i, j + d, w, h)) edge = d;
            if (edge) {
                int lit = !in_shape(kind, i - edge, j, w, h) || !in_shape(kind, i, j - edge, w, h);   /* the moulding, lit from above left */
                int shad = !in_shape(kind, i + edge, j, w, h) || !in_shape(kind, i, j + edge, w, h);
                if (edge == 1) s = 1;
                else if (edge == 2 || edge == 3) s = lit && !shad ? (edge == 2 ? 12 : 10) : shad && !lit ? (edge == 2 ? 3 : 4) : 7;
                else if (edge == 4) s = lit && !shad ? 2 : shad && !lit ? 9 : 4;   /* the groove inside the moulding */
                else s = s > 8 ? 8 : s;
            }
            gfx_px(f, x + i, y + j, UI_M(s));
        }
    if (orn && kind != SLAB_TABLET) {                          /* the owner's sign in the top corners */
        ui_orn(f, x + 7, y + (kind == SLAB_HEAD ? h / 6 + 4 : 7), orn, t);
        ui_orn(f, x + w - 15, y + (kind == SLAB_HEAD ? h / 6 + 4 : 7), orn, t + 0.37f);
    }
}

void ui_well(Frame *f, int x, int y, int w, int h) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int s;
            if (i == 0 || j == 0) s = -1;                          /* the cut's shadow, top and left */
            else if (i == w - 1 || j == h - 1) s = -2;             /* its lit lip, foot and right */
            else s = mshade(x + i, y + j) - (i == 1 || j == 1);
            uint8_t c = s == -1 ? UI_W(0) : s == -2 ? UI_M(10) : UI_W(s < 2 ? 2 : s > 8 ? 8 : s);
            gfx_px(f, x + i, y + j, c);
        }
}

void ui_rule(Frame *f, int x0, int x1, int y, int orn) {
    int mid = (x0 + x1) / 2;
    for (int x = x0; x <= x1; x++) {
        if (orn && abs(x - mid) < 6) continue;
        gfx_px(f, x, y, UI_M(1)); gfx_px(f, x, y + 1, UI_M(11));
    }
    if (orn) {                                                     /* a lozenge at its middle */
        for (int k = 0; k < 4; k++) {
            gfx_px(f, mid - k, y - 3 + k, UI_M(1)); gfx_px(f, mid + k, y - 3 + k, UI_M(1));
            gfx_px(f, mid - k, y + 4 - k, UI_M(11)); gfx_px(f, mid + k, y + 4 - k, UI_M(11));
        }
        gfx_px(f, mid, y, UI_M(13)); gfx_px(f, mid, y + 1, UI_M(13));
    }
}

/* the owners' signs, 8 by 8, cut in the stone: '#' groove, '+' lit edge, 'f' a flame */
static const char *const ORN_ART[NORN][8] = {
    { 0 },
    { "...f....", "..fff...", "...f....", "..###...", "..#+#...", "..#+#...", "..#+#...", ".#####.." },   /* the Lampstand's candle */
    { ".#..#...", ".#..#...", "........", "#.##.#..", "........", ".####...", "######..", ".####..." },   /* her paw */
    { "#....#..", ".#..#...", "..##....", "..##....", ".#..#...", "#....#..", "........", "........" },   /* bones crossed */
    { ".##.....", "#..#....", "#..#....", ".##.....", "..#.....", "..##....", "..#.....", "..##...." },   /* a key */
    { "..#.#...", ".#+#+#..", "#+#.#+#.", ".#...#..", "#+#.#+#.", ".#+#+#..", "..#.#...", "........" },   /* a rosette */
    { "........", "..####..", ".#....#.", "#..##..#", ".#....#.", "..####..", "........", "........" },   /* an eye */
};
void ui_orn(Frame *f, int x, int y, int orn, float t) {
    if (orn <= 0 || orn >= NORN) return;
    int fl = (int)(t * 9.0f) & 1;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            char ch = ORN_ART[orn][r][c];
            if (ch == '#') { gfx_px(f, x + c, y + r, UI_M(1)); if (c < 7 && ORN_ART[orn][r][c + 1] == '.') gfx_px(f, x + c + 1, y + r, UI_M(10)); }
            else if (ch == '+') gfx_px(f, x + c, y + r, UI_M(11));
            else if (ch == 'f') gfx_px(f, x + c + (r == 0 ? fl : 0), y + r, (uint8_t)(R_FLAME + 15 - r * 3));
        }
}

/* the fit checks (section 2.4): a line that runs out of its box is told, and counted */
int ui_overflows;
void ui_overflow(const char *where, const char *what, int by) {
    ui_overflows++;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "UI overflow %s \"%s\" by %d\n", where, what, by);
#else
    (void)where; (void)what; (void)by;
#endif
}

/* ---- text ---- */

extern unsigned char ui_face8[NFACE][128][8];              /* ui_art.c, made by ui_face_init: what a face lacks, the BIOS gives */
extern unsigned char ui_face16[128][16];
extern unsigned char ui_has8[NFACE][128], ui_has16[128];

static const unsigned char *glyph8(uint8_t ch, int face) {
    if (face > FACE_BIOS && face < NFACE && ch < 128 && ui_has8[face][ch]) return ui_face8[face][ch];
    return font_bios8[ch];
}
static void plot8(Frame *f, int x, int y, const unsigned char *g, int col) {
    for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) if (g[r] >> (7 - c) & 1) gfx_px(f, x + c, y + r, (uint8_t)col);
}
void ui_glyph8(Frame *f, int x, int y, uint8_t ch, int face, int col, int style) {
    const unsigned char *g = glyph8(ch, face);
    if (style == TX_CUT) { plot8(f, x + 1, y + 1, g, UI_M(13)); plot8(f, x, y, g, UI_M(1)); }
    else if (style == TX_INLAID) { plot8(f, x + 1, y + 1, g, UI_M(0)); plot8(f, x, y, g, col); }
    else plot8(f, x, y, g, col);
}
void ui_text8(Frame *f, int x, int y, const char *s, int face, int col, int style) {
    for (; *s; s++, x += 8) ui_glyph8(f, x, y, (uint8_t)*s, face, col, style);
}
static void plot16(Frame *f, int x, int y, const unsigned char *g, int col, int sc) {
    for (int r = 0; r < 16; r++) for (int c = 0; c < 8; c++) if (g[r] >> (7 - c) & 1) gfx_rect(f, x + c * sc, y + r * sc, sc, sc, (uint8_t)col);
}
void ui_text16(Frame *f, int x, int y, const char *s, int face, int col, int style, int sc) {
    if (sc < 1) sc = 1;
    for (; *s; s++, x += 8 * sc) {
        uint8_t ch = (uint8_t)*s;
        const unsigned char *g = face == FACE_INSC && ch < 128 && ui_has16[ch] ? ui_face16[ch] : font_vga16[ch];
        if (style == TX_CUT) { plot16(f, x + 1, y + 1, g, UI_M(12), sc); plot16(f, x, y, g, UI_M(1), sc); }
        else if (style == TX_INLAID) { plot16(f, x + sc, y + sc, g, UI_M(0), sc); plot16(f, x, y, g, col, sc); }
        else plot16(f, x, y, g, col, sc);
    }
}

/* word-wrapped lines of s, cols to a line: each line's start and length; how many */
static int wrap_lines(const char *s, int cols, const char *st[], int len[], int max) {
    int n = 0;
    while (*s == ' ') s++;
    while (*s && n < max) {
        int k = (int)strlen(s);
        if (k > cols) { k = cols; while (k > 0 && s[k] != ' ') k--; if (k == 0) k = cols; }
        st[n] = s; len[n] = k; n++;
        s += k; while (*s == ' ') s++;
    }
    return n;
}
int ui_wrap_count(const char *s, int cols) { const char *st[32]; int len[32]; return wrap_lines(s, cols, st, len, 32); }
int ui_wrap(Frame *f, int x, int y, const char *s, int cols, int lh, int maxl, int face, int col, int style) {
    const char *st[32]; int len[32];
    int n = wrap_lines(s, cols, st, len, maxl < 32 ? maxl : 32);
    for (int i = 0; i < n; i++) {
        char b[64]; int k = len[i] < 63 ? len[i] : 63;
        memcpy(b, st[i], (size_t)k); b[k] = 0;
        ui_text8(f, x, y + i * lh, b, face, col, style);
    }
    return n;
}

/* a smooth raised plate, for words cut in the stone to read on */
void ui_plate(Frame *f, int x, int y, int w, int h) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int s = i == 0 || j == 0 ? 11 : i == w - 1 || j == h - 1 ? 3 : 7 + ((mshade(x + i, y + j) > 7) & ((i + j) & 1));
            gfx_px(f, x + i, y + j, UI_M(s));
        }
}

/* a hint line: in each group (two spaces apart) its first word is the key, set on a raised cap behind the 8 px cells,
 * so the line is as wide as ever */
void ui_keys(Frame *f, int x, int y, const char *s, int col) {
    int key = 1;
    for (int i = 0; s[i]; i++) {
        int cx = x + i * 8;
        if (s[i] == ' ') { if (s[i + 1] == ' ') key = 1; else if (key == 1 && i > 0 && s[i - 1] != ' ') key = 0; continue; }
        if (key) {
            int first = i == 0 || s[i - 1] == ' ', last = s[i + 1] == 0 || s[i + 1] == ' ';
            for (int yy = -1; yy < 9; yy++)
                for (int xx = first ? -2 : 0; xx < (last ? 9 : 8); xx++) {
                    int edge_t = yy == -1, edge_b = yy == 8, edge_l = first && xx == -2, edge_r = last && xx == 8;
                    gfx_px(f, cx + xx, y + yy, edge_t || edge_l ? UI_M(12) : edge_b || edge_r ? UI_M(2) : UI_M(8));
                }
            ui_glyph8(f, cx, y, (uint8_t)s[i], FACE_BODY, UI_W(1), TX_PLAIN);
        } else ui_glyph8(f, cx, y, (uint8_t)s[i], FACE_BODY, col, TX_INLAID);
    }
}

/* ---- icons and sockets ---- */


void ui_icon(Frame *f, int x, int y, int icon, int ramp, int mode, int sc) {
    if (icon < 0 || icon >= NICON) return;
    static const int CUT[5] = { 1, 3, 5, 9, 12 }, DIM[5] = { 2, 3, 5, 7, 9 };
    static const int INL[5] = { 6, 9, 11, 13, 15 }, HALF[5] = { 3, 5, 7, 9, 11 };
    int top = ramp == R_GOLD ? 12 : 15;
    for (int r = 0; r < 16; r++)
        for (int c = 0, n = ui_icon_row(icon, r) ? (int)strlen(ui_icon_row(icon, r)) : 0; c < n && c < 16; c++) {
            char ch = ui_icon_row(icon, r)[c];
            if (ch < 'a' || ch > 'e') continue;
            int k = ch - 'a';
            uint8_t col;
            if (mode == IM_CUT) col = UI_M(CUT[k]);
            else if (mode == IM_DIM) col = UI_M(DIM[k]);
            else if (mode == IM_GLOW) col = k >= 3 ? (uint8_t)(ramp + (k == 4 ? 9 : 6)) : UI_M(CUT[k]);
            else if (mode == IM_HALF) col = (uint8_t)(ramp + (HALF[k] > top ? top : HALF[k]));
            else col = (uint8_t)(ramp + (INL[k] > top ? top : INL[k]));
            gfx_rect(f, x + c * sc, y + r * sc, sc, sc, col);
        }
}

static int chamfer_in(int i, int j, int s, int c) {         /* inside a square of side s with its corners cut by c */
    if (i < 0 || j < 0 || i >= s || j >= s) return 0;
    int a = i < c ? c - i : i >= s - c ? i - (s - 1 - c) : 0, b = j < c ? c - j : j >= s - c ? j - (s - 1 - c) : 0;
    return a + b <= c;
}

void ui_socket(Frame *f, int cx, int cy, int r, int icon, int ramp, int state, int sel, int ult, float t) {
    int s = r * 2, x0 = cx - r, y0 = cy - r, c = r / 3 + 1;
    float br = 0.5f + 0.5f * sinf(t * 3.2f);
    int top = ramp == R_GOLD ? 12 : 15;
    if (ult) {                                                    /* an ultimate's outer ring */
        for (int j = -3; j < s + 3; j++) for (int i = -3; i < s + 3; i++) {
            if (!chamfer_in(i + 3, j + 3, s + 6, c + 2) || chamfer_in(i + 1, j + 1, s + 2, c + 1)) continue;
            int lit = (i + j) < s;
            uint8_t col = state == SK_HELD ? (uint8_t)(R_GOLD + (lit ? 11 : 7)) : UI_M(lit ? 10 : 3);
            if (state != SK_HELD && (i == -3 || j == -3 || i == s + 2 || j == s + 2)) col = UI_M(1);
            gfx_px(f, x0 + i, y0 + j, col);
        }
    }
    for (int j = 0; j < s; j++) for (int i = 0; i < s; i++) {
        if (!chamfer_in(i, j, s, c)) continue;
        int edge = !chamfer_in(i - 1, j, s, c) || !chamfer_in(i, j - 1, s, c) ? 1 : !chamfer_in(i + 1, j, s, c) || !chamfer_in(i, j + 1, s, c) ? 2 : 0;
        int edge2 = !edge && (!chamfer_in(i - 2, j, s, c) || !chamfer_in(i, j - 2, s, c) || !chamfer_in(i + 2, j, s, c) || !chamfer_in(i, j + 2, s, c));
        uint8_t col;
        if (edge == 1) col = UI_M(1);                                  /* sunk: shadow on the upper left, */
        else if (edge == 2) col = UI_M(11);                            /* light on the lower right */
        else if (edge2) {                                              /* the rim, by its state */
            col = state == SK_HELD ? (uint8_t)(R_GOLD + 10)
                : state == SK_READY ? (uint8_t)(ramp + (int)(7 + 5 * br > top ? top : 7 + 5 * br))
                : state == SK_PART ? (uint8_t)(ramp + 8)
                : state == SK_SHORT ? UI_M(8) : UI_M(state == SK_CLOSED ? 3 : 5);
        } else col = state == SK_HELD ? UI_W(3) : UI_W(state == SK_CLOSED ? 1 : 2);
        gfx_px(f, x0 + i, y0 + j, col);
    }
    if (state == SK_HELD)                                              /* the inlay's light, under the icon */
        for (int j = 3; j < s - 3; j++) for (int i = 3; i < s - 3; i++) {
            float dx = (i - r + 0.5f) / (r - 3), dy = (j - r + 0.5f) / (r - 3);
            if (dx * dx + dy * dy < 0.8f && ((i + j) & 1)) gfx_px(f, x0 + i, y0 + j, (uint8_t)(ramp + 2));
        }
    int sc = s >= 34 ? 2 : 1, iw = 16 * sc;
    int mode = state == SK_HELD ? IM_INLAID : state == SK_PART ? IM_HALF : state == SK_READY ? IM_GLOW : state == SK_CLOSED ? IM_DIM
             : state == SK_REACH ? IM_DIM : IM_CUT;
    ui_icon(f, cx - iw / 2, cy - iw / 2, icon, ramp, mode, sc);
    if (state == SK_CLOSED) {                                          /* cracked across, a chain over the socket */
        for (int i = 1; i < s - 1; i++) {
            int yy = cy - 1 + ((i / 3) & 1);
            gfx_px(f, x0 + i, yy, (i % 3) ? UI_M(9) : UI_M(2));
            gfx_px(f, x0 + i, yy + 1, UI_M(2));
        }
        for (int k = 0; k < 4; k++) gfx_px(f, x0 + 2 + k, y0 + 2 + k * 2, UI_M(0));
    }
    if (sel) {                                                         /* gilt brackets at the four corners */
        uint8_t g = (uint8_t)(R_GOLD + (br > 0.5f ? 12 : 9));
        int o = ult ? 5 : 3, x1 = x0 - o, y1 = y0 - o, x2 = x0 + s - 1 + o, y2 = y0 + s - 1 + o;
        for (int k = 0; k < 5; k++) {
            gfx_px(f, x1 + k, y1, g); gfx_px(f, x1, y1 + k, g); gfx_px(f, x2 - k, y1, g); gfx_px(f, x2, y1 + k, g);
            gfx_px(f, x1 + k, y2, g); gfx_px(f, x1, y2 - k, g); gfx_px(f, x2 - k, y2, g); gfx_px(f, x2, y2 - k, g);
        }
    }
}

void ui_groove(Frame *f, int x0, int y0, int x1, int y1, int ramp, float fill) {
    int n = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
    if (n < 1) return;
    int top = ramp == R_GOLD ? 12 : 15;
    for (int i = 0; i <= n; i++) {
        int x = x0 + (x1 - x0) * i / n, y = y0 + (y1 - y0) * i / n;
        int lit = (float)i / n <= fill;
        gfx_px(f, x, y, lit ? (uint8_t)(ramp + (11 > top ? top : 11)) : UI_W(0));
        gfx_px(f, x + 1, y, lit ? (uint8_t)(ramp + top) : UI_W(1));
        gfx_px(f, x + 1, y + 1, UI_M(11));
        gfx_px(f, x + 2, y + 1, UI_M(10));
    }
}

/* ---- the Alignment's stone ---- */

static const int TREE_X[10] = { 90, 46, 134, 90, 46, 46, 46, 134, 134, 134 };
static const int TREE_Y[10] = { 56, 78, 78, 100, 122, 143, 164, 122, 143, 164 };
static const int TREE_R[10] = { 9, 9, 9, 10, 9, 10, 11, 9, 10, 11 };
static const int TREE_UP[10] = { -1, 0, 0, 1, 3, 4, 5, 3, 7, 8 };    /* the node each hangs from (the ability from both) */

static void seal(Frame *f, int x, int y, int e, int rank, int ramp, int chosen, int cls, float t) {
    int cx = x + 7, cy = y + 7;
    for (int j = -7; j <= 7; j++) for (int i = -7; i <= 7; i++) {
        int d2 = i * i + j * j;
        if (d2 > 50) continue;
        uint8_t c;
        if (d2 > 36) c = (i + j < 0) == !chosen ? UI_M(chosen ? 12 : 1) : UI_M(chosen ? 3 : 11);
        else c = chosen ? UI_M(9 + ((i * 3 + j * 5) & 1)) : UI_W(2 + ((i + j) & 1));
        gfx_px(f, cx + i, cy + j, c);
    }
    if (cls) for (int i = -6; i <= 6; i++) { gfx_px(f, cx + i, cy + 9, R_GOLD + 11); }
    extern const char *const ELEM_GLYPH[6][7];
    for (int r = 0; r < 7; r++) for (int c = 0; c < 7; c++) {
        char ch = ELEM_GLYPH[e][r][c];
        if (ch != '#' && ch != 'o') continue;
        uint8_t col = rank ? (uint8_t)(ramp + (ch == 'o' ? (ramp == R_GOLD ? 12 : 15) : 11)) : UI_M(ch == 'o' ? 4 : 2);
        gfx_px(f, cx - 3 + c, cy - 3 + r, col);
    }
    char b[8];
    if (rank) snprintf(b, sizeof b, "%d", rank); else snprintf(b, sizeof b, "-");
    ui_text8(f, x + 17, y + 4, b, FACE_INSC, rank ? ramp + (ramp == R_GOLD ? 12 : 14) : UI_M(3), rank ? TX_INLAID : TX_CUT);
    (void)t;
}

void ui_tree(Frame *f, const UiTree *tr) {
    float t = tr->t;
    ui_slab(f, 4, 4, 312, 192, SLAB_LEDGER, ORN_CANDLE, t);
    for (int k = 0; k < 6; k++) seal(f, 12 + k * 34, 9, k, tr->ranks[k], tr->ramps[k], k == tr->sel_el, k == tr->cls, t);
    char b[48];
    snprintf(b, sizeof b, "%d POINT%s", tr->points, tr->points == 1 ? "" : "S");
    int breathe = tr->points > 0 && ((int)(t * 2.0f) & 1);
    ui_text8(f, 296 - 8 * (int)strlen(b), 13, b, FACE_INSC, breathe ? R_GOLD + 12 : R_GOLD + 9, TX_INLAID);
    ui_well(f, 9, 28, 163, 152);                                   /* the tree */
    ui_well(f, 176, 28, 135, 152);                                 /* the pane */
    if (tr->cls >= 0) {
        extern const char *const CLASS_MARK[6][11];
        for (int r = 0; r < 11; r++) for (int c = 0; c < 11; c++) {
            char ch = CLASS_MARK[tr->cls][r][c];
            if (ch == '#' || ch == 'o') gfx_px(f, 13 + c, 32 + r, (uint8_t)(tr->ramps[tr->cls] + (ch == 'o' ? 14 : 10)));
        }
    }
    int tw = 8 * (int)strlen(tr->title), tx = 98 - tw / 2;
    ui_text8(f, tx, 34, tr->title, FACE_INSC, tr->ramp + (tr->ramp == R_GOLD ? 12 : 13), TX_INLAID);
    for (int m = 1; m < 10; m++) {                                 /* the grooves first, the sockets over them */
        int u = TREE_UP[m];
        float fill = tr->n[m].state == SK_HELD ? 1.0f : tr->n[m].state == SK_PART ? 0.5f : 0.0f;
        if (m == 3) {
            ui_groove(f, TREE_X[1] + 6, TREE_Y[1] + 6, TREE_X[3] - 7, TREE_Y[3] - 6, tr->ramp, fill);
            ui_groove(f, TREE_X[2] - 7, TREE_Y[2] + 6, TREE_X[3] + 6, TREE_Y[3] - 6, tr->ramp, fill);
            continue;
        }
        int dx = TREE_X[m] - TREE_X[u];
        ui_groove(f, TREE_X[u] + (dx > 0 ? 6 : dx < 0 ? -7 : 0), TREE_Y[u] + TREE_R[u] - (dx ? 2 : 0),
                  TREE_X[m] + (dx > 0 ? -7 : dx < 0 ? 6 : 0), TREE_Y[m] - TREE_R[m] + (dx ? 2 : 0), tr->ramp, fill);
    }
    for (int m = 0; m < 10; m++) {
        const UiNode *nd = &tr->n[m];
        int icon = UI_NODE_ICON[tr->elem][m];
        ui_socket(f, TREE_X[m], TREE_Y[m], TREE_R[m], icon, tr->ramp, nd->state, m == tr->sel, m == 6 || m == 9, t);
        if (nd->cost > 1 && nd->state != SK_HELD) {                   /* its points as pips under it */
            int px = TREE_X[m] - (nd->cost * 4 - 2) / 2, py = TREE_Y[m] + TREE_R[m] + 2;
            for (int k = 0; k < nd->cost; k++) {
                uint8_t c = k < nd->got ? (uint8_t)(tr->ramp + 13) : UI_W(1);
                gfx_rect(f, px + k * 4, py, 2, 2, c);
                gfx_px(f, px + k * 4 + 2, py + 1, UI_M(10));
            }
        }
    }
    /* the pane: the chosen node, large; its name and kind; its cost; what it does; its state */
    const UiNode *nd = &tr->n[tr->sel < 0 || tr->sel > 9 ? 0 : tr->sel];
    int m = tr->sel;
    int icon = UI_NODE_ICON[tr->elem][m];
    ui_socket(f, 199, 53, 19, icon, tr->ramp, nd->state, 0, 0, t);
    int nl = ui_wrap_count(nd->name, 11);
    ui_wrap(f, 223, nl > 1 ? 36 : 40, nd->name, 11, 9, 2, FACE_INSC, tr->ramp + (tr->ramp == R_GOLD ? 12 : 14), TX_INLAID);
    ui_text8(f, 223, 56, nd->kind, FACE_BODY, UI_M(10), TX_PLAIN);
    if (nd->state == SK_HELD) snprintf(b, sizeof b, "YOURS");
    else if (nd->cost > 1 && nd->got > 0) snprintf(b, sizeof b, "%d OF %d POINTS", nd->got, nd->cost);
    else snprintf(b, sizeof b, "%d POINT%s", nd->cost, nd->cost == 1 ? "" : "S");
    ui_text8(f, 182, 74, b, FACE_INSC, nd->state == SK_HELD ? R_GOLD + 12 : R_GOLD + 10, TX_INLAID);
    ui_rule(f, 182, 304, 85, 0);
    if (tr->warn) {                                                /* a warning stands in the description's place, as it always has */
        int need = ui_wrap_count(tr->warn, 16);
        ui_wrap(f, 182, 90, tr->warn, 16, 9, 9, FACE_BODY, R_GOLD + 12, TX_INLAID);
        if (need > 9) ui_overflow("tree warning", tr->warn, need - 9);
    } else {
        const char *foot = nd->note;
        int fl = foot ? ui_wrap_count(foot, 16) : 0, fy = 176 - fl * 9;
        int dl = (fy - 3 - 90) / 9, need = ui_wrap_count(nd->does, 16);
        ui_wrap(f, 182, 90, nd->does, 16, 9, dl, FACE_BODY, UI_M(14), TX_INLAID);
        if (need > dl) ui_overflow("tree", nd->name, need - dl);
        if (foot) ui_wrap(f, 182, fy, foot, 16, 9, fl, FACE_BODY, UI_M(11), TX_INLAID);
    }
    if (tr->keys) ui_keys(f, 160 - 4 * (int)strlen(tr->keys), 183, tr->keys, UI_M(13));
}
