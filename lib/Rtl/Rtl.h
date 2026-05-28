// RTL/Hebrew support. Implementation lives here so upstream files only carry
// one-line hooks tagged RTL_FORK. See README.md for the merge protocol.
#pragma once

#include <EpdFontFamily.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class GfxRenderer;
class ParsedText;
struct BlockStyle;
struct CssStyle;
enum class CssTextAlign : uint8_t;

namespace Rtl {

inline bool isStrongRtlCodepoint(const uint32_t cp) {
  return (cp >= 0x0590 && cp <= 0x05FF)      // Hebrew
         || (cp >= 0xFB1D && cp <= 0xFB4F)   // Alphabetic Presentation Forms (Hebrew)
         || (cp >= 0x0600 && cp <= 0x06FF)   // Arabic
         || (cp >= 0xFB50 && cp <= 0xFDFF)   // Arabic Presentation Forms-A
         || (cp >= 0xFE70 && cp <= 0xFEFF);  // Arabic Presentation Forms-B
}

inline bool isStrongLtrCodepoint(const uint32_t cp) {
  return (cp >= 0x0041 && cp <= 0x005A)      // Latin uppercase
         || (cp >= 0x0061 && cp <= 0x007A)   // Latin lowercase
         || (cp >= 0x00C0 && cp <= 0x024F)   // Latin Extended
         || (cp >= 0x0370 && cp <= 0x03FF)   // Greek
         || (cp >= 0x0400 && cp <= 0x04FF)   // Cyrillic
         || (cp >= 0x1E00 && cp <= 0x1EFF);  // Latin Extended Additional
}

// First strong character is RTL. Skips digits/punctuation/combining marks.
bool wordIsRtl(const char* word);

// Word contains a strong LTR char or ASCII digit. Used to detect Latin/number
// runs inside an RTL line.
bool wordHasLtrContent(const char* word);

// Reverse grapheme clusters in place. Base char + combining marks stay glued;
// digit runs (with internal . , /) stay atomic so "3.14" doesn't get flipped.
void reverseGraphemeClusters(std::string& word);

// Bidi bracket mirroring on a string (( ↔ ), [ ↔ ], { ↔ }).
void mirrorBrackets(std::string& text);

// Convert a string from logical to visual order for the LTR renderer. Returns
// input unchanged when no RTL codepoint is present. Used for short UI strings
// (status bar, popups) that don't go through the EPUB layout path.
std::string toVisualOrder(const char* text);

namespace Direction {
// True for he/iw/ar/fa (and their regional variants like "he-IL").
bool fromLanguageCode(std::string_view lang);
}  // namespace Direction

namespace ParserHook {

// RAII attach. Construct after the parser, destroy before the parser is
// destroyed. While in scope, the hook functions below operate on this context.
class ScopedAttach {
 public:
  explicit ScopedAttach(bool bookIsRtl);
  ~ScopedAttach();
  ScopedAttach(const ScopedAttach&) = delete;
  ScopedAttach& operator=(const ScopedAttach&) = delete;

 private:
  void* prev_;
};

// Reads dir/lang/xml:lang from atts and writes cssStyle->direction. Tracks
// inherited direction when html/body declares it.
void onStartElement(const char* tagName, const char** atts, CssStyle* cssStyle);

// Run after BlockStyle::fromCssStyle. Sets bs->isRtl and resolves logical
// alignment (Start/End → Left/Right).
void resolveBlockStyle(BlockStyle* bs, const CssStyle& cssStyle);

// For manually built BlockStyles (centered, table cell, paragraph default).
void setInheritedDirection(BlockStyle* bs);

// Inherited RTL from the active attach.
bool currentInheritedRtl();

}  // namespace ParserHook

namespace WordEmitter {

// Try RTL-aware emission. Returns true if handled (caller skips its upstream
// addWord). Returns false for LTR blocks or when no special handling applies.
// On true, sets nextWordContinues = false; caller resets partWordBufferIndex.
bool emit(ParsedText* currentTextBlock, char* partWordBuffer, int partWordBufferIndex, EpdFontFamily::Style fontStyle,
          bool& nextWordContinues);

}  // namespace WordEmitter

namespace LineLayout {

// True when align is the natural direction for the block (Justify always;
// Left for LTR; Right for RTL). Used to gate first-line-indent.
bool isNaturalAlignment(bool isRtl, CssTextAlign align);

// RTL x-positions for a single line. Inputs come from extractLine().
std::vector<int16_t> positionLineRtl(const BlockStyle& blockStyle, const std::vector<std::string>& words,
                                     const std::vector<EpdFontFamily::Style>& wordStyles,
                                     const std::vector<uint16_t>& wordWidths, const std::vector<bool>& continuesVec,
                                     size_t lastBreakAt, size_t lineWordCount, int effectivePageWidth,
                                     int totalNaturalGaps, int lineWordWidthSum, int justifyExtra, bool isLastLine,
                                     int fontId, const GfxRenderer& renderer);

}  // namespace LineLayout

}  // namespace Rtl
