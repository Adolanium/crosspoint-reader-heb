#!/bin/bash
# Hebrew-aware font generation for this fork.
#
# Runs the upstream conversion first, then re-runs the fonts that
# ship without Hebrew glyphs (NotoSans reader, Ubuntu UI, notosans_8),
# layering NotoSansHebrew as a fallback. Bookerly and OpenDyslexic are
# left as upstream produced them — Bookerly is Latin-only; Hebrew
# readers fall back to NotoSans.
#
# NotoSansHebrew only ships Regular and Bold — Italic/BoldItalic reuse
# the Regular/Bold Hebrew masters (Hebrew script has no italic tradition).

set -e

cd "$(dirname "$0")"

bash convert-builtin-fonts.sh

echo ""
echo "Applying Hebrew fallback overrides..."

READER_FONT_STYLES=("Regular" "Italic" "Bold" "BoldItalic")
NOTOSANS_FONT_SIZES=(12 14 16 18)

NOTOSANS_HEBREW_FALLBACK_Regular="../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Regular.ttf"
NOTOSANS_HEBREW_FALLBACK_Italic="../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Regular.ttf"
NOTOSANS_HEBREW_FALLBACK_Bold="../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Bold.ttf"
NOTOSANS_HEBREW_FALLBACK_BoldItalic="../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Bold.ttf"

for size in ${NOTOSANS_FONT_SIZES[@]}; do
  for style in ${READER_FONT_STYLES[@]}; do
    font_name="notosans_${size}_$(echo $style | tr '[:upper:]' '[:lower:]')"
    font_path="../builtinFonts/source/NotoSans/NotoSans-${style}.ttf"
    hebrew_fallback_var="NOTOSANS_HEBREW_FALLBACK_${style}"
    hebrew_fallback="${!hebrew_fallback_var}"
    output_path="../builtinFonts/${font_name}.h"
    python fontconvert.py $font_name $size $font_path $hebrew_fallback --2bit --compress --pnum > $output_path
    echo "Regenerated $output_path (with Hebrew fallback)"
  done
done

UI_FONT_SIZES=(10 12)
UI_FONT_STYLES=("Regular" "Bold")
UI_HEBREW_FALLBACK_Regular="../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Regular.ttf"
UI_HEBREW_FALLBACK_Bold="../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Bold.ttf"

for size in ${UI_FONT_SIZES[@]}; do
  for style in ${UI_FONT_STYLES[@]}; do
    font_name="ubuntu_${size}_$(echo $style | tr '[:upper:]' '[:lower:]')"
    font_path="../builtinFonts/source/Ubuntu/Ubuntu-${style}.ttf"
    hebrew_fallback_var="UI_HEBREW_FALLBACK_${style}"
    hebrew_fallback="${!hebrew_fallback_var}"
    output_path="../builtinFonts/${font_name}.h"
    python fontconvert.py $font_name $size $font_path $hebrew_fallback > $output_path
    echo "Regenerated $output_path (with Hebrew fallback)"
  done
done

python fontconvert.py notosans_8_regular 8 ../builtinFonts/source/NotoSans/NotoSans-Regular.ttf ../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Regular.ttf > ../builtinFonts/notosans_8_regular.h
echo "Regenerated ../builtinFonts/notosans_8_regular.h (with Hebrew fallback)"

echo ""
echo "Running compression verification..."
python verify_compression.py ../builtinFonts/
