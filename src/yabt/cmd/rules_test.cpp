#include <span>
#include <string>
#include <string_view>

#include "yabt/build/build.h"
#include "yabt/cmd/rules_test.h"
#include "yabt/process/process.h"
#include "yabt/runtime/result.h"
#include "yabt/workspace/utils.h"

namespace yabt::cmd {

namespace {

const std::string_view SHORT_DESCRIPTION =
    "Interact with the rules of the repository";
const std::string_view LONG_DESCRIPTION =
    "Interact with the rules of the repository.";

[[nodiscard]] runtime::Result<std::string, std::string>
get_luarocks_path(const bool cpath) noexcept {
  const char *type = "--lr-path";
  if (cpath) {
    type = "--lr-cpath";
  }

  process::Process proc{"luarocks", "path", "--lua-version", "5.1", type};
  RESULT_PROPAGATE_DISCARD(proc.start(true));

  const auto output = proc.process_output();
  RESULT_PROPAGATE_DISCARD(output.to_result());

  return runtime::Result<std::string, std::string>::ok(
      output.stdout.value_or(""));
}

} // namespace

[[nodiscard]] runtime::Result<void, std::string>
RulesTestCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "rules_test", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);
  static_cast<void>(subcommand);
  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string>
RulesTestCommand::handle_subcommand(
    std::span<const std::string_view> /*extra_args*/) noexcept {
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

  const auto modules =
      RESULT_PROPAGATE(workspace::open_workspace(ws_root.value()));
  auto lua_modules =
      build::construct_lua_modules(ws_root.value(), build_dir, modules);
  auto lua_engine = RESULT_PROPAGATE(
      prepare_lua_engine(ws_root.value(), *lua_modules, modules,
                         std::array{build::LuaPath{
                             .path{RESULT_PROPAGATE(get_luarocks_path(false))},
                             .cpath{RESULT_PROPAGATE(get_luarocks_path(true))},
                         }}));

  std::vector<std::string> spec_dirs{};
  for (const auto &mod : modules) {
    const auto rules_dir = mod->rules_dir();
    if (rules_dir.has_value()) {
      const std::filesystem::path spec_dir = rules_dir.value() / "spec";
      if (std::filesystem::exists(spec_dir)) {
        spec_dirs.push_back(spec_dir.string());
      }
    }
  }

  using std::string_literals::operator""s;
  RESULT_PROPAGATE_DISCARD(lua_engine.set_args(spec_dirs));
  RESULT_PROPAGATE_DISCARD(
      lua_engine.exec_string("require 'busted.runner' { standalone = false }"));

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
