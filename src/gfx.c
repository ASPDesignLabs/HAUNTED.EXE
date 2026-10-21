#include "gfx.h"
#include <stdlib.h>

void gfx_clear(Frame *f, uint8_t c) {
    memset(f->pix, c, (size_t)f->w * f->h);
}

void gfx_rect(Frame *f, int x, int y, int w, int h, uint8_t c) {
    int x0 = clampi(x, 0, f->w), x1 = clampi(x + w, 0, f->w);
    int y0 = clampi(y, 0, f->h), y1 = clampi(y + h, 0, f->h);
    for (int yy = y0; yy < y1; yy++)
        memset(&f->pix[yy * f->w + x0], c, (size_t)(x1 - x0));
}

void gfx_line(Frame *f, int x0, int y0, int x1, int y1, uint8_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        gfx_px(f, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_disc(Frame *f, int cx, int cy, int r, uint8_t c) {
    for (int y = -r; y <= r; y++) {
        int w = (int)sqrtf((float)(r * r - y * y) + 0.5f);
        gfx_rect(f, cx - w, cy + y, 2 * w + 1, 1, c);
    }
}

void gfx_pal(Frame *f, int i, int r, int g, int b) {
    f->pal[i][0] = (uint8_t)clampi(r, 0, 63);
    f->pal[i][1] = (uint8_t)clampi(g, 0, 63);
    f->pal[i][2] = (uint8_t)clampi(b, 0, 63);
}

void gfx_pal_ramp(Frame *f, int first, int last,
                  int r0, int g0, int b0, int r1, int g1, int b1) {
    int n = last - first;
    for (int i = 0; i <= n; i++) {
        float t = n ? (float)i / n : 0.0f;
        gfx_pal(f, first + i, (int)(r0 + (r1 - r0) * t + 0.5f),
                (int)(g0 + (g1 - g0) * t + 0.5f), (int)(b0 + (b1 - b0) * t + 0.5f));
    }
}

void gfx_pal_black(Frame *f) { memset(f->pal, 0, sizeof f->pal); }

void gfx_glyph16(Frame *f, int x, int y, uint8_t ch, int fg, int bg, int scale) {
    const unsigned char *g = font_vga16[ch];
    for (int row = 0; row < 16; row++) {
        unsigned bits = g[row];
        for (int col = 0; col < 8; col++) {
            int on = (bits >> (7 - col)) & 1;
            int c = on ? fg : bg;
            if (c < 0) continue;
            if (scale == 1) gfx_px(f, x + col, y + row, (uint8_t)c);
            else gfx_rect(f, x + col * scale, y + row * scale, scale, scale, (uint8_t)c);
        }
    }
}

void gfx_glyph8(Frame *f, int x, int y, uint8_t ch, int fg, int bg, int scale) {
    const unsigned char *g = font_bios8[ch];
    for (int row = 0; row < 8; row++) {
        unsigned bits = g[row];
        for (int col = 0; col < 8; col++) {
            int on = (bits >> (7 - col)) & 1;
            int c = on ? fg : bg;
            if (c < 0) continue;
            if (scale == 1) gfx_px(f, x + col, y + row, (uint8_t)c);
            else gfx_rect(f, x + col * scale, y + row * scale, scale, scale, (uint8_t)c);
        }
    }
}

void gfx_text16(Frame *f, int x, int y, const char *s, int fg, int scale) {
    for (; *s; s++, x += 8 * scale) gfx_glyph16(f, x, y, (uint8_t)*s, fg, -1, scale);
}

void gfx_text8(Frame *f, int x, int y, const char *s, int fg, int scale) {
    for (; *s; s++, x += 8 * scale) gfx_glyph8(f, x, y, (uint8_t)*s, fg, -1, scale);
}

int gfx_text_width(const char *s, int scale) { return (int)strlen(s) * 8 * scale; }
