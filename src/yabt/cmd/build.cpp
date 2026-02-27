#include <span>
#include <string>
#include <string_view>

#include "yabt/build/build.h"
#include "yabt/cli/args.h"
#include "yabt/cmd/build.h"
#include "yabt/log/log.h"
#include "yabt/ninja/ninja.h"
#include "yabt/process/process.h"
#include "yabt/runtime/result.h"
#include "yabt/workspace/utils.h"

namespace yabt::cmd {

namespace {
const std::string_view SHORT_DESCRIPTION = "Builds the requested targets";
const std::string_view LONG_DESCRIPTION =
    "Builds requested targets using the specified target spec. A target spec\n"
    "is ... (TBD)";

[[nodiscard]] runtime::Result<void, std::string>
build_inner(const int threads,
            const std::optional<std::filesystem::path> &req_build_dir) {
  const std::optional<std::filesystem::path> ws_root =
      workspace::get_workspace_root();
  if (!ws_root.has_value()) {
    return runtime::Result<void, std::string>::error(
        std::format("Could not find workspace root. Are you sure your "
                    "directory tree contains a {} file?",
                    module::MODULE_FILE_NAME));
  }

  const std::filesystem::path build_dir = std::filesystem::absolute(
      req_build_dir.value_or(ws_root.value() / workspace::BUILD_DIR_NAME));

  auto modules = RESULT_PROPAGATE(workspace::open_workspace(ws_root.value()));
  auto lua_engine = RESULT_PROPAGATE(
      build::prepare_lua_engine(ws_root.value(), build_dir, modules));
  RESULT_PROPAGATE_DISCARD(
      build::invoke_rule_initializers(lua_engine, modules));
  RESULT_PROPAGATE_DISCARD(build::invoke_build_targets(lua_engine, modules));

  const std::filesystem::path ninja_file =
      build_dir / workspace::NINJA_FILE_PATH;

  RESULT_PROPAGATE_DISCARD(ninja::save_ninja_file(
      ninja_file, lua_engine.build_rules(), lua_engine.build_steps(),
      lua_engine.build_steps_with_rule()));

  const std::string threads_str = std::format("{}", threads);
  process::Process ninja{"ninja", "-j", threads_str};
  ninja.set_cwd((build_dir).native());
  RESULT_PROPAGATE_DISCARD(ninja.start());
  return ninja.process_output().to_result();
}

} // namespace

[[nodiscard]] runtime::Result<void, std::string>
BuildCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "build", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);

  RESULT_PROPAGATE_DISCARD(subcommand.register_flag({
      .name{"threads"},
      .short_name{},
      .optional = true,
      .type = yabt::cli::FlagType::INTEGER,
      .description{"The number of threads to use during the build process"},
      .handler{[this](const cli::Arg &a) {
        const cli::IntegerArg arg = std::get<cli::IntegerArg>(a);
        this->m_threads = arg.value;
        return runtime::Result<void, std::string>::ok();
      }},
  }));

  return subcommand.register_flag({
      .name{"build-dir"},
      .short_name{},
      .optional = true,
      .type = yabt::cli::FlagType::STRING,
      .description{"Overrides the build directory with the given path."},
      .handler{[this](const cli::Arg &a) {
        const cli::StringArg arg = std::get<cli::StringArg>(a);
        this->m_build_dir = arg.value;
        return runtime::Result<void, std::string>::ok();
      }},
  });
}

[[nodiscard]] runtime::Result<void, std::string>
BuildCommand::handle_subcommand(
    std::span<const std::string_view> /* unparsed_args */) noexcept {

  if (runtime::Result result = build_inner(m_threads, m_build_dir);
      result.is_error()) {
    yabt_error("Build failed: {}", result.error_value());
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
