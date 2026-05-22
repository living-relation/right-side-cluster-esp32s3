#!/usr/bin/env bash
set -e
FONTS_DIR="../../../fonts"
OUT_DIR="$(cd "$(dirname "$0")" && pwd)"
run() {
    local name=$1 src=$2 size=$3 bpp=$4 glyphs=$5
    local out="$OUT_DIR/.c"
    [ -f "$out" ] && { echo "  skip $name.c"; return; }
    lv_font_conv --font "$FONTS_DIR/$src" --size "$size" --bpp "$bpp" \
        --format lvgl --range "$glyphs" -o "$out"
}
echo "TrackCluster right — font conversion"
run aerospace_56 Aerospace.otf 56 4 "0x30-0x39,0x2E"
run racehead_14  RaceHead.ttf  14 4 "0x20,0x41-0x5A"
run racehead_12  RaceHead.ttf  12 4 "0x20,0x25,0x41-0x5A,0xB0"
echo "Done."
