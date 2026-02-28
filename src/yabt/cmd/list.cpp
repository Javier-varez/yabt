#include <regex>
#include <span>
#include <string>
#include <string_view>

#include "yabt/build/build.h"
#include "yabt/cmd/list.h"
#include "yabt/log/log.h"
#include "yabt/runtime/result.h"
#include "yabt/workspace/utils.h"

namespace yabt::cmd {

namespace {
const std::string_view SHORT_DESCRIPTION = "Lists the requested targets";
const std::string_view LONG_DESCRIPTION =
    "Lists requested targets that match the specified target spec.";

[[nodiscard]] runtime::Result<void, std::string>
list_inner(const std::span<const std::string_view> target_patterns) {
  const std::optional<std::filesystem::path> ws_root =
      workspace::get_workspace_root();
  if (!ws_root.has_value()) {
    return runtime::Result<void, std::string>::error(
        std::format("Could not find workspace root. Are you sure your "
                    "directory tree contains a {} file?",
                    module::MODULE_FILE_NAME));
  }

  const std::filesystem::path build_dir =
      std::filesystem::absolute(ws_root.value() / workspace::BUILD_DIR_NAME);

  auto modules = RESULT_PROPAGATE(workspace::open_workspace(ws_root.value()));
  auto lua_engine = RESULT_PROPAGATE(
      build::prepare_lua_engine(ws_root.value(), build_dir, modules));
  RESULT_PROPAGATE_DISCARD(
      build::invoke_rule_initializers(lua_engine, modules));
  RESULT_PROPAGATE_DISCARD(build::invoke_build_targets(lua_engine, modules));

  // Find all matching targets for a pattern
  std::vector<std::regex> patterns;
  for (const std::string_view target_pattern : target_patterns) {
    patterns.emplace_back(std::string{target_pattern});
  }

  std::vector<std::string> targets{};
  if (target_patterns.size() == 0) {
    for (const std::string &target : lua_engine.all_targets()) {
      targets.push_back(target);
    }
  } else {
    for (const std::string &target : lua_engine.all_targets()) {
      for (const std::regex &regex : patterns) {
        if (std::regex_match(target, regex)) {
          targets.push_back(target);
          break; // don't add a target more than once
        }
      }
    }

    if (targets.size() == 0) {
      yabt_error("No matched targets");
    }
  }

  for (const auto &target : targets) {
    puts(std::format("{}", target).c_str());
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace

[[nodiscard]] runtime::Result<void, std::string>
ListCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "list", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);
  static_cast<void>(subcommand);
  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string> ListCommand::handle_subcommand(
    std::span<const std::string_view> target_patterns) noexcept {
  if (runtime::Result result = list_inner(target_patterns); result.is_error()) {
    yabt_error("List failed: {}", result.error_value());
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
