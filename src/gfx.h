/* Drawing primitives on the indexed Frame. */
#ifndef GFX_H
#define GFX_H

#include "core.h"

static inline void gfx_px(Frame *f, int x, int y, uint8_t c) {
    if ((unsigned)x < (unsigned)f->w && (unsigned)y < (unsigned)f->h)
        f->pix[y * f->w + x] = c;
}

void gfx_clear(Frame *f, uint8_t c);
void gfx_rect(Frame *f, int x, int y, int w, int h, uint8_t c);
void gfx_line(Frame *f, int x0, int y0, int x1, int y1, uint8_t c);
void gfx_disc(Frame *f, int cx, int cy, int r, uint8_t c);

/* DAC values are 0..63 */
void gfx_pal(Frame *f, int i, int r, int g, int b);
void gfx_pal_ramp(Frame *f, int first, int last,
                  int r0, int g0, int b0, int r1, int g1, int b1);
void gfx_pal_black(Frame *f);

/* Glyph drawing; bg < 0 means transparent.  scale is an integer multiplier. */
void gfx_glyph16(Frame *f, int x, int y, uint8_t ch, int fg, int bg, int scale);
void gfx_glyph8(Frame *f, int x, int y, uint8_t ch, int fg, int bg, int scale);
void gfx_text16(Frame *f, int x, int y, const char *s, int fg, int scale);
void gfx_text8(Frame *f, int x, int y, const char *s, int fg, int scale);
int  gfx_text_width(const char *s, int scale);     /* 8 px per glyph */

#endif
