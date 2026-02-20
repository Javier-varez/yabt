#pragma once

extern "C" {
#include "lua.h"
}

[[nodiscard]] lua_State *init_lua_state() noexcept;

void uninit_lua_state(lua_State *const L) noexcept;
