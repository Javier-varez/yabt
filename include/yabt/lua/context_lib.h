#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "yabt/lua/module.h"
#include "yabt/ninja/build_rule.h"
#include "yabt/ninja/build_step.h"

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

public:
  std::vector<ninja::BuildStep> build_steps;
  std::vector<ninja::BuildStepWithRule> build_steps_with_rule;
  std::map<std::string, ninja::BuildRule> build_rules;
  std::vector<std::string> all_targets;

  lua_State *state;
  std::string current_target;
  std::set<std::string> leaf_paths;
};

} // namespace yabt::lua
