#include <fcntl.h>
#include <sys/stat.h>

#include "yabt/ninja/ninja.h"

namespace yabt::ninja {

runtime::Result<void, std::string> save_ninja_file(
    const std::filesystem::path ninja_filepath,
    const std::map<std::string, BuildRule> &build_rules,
    const std::span<const BuildStep> build_steps,
    const std::span<const BuildStepWithRule> build_steps_with_rule) noexcept {

  // FIXME: This code deserves a bit of cleanup. But for now it does the job.
  std::filesystem::create_directories(ninja_filepath.parent_path());
  FILE *file = fopen(ninja_filepath.c_str(), "wb");
  runtime::check(file != nullptr, "Error opening ninja file");

  // Rules
  size_t i = 0;
  for (const ninja::BuildStep &step : build_steps) {
    fprintf(file, "rule step%ld\n", i++);
    fprintf(file, "    command = %s\n", step.cmd.c_str());
    fprintf(file, "    description = %s\n", step.descr.c_str());
  }

  for (const auto &[name, rule] : build_rules) {
    fprintf(file, "rule %s\n", rule.name.c_str());
    fprintf(file, "    command = %s\n", rule.cmd.c_str());
    fprintf(file, "    description = %s\n", rule.descr.c_str());
    for (const auto &v : rule.variables) {
      fprintf(file, "    %s = %s\n", v.first.c_str(), v.second.c_str());
    }
  }

  // Build vals
  i = 0;
  for (const ninja::BuildStep &step : build_steps) {
    fprintf(file, "build ");
    for (const std::string &out : step.outs) {
      fprintf(file, "%s ", out.c_str());
    }

    fprintf(file, ": step%ld", i++);
    for (const std::string &in : step.ins) {
      fprintf(file, "%s ", in.c_str());
    }
    fprintf(file, "\n");
  }

  for (const ninja::BuildStepWithRule &step : build_steps_with_rule) {
    fprintf(file, "build ");
    for (const std::string &out : step.outs) {
      fprintf(file, "%s ", out.c_str());
    }

    fprintf(file, ": %s ", step.rule_name.c_str());
    for (const std::string &in : step.ins) {
      fprintf(file, "%s ", in.c_str());
    }
    fprintf(file, "\n");
    for (const auto &v : step.variables) {
      if (v.first.length() == 0 || v.second.length() == 0)
        continue;
      fprintf(file, "    %s = %s\n", v.first.c_str(), v.second.c_str());
    }
  }
  fflush(file);
  fclose(file);

  return runtime::Result<void, std::string>::ok();
}
} // namespace yabt::ninja
