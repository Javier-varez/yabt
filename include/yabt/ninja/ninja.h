#pragma once

#include <filesystem>
#include <map>
#include <span>
#include <string>

#include "yabt/ninja/build_rule.h"
#include "yabt/ninja/build_step.h"
#include "yabt/runtime/result.h"

namespace yabt::ninja {

[[nodiscard]] runtime::Result<void, std::string> save_ninja_file(
    const std::filesystem::path ninja_filepath,
    const std::map<std::string, BuildRule> &build_rules,
    std::span<const BuildStep> build_steps,
    std::span<const BuildStepWithRule> build_steps_with_rule) noexcept;

}
