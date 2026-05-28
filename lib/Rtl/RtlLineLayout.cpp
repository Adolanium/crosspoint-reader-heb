#include <GfxRenderer.h>
#include <Rtl.h>
#include <Utf8.h>

#include <cstdint>

#include "Epub/blocks/BlockStyle.h"
#include "Epub/css/CssStyle.h"

namespace Rtl::LineLayout {

namespace {

constexpr uint32_t SOFT_HYPHEN_CP = 0x00AD;

uint32_t firstCodepoint(const std::string& word) {
  const auto* ptr = reinterpret_cast<const unsigned char*>(word.c_str());
  while (true) {
    const uint32_t cp = utf8NextCodepoint(&ptr);
    if (cp == 0) return 0;
    if (cp != SOFT_HYPHEN_CP) return cp;
  }
}

uint32_t lastCodepoint(const std::string& word) {
  if (word.empty()) return 0;
  size_t i = word.size() - 1;
  while (i > 0 && (static_cast<uint8_t>(word[i]) & 0xC0) == 0x80) {
    --i;
  }
  const auto* ptr = reinterpret_cast<const unsigned char*>(word.c_str() + i);
  return utf8NextCodepoint(&ptr);
}

}  // namespace

bool isNaturalAlignment(const bool isRtl, const CssTextAlign align) {
  if (align == CssTextAlign::Justify) return true;
  return isRtl ? (align == CssTextAlign::Right) : (align == CssTextAlign::Left);
}

std::vector<int16_t> positionLineRtl(const BlockStyle& blockStyle, const std::vector<std::string>& words,
                                     const std::vector<EpdFontFamily::Style>& wordStyles,
                                     const std::vector<uint16_t>& wordWidths, const std::vector<bool>& continuesVec,
                                     const size_t lastBreakAt, const size_t lineWordCount, const int effectivePageWidth,
                                     const int totalNaturalGaps, const int lineWordWidthSum, const int justifyExtra,
                                     const bool isLastLine, const int fontId, const GfxRenderer& renderer) {
  std::vector<int16_t> lineXPos;
  lineXPos.reserve(lineWordCount);

  // Hebrew logical order = reading order: first word in the vector reads first and
  // sits at the rightmost position. Iterate in logical order, advancing leftward.
  auto xpos = static_cast<int16_t>(effectivePageWidth);
  if (blockStyle.alignment == CssTextAlign::Left) {
    xpos = static_cast<int16_t>(lineWordWidthSum + totalNaturalGaps);
  } else if (blockStyle.alignment == CssTextAlign::Center) {
    xpos = static_cast<int16_t>((effectivePageWidth + lineWordWidthSum + totalNaturalGaps) / 2);
  }

  for (size_t wordIdx = 0; wordIdx < lineWordCount; wordIdx++) {
    xpos -= static_cast<int16_t>(wordWidths[lastBreakAt + wordIdx]);
    lineXPos.push_back(xpos);

    if (wordIdx + 1 < lineWordCount) {
      const bool nextIsContinuation = continuesVec[lastBreakAt + wordIdx + 1];
      if (nextIsContinuation) {
        xpos -=
            renderer.getKerning(fontId, lastCodepoint(words[lastBreakAt + wordIdx]),
                                firstCodepoint(words[lastBreakAt + wordIdx + 1]), wordStyles[lastBreakAt + wordIdx]);
      } else {
        int gap = renderer.getSpaceAdvance(fontId, lastCodepoint(words[lastBreakAt + wordIdx]),
                                           firstCodepoint(words[lastBreakAt + wordIdx + 1]),
                                           wordStyles[lastBreakAt + wordIdx]);
        if (blockStyle.alignment == CssTextAlign::Justify && !isLastLine) {
          gap += justifyExtra;
        }
        xpos -= gap;
      }
    }
  }

  // Fix embedded LTR runs: consecutive LTR-content words read left-to-right within
  // the RTL line. Punctuation-only words (e.g. ",") are excluded so they follow RTL flow.
  for (size_t i = 0; i < lineWordCount;) {
    if (wordHasLtrContent(words[lastBreakAt + i].c_str())) {
      const size_t runStart = i;
      size_t runEnd = i + 1;
      while (runEnd < lineWordCount && wordHasLtrContent(words[lastBreakAt + runEnd].c_str())) {
        runEnd++;
      }
      if (runEnd - runStart > 1) {
        auto ltrXpos = lineXPos[runEnd - 1];  // leftmost position in the run
        for (size_t j = runStart; j < runEnd; j++) {
          lineXPos[j] = ltrXpos;
          ltrXpos += wordWidths[lastBreakAt + j];
          if (j + 1 < runEnd) {
            ltrXpos +=
                renderer.getSpaceAdvance(fontId, lastCodepoint(words[lastBreakAt + j]),
                                         firstCodepoint(words[lastBreakAt + j + 1]), wordStyles[lastBreakAt + j]);
          }
        }
      }
      i = runEnd;
    } else {
      i++;
    }
  }

  return lineXPos;
}

}  // namespace Rtl::LineLayout
