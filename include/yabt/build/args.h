#pragma once

#include <regex>
#include <span>
#include <string_view>
#include <vector>

#include "yabt/lua/flag_lib.h"

namespace yabt::build {

struct BuildArgs {
  std::vector<std::regex> target_patterns;
  std::vector<lua::RawFlag> flags;
  std::span<const std::string_view> exec_args;
};

[[nodiscard]] BuildArgs
parse_args(std::span<const std::string_view> arguments) noexcept;

} // namespace yabt::build
