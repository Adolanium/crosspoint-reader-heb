#include <Rtl.h>
#include <Utf8.h>

#include <string>

#include "Epub/ParsedText.h"
#include "Epub/blocks/BlockStyle.h"

namespace Rtl::WordEmitter {

namespace {

bool isBracket(const char c) {
  return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
}

bool isTrailPunc(const char c) {
  return c == ',' || c == '.' || c == ';' || c == ':' || c == '!' || c == '?';
}

char mirrorBracket(const char c) {
  switch (c) {
    case '(': return ')';
    case ')': return '(';
    case '[': return ']';
    case ']': return '[';
    case '{': return '}';
    case '}': return '{';
    default: return c;
  }
}

bool wordHasLatinLetters(const char* word) {
  const auto* scan = reinterpret_cast<const unsigned char*>(word);
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&scan))) {
    if (isStrongLtrCodepoint(cp)) return true;
  }
  return false;
}

}  // namespace

bool emit(ParsedText* currentTextBlock,
          char* partWordBuffer,
          const int partWordBufferIndex,
          const EpdFontFamily::Style fontStyle,
          bool& nextWordContinues) {
  if (!currentTextBlock || !currentTextBlock->getBlockStyle().isRtl) {
    return false;
  }

  // In RTL blocks: split leading/trailing brackets and punctuation from non-English words
  // so they get positioned independently in RTL flow.
  if (partWordBufferIndex > 1) {
    const bool hasLatin = wordHasLatinLetters(partWordBuffer);

    int leadEnd = 0;
    if (!hasLatin) {
      while (leadEnd < partWordBufferIndex && isBracket(partWordBuffer[leadEnd])) {
        leadEnd++;
      }
    }

    int trailStart = partWordBufferIndex;
    while (trailStart > leadEnd) {
      const char c = partWordBuffer[trailStart - 1];
      if (isBracket(c)) {
        if (!hasLatin) {
          trailStart--;
        } else {
          break;
        }
      } else if (isTrailPunc(c)) {
        trailStart--;
      } else {
        break;
      }
    }

    if ((leadEnd > 0 || trailStart < partWordBufferIndex) && trailStart > leadEnd) {
      for (int i = 0; i < leadEnd; i++) {
        char bracket[2] = {mirrorBracket(partWordBuffer[i]), '\0'};
        currentTextBlock->addWord(bracket, fontStyle, false, nextWordContinues);
        nextWordContinues = true;
      }

      const char savedTrail = partWordBuffer[trailStart];
      partWordBuffer[trailStart] = '\0';
      const char* core = &partWordBuffer[leadEnd];
      if (wordIsRtl(core)) {
        std::string coreStr(core);
        reverseGraphemeClusters(coreStr);
        currentTextBlock->addWord(std::move(coreStr), fontStyle, false, nextWordContinues);
      } else {
        currentTextBlock->addWord(core, fontStyle, false, nextWordContinues);
      }
      partWordBuffer[trailStart] = savedTrail;

      for (int i = trailStart; i < partWordBufferIndex; i++) {
        char c = partWordBuffer[i];
        if (isBracket(c)) c = mirrorBracket(c);
        const char punc[2] = {c, '\0'};
        currentTextBlock->addWord(punc, fontStyle, false, true);
      }

      nextWordContinues = false;
      return true;
    }
  }

  // No split needed. Reverse grapheme clusters if the word is Hebrew/Arabic.
  if (wordIsRtl(partWordBuffer)) {
    std::string reversedBuf(partWordBuffer);
    reverseGraphemeClusters(reversedBuf);
    currentTextBlock->addWord(std::move(reversedBuf), fontStyle, false, nextWordContinues);
    nextWordContinues = false;
    return true;
  }

  // Plain LTR word inside an RTL block — let the caller emit normally.
  return false;
}

}  // namespace Rtl::WordEmitter
