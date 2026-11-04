#!/bin/sh
# CATACOMB's pieces for the ear (docs/PLAN-music.md): each run's piece through its whole order, her hall (her room,
# then her back rooms), the menus' tune in its four guises, and each run's piece through a fight that grows, the
# fight's end, the failing, and a menu opened and closed. Into build/music/, as MP3 when ffmpeg is there.
#   sh tools/music.sh            all of it
#   sh tools/music.sh crypts     one piece and its moments
set -e
cd "$(dirname "$0")/.."
make -s host
out=build/music
mkdir -p "$out"

enc() {   # a WAV to MP3 if it can, then the WAV goes
    if command -v ffmpeg >/dev/null 2>&1; then
        ffmpeg -loglevel error -y -i "$out/$1.wav" -codec:a libmp3lame -b:a 160k "$out/$1.mp3" && rm "$out/$1.wav"
    fi
}
piece() {  # name seconds [events]
    n=$1; s=$2; shift 2
    build/host music "$n" "$s" "$out/$n.wav" "$@"; enc "$n"
}
moments() {  # a run's piece: calm, a fight at 1, 2 and 3, its end, the failing, a menu
    build/host music "$1" 140 "$out/$1-moments.wav" fight1@15 fight2@35 fight3@55 fight0@75 frail@90 well@110 menu@115 back@130
    enc "$1-moments"
}
want() { [ -z "$1" ] || [ "$1" = "$2" ]; }

# the whole order of each: 8 sections of 8 bars, and a few seconds over
want "$1" crypts    && { piece crypts 240;    moments crypts; }
want "$1" catacombs && { piece catacombs 160; moments catacombs; }
want "$1" hollows   && { piece hollows 190;   moments hollows; }
want "$1" deep      && { piece deep 280;      moments deep; }
want "$1" merchant  && piece merchant 100 place2@60
if want "$1" menus; then
    for k in 1 2 3 4; do build/host music menu$k 40 "$out/menu$k.wav"; done
    if command -v ffmpeg >/dev/null 2>&1; then
        ffmpeg -loglevel error -y -i "$out/menu1.wav" -i "$out/menu2.wav" -i "$out/menu3.wav" -i "$out/menu4.wav" \
            -filter_complex "concat=n=4:v=0:a=1" -codec:a libmp3lame -b:a 160k "$out/menus.mp3"
        rm "$out"/menu[1-4].wav
    fi
fi
ls -l "$out"
