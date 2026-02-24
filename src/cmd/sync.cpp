#include <span>
#include <string>
#include <string_view>

#include "yabt/cmd/sync.h"
#include "yabt/log/log.h"
#include "yabt/process/process.h"
#include "yabt/runtime/result.h"

namespace yabt::cmd {

using std::string_view_literals::operator""sv;

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
  static_cast<void>(subcommand);
  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string> SyncCommand::handle_subcommand(
    std::span<const std::string_view> unparsed_args) noexcept {
  if (unparsed_args.size() != 0) {
    return runtime::Result<void, std::string>::error(
        "Unexpected arguments passed to \"sync\" subcommand");
  }

  process::Process process{"ls"sv, std::vector{std::string{"/home/javier"}}};
  RESULT_PROPAGATE_DISCARD(process.start(true));
  const process::Process::ProcessOutput output = process.capture_output();

  yabt_info("Process stdout:\n{}", output.stdout);
  yabt_info("Process stderr:\n{}", output.stderr);

  std::visit(
      []<typename T>(const T &v) {
        if constexpr (std::same_as<T, process::Process::NormalExit>) {
          yabt_info("Process exited normally: {}", v.exit_code);
        } else if constexpr (std::same_as<T, process::Process::NormalExit>) {
          yabt_info("Process received signal: {}", v.signal);
        }
      },
      output.exit_reason);

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
