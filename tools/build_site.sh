#!/bin/sh
# Assembles the project page into _site/ (or the folder given): site/, the previews from docs/ and the
# VGA fonts, with @VERSION@ replaced from VERSION. The Pages workflow runs this; so can you, then
# open _site/index.html or serve it with "python3 -m http.server -d _site".
set -eu
cd "$(dirname "$0")/.."
OUT=${1:-_site}
VERSION=$(cat VERSION)
rm -rf "$OUT"
mkdir -p "$OUT/img" "$OUT/fonts"
cp -R site/. "$OUT/"
cp docs/*.png "$OUT/img/"
cp third_party/font/Px437_IBM_VGA_8x16.ttf third_party/font/Px437_IBM_BIOS.ttf \
   third_party/font/LICENSE-oldschool-pc-font-pack.txt "$OUT/fonts/"
sed "s/@VERSION@/$VERSION/g" site/index.html > "$OUT/index.html"
echo "site for $VERSION in $OUT"
