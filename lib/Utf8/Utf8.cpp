#include "Utf8.h"

#include <algorithm>
#include <vector>

int utf8CodepointLen(const unsigned char c) {
  if (c < 0x80) return 1;          // 0xxxxxxx
  if ((c >> 5) == 0x6) return 2;   // 110xxxxx
  if ((c >> 4) == 0xE) return 3;   // 1110xxxx
  if ((c >> 3) == 0x1E) return 4;  // 11110xxx
  return 1;                        // fallback for invalid
}

uint32_t utf8NextCodepoint(const unsigned char** string) {
  if (**string == 0) {
    return 0;
  }

  const unsigned char lead = **string;
  const int bytes = utf8CodepointLen(lead);
  const uint8_t* chr = *string;

  // Invalid lead byte (stray continuation byte 0x80-0xBF, or 0xFE/0xFF)
  if (bytes == 1 && lead >= 0x80) {
    (*string)++;
    return REPLACEMENT_GLYPH;
  }

  if (bytes == 1) {
    (*string)++;
    return chr[0];
  }

  // Validate continuation bytes before consuming them
  for (int i = 1; i < bytes; i++) {
    if ((chr[i] & 0xC0) != 0x80) {
      // Missing or invalid continuation byte — skip all bytes consumed so far
      *string += i;
      return REPLACEMENT_GLYPH;
    }
  }

  uint32_t cp = chr[0] & ((1 << (7 - bytes)) - 1);  // mask header bits

  for (int i = 1; i < bytes; i++) {
    cp = (cp << 6) | (chr[i] & 0x3F);
  }

  // Reject overlong encodings, surrogates, and out-of-range values
  const bool overlong = (bytes == 2 && cp < 0x80) || (bytes == 3 && cp < 0x800) || (bytes == 4 && cp < 0x10000);
  const bool surrogate = (cp >= 0xD800 && cp <= 0xDFFF);
  if (overlong || surrogate || cp > 0x10FFFF) {
    (*string)++;
    return REPLACEMENT_GLYPH;
  }

  *string += bytes;

  return cp;
}

int utf8SafeTruncateBuffer(const char* buf, int len) {
  if (len <= 0) return 0;

  // Walk back past continuation bytes (10xxxxxx) to find the lead byte
  int leadPos = len - 1;
  while (leadPos > 0 && (static_cast<uint8_t>(buf[leadPos]) & 0xC0) == 0x80) {
    leadPos--;
  }

  // Determine expected length of the sequence starting at leadPos
  int expectedLen = utf8CodepointLen(static_cast<unsigned char>(buf[leadPos]));
  int actualLen = len - leadPos;

  if (actualLen < expectedLen && leadPos > 0) {
    // Incomplete UTF-8 sequence at the end — exclude it
    return leadPos;
  }
  return len;
}

size_t utf8RemoveLastChar(std::string& str) {
  if (str.empty()) return 0;
  size_t pos = str.size() - 1;
  while (pos > 0 && (static_cast<unsigned char>(str[pos]) & 0xC0) == 0x80) {
    --pos;
  }
  str.resize(pos);
  return pos;
}

// Truncate string by removing N UTF-8 characters from the end
void utf8TruncateChars(std::string& str, const size_t numChars) {
  for (size_t i = 0; i < numChars && !str.empty(); ++i) {
    utf8RemoveLastChar(str);
  }
}

// Returns true if the string contains any strong RTL codepoint.
static bool containsRtl(const char* text) {
  if (!text) return false;
  const auto* ptr = reinterpret_cast<const unsigned char*>(text);
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&ptr))) {
    if (isStrongRtlCodepoint(cp)) return true;
  }
  return false;
}

std::string toVisualOrder(const char* text) {
  if (!text || *text == '\0') return "";
  if (!containsRtl(text)) return text;  // Pure LTR — no transformation needed

  // Split into words on spaces
  std::vector<std::string> words;
  std::string current;
  const char* p = text;
  while (*p) {
    if (*p == ' ') {
      if (!current.empty()) {
        words.push_back(std::move(current));
        current.clear();
      }
      ++p;
    } else {
      current += *p;
      ++p;
    }
  }
  if (!current.empty()) {
    words.push_back(std::move(current));
  }

  // Determine paragraph direction from first strong character
  bool paragraphRtl = wordIsRtl(text);

  // In RTL paragraphs, split leading/trailing brackets and punctuation from non-English words
  // so they get positioned independently in RTL flow.
  if (paragraphRtl) {
    auto isBracket = [](char c) { return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}'; };
    auto isTrailPunc = [](char c) {
      return c == ',' || c == '.' || c == ';' || c == ':' || c == '!' || c == '?';
    };

    std::vector<std::string> expanded;
    expanded.reserve(words.size() + 8);
    for (auto& w : words) {
      if (w.size() <= 1) {
        expanded.push_back(std::move(w));
        continue;
      }

      // Check for Latin letters
      bool hasLatin = false;
      {
        const auto* scan = reinterpret_cast<const unsigned char*>(w.c_str());
        uint32_t cp;
        while ((cp = utf8NextCodepoint(&scan))) {
          if (isStrongLtrCodepoint(cp)) { hasLatin = true; break; }
        }
      }

      // Find leading brackets (not for English words)
      size_t leadEnd = 0;
      if (!hasLatin) {
        while (leadEnd < w.size() && isBracket(w[leadEnd])) leadEnd++;
      }

      // Find trailing brackets + punctuation
      size_t trailStart = w.size();
      while (trailStart > leadEnd) {
        const char c = w[trailStart - 1];
        if (isBracket(c) && !hasLatin) { trailStart--; }
        else if (isTrailPunc(c)) { trailStart--; }
        else { break; }
      }

      if ((leadEnd > 0 || trailStart < w.size()) && trailStart > leadEnd) {
        // Mirror bracket for RTL display (Unicode BiDi mirroring)
        auto mirror = [](char c) -> char {
          switch (c) {
            case '(': return ')';  case ')': return '(';
            case '[': return ']';  case ']': return '[';
            case '{': return '}';  case '}': return '{';
            default: return c;
          }
        };
        // Emit leading brackets individually (mirrored)
        for (size_t i = 0; i < leadEnd; i++) {
          expanded.push_back(std::string(1, mirror(w[i])));
        }
        // Emit core
        expanded.push_back(w.substr(leadEnd, trailStart - leadEnd));
        // Emit trailing brackets/punctuation individually (brackets mirrored)
        for (size_t i = trailStart; i < w.size(); i++) {
          char c = w[i];
          if (isBracket(c)) c = mirror(c);
          expanded.push_back(std::string(1, c));
        }
      } else {
        expanded.push_back(std::move(w));
      }
    }
    words = std::move(expanded);
  }

  // Reverse grapheme clusters within each Hebrew word (logical → visual character order)
  for (auto& w : words) {
    if (wordIsRtl(w.c_str())) {
      reverseGraphemeClusters(w);
    }
  }

  if (paragraphRtl) {
    // Reverse overall word order for RTL paragraph
    std::reverse(words.begin(), words.end());

    // Fix embedded LTR runs: consecutive words with strong LTR content keep internal order.
    // Punctuation-only words are excluded from LTR runs so they follow RTL flow.
    for (size_t i = 0; i < words.size();) {
      if (wordHasLtrContent(words[i].c_str())) {
        size_t runStart = i;
        size_t runEnd = i + 1;
        while (runEnd < words.size() && wordHasLtrContent(words[runEnd].c_str())) {
          runEnd++;
        }
        if (runEnd - runStart > 1) {
          std::reverse(words.begin() + runStart, words.begin() + runEnd);
        }
        i = runEnd;
      } else {
        i++;
      }
    }
  }

  // Reassemble
  std::string result;
  for (size_t i = 0; i < words.size(); i++) {
    if (i > 0) result += ' ';
    result += words[i];
  }
  return result;
}

void mirrorBrackets(std::string& text) {
  for (auto& ch : text) {
    switch (ch) {
      case '(': ch = ')'; break;
      case ')': ch = '('; break;
      case '[': ch = ']'; break;
      case ']': ch = '['; break;
      case '{': ch = '}'; break;
      case '}': ch = '{'; break;
      default: break;
    }
  }
}

bool wordIsRtl(const char* word) {
  if (!word || *word == '\0') return false;

  const auto* ptr = reinterpret_cast<const unsigned char*>(word);
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&ptr))) {
    if (isStrongRtlCodepoint(cp)) return true;
    if (isStrongLtrCodepoint(cp)) return false;
    // Digits, punctuation, combining marks are neutral — keep scanning
  }
  return false;  // No strong character found; treat as LTR
}

bool wordHasLtrContent(const char* word) {
  if (!word || *word == '\0') return false;

  const auto* ptr = reinterpret_cast<const unsigned char*>(word);
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&ptr))) {
    if (isStrongLtrCodepoint(cp)) return true;
    if (cp >= 0x30 && cp <= 0x39) return true;  // ASCII digit
  }
  return false;  // Punctuation-only or empty
}

void reverseGraphemeClusters(std::string& word) {
  if (word.empty()) return;

  // Parse into clusters: each cluster is a byte range [start, end).
  // - A base character + following combining marks = one cluster
  // - Digit sequences including internal separators (commas, periods) = one atomic cluster
  //   so that numbers like "8,000" or "3.14" stay in LTR order after reversal.
  struct Cluster {
    size_t start;
    size_t end;
  };

  std::vector<Cluster> clusters;
  clusters.reserve(16);

  const auto* data = reinterpret_cast<const unsigned char*>(word.c_str());
  const size_t dataLen = word.size();
  const auto* ptr = data;
  size_t clusterStart = 0;
  bool inCluster = false;
  bool inDigitRun = false;

  // Peek ahead to check if a separator (,./) between digits is part of a number
  auto nextIsDigit = [&](const unsigned char* pos) -> bool {
    if (!*pos) return false;
    const unsigned char* tmp = pos;
    const uint32_t nextCp = utf8NextCodepoint(&tmp);
    return nextCp >= 0x30 && nextCp <= 0x39;
  };

  while (*ptr) {
    const auto* before = ptr;
    const uint32_t cp = utf8NextCodepoint(&ptr);
    if (cp == 0) break;

    const size_t bytePos = before - data;
    const bool isDigit = (cp >= 0x30 && cp <= 0x39);
    const bool isNumberSep = (cp == ',' || cp == '.' || cp == '/');

    if (utf8IsCombiningMark(cp)) {
      // Combining mark extends current cluster
      if (!inCluster) {
        clusterStart = bytePos;
        inCluster = true;
        inDigitRun = false;
      }
    } else if (inDigitRun && (isDigit || (isNumberSep && nextIsDigit(ptr)))) {
      // Continue digit run: either another digit, or a separator followed by a digit
    } else {
      // New base character or digit run start: close previous cluster
      if (inCluster) {
        clusters.push_back({clusterStart, bytePos});
      }
      clusterStart = bytePos;
      inCluster = true;
      inDigitRun = isDigit;
    }
  }
  if (inCluster) {
    clusters.push_back({clusterStart, word.size()});
  }

  if (clusters.size() <= 1) return;

  // Build reversed string by appending clusters in reverse order
  std::string reversed;
  reversed.reserve(word.size());
  for (size_t i = clusters.size(); i > 0; --i) {
    const auto& c = clusters[i - 1];
    reversed.append(word, c.start, c.end - c.start);
  }

  word = std::move(reversed);
}
