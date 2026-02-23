#pragma once

#include <cstdlib>
#include <format>

#include "yabt/runtime/result.h"

namespace yabt::runtime {

template <typename Ok, typename Err, typename... Args>
void check(const Result<Ok, Err> &result,
           std::format_string<const Err &, Args...> fmt,
           Args &&...args) noexcept {
  if (result.is_error()) {
    puts(std::format(fmt, result.error_value(), std::forward<Args>(args)...)
             .c_str());
    // TODO: Get stack trace and symbolize it as well
    exit(EXIT_FAILURE);
  }
}

} // namespace yabt::runtime
