#pragma once

#include "yabt/cli/cli_parser.h"
#include "yabt/cli/subcommand.h"
#include "yabt/workspace/utils.h"

namespace yabt::cmd {

class SyncCommand final : public cli::SubcommandHandler {
public:
  SyncCommand() noexcept = default;

  [[nodiscard]] runtime::Result<void, std::string>
  register_command(cli::CliParser &parser) noexcept;

  [[nodiscard]] runtime::Result<void, std::string> handle_subcommand(
      std::span<const std::string_view> unparsed_args) noexcept final;

private:
  workspace::SyncMode m_sync_mode{workspace::SyncMode::NORMAL};
};

} // namespace yabt::cmd
