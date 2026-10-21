#!/bin/sh
# Assembles the project page into _site/ (or the folder given): site/, the previews from docs/, the
# VGA fonts and the title card chosen in site/card.txt, with @VERSION@ and the card's details filled
# in. The Pages workflow runs this; so can you, then open _site/index.html or serve it with
# "python3 -m http.server -d _site".
set -eu
cd "$(dirname "$0")/.."
OUT=${1:-_site}
VERSION=$(cat VERSION)
CARD=$(awk -v v="$VERSION" '/^#/ { next } $1 == v { c = $2 } $1 == "*" && d == "" { d = $2 }
                            END { print (c != "" ? c : d) }' site/card.txt)
if [ ! -f "site/cards/$CARD.png" ] || [ ! -f "site/cards/$CARD.txt" ]; then
  echo "site/card.txt names \"$CARD\" for $VERSION, but site/cards/$CARD.png or .txt is missing" >&2
  exit 1
fi
ALT=$(cat "site/cards/$CARD.txt")
rm -rf "$OUT"
mkdir -p "$OUT/img" "$OUT/fonts"
cp -R site/. "$OUT/"
rm "$OUT/card.txt"
cp "site/cards/$CARD.png" "$OUT/card.png"
cp docs/*.png "$OUT/img/"
cp third_party/font/Px437_IBM_VGA_8x16.ttf third_party/font/Px437_IBM_BIOS.ttf \
   third_party/font/LICENSE-oldschool-pc-font-pack.txt "$OUT/fonts/"
sed -e "s/@VERSION@/$VERSION/g" -e "s/@CARD@/$CARD/g" -e "s|@CARD_ALT@|$ALT|g" site/index.html > "$OUT/index.html"
echo "site for $VERSION in $OUT, title card $CARD"
