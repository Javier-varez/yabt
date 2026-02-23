#pragma once

#include <functional>
#include <optional>
#include <string>

#include "yabt/cli/args.h"
#include "yabt/runtime/result.h"

namespace yabt::cli {

enum class FlagType {
  BOOL,
  STRING,
  INTEGER,
};

struct Flag final {
  std::string name;
  std::optional<char> short_name;
  bool optional;
  FlagType type;
  std::string description;
  std::function<runtime::Result<void, std::string>(Arg)> handler;

  [[nodiscard]] runtime::Result<void, std::string> validate() const noexcept;
};

} // namespace yabt::cli
