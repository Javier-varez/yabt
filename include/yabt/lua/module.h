#pragma once

#include "lua.hpp"

namespace yabt::lua {

class LuaModule {
public:
  virtual void register_in_engine(lua_State *const L) = 0;

  virtual ~LuaModule() = default;
};

} // namespace yabt::lua
