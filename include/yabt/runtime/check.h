#pragma once

#include <unistd.h>

#include <cstdlib>
#include <format>

namespace yabt::runtime {

template <typename... Args>
[[noreturn]] void fatal(std::format_string<Args...> fmt,
                        Args &&...args) noexcept {
  std::string s = std::format(fmt, std::forward<Args>(args)...);
  s.push_back('\n');

  size_t written = 0;
  while (written < s.size()) {
    const int cur =
        write(STDERR_FILENO, s.data() + written, s.size() - written);
    if (cur < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    written += cur;
  }

  // TODO: Get stack trace and symbolize it as well
  exit(EXIT_FAILURE);
}

template <typename... Args>
void check(const bool b, std::format_string<Args...> fmt,
           Args &&...args) noexcept {
  if (!b) {
    fatal(fmt, std::forward<Args>(args)...);
  }
}

} // namespace yabt::runtime
