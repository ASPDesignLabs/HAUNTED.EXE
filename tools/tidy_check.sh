#!/bin/sh
# Proves that a tidy-up changed nothing of the game (docs/PLAN-tidy.md): this tree against the tree at another
# commit, by default 1.0.0b21.5 as it stood before the tidy-up, each built in its own place and compared.
#
#   sh tools/tidy_check.sh [objects|all] [rev]      needs gcc, zlib and git; all needs python3 too
#
# objects (the default): every translation unit of the harness and the tools, compiled in the release and the
#   debug configurations with the Makefile's flags. Each object must be the other tree's, byte for byte. A file
#   is found by its name wherever it lives under src/ and tools/, so the check holds across a move, and across
#   a file cut into parts that it #includes. About a minute and a half a tree on four cores.
# all: the objects, then what the Makefile builds on Linux: the binaries, the smoke test's frames, the sheets,
#   the items and the balance sheet, the assembled site, and two fixed runs of the dream, line for line (a line
#   that prints a time taken is left out). About five minutes a tree.
#
# The other tree is a worktree under build/tidy/<rev>, measured once and kept (make clean takes it away; git
# worktree prune then forgets it). Both sides are given this tree's build/version.h, so a bumped VERSION
# doesn't show as a difference. It can't build for Windows or the browser: CI does that.
set -eu
CFLAGS="-O2 -std=gnu11 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation"
UNITS="scene_crawl crawl_art app gfx font_vga snd hand_img crawl_gear face_img cat_img scene_haunt haunt_art
       crawl_ui ui_art crawl_title crawl_portrait crawl_tutor host sheet uisheet titlemock portraitmock items"
BINS="build/host build/host_debug build/sheet build/uisheet build/titlemock build/portraitmock build/items"

if [ "${1:-}" = --one ]; then                       # --one <config> <unit>: a single object, called back by xargs
    f=$(find "$TREE/src" "$TREE/tools" -maxdepth 2 -name "$3.c" | head -1)   # not deeper: there lie the parts of a file
    [ -n "$f" ] || { echo "no $3.c in $TREE" > "$TMP/warn-$2-$3.txt"; exit 0; }
    d=""; [ "$2" = debug ] && d="-DHAUNT_DEBUG -DCRAWL_DEBUG"
    cd "$(dirname "$f")"                            # from the file's own folder, under its bare name
    gcc $CFLAGS $d $INC -c -o "$TMP/$2-$3.o" "$(basename "$f")" 2> "$TMP/warn-$2-$3.txt" || true
    exit 0
fi

MODE=${1:-objects}
REV=${2:-a38b19c}
case "$MODE" in objects|all) ;; *) echo "usage: sh tools/tidy_check.sh [objects|all] [rev]" >&2; exit 2 ;; esac
ROOT=$(cd "$(dirname "$0")/.." && pwd)
J=$(nproc 2>/dev/null || echo 2)
KEEP=$ROOT/build/tidy
BASE=$KEEP/$REV
mkdir -p "$KEEP"
( cd "$ROOT" && make -s build/version.h )
if [ ! -d "$BASE" ]; then
    git -C "$ROOT" worktree prune
    git -C "$ROOT" worktree add -q --detach "$BASE" "$REV"
fi
mkdir -p "$BASE/build" && cp "$ROOT/build/version.h" "$BASE/build/version.h"

objects() {                                         # tree, out file
    TREE=$1; TMP=$(mktemp -d)
    INC="-I$TREE $(find "$TREE/src" -type d | sed 's/^/-I/' | tr '\n' ' ')"
    export TREE TMP CFLAGS INC
    for c in rel debug; do for u in $UNITS; do echo "$c $u"; done; done | xargs -P "$J" -L 1 sh "$0" --one
    for c in rel debug; do for u in $UNITS; do
        if [ -f "$TMP/$c-$u.o" ]; then printf '%s %s %s\n' "$c" "$u" "$(sha256sum < "$TMP/$c-$u.o" | cut -d' ' -f1)"
        else printf '%s %s FAILED: %s\n' "$c" "$u" "$(head -c 300 "$TMP/warn-$c-$u.txt" | tr '\n' ' ')"; fi
    done; done | sort > "$2"
    cat "$TMP"/warn-*.txt | grep -c 'warning:' > "$2.warnings" || true
    rm -rf "$TMP"
}

sums() { ( cd "$1" && find . -type f | sort | xargs sha256sum ); }   # every file under a folder, by its name there

outputs() {                                         # tree, out file
    T=$1; O=$2; L=$2.log; : > "$O"
    ( cd "$T"
      make -j"$J" $BINS > "$L" 2>&1
      echo "# binaries"; sha256sum $BINS
      rm -rf build/smoke build/sheets
      make smoke >> "$L" 2>&1;        echo "# smoke frames";  sums build/smoke
      make sheets >> "$L" 2>&1;       echo "# sheets";        sums build/sheets
      make items combat >> "$L" 2>&1; echo "# items, combat"; sha256sum build/items.json build/combat.txt
      rm -rf build/tidy_site
      sh tools/build_site.sh build/tidy_site >> "$L" 2>&1; echo "# site"; sums build/tidy_site
      rm -rf build/tidy_site
      echo "# the dream"
      ( CRAWL_SEED=3 CRAWL_RUNID=7 ./build/host_debug apl crawl 1200 2>&1 | grep -vE ' in [0-9.]+ ms$' > build/tidy_dream1.log ) &
      ( CRAWL_SEED=9 CRAWL_RUNID=7 CRAWL_LEVEL=21 CRAWL_GEAR=5 ./build/host_debug apl crawl 1200 2>&1 \
            | grep -vE ' in [0-9.]+ ms$' > build/tidy_dream2.log ) &
      wait
      for d in 1 2; do printf '%s  dream%s (%s lines)\n' "$(sha256sum < build/tidy_dream$d.log | cut -d' ' -f1)" $d \
          "$(wc -l < build/tidy_dream$d.log | tr -d ' ')"; done
    ) >> "$O"
}

verdict() {                                         # what, base file, this file
    if cmp -s "$2" "$3"; then echo "$1: alike ($(grep -vc '^#' "$3") checksums)"
    else echo "$1: DIFFER (< there, > here)"; diff "$2" "$3" | grep '^[<>]' | cut -c1-120 | head -40; bad=1; fi
}

bad=0
echo "this tree against $REV ($(git -C "$ROOT" log -1 --format=%s "$REV"))"
[ -s "$KEEP/$REV.objects" ] || { echo "measuring $REV's objects (kept for next time) ..."; objects "$BASE" "$KEEP/$REV.objects"; }
echo "compiling this tree's objects ..."
objects "$ROOT" "$KEEP/now.objects"
if grep -q FAILED "$KEEP/$REV.objects" "$KEEP/now.objects"; then
    echo "objects: DID NOT COMPILE"; grep -h FAILED "$KEEP/$REV.objects" "$KEEP/now.objects" | cut -c1-400; bad=1
else
    verdict "objects" "$KEEP/$REV.objects" "$KEEP/now.objects"
fi
echo "compiler warnings: $(cat "$KEEP/$REV.objects.warnings") there, $(cat "$KEEP/now.objects.warnings") here"
if [ "$MODE" = all ]; then
    [ -s "$KEEP/$REV.outputs" ] || { echo "building $REV and running it (kept for next time) ..."; outputs "$BASE" "$KEEP/$REV.outputs"; }
    echo "building this tree and running it ..."
    outputs "$ROOT" "$KEEP/now.outputs"
    for part in "binaries" "smoke frames" "sheets" "items, combat" "site" "the dream"; do
        for side in "$REV" now; do
            awk -v p="# $part" '$0 == p { on = 1; next } /^#/ { on = 0 } on' "$KEEP/$side.outputs" > "$KEEP/$side.part"
        done
        verdict "$part" "$KEEP/$REV.part" "$KEEP/now.part"
    done
    rm -f "$KEEP"/*.part
fi
[ "$bad" = 0 ] && echo "NOTHING MOVED" || { echo "SOMETHING MOVED"; exit 1; }
