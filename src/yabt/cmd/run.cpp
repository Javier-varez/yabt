#include <string>
#include <string_view>

#include "yabt/build/build.h"
#include "yabt/cli/args.h"
#include "yabt/cmd/run.h"
#include "yabt/log/log.h"
#include "yabt/runtime/result.h"

namespace yabt::cmd {

namespace {
const std::string_view SHORT_DESCRIPTION = "Runs the requested targets";
const std::string_view LONG_DESCRIPTION =
    "Builds the requested targets and runs them.";
} // namespace

[[nodiscard]] runtime::Result<void>
RunCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "run", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);

  RESULT_PROPAGATE_DISCARD(subcommand.register_flag({
      .name{"threads"},
      .short_name{},
      .optional = true,
      .type = yabt::cli::FlagType::INTEGER,
      .description{"The number of threads to use during the build process"},
      .handler{[this](const cli::Arg &a) {
        const cli::IntegerArg arg = std::get<cli::IntegerArg>(a);
        this->m_threads = arg.value;
        return runtime::Result<void>::ok();
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
        return runtime::Result<void>::ok();
      }},
  });
}

[[nodiscard]] runtime::Result<void> RunCommand::handle_subcommand(
    std::span<const std::string_view> unparsed_args) noexcept {
  const build::BuildArgs build_args = build::parse_args(unparsed_args);
  if (const runtime::Result result = build::execute_build(
          m_threads, false, m_build_dir, build_args, build::PostBuildMode::Run);
      result.is_error()) {
    yabt_error("Run failed: {}", result.error_value());
    exit(EXIT_FAILURE);
  }

  return runtime::Result<void>::ok();
}

} // namespace yabt::cmd
