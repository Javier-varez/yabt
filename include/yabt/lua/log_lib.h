#pragma once

#include "yabt/lua/module.h"

#include "lua.hpp"

namespace yabt::lua {

// Native utilities for logging
struct LogLib : public LuaModule {
  LogLib() noexcept = default;

  void register_in_engine(lua_State *const L) noexcept final;

  LogLib(const LogLib &) noexcept = delete;
  LogLib &operator=(const LogLib &) noexcept = delete;

  LogLib(LogLib &&) noexcept = default;
  LogLib &operator=(LogLib &&) noexcept = default;
};

} // namespace yabt::lua
