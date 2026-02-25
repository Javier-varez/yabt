#include <span>
#include <string>
#include <string_view>

#include "yabt/cmd/sync.h"
#include "yabt/log/log.h"
#include "yabt/module/module_file.h"
#include "yabt/runtime/result.h"

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
  static_cast<void>(subcommand);
  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string> SyncCommand::handle_subcommand(
    std::span<const std::string_view> unparsed_args) noexcept {
  if (unparsed_args.size() != 0) {
    return runtime::Result<void, std::string>::error(
        "Unexpected arguments passed to \"sync\" subcommand");
  }

  const module::ModuleFile modfile =
      RESULT_PROPAGATE(module::ModuleFile::load_module_file("./MODULE.lua"));

  yabt_info("Module name: {}", modfile.name);
  yabt_info("Module version: {}", modfile.version);
  yabt_info("Module deps:");
  for (const auto &[dep_name, dep] : modfile.deps) {
    yabt_info("\tdep name: {}", dep_name);
    yabt_info("\tdep url: {}", dep.url);
    yabt_info("\tdep version: {}", dep.version);
    yabt_info("\tdep hash: {}", dep.hash);
    yabt_info("\tdep type: {}", dep.type);
  }
  yabt_info("Module flags:");
  for (const auto &[key, value] : modfile.flags) {
    yabt_info("\t{} = {}", key, value);
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
