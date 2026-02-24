#include <algorithm>
#include <cstdio>
#include <string_view>

#include "yabt/cli/cli_parser.h"

namespace yabt::cli {

using std::string_view_literals::operator""sv;

namespace {

[[nodiscard]] std::string_view trim_left_charset(std::string_view sv,
                                                 std::string_view chars) {
  const size_t n = sv.find_first_not_of(chars);
  if (n == std::string_view::npos) {
    return sv;
  }
  return sv.substr(n);
}

[[nodiscard]] std::string_view trim_right_charset(std::string_view sv,
                                                  std::string_view chars) {
  const size_t n = sv.find_last_not_of(chars);
  if (n == std::string_view::npos) {
    return sv;
  }
  return sv.substr(0, n + 1);
}

[[nodiscard]] runtime::Result<std::pair<Flag, Arg>, std::string>
validate_flag(const Flag &flag, std::string_view name, std::string_view value) {
  using Result = runtime::Result<std::pair<Flag, Arg>, std::string>;
  switch (flag.type) {
  case FlagType::STRING: {
    if (value.length() == 0) {
      return Result::error(std::format("Expected value for flag \"{}\"", name));
    }

    return Result::ok(std::make_pair(flag, Arg{StringArg{
                                               .name{name},
                                               .value{value},
                                           }}));
  }
  case FlagType::BOOL: {
    if (value.length() != 0) {
      return Result::error(
          std::format("Got value for bool flag \"{}\" = \"{}\"", name, value));
    }

    return Result::ok(std::make_pair(flag, Arg{BoolArg{
                                               .name{name},
                                           }}));
  }
  case FlagType::INTEGER: {
    if (value.length() == 0) {
      return Result::error(std::format("Expected value for flag \"{}\"", name));
    }

    const long long integer = std::strtoll(value.data(), nullptr, 0);

    return Result::ok(std::make_pair(flag, Arg{IntegerArg{
                                               .name{name},
                                               .value = integer,
                                           }}));
  }
  }
  runtime::fatal("BUG: Invalid flag type registered: {}\n",
                 static_cast<std::underlying_type_t<FlagType>>(flag.type));
}

[[nodiscard]] runtime::Result<std::pair<Flag, Arg>, std::string>
parse_long_flag(const std::span<const Flag> &flags, std::string_view sv) {
  constexpr static std::string_view DELIMITERS = " \t\r\n="sv;
  // Find delimiter (whitespace or equals)
  const size_t eon = sv.find_first_of(DELIMITERS);
  const std::string_view flag_name = sv.substr(0, eon);
  const auto iter =
      std::find_if(flags.begin(), flags.end(),
                   [flag_name](const Flag &flag) noexcept -> bool {
                     return flag.name == flag_name;
                   });
  if (iter == flags.end()) {
    return runtime::Result<std::pair<Flag, Arg>, std::string>::error(
        std::format("Unknown long flag: {}", flag_name));
  }

  std::string_view flag_value;
  if (eon != std::string_view::npos) {
    flag_value = trim_right_charset(
        trim_left_charset(sv.substr(eon), DELIMITERS), DELIMITERS);
  }
  return validate_flag(*iter, flag_name, flag_value);
}

[[nodiscard]] runtime::Result<std::vector<std::pair<Flag, Arg>>, std::string>
parse_short_flags(const std::span<const Flag> &flags, std::string_view sv) {
  using Result =
      runtime::Result<std::vector<std::pair<Flag, Arg>>, std::string>;

  std::vector<std::pair<Flag, Arg>> args;
  for (const char flag_char : sv) {
    const auto iter =
        std::find_if(flags.begin(), flags.end(),
                     [flag_char](const Flag &flag) noexcept -> bool {
                       return flag.short_name.has_value() &&
                              flag.short_name.value() == flag_char;
                     });
    if (iter == flags.end()) {
      return Result::error(std::format("Unknown short flag: {}", flag_char));
    }

    const char flag[]{flag_char, '\0'};

    const std::pair<Flag, Arg> arg =
        RESULT_PROPAGATE(validate_flag(*iter, std::string_view{flag}, ""sv));
    args.push_back(arg);
  }

  return Result::ok(args);
}

void print_flags(const std::span<const Flag> flags, const char *title) {
  if (flags.size() > 0) {
    printf("%s:\n", title);
  }

  const auto optional_str = [](const Flag &flag) {
    if (flag.optional || flag.type == FlagType::BOOL) {
      return "(Optional) ";
    }
    return "";
  };

  const auto required_arg_str = [](const Flag &flag) {
    switch (flag.type) {
    case yabt::cli::FlagType::BOOL: {
      return std::format("--{}", flag.name);
    }
    case yabt::cli::FlagType::STRING: {
      return std::format("--{} <string>", flag.name);
    }
    case yabt::cli::FlagType::INTEGER: {
      return std::format("--{} <int>", flag.name);
    }
    }
    return std::string{""};
  };

  for (const Flag &flag : flags) {
    if (flag.short_name.has_value()) {
      printf("  -%c, %-20s%s%s\n", flag.short_name.value(),
             required_arg_str(flag).c_str(), optional_str(flag),
             flag.description.c_str());
    } else {
      printf("      %-20s%s%s\n", required_arg_str(flag).c_str(),
             optional_str(flag), flag.description.c_str());
    }
  }
}

} // namespace

CliParser::CliParser(const char *argv0) noexcept : m_command_name{argv0} {}

[[nodiscard]] Subcommand &
CliParser::register_subcommand(std::string_view name,
                               SubcommandHandler &handler,
                               std::string_view short_description,
                               std::string_view long_description) noexcept {
  const auto iter =
      std::find_if(m_subcommands.begin(), m_subcommands.end(),
                   [&name](const Subcommand &subcommand) noexcept -> bool {
                     return subcommand.name() == name;
                   });
  if (iter != m_subcommands.end()) {
    iter->set_handler(handler);
    return *iter;
  }

  m_subcommands.push_back(
      Subcommand{name, handler, short_description, long_description});
  return m_subcommands.back();
}

runtime::Result<void, std::string>
CliParser::register_flag(Flag config) noexcept {
  RESULT_PROPAGATE_DISCARD(config.validate());

  const auto iter = std::find_if(m_global_flags.begin(), m_global_flags.end(),
                                 [&config](const Flag &flag) noexcept -> bool {
                                   return flag.name == config.name;
                                 });
  if (iter != m_global_flags.end()) {
    *iter = std::move(config);
    return runtime::Result<void, std::string>::ok();
  }
  m_global_flags.push_back(std::move(config));

  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string>
CliParser::parse(const size_t argc, const char *argv[]) const noexcept {
  std::vector<std::pair<Flag, Arg>> global_args{};

  // Parse global flags
  size_t arg_idx = 1;
  for (; arg_idx < argc; arg_idx++) {
    std::string_view sv{argv[arg_idx]};
    if (sv.starts_with("--")) {
      // Long flag, strip prefix
      sv = sv.substr(2);
      const std::pair<Flag, Arg> arg =
          RESULT_PROPAGATE(parse_long_flag(std::span{m_global_flags}, sv));
      global_args.push_back(arg);
      continue;
    }

    if (sv.starts_with("-")) {
      // short flag
      sv = sv.substr(1);
      std::vector<std::pair<Flag, Arg>> args =
          RESULT_PROPAGATE(parse_short_flags(std::span{m_global_flags}, sv));
      for (std::pair<Flag, Arg> &arg : args) {
        global_args.push_back(std::move(arg));
      }
      continue;
    }

    // Not a flag, we deal with it as a subcommand
    break;
  }

  // TODO Check that all non-optional global flags were given

  if (arg_idx >= argc) {
    return runtime::Result<void, std::string>::error(
        "Did not get a valid subcommand");
  }

  std::string_view given_subcommand{argv[arg_idx++]};
  const auto subcommand_iter =
      std::find_if(m_subcommands.cbegin(), m_subcommands.cend(),
                   [given_subcommand](const Subcommand &subcommand) {
                     return subcommand.name() == given_subcommand;
                   });
  if (subcommand_iter == m_subcommands.cend()) {
    return runtime::Result<void, std::string>::error(
        std::format("Unknown subcommand \"{}\"", given_subcommand));
  }

  const Subcommand &subcommand = *subcommand_iter;
  const std::span<const Flag> subcommand_flags = subcommand.flags();

  // Parse subcommand flags
  std::vector<std::pair<Flag, Arg>> subcommand_args{};
  for (; arg_idx < argc; arg_idx++) {
    std::string_view sv{argv[arg_idx]};

    if (sv.starts_with("--")) {
      // Long flag, strip prefix
      sv = sv.substr(2);
      const std::pair<Flag, Arg> arg =
          RESULT_PROPAGATE(parse_long_flag(std::span{subcommand_flags}, sv));
      subcommand_args.push_back(arg);
      continue;
    }

    if (sv.starts_with("-")) {
      // short flag
      std::vector<std::pair<Flag, Arg>> args = RESULT_PROPAGATE(
          parse_short_flags(std::span{subcommand_flags}, sv.substr(1)));
      for (std::pair<Flag, Arg> &arg : args) {
        subcommand_args.push_back(std::move(arg));
      }
      continue;
    }

    // Not a flag, we deal with it as unparsed args
    break;
  }

  // TODO Check that all non-optional subcommand flags were given

  std::vector<std::string_view> unparsed_args;
  for (; arg_idx < argc; arg_idx++) {
    unparsed_args.push_back(std::string_view{argv[arg_idx]});
  }

  for (const auto &[flag, arg] : global_args) {
    if (flag.handler) {
      RESULT_PROPAGATE_DISCARD(flag.handler(arg));
    }
  }
  for (const auto &[flag, arg] : subcommand_args) {
    if (flag.handler) {
      RESULT_PROPAGATE_DISCARD(flag.handler(arg));
    }
  }

  return subcommand.invoke(unparsed_args);
  return runtime::Result<void, std::string>::ok();
}

void CliParser::set_description(std::string_view descr) noexcept {
  m_descr = descr;
}

void CliParser::print_help() const noexcept {
  printf("%s\n\n", m_descr.c_str());
  printf("Usage:\n"
         "  %s [global options] <Subcommand> [subcommand options]\n\n",
         m_command_name.c_str());

  print_flags(m_global_flags, "Flags");
  printf("\n");

  if (m_subcommands.size() > 0) {
    printf("Available Commands:\n");
  }
  for (const Subcommand &cmd : m_subcommands) {
    printf("  %-20s%s\n", cmd.name().c_str(), cmd.short_description().c_str());
  }
}

void CliParser::print_subcommand_help(
    std::string_view subcommand) const noexcept {
  const auto iter =
      std::find_if(m_subcommands.cbegin(), m_subcommands.cend(),
                   [subcommand](const Subcommand &cmd) noexcept -> bool {
                     return cmd.name() == subcommand;
                   });
  if (iter == m_subcommands.cend()) {
    printf("Unknown command %s.\n\n", std::string(subcommand).c_str());
    print_help();
    return;
  }

  const Subcommand &cmd = *iter;

  printf("%s\n\n", cmd.long_description().c_str());
  printf("Usage:\n"
         "  %s [global options] %s [subcommand options]\n\n",
         m_command_name.c_str(), cmd.name().c_str());

  print_flags(m_global_flags, "Global Flags");
  printf("\n");

  print_flags(cmd.flags(), "Subcommand Flags");
  printf("\n");
}

} // namespace yabt::cli
