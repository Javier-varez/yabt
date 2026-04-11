#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <span>
#include <string>
#include <vector>

#include "yabt/lua/module.h"
#include "yabt/lua/path_lib.h"
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
  construct(const std::filesystem::path &workspace_root) noexcept;

  LuaEngine(const LuaEngine &) noexcept = delete;
  LuaEngine &operator=(const LuaEngine &) noexcept = delete;

  LuaEngine(LuaEngine &&) noexcept;
  LuaEngine &operator=(LuaEngine &&) noexcept;

  ~LuaEngine() noexcept;

  void set_path(std::span<const std::string> paths) noexcept;

  // TODO: Move to preload module
  void set_preloaded_lua_packages(
      std::map<std::string, std::string_view> packages) noexcept;

  // TODO: Move to runtime lua module
  [[nodiscard]] runtime::Result<void, std::string>
  register_yabt_module(const std::string &name,
                       const std::filesystem::path &path,
                       std::span<const std::string> target_specs) noexcept;

  void register_lua_module(LuaModule &module) noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  exec_file(std::string_view file_path) noexcept;

  [[nodiscard]] runtime::Result<void, std::string>
  exec_string(const char *string) noexcept;

private:
  friend int l_do_yabt_preload(lua_State *const L) noexcept;

  LuaEngine() noexcept = default;
  [[nodiscard]] int do_yabt_preload() noexcept;

  lua_State *m_state;
  std::filesystem::path m_workspace_root;
  std::map<std::string, std::string_view> m_preloaded_packages;
};

} // namespace yabt::lua
