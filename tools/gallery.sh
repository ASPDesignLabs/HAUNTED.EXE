#!/bin/sh
# The page's gallery (site/index.html, "Then and now"): the same moments in the first beta (1.0.0b1), the thirteenth
# (1.0.0b13) and this build, each from its own build of the harness, as single frames of the game's own 320x200 in
# docs/gallery/<set>-<name>-<era>.png. The old builds are made in worktrees under build/gallery/; the thirteenth is given
# one switch it lacks, CRAWL_SEENALL (every kind met, for the bestiary's pages), which changes nothing it draws.
#
#   sh tools/gallery.sh            needs gcc, zlib, git and python3
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=$ROOT/build/gallery
mkdir -p "$OUT" "$ROOT/docs/gallery"
host_of() {                                                        # the harness of an era, built once
    case "$1" in now) echo "$ROOT/build/host_debug"; return ;; b1) rev=98c26ad ;; b13) rev=31ab8d5 ;; esac
    wt=$OUT/wt-$1
    if [ ! -x "$wt/build/host_debug" ]; then
        [ -d "$wt" ] || git -C "$ROOT" worktree add -q "$wt" "$rev"
        python3 - "$wt/src/scene_crawl.c" <<'EOF'
import sys
p = sys.argv[1]; s = open(p).read()
a = '    if (getenv("CRAWL_GOLD")) RUN.treasure = atoi(getenv("CRAWL_GOLD"));'
if 'CRAWL_SEENALL' not in s and a in s and 'uint8_t seen[REC_KINDS]' in s:
    s = s.replace(a, a + '''
    if (getenv("CRAWL_SEENALL")) for (int k = 0; k < NKINDS && k < REC_KINDS; k++) {
        RUN.rec.seen[k] = 0x1f;
        for (int r = 0; r < NRANK; r++) RUN.rec.kills[k][r] = (uint16_t)((k * 7 + r * 5) % 23 + (r == 0 ? 30 : 0));
    }''', 1)
    open(p, 'w').write(s)
EOF
        ( cd "$wt" && make build/host_debug >/dev/null 2>&1 )
    fi
    echo "$wt/build/host_debug"
}
[ -x "$ROOT/build/host_debug" ] || ( cd "$ROOT" && make build/host_debug >/dev/null )
T="1:+H;4.3:-H"
B="$T;5:+B;5.1:-B"
pages() {                                                          # the bestiary, then D to its n-th page; "input|time"
    python3 -c "n=$1; print('$B' + ''.join(';%.2f:+D;%.2f:-D' % (5.5 + 0.35 * i, 5.6 + 0.35 * i) for i in range(n)) + '|%.2f' % (5.8 + 0.35 * n))"
}
# set | name | eras | environment | input | time of the shot
RECIPES="crypt|level1|b1 b13 now|CRAWL_SEED=3||46
crypt|ossuary|b1 b13 now|CRAWL_SEED=5||75
crypt|shop|b1 b13 now|CRAWL_SEED=3||3
crypt|cast|b1 b13 now|CRAWL_SEED=3|30:+H;33.3:-H;35:+MOUSE1;35.2:-MOUSE1|35.5
crypt|deep|b13|CRAWL_SEED=7 CRAWL_LEVEL=33||40
crypt|deep|now|CRAWL_SEED=7 CRAWL_LEVEL=33||90
screens|quit|b1 b13 now|CRAWL_SEED=3|30:+H;33.3:-H;35:+ESC;35.1:-ESC|36
screens|wardrobe|b13 now|CRAWL_SEED=3 CRAWL_GEAR=3|$T;5:+TAB;5.1:-TAB|6
screens|merchant|b13 now|CRAWL_TPCAT=1|$T;5:+E;5.1:-E|6
screens|lamp|b13 now|CRAWL_LEVEL=31 CRAWL_TPLAMP=1 CRAWL_ALIGN=c2b CRAWL_ATONE=24 CRAWL_ALPTS=4|$T;5:+E;5.1:-E;5.6:+S;5.7:-S;5.9:+S;6:-S;6.2:+D;6.3:-D|6.8
screens|tally|b13 now|CRAWL_SEED=3 CRAWL_TPEXIT=1|60:+H;63.3:-H;65:+E;65.1:-E;66:+S;66.1:-S;66.5:+E;66.6:-E|77.5
screens|codex|b13 now|CRAWL_GEAR=3|$T;5:+C;5.1:-C|6
screens|death|b13 now|CRAWL_SEED=3 CRAWL_KILLAT=64|60:+H;63.3:-H|67.5
dead|skeleton|b13 now|CRAWL_SEENALL=1|$(pages 0)
dead|wallskull|b13 now|CRAWL_SEENALL=1|$(pages 4)
dead|shambler|b13 now|CRAWL_SEENALL=1|$(pages 3)
dead|drowned|b13 now|CRAWL_SEENALL=1|$(pages 7)
dead|hound|b13 now|CRAWL_SEENALL=1|$(pages 8)
dead|ghoul|b13 now|CRAWL_SEENALL=1|$(pages 11)
dead|knight|b13 now|CRAWL_SEENALL=1|$(pages 12)
dead|imp|b13 now|CRAWL_SEENALL=1|$(pages 16)
dead|gargoyle|b13 now|CRAWL_SEENALL=1|$(pages 18)
new|whipper|now|CRAWL_SEENALL=1 CRAWL_LEVEL=21|$(pages 19)
new|sexton|now|CRAWL_SEENALL=1 CRAWL_LEVEL=21|$(pages 20)
new|lightless|now|CRAWL_SEENALL=1 CRAWL_LEVEL=31|$(pages 21)
new|bossfight|now|CRAWL_SEED=7 CRAWL_LEVEL=10 CRAWL_TPBOSS=1 CRAWL_GEAR=3|60:+H;63.3:-H;64:+W;70:-W|73
new|totem|now|CRAWL_TPTOTEM=1 CRAWL_TOTEM=2130 CRAWL_SCRIP=2|$T;5:+E;5.1:-E|6.5
new|class|now|CRAWL_TPCAT=1 CRAWL_COINS=6 CRAWL_ATONE=25|$T;5:+E;5.1:-E;5.6:+S;5.7:-S;5.9:+S;6:-S;6.2:+S;6.3:-S;6.6:+E;6.7:-E;7.2:+D;7.3:-D|8
new|stone|now|CRAWL_TPSTONE=1 CRAWL_STONE=3 CRAWL_GEAR=3|$T;5:+E;5.1:-E|6.5
new|pocket|now|CRAWL_SEED=3 CRAWL_TPDOOR=2|60:+H;63.3:-H|74"
echo "$RECIPES" | while IFS='|' read -r SET NAME ERAS ENVS INPUT AT; do
    for ERA in $ERAS; do
        H=$(host_of "$ERA")
        D=$OUT/$ERA; mkdir -p "$D"
        ( cd "$D" && rm -f "$NAME"_*.png && env $ENVS HOST_INPUT="$INPUT" "$H" shot crawl "$AT" "$NAME" 1 >/dev/null 2>&1 \
              && mv "$NAME"_*.png "$ROOT/docs/gallery/$SET-$NAME-$ERA.png" ) || echo "no frame: $SET $NAME $ERA" >&2
    done
done
ls "$ROOT/docs/gallery" | wc -l
