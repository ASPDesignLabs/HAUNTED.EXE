#!/bin/sh
# Every screen of CATACOMB's that the harness can open, at each run's palette, laid out as contact sheets
# for docs/PLAN-ui.md (the pass over the screens). Needs build/host_debug and ImageMagick's montage.
#
#   sh tools/uishots.sh <label> [levels]     e.g. sh tools/uishots.sh before "1 11 21 31"
#
# Frames go to build/uishots/<label>/, the sheets to docs/uipass/<label>-L<level>.png; what the fit checks found
# (text over text, or out of its stone) to build/uishots/<label>/L<level>/fit.txt.
set -e
LABEL=${1:-now}
LEVELS=${2:-"1 11 21 31"}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
HOST=$ROOT/build/host_debug
OUT=$ROOT/build/uishots/$LABEL
[ -x "$HOST" ] || { echo "make build/host_debug first" >&2; exit 1; }
mkdir -p "$OUT" "$ROOT/docs/uipass"
T="1:+H;4.3:-H"                                   # take over, then the key that opens the screen at 5 s
E="$T;5:+E;5.1:-E"
# name | environment | input | time of the shot
RECIPES="lamp|CRAWL_TPLAMP=1 CRAWL_ALPTS=6|$E|6.5
totem|CRAWL_TPTOTEM=1 CRAWL_TOTEM=2130 CRAWL_SCRIP=2|$E|6.5
class|CRAWL_TPCAT=1 CRAWL_COINS=6 CRAWL_ATONE=25|$E;5.6:+S;5.7:-S;5.9:+S;6:-S;6.2:+S;6.3:-S;6.6:+E;6.7:-E;7.2:+D;7.3:-D|8
portal|CRAWL_TPPORTAL=1 CRAWL_RUNTOP=2 CRAWL_SCRIP=1 CRAWL_CLEARED=12|$E|6.5
compendium|CRAWL_TPLECTERN=1 CRAWL_GEAR=3|$E|6.5
repository|CRAWL_TPREPO=1|$E|6.5
merchant|CRAWL_TPCAT=1|$E|6.5
ascend|CRAWL_TPCAT=1 CRAWL_XP=20000 CRAWL_CORNER=31|$E;5.6:+S;5.7:-S;6.2:+E;6.3:-E|7.5
shrine|CRAWL_TPSHRINE=1 CRAWL_COINS=6 CRAWL_SHRINE=2 CRAWL_RUNES=2|$E|6.5
ledger|CRAWL_TPCAT=1|$E;5.6:+ENTER;5.7:-ENTER|6.8
stone|CRAWL_TPSTONE=1 CRAWL_STONE=3 CRAWL_GEAR=3|$E|6.5
stonepiece|CRAWL_TPSTONE=1 CRAWL_STONE=3 CRAWL_GEAR=3|$E;5.6:+S;5.7:-S|6.8
letgo|CRAWL_GEAR=3 CRAWL_BAG=1 CRAWL_LETGO=1|$T|5.5
bell|CRAWL_TPBELL=1|$E|6.5
font|CRAWL_TPFONT=1|$E|6.5
quit||$T;5:+ESC;5.1:-ESC|6.5
wardrobe|CRAWL_GEAR=3|$T;5:+TAB;5.1:-TAB|6.5
caster|CRAWL_GEAR=3|$T;5:+TAB;5.1:-TAB;5.5:+D;5.6:-D|6.5
cornerstones|CRAWL_GEAR=3 CRAWL_CORNER=5|$T;5:+TAB;5.1:-TAB;5.5:+D;5.6:-D;5.8:+D;5.9:-D|6.5
alignment|CRAWL_GEAR=3|$T;5:+TAB;5.1:-TAB;5.5:+A;5.6:-A;5.8:+A;5.9:-A;6.1:+A;6.2:-A|6.8
bestiary||$T;5:+B;5.1:-B|6.5
elements|CRAWL_GEAR=3|$T;5:+TAB;5.1:-TAB;5.5:+A;5.6:-A;5.8:+A;5.9:-A|6.8
documents|CRAWL_GEAR=3|$T;5:+TAB;5.1:-TAB;5.5:+A;5.6:-A|6.8
achieve||$T;5:+B;5.1:-B;5.4:+A;5.5:-A|6.5
codex|CRAWL_GEAR=3|$T;5:+C;5.1:-C|6.5
controls||$T|5.5
wisps|CRAWL_SEED=3|60:+H;63.3:-H;65:+L;65.1:-L|66.5
altar|CRAWL_LEVEL=AT+6 CRAWL_SEED=3 CRAWL_TPALTAR=1 CRAWL_SOULS=40|60:+H;63.3:-H;65:+E;65.1:-E|66.5
bshrine|CRAWL_LEVEL=AT+7 CRAWL_SEED=3 CRAWL_TPBSHRINE=1 CRAWL_SOULS=40|60:+H;63.3:-H;65:+E;65.1:-E|66.5
levelcard|CRAWL_SEED=3 CRAWL_HOLDCARD=1||60
bosscard|CRAWL_LEVEL=AT+9 CRAWL_TPBOSS=2 CRAWL_HOLDCARD=1||80
death|CRAWL_SEED=3 CRAWL_KILLAT=64|60:+H;63.3:-H|67.5
perished|CRAWL_SEED=3 CRAWL_KILLAT=64||66.5
claim|CRAWL_SEED=3 CRAWL_CLAIM=1|60:+H;63.3:-H|65
tally|CRAWL_SEED=3 CRAWL_TPEXIT=1 CRAWL_CORRUPT=1|60:+H;63.3:-H;65:+E;65.1:-E;66:+S;66.1:-S;66.5:+E;66.6:-E|77.5"
for L in $LEVELS; do
    D=$OUT/L$L
    mkdir -p "$D"
    rm -f "$D"/*.png
    echo "$RECIPES" | { while IFS='|' read -r NAME ENVS INPUT AT; do
        ENVS=$(echo "$ENVS" | sed "s/AT+6/$((L + 6))/; s/AT+7/$((L + 7))/; s/AT+9/$((L + 9))/")   # this run's levels with an altar, a puzzle, its boss
        ( cd "$D" && env CRAWL_LEVEL=$L $ENVS HOST_INPUT="$INPUT" "$HOST" shot crawl "$AT" "$NAME" 2 >/dev/null 2>"$NAME.log" \
              && for f in "$NAME"_*.png; do mv "$f" "$NAME.png"; done ) &
    done; wait; }
    cat "$D"/*.log | grep "^UI overflow" | sort -u > "$D/fit.txt" || true   # the fit checks' findings
    echo "L$L: $(wc -l < "$D/fit.txt") fit findings (build/uishots/$LABEL/L$L/fit.txt)"
    montage -label '%t' "$D"/*.png -tile 4x -geometry 640x400+6+18 -pointsize 14 -fill '#ccc' -background '#161616' \
        "$ROOT/docs/uipass/$LABEL-L$L.png"
    echo "docs/uipass/$LABEL-L$L.png"
done
