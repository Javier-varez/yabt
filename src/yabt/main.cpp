#include <string>

#include "yabt/cli/cli_parser.h"
#include "yabt/cmd/build.h"
#include "yabt/cmd/clean.h"
#include "yabt/cmd/help.h"
#include "yabt/cmd/list.h"
#include "yabt/cmd/sync.h"
#include "yabt/log/log.h"
#include "yabt/runtime/check_result.h"

namespace {

void register_global_options(yabt::cli::CliParser &cli_parser) {
  yabt::runtime::check(
      cli_parser.register_flag({
          .name{"verbose"},
          .short_name{'v'},
          .optional = true,
          .type = yabt::cli::FlagType::BOOL,
          .description{"Prints additional information for debugging purposes."},
          .handler{[](const yabt::cli::Arg &) {
            yabt::log::set_log_level(yabt::log::LogLevel::VERBOSE);
            return yabt::runtime::Result<void, std::string>::ok();
          }},
      }),
      "Error registering flag {}");

  yabt::runtime::check(
      cli_parser.register_flag({
          .name{"log-level"},
          .short_name{},
          .optional = true,
          .type = yabt::cli::FlagType::STRING,
          .description{"Sets the log level to one of: \"ERROR\", \"WARNING\", "
                       "\"INFO\", \"DEBUG\", \"VERBOSE\"."},
          .handler{[](const yabt::cli::Arg &level) {
            yabt::cli::StringArg level_arg =
                std::get<yabt::cli::StringArg>(level);
            return yabt::log::set_log_level(level_arg.value);
          }},
      }),
      "Error registering flag {}");

  yabt::runtime::check(
      cli_parser.register_flag({
          .name{"no-color"},
          .short_name{},
          .optional = true,
          .type = yabt::cli::FlagType::BOOL,
          .description{"Disables coloring of the output provided by yabt."},
          .handler{[](const yabt::cli::Arg &) {
            yabt::log::disable_color();
            return yabt::runtime::Result<void, std::string>::ok();
          }},
      }),
      "Error registering flag {}");
}

yabt::cmd::BuildCommand build_cmd;
yabt::cmd::HelpCommand help_cmd;
yabt::cmd::SyncCommand sync_cmd;
yabt::cmd::CleanCommand clean_cmd;
yabt::cmd::ListCommand list_cmd;

void register_subcommands(yabt::cli::CliParser &cli_parser) {
  yabt::runtime::check(build_cmd.register_command(cli_parser),
                       "Unable to register build command: {}");
  yabt::runtime::check(help_cmd.register_command(cli_parser),
                       "Unable to register help command: {}");
  yabt::runtime::check(sync_cmd.register_command(cli_parser),
                       "Unable to register sync command: {}");
  yabt::runtime::check(clean_cmd.register_command(cli_parser),
                       "Unable to register clean command: {}");
  yabt::runtime::check(list_cmd.register_command(cli_parser),
                       "Unable to register list command: {}");
}

} // namespace

int main(int argc, const char *argv[]) noexcept {
  yabt::cli::CliParser cli_parser{argv[0]};
  cli_parser.set_description(
      "Yet Another Build Tool (yabt) is a build system inspired by the "
      "Daedalean Build Tool (dbt).\n"
      "This tool may be used to manage project dependencies and automate "
      "build and CI steps.");

  register_global_options(cli_parser);
  register_subcommands(cli_parser);

  yabt::runtime::Result cli_result = cli_parser.parse(argc, argv);
  if (!cli_result.is_ok()) {
    yabt_error("{}\n", cli_result.error_value());
    cli_parser.print_help();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
