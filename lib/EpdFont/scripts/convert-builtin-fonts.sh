#!/bin/bash

set -e

cd "$(dirname "$0")"

READER_FONT_STYLES=("Regular" "Italic" "Bold" "BoldItalic")
BOOKERLY_FONT_SIZES=(12 14 16 18)
NOTOSANS_FONT_SIZES=(12 14 16 18)
OPENDYSLEXIC_FONT_SIZES=(8 10 12 14)

for size in ${BOOKERLY_FONT_SIZES[@]}; do
  for style in ${READER_FONT_STYLES[@]}; do
    font_name="bookerly_${size}_$(echo $style | tr '[:upper:]' '[:lower:]')"
    font_path="../builtinFonts/source/Bookerly/Bookerly-${style}.ttf"
    output_path="../builtinFonts/${font_name}.h"
    python fontconvert.py $font_name $size $font_path --2bit --compress > $output_path
    echo "Generated $output_path"
  done
done

# NotoSansHebrew provides Regular and Bold only (Hebrew script has no italic tradition).
# For Italic/BoldItalic variants, the Regular/Bold Hebrew fallback is used respectively.
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
    python fontconvert.py $font_name $size $font_path $hebrew_fallback --2bit --compress > $output_path
    echo "Generated $output_path (with Hebrew fallback)"
  done
done

for size in ${OPENDYSLEXIC_FONT_SIZES[@]}; do
  for style in ${READER_FONT_STYLES[@]}; do
    font_name="opendyslexic_${size}_$(echo $style | tr '[:upper:]' '[:lower:]')"
    font_path="../builtinFonts/source/OpenDyslexic/OpenDyslexic-${style}.otf"
    output_path="../builtinFonts/${font_name}.h"
    python fontconvert.py $font_name $size $font_path --2bit --compress > $output_path
    echo "Generated $output_path"
  done
done

UI_FONT_SIZES=(10 12)
UI_FONT_STYLES=("Regular" "Bold")

# Hebrew fallback for UI fonts (Ubuntu has no Hebrew glyphs)
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
    echo "Generated $output_path (with Hebrew fallback)"
  done
done

python fontconvert.py notosans_8_regular 8 ../builtinFonts/source/NotoSans/NotoSans-Regular.ttf ../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Regular.ttf > ../builtinFonts/notosans_8_regular.h

echo ""
echo "Running compression verification..."
python verify_compression.py ../builtinFonts/
