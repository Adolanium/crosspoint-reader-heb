# Hebrew-aware font generation for this fork (Windows PowerShell version).
#
# Runs the upstream conversion first, then re-runs the fonts that ship
# without Hebrew glyphs (NotoSans reader, Ubuntu UI, notosans_8),
# layering NotoSansHebrew as a fallback.
#
# Requires: python with freetype-py (pip install freetype-py==2.5.1).

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

# Upstream script handles Bookerly, NotoSans (Latin-only), OpenDyslexic,
# Ubuntu, notosans_8. We then override the non-Latin-capable ones.
bash convert-builtin-fonts.sh

Write-Host ""
Write-Host "Applying Hebrew fallback overrides..."

$readerStyles = @('Regular', 'Italic', 'Bold', 'BoldItalic')
$notoSizes = @(12, 14, 16, 18)
$hebrewNotoRegular = '../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Regular.ttf'
$hebrewNotoBold    = '../builtinFonts/source/NotoSansHebrew/NotoSansHebrew-Bold.ttf'
$hebrewByStyle = @{
  'Regular'    = $hebrewNotoRegular
  'Italic'     = $hebrewNotoRegular
  'Bold'       = $hebrewNotoBold
  'BoldItalic' = $hebrewNotoBold
}

# Use cmd.exe's `>` so output is written as raw bytes (ASCII), matching
# the bash script. PowerShell's native `>` would prepend a UTF-16 BOM.
foreach ($size in $notoSizes) {
  foreach ($style in $readerStyles) {
    $fontName = "notosans_${size}_$($style.ToLower())"
    $fontPath = "../builtinFonts/source/NotoSans/NotoSans-${style}.ttf"
    $hebrewFallback = $hebrewByStyle[$style]
    $outPath = "../builtinFonts/${fontName}.h"
    cmd /c "python fontconvert.py $fontName $size $fontPath $hebrewFallback --2bit --compress --pnum > $outPath"
    if ($LASTEXITCODE -ne 0) { throw "fontconvert failed for $fontName" }
    Write-Host "Regenerated $outPath (with Hebrew fallback)"
  }
}

$uiSizes = @(10, 12)
$uiStyles = @('Regular', 'Bold')
$uiHebrewByStyle = @{
  'Regular' = $hebrewNotoRegular
  'Bold'    = $hebrewNotoBold
}

foreach ($size in $uiSizes) {
  foreach ($style in $uiStyles) {
    $fontName = "ubuntu_${size}_$($style.ToLower())"
    $fontPath = "../builtinFonts/source/Ubuntu/Ubuntu-${style}.ttf"
    $hebrewFallback = $uiHebrewByStyle[$style]
    $outPath = "../builtinFonts/${fontName}.h"
    cmd /c "python fontconvert.py $fontName $size $fontPath $hebrewFallback > $outPath"
    if ($LASTEXITCODE -ne 0) { throw "fontconvert failed for $fontName" }
    Write-Host "Regenerated $outPath (with Hebrew fallback)"
  }
}

cmd /c "python fontconvert.py notosans_8_regular 8 ../builtinFonts/source/NotoSans/NotoSans-Regular.ttf $hebrewNotoRegular > ../builtinFonts/notosans_8_regular.h"
if ($LASTEXITCODE -ne 0) { throw "fontconvert failed for notosans_8_regular" }
Write-Host "Regenerated ../builtinFonts/notosans_8_regular.h (with Hebrew fallback)"

Write-Host ""
Write-Host "Running compression verification..."
python verify_compression.py ../builtinFonts/
