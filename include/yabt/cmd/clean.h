#pragma once

#include <filesystem>

#include "yabt/cli/cli_parser.h"
#include "yabt/cli/subcommand.h"

namespace yabt::cmd {

class CleanCommand final : public cli::SubcommandHandler {
public:
  CleanCommand() noexcept = default;

  [[nodiscard]] runtime::Result<void, std::string>
  register_command(cli::CliParser &parser) noexcept;

  [[nodiscard]] runtime::Result<void, std::string> handle_subcommand(
      std::span<const std::string_view> unparsed_args) noexcept final;

private:
  bool m_deps;
  std::optional<std::filesystem::path> m_build_dir{};
};

} // namespace yabt::cmd
