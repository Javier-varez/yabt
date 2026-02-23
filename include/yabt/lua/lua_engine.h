#pragma once

#include "yabt/ninja/build_rule.h"
#include "yabt/ninja/build_step.h"
#include "yabt/runtime/result.h"

extern "C" {
typedef struct lua_State lua_State;
}

namespace yabt::lua {

class LuaEngine {
public:
  [[nodiscard]] static yabt::runtime::Result<LuaEngine, std::string>
  construct() noexcept;

  LuaEngine(const LuaEngine &) noexcept = delete;
  LuaEngine(LuaEngine &&) noexcept;
  LuaEngine &operator=(const LuaEngine &) noexcept = delete;
  LuaEngine &operator=(LuaEngine &&) noexcept;

  ~LuaEngine() noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  exec_file(std::string_view file_path) noexcept;

  [[nodiscard]] std::span<const ninja::BuildStep> build_steps() const noexcept;
  [[nodiscard]] std::span<const ninja::BuildStepWithRule>
  build_steps_with_rule() const noexcept;
  [[nodiscard]] const std::map<std::string, ninja::BuildRule> &
  build_rules() const noexcept;

private:
  LuaEngine() noexcept = default;

  lua_State *m_state;
};

} // namespace yabt::lua
