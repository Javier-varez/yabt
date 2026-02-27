#include <span>
#include <string>
#include <string_view>

#include "yabt/cmd/clean.h"
#include "yabt/runtime/result.h"
#include "yabt/workspace/utils.h"

namespace yabt::cmd {

namespace {
const std::string_view SHORT_DESCRIPTION =
    "Cleans all the yabt build artifacts";
const std::string_view LONG_DESCRIPTION = "Cleans all the yabt build artifacts";
} // namespace

[[nodiscard]] runtime::Result<void, std::string>
CleanCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "clean", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);
  static_cast<void>(subcommand);
  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string>
CleanCommand::handle_subcommand(
    std::span<const std::string_view> unparsed_args) noexcept {
  if (unparsed_args.size() != 0) {
    return runtime::Result<void, std::string>::error(
        "Additional unparsed arguments were given to the yabt clean command.\n"
        "This command does not take any more arguments.");
  }

  const std::optional<std::filesystem::path> ws_root =
      workspace::get_workspace_root();
  if (!ws_root.has_value()) {
    return runtime::Result<void, std::string>::error(
        std::format("Could not find workspace root. Are you sure your "
                    "directory tree contains a {} file?",
                    module::MODULE_FILE_NAME));
  }

  std::filesystem::remove_all(ws_root.value() / workspace::BUILD_DIR_NAME);
  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
