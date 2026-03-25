#include <span>
#include <string>
#include <string_view>

#include "yabt/build/build.h"
#include "yabt/cli/args.h"
#include "yabt/cmd/build.h"
#include "yabt/log/log.h"
#include "yabt/runtime/result.h"

namespace yabt::cmd {

namespace {
const std::string_view SHORT_DESCRIPTION = "Builds the requested targets";
const std::string_view LONG_DESCRIPTION =
    "Builds requested targets using the specified target spec. A target spec\n"
    "is ... (TBD)";
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

  RESULT_PROPAGATE_DISCARD(subcommand.register_flag({
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
  }));

  return subcommand.register_flag({
      .name{"compdb"},
      .short_name{},
      .optional = true,
      .type = yabt::cli::FlagType::BOOL,
      .description{"Generates a compilation database in the build directory"},
      .handler{[this](const cli::Arg &) {
        this->m_compdb = true;
        return runtime::Result<void, std::string>::ok();
      }},
  });
}

[[nodiscard]] runtime::Result<void, std::string>
BuildCommand::handle_subcommand(
    std::span<const std::string_view> target_patterns) noexcept {
  if (runtime::Result result = build::execute_build(
          m_threads, m_compdb, m_build_dir, target_patterns);
      result.is_error()) {
    yabt_error("Build failed: {}", result.error_value());
    exit(EXIT_FAILURE);
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
