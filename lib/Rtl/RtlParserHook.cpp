#include <Rtl.h>

#include <cstring>

#include "Epub/blocks/BlockStyle.h"
#include "Epub/css/CssStyle.h"

namespace Rtl::ParserHook {

namespace {

struct State {
  // Default direction at the document root. Set from EPUB language metadata,
  // overridden by html/body dir= or matching lang=.
  bool defaultRtl = false;
};

thread_local State* g_current = nullptr;

const char* findAttr(const char** atts, const char* name) {
  if (!atts) return nullptr;
  for (int i = 0; atts[i]; i += 2) {
    if (strcmp(atts[i], name) == 0) return atts[i + 1];
  }
  return nullptr;
}

bool langIsRtl(const char* lang) {
  if (!lang) return false;
  return Rtl::Direction::fromLanguageCode(lang);
}

}  // namespace

ScopedAttach::ScopedAttach(const bool bookIsRtl) {
  auto* state = new State{};
  state->defaultRtl = bookIsRtl;
  prev_ = g_current;
  g_current = state;
}

ScopedAttach::~ScopedAttach() {
  delete g_current;
  g_current = static_cast<State*>(prev_);
}

void onStartElement(const char* tagName, const char** atts, CssStyle* cssStyle) {
  if (!g_current || !cssStyle) return;

  const char* dirAttr = findAttr(atts, "dir");
  const bool isHtmlOrBody = (strcmp(tagName, "html") == 0 || strcmp(tagName, "body") == 0);

  // HTML dir= overrides CSS direction (per spec).
  if (dirAttr) {
    cssStyle->direction = (strcmp(dirAttr, "rtl") == 0) ? CssDirection::Rtl : CssDirection::Ltr;
    cssStyle->directionDefined = true;
    if (isHtmlOrBody) {
      g_current->defaultRtl = (cssStyle->direction == CssDirection::Rtl);
    }
  } else if (cssStyle->hasDirection() && isHtmlOrBody) {
    g_current->defaultRtl = (cssStyle->direction == CssDirection::Rtl);
  }

  // Some EPUBs have dc:language="en" but body lang="he-IL". Pick up the
  // direction from there if dir= and CSS direction haven't decided yet.
  if (!g_current->defaultRtl && isHtmlOrBody) {
    const char* lang = findAttr(atts, "lang");
    if (!lang) lang = findAttr(atts, "xml:lang");
    if (langIsRtl(lang)) {
      g_current->defaultRtl = true;
      if (!cssStyle->hasDirection()) {
        cssStyle->direction = CssDirection::Rtl;
        cssStyle->directionDefined = true;
      }
    }
  }
}

void resolveBlockStyle(BlockStyle* bs, const CssStyle& cssStyle) {
  if (!bs) return;
  const bool inheritedRtl = g_current ? g_current->defaultRtl : false;
  bs->isRtl = cssStyle.hasDirection() ? (cssStyle.direction == CssDirection::Rtl) : inheritedRtl;

  // Resolve logical alignment (Start/End) to physical (Left/Right) using direction
  if (bs->alignment == CssTextAlign::Start) {
    bs->alignment = bs->isRtl ? CssTextAlign::Right : CssTextAlign::Left;
  } else if (bs->alignment == CssTextAlign::End) {
    bs->alignment = bs->isRtl ? CssTextAlign::Left : CssTextAlign::Right;
  }
}

void setInheritedDirection(BlockStyle* bs) {
  if (!bs) return;
  bs->isRtl = g_current ? g_current->defaultRtl : false;
}

bool currentInheritedRtl() { return g_current ? g_current->defaultRtl : false; }

}  // namespace Rtl::ParserHook
