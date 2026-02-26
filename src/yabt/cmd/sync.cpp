#include <span>
#include <string>
#include <string_view>

#include "yabt/cmd/sync.h"
#include "yabt/log/log.h"
#include "yabt/runtime/result.h"
#include "yabt/workspace/utils.h"

namespace yabt::cmd {

namespace {
const std::string_view SHORT_DESCRIPTION =
    "Sets up the repository by syncing its dependencies";
const std::string_view LONG_DESCRIPTION =
    "Sets up the repository by syncing its dependencies";
} // namespace

[[nodiscard]] runtime::Result<void, std::string>
SyncCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "sync", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);

  return subcommand.register_flag({
      .name{"strict"},
      .short_name{},
      .optional = true,
      .type = yabt::cli::FlagType::BOOL,
      .description{"Refuses to sync if any of the dependencies are not pinned "
                   "with a hash"},
      .handler{[this](const cli::Arg &) {
        this->m_sync_mode = workspace::SyncMode::STRICT;
        return runtime::Result<void, std::string>::ok();
      }},
  });
}

[[nodiscard]] runtime::Result<void, std::string> SyncCommand::handle_subcommand(
    std::span<const std::string_view> unparsed_args) noexcept {
  if (unparsed_args.size() != 0) {
    return runtime::Result<void, std::string>::error(
        "Unexpected arguments passed to \"sync\" subcommand");
  }

  const runtime::Result result = workspace::sync_workspace(m_sync_mode);
  if (!result.is_ok()) {
    yabt_error("Error syncing dependencies: {}", result.error_value());
  } else {
    yabt_info("Dependencies have been synced successfully");
  }
  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
