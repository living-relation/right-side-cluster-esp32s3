#!/usr/bin/env bash
# Regenerate LVGL fonts from ../../../fonts (RaceHead.ttf, Aerospace.otf).
# Requires: npm i -g lv_font_conv
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR"
for candidate in \
    "../../../fonts" \
    "../../../../REPOS_OLD/TrackCluster_ECU_dashboard_design_system/fonts" \
    "/c/projects/REPOS_OLD/TrackCluster_ECU_dashboard_design_system/fonts"; do
    if [ -f "${candidate}/RaceHead.ttf" ]; then
        FONTS_DIR="$(cd "$(dirname "${candidate}/RaceHead.ttf")" && pwd)"
        break
    fi
done
: "${FONTS_DIR:?RaceHead.ttf not found — set FONTS_DIR}"

run() {
    local name=$1 src=$2 size=$3 bpp=$4 glyphs=$5
    local out="${OUT_DIR}/${name}.c"
    local force="${FONT_CONV_FORCE:-0}"
    if [ -f "$out" ] && [ "$force" != "1" ]; then
        echo "  skip ${name}.c (delete or FONT_CONV_FORCE=1 to rebuild)"
        return
    fi
    lv_font_conv --font "${FONTS_DIR}/${src}" --size "$size" --bpp "$bpp" \
        --format lvgl --range "$glyphs" --lv-font-name "$name" -o "$out"
}

echo "TrackCluster right — font conversion"
run aerospace_56 Aerospace.otf 56 4 "0x30-0x39,0x2E"
run aerospace_22 Aerospace.otf 22 4 "0x30-0x39,0x2E,0x2D,0x20,0x46,0x49,0x50,0x53,0xB0"
run racehead_22  RaceHead.ttf  22 4 "0x20,0x2D,0x41-0x5A"
run racehead_19  RaceHead.ttf  19 4 "0x20,0x41-0x5A"
run racehead_17  RaceHead.ttf  17 4 "0x20,0x2E,0x30-0x39,0x41-0x5A"
run racehead_14  RaceHead.ttf  14 4 "0x20,0x30-0x39,0x41-0x5A,0xB0"
run racehead_12  RaceHead.ttf  12 4 "0x20,0x25,0x41-0x5A,0xB0"
echo "Done."
