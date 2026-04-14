#pragma once

#include <cstdint>
#include <string>
#define REPLACEMENT_GLYPH 0xFFFD

uint32_t utf8NextCodepoint(const unsigned char** string);
// Remove the last UTF-8 codepoint from a std::string and return the new size.
size_t utf8RemoveLastChar(std::string& str);
// Truncate string by removing N UTF-8 codepoints from the end.
void utf8TruncateChars(std::string& str, size_t numChars);

// Truncate a raw char buffer to the last complete UTF-8 codepoint boundary.
// Returns the new length (<= len). If the buffer ends mid-sequence, the
// incomplete trailing bytes are excluded.
int utf8SafeTruncateBuffer(const char* buf, int len);

// Returns true if the codepoint belongs to a strong right-to-left script (Hebrew, Arabic).
inline bool isStrongRtlCodepoint(const uint32_t cp) {
  return (cp >= 0x0590 && cp <= 0x05FF)    // Hebrew
         || (cp >= 0xFB1D && cp <= 0xFB4F)  // Alphabetic Presentation Forms (Hebrew)
         || (cp >= 0x0600 && cp <= 0x06FF)  // Arabic
         || (cp >= 0xFB50 && cp <= 0xFDFF)  // Arabic Presentation Forms-A
         || (cp >= 0xFE70 && cp <= 0xFEFF);  // Arabic Presentation Forms-B
}

// Returns true if the codepoint belongs to a strong left-to-right script.
inline bool isStrongLtrCodepoint(const uint32_t cp) {
  return (cp >= 0x0041 && cp <= 0x005A)    // Latin uppercase
         || (cp >= 0x0061 && cp <= 0x007A)  // Latin lowercase
         || (cp >= 0x00C0 && cp <= 0x024F)  // Latin Extended
         || (cp >= 0x0370 && cp <= 0x03FF)  // Greek
         || (cp >= 0x0400 && cp <= 0x04FF)  // Cyrillic
         || (cp >= 0x1E00 && cp <= 0x1EFF);  // Latin Extended Additional
}

// Returns true if the first strong (non-neutral) character in the word is RTL.
bool wordIsRtl(const char* word);

// Returns true if the word contains a strong LTR character or digit.
// Neutral-only words (pure punctuation like ",") return false.
bool wordHasLtrContent(const char* word);

// Reverse grapheme clusters in a string, converting from Unicode logical order
// to visual (LTR-renderable) order. Combining marks stay attached to their base character.
void reverseGraphemeClusters(std::string& word);

// Convert a mixed Hebrew/Latin string from Unicode logical order to visual display order
// for use with a left-to-right renderer. Reverses Hebrew word characters and reorders
// words for RTL paragraphs. Returns the input unchanged if no Hebrew is detected.
std::string toVisualOrder(const char* text);

// Mirror bracket/parenthesis characters in a string for RTL display.
// Swaps ( ↔ ), [ ↔ ], { ↔ } per Unicode BiDi mirroring rules.
void mirrorBrackets(std::string& text);

// Returns true for Unicode combining diacritical marks that should not advance the cursor.
inline bool utf8IsCombiningMark(const uint32_t cp) {
  return (cp >= 0x0300 && cp <= 0x036F)      // Combining Diacritical Marks
         || (cp >= 0x0591 && cp <= 0x05BD)   // Hebrew cantillation marks + vowel points (nikkud)
         || (cp == 0x05BF)                    // Hebrew point rafe
         || (cp >= 0x05C1 && cp <= 0x05C2)   // Hebrew point shin/sin dot
         || (cp >= 0x05C4 && cp <= 0x05C5)   // Hebrew mark upper/lower dot
         || (cp == 0x05C7)                    // Hebrew point qamats qatan
         || (cp >= 0x1DC0 && cp <= 0x1DFF)   // Combining Diacritical Marks Supplement
         || (cp >= 0x20D0 && cp <= 0x20FF)   // Combining Diacritical Marks for Symbols
         || (cp >= 0xFE20 && cp <= 0xFE2F);  // Combining Half Marks
}
