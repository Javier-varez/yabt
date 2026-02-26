#include <string_view>

#include "yabt/utils/string.h"

namespace yabt::utils {

constexpr static const char *WHITESPACE_CHARSET = " \t\r\n";

[[nodiscard]] std::string_view trim_whitespace(std::string_view sv) noexcept {
  return trim_right_charset(trim_left_charset(sv, WHITESPACE_CHARSET),
                            WHITESPACE_CHARSET);
}

[[nodiscard]] std::string_view
trim_left_charset(std::string_view sv, std::string_view chars) noexcept {
  const size_t n = sv.find_first_not_of(chars);
  if (n == std::string_view::npos) {
    return sv;
  }
  return sv.substr(n);
}

[[nodiscard]] std::string_view
trim_right_charset(std::string_view sv, std::string_view chars) noexcept {
  const size_t n = sv.find_last_not_of(chars);
  if (n == std::string_view::npos) {
    return sv;
  }
  return sv.substr(0, n + 1);
}

} // namespace yabt::utils
