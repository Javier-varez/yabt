#include "yabt/cmd/help.h"

namespace yabt::cmd {

namespace {
const std::string_view SHORT_DESCRIPTION =
    "Shows usage information about the tool.";
const std::string_view LONG_DESCRIPTION =
    "Shows usage information about the tool and available commands.";
} // namespace

[[nodiscard]] runtime::Result<void, std::string>
HelpCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "help", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);
  static_cast<void>(subcommand);
  m_parser = &cli_parser;
  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string> HelpCommand::handle_subcommand(
    std::span<const std::string_view> unparsed_args) noexcept {

  if (unparsed_args.size() == 0) {
    m_parser->print_help();
  } else {
    m_parser->print_subcommand_help(unparsed_args[0]);
  }
  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
