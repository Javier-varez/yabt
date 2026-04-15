#pragma once

#include "yabt/lua/module.h"

#include "lua.hpp"

namespace yabt::lua {

// Native utilities for logging
struct LogLib : public LuaModule {
  LogLib() = default;

  void register_in_engine(lua_State *const L) final;

  LogLib(const LogLib &) = delete;
  LogLib &operator=(const LogLib &) = delete;

  LogLib(LogLib &&) = default;
  LogLib &operator=(LogLib &&) = default;
};

} // namespace yabt::lua
