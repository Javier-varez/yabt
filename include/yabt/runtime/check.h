#pragma once

#include <cstdlib>
#include <format>

namespace yabt::runtime {

template <typename... Args>
[[noreturn]] void fatal(std::format_string<Args...> fmt,
                        Args &&...args) noexcept {
  puts(std::format(fmt, std::forward<Args>(args)...).c_str());
  // TODO: Get stack trace and symbolize it as well
  exit(EXIT_FAILURE);
}

template <typename... Args>
void check(const bool b, std::format_string<Args...> fmt,
           Args &&...args) noexcept {
  if (!b) {
    puts(std::format(fmt, std::forward<Args>(args)...).c_str());
    // TODO: Get stack trace and symbolize it as well
    exit(EXIT_FAILURE);
  }
}

} // namespace yabt::runtime
