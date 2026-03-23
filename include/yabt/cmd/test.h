#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>

#include "yabt/cli/cli_parser.h"
#include "yabt/cli/subcommand.h"
#include "yabt/runtime/result.h"

namespace yabt::cmd {

class TestCommand final : public cli::SubcommandHandler {
public:
  TestCommand() noexcept = default;

  [[nodiscard]] runtime::Result<void, std::string>
  register_command(cli::CliParser &parser) noexcept;

  [[nodiscard]] runtime::Result<void, std::string> handle_subcommand(
      std::span<const std::string_view> unparsed_args) noexcept final;

private:
  int m_threads{0};
  std::optional<std::filesystem::path> m_build_dir{};
};

} // namespace yabt::cmd
