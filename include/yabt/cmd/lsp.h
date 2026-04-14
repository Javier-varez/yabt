#pragma once

#include <string>

#include "yabt/cli/cli_parser.h"
#include "yabt/cli/subcommand.h"

namespace yabt::cmd {

class LspCommand final : public cli::SubcommandHandler {
public:
  LspCommand() noexcept = default;

  [[nodiscard]] runtime::Result<void, std::string>
  register_command(cli::CliParser &parser) noexcept;

  [[nodiscard]] runtime::Result<void, std::string> handle_subcommand(
      std::span<const std::string_view> unparsed_args) noexcept final;

private:
  std::string m_lsp_binary{"lua-language-server"};
};

} // namespace yabt::cmd
