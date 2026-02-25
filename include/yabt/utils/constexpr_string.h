#pragma once

#include <string_view>

namespace yabt::utils {

template <char... Chars> struct ConstexprString {
  constexpr static char data[sizeof...(Chars) + 1]{Chars..., '\0'};

  operator std::string_view() noexcept { return std::string_view{data}; }
};

template <typename T, T... Chars>
constexpr ConstexprString<Chars...> operator""_cs() {
  return {};
}

} // namespace yabt::utils

using ::yabt::utils::operator""_cs;
