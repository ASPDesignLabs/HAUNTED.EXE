#!/bin/sh
# Regenerates the palette files here from the game's own build_palette (run from the repo root: sh tools/palette/palette.sh).
set -e
S=src; T=$(mktemp -d); O=tools/palette
sed -n '/^static void ramp3/,/^\/\* Darkening/p' $S/art/parts/palette.c | sed '$d' > $T/palette_only.inc
cp $O/dump.c $T/
(cd $T && gcc -I"$OLDPWD/$S/crawl" -I"$OLDPWD/$S/core" -o dump dump.c && ./dump)
cp $T/haunted_catacomb.* $O/
python3 $O/palette_png.py $O/haunted_catacomb.csv $O/haunted_catacomb.png
rm -rf $T
