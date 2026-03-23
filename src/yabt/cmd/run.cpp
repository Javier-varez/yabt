#include <string>
#include <string_view>

#include "yabt/cli/args.h"
#include "yabt/cmd/build.h"
#include "yabt/cmd/run.h"
#include "yabt/log/log.h"
#include "yabt/runtime/result.h"

namespace yabt::cmd {

namespace {
const std::string_view SHORT_DESCRIPTION = "Runs the requested targets";
const std::string_view LONG_DESCRIPTION =
    "Alias for the build command. Builds requested targets using the specified "
    "target spec.";
} // namespace

[[nodiscard]] runtime::Result<void, std::string>
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

[[nodiscard]] runtime::Result<void, std::string> RunCommand::handle_subcommand(
    std::span<const std::string_view> target_patterns) noexcept {
  if (runtime::Result result =
          build_inner(m_threads, false, m_build_dir, target_patterns);
      result.is_error()) {
    yabt_error("Build failed: {}", result.error_value());
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
