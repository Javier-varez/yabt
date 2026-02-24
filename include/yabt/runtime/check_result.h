#pragma once

#include <format>

#include "yabt/runtime/check.h"
#include "yabt/runtime/result.h"

namespace yabt::runtime {

template <typename Ok, typename Err, typename... Args>
void check(const Result<Ok, Err> &result,
           std::format_string<const Err &, Args...> fmt,
           Args &&...args) noexcept {
  if (result.is_error()) {
    fatal(fmt, result.error_value(), std::forward<Args>(args)...);
  }
}

} // namespace yabt::runtime
