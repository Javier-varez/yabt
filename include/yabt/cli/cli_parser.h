#pragma once

#include <string_view>
#include <vector>

#include "yabt/cli/flag.h"
#include "yabt/cli/subcommand.h"
#include "yabt/runtime/result.h"

namespace yabt::cli {

class CliParser final {
public:
  CliParser(const char *argv0) noexcept;

  [[nodiscard]] Subcommand &register_subcommand(
      std::string_view name, SubcommandHandler &handler,
      std::string_view short_description,
      std::string_view long_description = std::string_view{}) noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  register_flag(Flag config) noexcept;

  void set_description(std::string_view description) noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  parse(const size_t argc, const char *argv[]) const noexcept;

  void print_help() const noexcept;

  void print_subcommand_help(std::string_view subcommand) const noexcept;

private:
  std::string m_command_name;
  std::vector<Flag> m_global_flags;
  std::vector<Subcommand> m_subcommands;
  std::string m_descr;
};

} // namespace yabt::cli
