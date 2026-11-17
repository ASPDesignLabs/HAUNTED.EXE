/* Dumps CATACOMB's palette (build_palette, copied from src/art/parts/palette.c by palette.sh) as .pal, .gpl and .csv. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "crawl.h"
uint8_t crawl_pal[256][3];
#include "palette_only.inc"
int main(void) {
    build_palette();
    const char *names[] = { "STONE","WARM","BONE","BRICK","MOSS","WOOD","IRON","SKIN","GHOST","PURPLE","BLOOD","FLOOR","CEIL","FLAME","GLOW" };
    FILE *pal = fopen("haunted_catacomb.pal", "w"), *gpl = fopen("haunted_catacomb.gpl", "w"), *csv = fopen("haunted_catacomb.csv", "w");
    fprintf(pal, "JASC-PAL\r\n0100\r\n256\r\n");
    fprintf(gpl, "GIMP Palette\nName: HAUNTED.EXE CATACOMB\nColumns: 16\n#\n");
    fprintf(csv, "index,name,r6,g6,b6,r8,g8,b8,hex\n");
    for (int i = 0; i < 256; i++) {
        int r6 = crawl_pal[i][0], g6 = crawl_pal[i][1], b6 = crawl_pal[i][2];
        int r = (r6 * 255 + 31) / 63, g = (g6 * 255 + 31) / 63, b = (b6 * 255 + 31) / 63;
        char nm[32];
        if (i == 0) strcpy(nm, "BLACK/UNUSED");
        else if (i <= 240) sprintf(nm, "%s_%d", names[(i - 1) / 16], (i - 1) % 16);
        else if (i == 241) strcpy(nm, "WHITE");
        else if (i <= 254) sprintf(nm, "GOLD_%d", i - 242);
        else strcpy(nm, "MOUSE_EYE(C_CLEAR)");
        fprintf(pal, "%d %d %d\r\n", r, g, b);
        fprintf(gpl, "%3d %3d %3d\t%s\n", r, g, b, nm);
        fprintf(csv, "%d,%s,%d,%d,%d,%d,%d,%d,#%02X%02X%02X\n", i, nm, r6, g6, b6, r, g, b, r, g, b);
    }
    return 0;
}
