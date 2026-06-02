#pragma once

#include <regex>
#include <span>
#include <string_view>
#include <vector>

namespace yabt::build {

struct BuildFlag {
  std::string key;
  std::string value;
};

struct BuildArgs {
  std::vector<std::regex> target_patterns;
  std::vector<BuildFlag> flags;
  std::span<const std::string_view> exec_args;
};

[[nodiscard]] BuildArgs
parse_args(std::span<const std::string_view> arguments) noexcept;

} // namespace yabt::build
