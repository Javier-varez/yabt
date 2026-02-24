#include <span>
#include <string>
#include <string_view>

#include "yabt/cli/args.h"
#include "yabt/cmd/build.h"
#include "yabt/lua/lua_engine.h"
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

  return subcommand.register_flag({
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
  });
}

[[nodiscard]] runtime::Result<void, std::string>
BuildCommand::handle_subcommand(
    std::span<const std::string_view> /* unparsed_args */) noexcept {
  yabt::lua::LuaEngine engine =
      RESULT_PROPAGATE(yabt::lua::LuaEngine::construct());

  RESULT_PROPAGATE_DISCARD(engine.exec_file("main.lua"));

  const auto build_steps = engine.build_steps();
  const auto build_steps_with_rule = engine.build_steps_with_rule();
  const auto build_rules = engine.build_rules();

  // Rules
  size_t i = 0;
  for (const ninja::BuildStep &step : build_steps) {
    printf("rule step%ld\n", i++);
    printf("    command = %s\n", step.cmd.c_str());
    printf("    description = %s\n", step.descr.c_str());
  }

  for (const auto &[name, rule] : build_rules) {
    printf("rule %s\n", rule.name.c_str());
    printf("    command = %s\n", rule.cmd.c_str());
    printf("    description = %s\n", rule.descr.c_str());
    for (const auto &v : rule.variables) {
      printf("    %s = %s\n", v.first.c_str(), v.second.c_str());
    }
  }

  // Build vals
  i = 0;
  for (const ninja::BuildStep &step : build_steps) {
    printf("build ");
    for (const std::string &out : step.outs) {
      printf("%s ", out.c_str());
    }

    printf(": step%ld", i++);
    for (const std::string &in : step.ins) {
      printf("%s ", in.c_str());
    }
    printf("\n");
  }

  for (const ninja::BuildStepWithRule &step : build_steps_with_rule) {
    printf("build ");
    for (const std::string &out : step.outs) {
      printf("%s ", out.c_str());
    }

    printf(": %s ", step.ruleName.c_str());
    for (const std::string &in : step.ins) {
      printf("%s ", in.c_str());
    }
    printf("\n");
    for (const auto &v : step.variables) {
      if (v.first.length() == 0 || v.second.length() == 0)
        continue;
      printf("    %s = %s\n", v.first.c_str(), v.second.c_str());
    }
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cmd
