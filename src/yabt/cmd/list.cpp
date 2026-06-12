#include <regex>
#include <span>
#include <string>
#include <string_view>

#include "yabt/build/args.h"
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

[[nodiscard]] runtime::Result<void>
list_inner(const std::span<const std::string_view> arguments) {
  const std::optional<std::filesystem::path> ws_root =
      workspace::get_workspace_root();
  if (!ws_root.has_value()) {
    return runtime::Result<void>::error(
        std::format("Could not find workspace root. Are you sure your "
                    "directory tree contains a {} file?",
                    module::MODULE_FILE_NAME));
  }

  const std::filesystem::path build_dir =
      std::filesystem::absolute(ws_root.value() / workspace::BUILD_DIR_NAME);

  const build::BuildArgs build_args = build::parse_args(arguments);
  auto modules = RESULT_PROPAGATE(
      workspace::open_workspace(ws_root.value())
          .with_context("while opening workspace modules for list"));
  const module::ModuleFile mod_file = RESULT_PROPAGATE(
      workspace::load_root_module_file(ws_root.value())
          .with_context("while loading root module file for list"));

  auto lua_modules =
      build::construct_lua_modules(ws_root.value(), build_dir, modules);
  auto lua_engine = RESULT_PROPAGATE(
      build::prepare_lua_engine(ws_root.value(), *lua_modules, modules, {})
          .with_context("while preparing Lua environment for list"));
  RESULT_PROPAGATE_DISCARD(
      build::invoke_rule_initializers(lua_engine, modules)
          .with_context("while running rule initializers for list"));

  const auto flags = build::merge_yabt_flags(build_args, mod_file);
  RESULT_PROPAGATE_DISCARD(
      lua_modules->flaglib.resolve_flag_values(flags).with_context(
          "while resolving flags for list"));

  RESULT_PROPAGATE_DISCARD(
      build::invoke_build_targets(lua_engine, modules)
          .with_context("while evaluating targets for list"));

  // Find all matching targets for a pattern
  std::vector<std::string> targets{};
  if (build_args.target_patterns.size() == 0) {
    for (const std::string &target : lua_modules->contextlib.all_targets) {
      targets.push_back(target);
    }
  } else {
    for (const std::string &target : lua_modules->contextlib.all_targets) {
      for (const std::regex &regex : build_args.target_patterns) {
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

  return runtime::Result<void>::ok();
}

} // namespace

[[nodiscard]] runtime::Result<void>
ListCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "list", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);
  static_cast<void>(subcommand);
  return runtime::Result<void>::ok();
}

[[nodiscard]] runtime::Result<void> ListCommand::handle_subcommand(
    std::span<const std::string_view> target_patterns) noexcept {
  if (runtime::Result result = list_inner(target_patterns); result.is_error()) {
    yabt_error("List failed: {}", result.error_value());
  }

  return runtime::Result<void>::ok();
}

} // namespace yabt::cmd
