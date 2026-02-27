#pragma once

#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <vector>

#include "yabt/ninja/build_rule.h"
#include "yabt/ninja/build_step.h"
#include "yabt/runtime/result.h"

extern "C" {
struct lua_State;
}

namespace yabt::lua {

class LuaEngine {
public:
  [[nodiscard]] static yabt::runtime::Result<LuaEngine, std::string>
  construct(const std::filesystem::path &workspace_root,
            const std::filesystem::path &build_dir) noexcept;

  LuaEngine(const LuaEngine &) noexcept = delete;
  LuaEngine(LuaEngine &&) noexcept;
  LuaEngine &operator=(const LuaEngine &) noexcept = delete;
  LuaEngine &operator=(LuaEngine &&) noexcept;

  ~LuaEngine() noexcept;

  void set_path(std::span<const std::string> paths) noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  register_module(const std::string &name, const std::filesystem::path &path,
                  std::span<const std::string> target_specs) noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  exec_file(std::string_view file_path) noexcept;

  [[nodiscard]] std::span<const ninja::BuildStep> build_steps() const noexcept;
  [[nodiscard]] std::span<const ninja::BuildStepWithRule>
  build_steps_with_rule() const noexcept;
  [[nodiscard]] const std::map<std::string, ninja::BuildRule> &
  build_rules() const noexcept;

private:
  friend int l_add_build_step(lua_State *const L);
  friend int l_add_build_step_with_rule(lua_State *const L);

  LuaEngine() noexcept = default;

  void add_build_step() noexcept;
  [[nodiscard]] runtime::Result<void, std::string>
  add_build_step_impl() noexcept;

  void add_build_step_with_rule() noexcept;
  [[nodiscard]] runtime::Result<void, std::string>
  add_build_step_with_rule_impl() noexcept;

  lua_State *m_state;
  std::filesystem::path m_workspace_root;

  std::vector<ninja::BuildStep> m_build_steps;
  std::vector<ninja::BuildStepWithRule> m_build_steps_with_rule;
  std::map<std::string, ninja::BuildRule> m_build_rules;
};

} // namespace yabt::lua
