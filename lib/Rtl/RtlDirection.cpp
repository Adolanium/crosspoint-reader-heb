#include <Rtl.h>

namespace Rtl::Direction {

bool fromLanguageCode(const std::string_view lang) {
  if (lang.size() < 2) return false;
  const std::string_view prefix = lang.substr(0, 2);
  // Hebrew ("he", legacy "iw"), Arabic ("ar"), Persian/Farsi ("fa")
  return prefix == "he" || prefix == "iw" || prefix == "ar" || prefix == "fa";
}

}  // namespace Rtl::Direction
