#pragma once

#include "yabt/cli/cli_parser.h"
#include "yabt/cli/subcommand.h"

namespace yabt::cmd {

class BuildCommand final : public cli::SubcommandHandler {
public:
  BuildCommand() noexcept = default;

  [[nodiscard]] runtime::Result<void, std::string>
  register_command(cli::CliParser &parser) noexcept;

  [[nodiscard]] runtime::Result<void, std::string> handle_subcommand(
      std::span<const std::string_view> unparsed_args) noexcept final;

private:
  int m_threads{0};
};

} // namespace yabt::cmd
