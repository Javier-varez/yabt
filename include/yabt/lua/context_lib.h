#pragma once

#include <map>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "yabt/lua/module.h"
#include "yabt/ninja/build_rule.h"
#include "yabt/ninja/build_step.h"
#include "yabt/runtime/result.h"

#include "lua.hpp"

namespace yabt::lua {

// Manage targets, build rules and steps to build them
struct ContextLib : public LuaModule {
  ContextLib() = default;

  ContextLib(const ContextLib &) noexcept = delete;
  ContextLib &operator=(const ContextLib &) noexcept = delete;

  ContextLib(ContextLib &&) noexcept;
  ContextLib &operator=(ContextLib &&) noexcept;

  void register_in_engine(lua_State *const L) noexcept final;

  // Call the stored run/test function for a target with the given args.
  // Returns the argv list (executable + arguments) to execute.
  [[nodiscard]] runtime::Result<std::vector<std::string>, std::string>
  call_run_fn(const std::string &target,
              std::span<const std::string_view> args) noexcept;

  [[nodiscard]] runtime::Result<std::vector<std::string>, std::string>
  call_test_fn(const std::string &target,
               std::span<const std::string_view> args) noexcept;

public:
  std::vector<ninja::BuildStep> build_steps;
  std::vector<ninja::BuildStepWithRule> build_steps_with_rule;
  std::map<std::string, ninja::BuildRule> build_rules;
  std::vector<std::string> all_targets;
  std::map<std::string, int> run_fn_refs;  // target -> Lua registry reference
  std::map<std::string, int> test_fn_refs; // target -> Lua registry reference

  lua_State *state;
  std::string current_target;
  std::set<std::string> leaf_paths;
};

} // namespace yabt::lua
