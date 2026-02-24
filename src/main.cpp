#include <cstdio>
#include <string>

#include "yabt/cli/cli_parser.h"
#include "yabt/cmd/build.h"
#include "yabt/cmd/help.h"
#include "yabt/runtime/check_result.h"

namespace {

bool verbose{false};

void register_global_options(yabt::cli::CliParser &cli_parser) {
  yabt::runtime::check(
      cli_parser.register_flag({
          .name{"verbose"},
          .short_name{'v'},
          .optional = true,
          .type = yabt::cli::FlagType::BOOL,
          .description{"Prints additional information for debugging purposes"},
          .handler{[](const auto &) {
            verbose = true;
            return yabt::runtime::Result<void, std::string>::ok();
          }},
      }),
      "Error registering flag {}");
}

yabt::cmd::BuildCommand build_cmd;
yabt::cmd::HelpCommand help_cmd;

void register_subcommands(yabt::cli::CliParser &cli_parser) {
  yabt::runtime::check(build_cmd.register_command(cli_parser),
                       "Unable to register build command: {}");
  yabt::runtime::check(help_cmd.register_command(cli_parser),
                       "Unable to register help command: {}");
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
    printf("Error: %s\n\n", cli_result.error_value().c_str());
    cli_parser.print_help();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
