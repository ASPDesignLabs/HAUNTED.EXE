/* Rolled items, for checking names and numbers before the game leans on them (docs/PLAN-items.md):
 * what the merchant's wares pedestal would put out, so many a level, as JSON on stdout.
 *
 *   items [first last [n [seed]]]      levels first..last (default 1..20, then 21 and 41), n each (100)
 */
#include "../src/crawl_gear.h"
#include "../src/crawl.h"
#include <stdio.h>
#include <stdlib.h>

static void json_str(const char *s) {
    putchar('"');
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') putchar('\\');
        putchar(*s);
    }
    putchar('"');
}

static void level_items(int level, int n, uint32_t seed, int first) {
    uint32_t rs = seed ^ (uint32_t)level * 2654435761u;
    if (!rs) rs = 1;
    Gear g;
    gear_reset(&g);                                     /* a caster with no bag: bags come up as often as they can */
    printf("%s{\"level\":%d,\"set\":%d,\"loop\":%d,\"haul\":%.0f,\"items\":[", first ? "" : ",\n", level,
           crawl_set(level), crawl_loop(level), gear_haul(level));
    for (int i = 0; i < n; i++) {
        Offer o = gear_offer(POOL_GEAR, level, &g, &rs);
        const Piece *p = &o.piece;
        char l1[64], l2[64], buf[96];
        piece_name(p, l1, l2, sizeof l1);
        printf("%s\n {\"kind\":", i ? "," : "");
        json_str(KIND_NAME[p->kind]);
        printf(",\"q\":%d,\"anointed\":%d,\"line1\":", p->quality, p->anointed);
        json_str(l1);
        printf(",\"line2\":");
        json_str(l2);
        printf(",\"price\":%d,\"worth\":%.2f,\"stats\":[", o.price, piece_worth(p, level));
        for (int k = 0; k < p->nstat; k++) {
            piece_stat_text(p->stat[k], p->val[k], buf, sizeof buf);
            printf("%s{\"stat\":", k ? "," : "");
            json_str(STAT_NAME[p->stat[k]]);
            printf(",\"text\":");
            json_str(buf);
            printf(",\"v\":%d}", p->val[k]);
        }
        printf("],\"ability\":");
        json_str(ABIL_NAME[p->ability]);
        piece_ability_text(p, buf, sizeof buf);
        printf(",\"abtext\":");
        json_str(buf);
        if (p->kind == IK_BAG) printf(",\"holds\":%d", bag_capacity(p->quality));
        printf("}");
    }
    printf("]}");
}

int main(int argc, char **argv) {
    int first = argc > 2 ? atoi(argv[1]) : 1, last = argc > 2 ? atoi(argv[2]) : 20;
    int n = argc > 3 ? atoi(argv[3]) : 100;
    uint32_t seed = argc > 4 ? (uint32_t)strtoul(argv[4], NULL, 10) : 20261031u;
    printf("{\"qualities\":[");
    for (int q = 0; q < NQUAL; q++) { printf("%s", q ? "," : ""); json_str(QUAL_NAME[q]); }
    printf("],\"levels\":[\n");
    int any = 0;
    for (int l = first; l <= last; l++) { level_items(l, n, seed, !any); any = 1; }
    if (argc <= 2) { level_items(21, n, seed, 0); level_items(41, n, seed, 0); }   /* once round, twice round */
    printf("\n]}\n");
    return 0;
}
