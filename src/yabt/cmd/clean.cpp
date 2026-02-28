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
  RESULT_PROPAGATE_DISCARD(subcommand.register_flag({
      .name{"deps"},
      .short_name{'d'},
      .optional = true,
      .type = yabt::cli::FlagType::BOOL,
      .description{"Cleans the DEPS directory as well."},
      .handler{[this](const cli::Arg &) {
        this->m_deps = true;
        return runtime::Result<void, std::string>::ok();
      }},
  }));

  return subcommand.register_flag({
      .name{"build-dir"},
      .short_name{},
      .optional = true,
      .type = yabt::cli::FlagType::STRING,
      .description{"Uses the given location of the build directory."},
      .handler{[this](const cli::Arg &a) {
        const cli::StringArg arg = std::get<cli::StringArg>(a);
        this->m_build_dir = arg.value;
        return runtime::Result<void, std::string>::ok();
      }},
  });
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

  const std::filesystem::path build_dir = std::filesystem::absolute(
      m_build_dir.value_or(ws_root.value() / workspace::BUILD_DIR_NAME));
  std::filesystem::remove_all(build_dir);

  const std::filesystem::path deps_dir =
      std::filesystem::absolute(ws_root.value() / workspace::DEPS_DIR_NAME);
  if (m_deps) {
    std::filesystem::remove_all(deps_dir);
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
