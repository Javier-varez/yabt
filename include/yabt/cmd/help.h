#pragma once

#include "yabt/cli/cli_parser.h"
#include "yabt/cli/subcommand.h"

namespace yabt::cmd {

class HelpCommand final : public cli::SubcommandHandler {
public:
  HelpCommand() noexcept = default;

  [[nodiscard]] runtime::Result<void, std::string>
  register_command(cli::CliParser &parser) noexcept;

  [[nodiscard]] runtime::Result<void, std::string> handle_subcommand(
      std::span<const std::string_view> unparsed_args) noexcept final;

private:
  const cli::CliParser *m_parser;
};

} // namespace yabt::cmd
