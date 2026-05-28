#include <Rtl.h>
#include <Utf8.h>

#include <algorithm>
#include <vector>

namespace Rtl {

bool wordIsRtl(const char* word) {
  if (!word || *word == '\0') return false;
  const auto* ptr = reinterpret_cast<const unsigned char*>(word);
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&ptr))) {
    if (isStrongRtlCodepoint(cp)) return true;
    if (isStrongLtrCodepoint(cp)) return false;
    // Digits, punctuation, combining marks are neutral — keep scanning
  }
  return false;
}

bool wordHasLtrContent(const char* word) {
  if (!word || *word == '\0') return false;
  const auto* ptr = reinterpret_cast<const unsigned char*>(word);
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&ptr))) {
    if (isStrongLtrCodepoint(cp)) return true;
    if (cp >= 0x30 && cp <= 0x39) return true;  // ASCII digit
  }
  return false;
}

void reverseGraphemeClusters(std::string& word) {
  if (word.empty()) return;

  // Each cluster spans bytes [start, end).
  // Base char + combining marks = one cluster; digit runs (with internal . , /) = one cluster.
  struct Cluster {
    size_t start;
    size_t end;
  };

  std::vector<Cluster> clusters;
  clusters.reserve(16);

  const auto* data = reinterpret_cast<const unsigned char*>(word.c_str());
  const auto* ptr = data;
  size_t clusterStart = 0;
  bool inCluster = false;
  bool inDigitRun = false;

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
      if (!inCluster) {
        clusterStart = bytePos;
        inCluster = true;
        inDigitRun = false;
      }
    } else if (inDigitRun && (isDigit || (isNumberSep && nextIsDigit(ptr)))) {
      // Continue digit run
    } else {
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

  std::string reversed;
  reversed.reserve(word.size());
  for (size_t i = clusters.size(); i > 0; --i) {
    const auto& c = clusters[i - 1];
    reversed.append(word, c.start, c.end - c.start);
  }

  word = std::move(reversed);
}

void mirrorBrackets(std::string& text) {
  for (auto& ch : text) {
    switch (ch) {
      case '(':
        ch = ')';
        break;
      case ')':
        ch = '(';
        break;
      case '[':
        ch = ']';
        break;
      case ']':
        ch = '[';
        break;
      case '{':
        ch = '}';
        break;
      case '}':
        ch = '{';
        break;
      default:
        break;
    }
  }
}

namespace {
bool containsRtl(const char* text) {
  if (!text) return false;
  const auto* ptr = reinterpret_cast<const unsigned char*>(text);
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&ptr))) {
    if (isStrongRtlCodepoint(cp)) return true;
  }
  return false;
}
}  // namespace

std::string toVisualOrder(const char* text) {
  if (!text || *text == '\0') return "";
  if (!containsRtl(text)) return text;  // pure LTR — no transformation

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

  const bool paragraphRtl = wordIsRtl(text);

  if (paragraphRtl) {
    auto isBracket = [](char c) { return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}'; };
    auto isTrailPunc = [](char c) { return c == ',' || c == '.' || c == ';' || c == ':' || c == '!' || c == '?'; };

    std::vector<std::string> expanded;
    expanded.reserve(words.size() + 8);
    for (auto& w : words) {
      if (w.size() <= 1) {
        expanded.push_back(std::move(w));
        continue;
      }

      bool hasLatin = false;
      {
        const auto* scan = reinterpret_cast<const unsigned char*>(w.c_str());
        uint32_t cp;
        while ((cp = utf8NextCodepoint(&scan))) {
          if (isStrongLtrCodepoint(cp)) {
            hasLatin = true;
            break;
          }
        }
      }

      size_t leadEnd = 0;
      if (!hasLatin) {
        while (leadEnd < w.size() && isBracket(w[leadEnd])) leadEnd++;
      }

      size_t trailStart = w.size();
      while (trailStart > leadEnd) {
        const char c = w[trailStart - 1];
        if (isBracket(c) && !hasLatin) {
          trailStart--;
        } else if (isTrailPunc(c)) {
          trailStart--;
        } else {
          break;
        }
      }

      if ((leadEnd > 0 || trailStart < w.size()) && trailStart > leadEnd) {
        auto mirror = [](char c) -> char {
          switch (c) {
            case '(':
              return ')';
            case ')':
              return '(';
            case '[':
              return ']';
            case ']':
              return '[';
            case '{':
              return '}';
            case '}':
              return '{';
            default:
              return c;
          }
        };
        for (size_t i = 0; i < leadEnd; i++) {
          expanded.push_back(std::string(1, mirror(w[i])));
        }
        expanded.push_back(w.substr(leadEnd, trailStart - leadEnd));
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

  for (auto& w : words) {
    if (wordIsRtl(w.c_str())) {
      reverseGraphemeClusters(w);
    }
  }

  if (paragraphRtl) {
    std::reverse(words.begin(), words.end());

    // Fix embedded LTR runs: consecutive LTR words keep internal order.
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

  std::string result;
  for (size_t i = 0; i < words.size(); i++) {
    if (i > 0) result += ' ';
    result += words[i];
  }
  return result;
}

}  // namespace Rtl
