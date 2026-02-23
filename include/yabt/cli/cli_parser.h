#pragma once

#include <string_view>
#include <vector>

#include "yabt/cli/flag.h"
#include "yabt/cli/subcommand.h"
#include "yabt/runtime/result.h"

namespace yabt::cli {

class CliParser final {
public:
  CliParser() noexcept = default;

  [[nodiscard]] Subcommand &
  register_subcommand(std::string_view name,
                      SubcommandHandler &handler) noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  register_flag(Flag config) noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  parse(const size_t argc, const char *argv[]) const;

  void print_help() const noexcept;

private:
  std::vector<Flag> m_global_flags;
  std::vector<Subcommand> m_subcommands;
};

} // namespace yabt::cli
