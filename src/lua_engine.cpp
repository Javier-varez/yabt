extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

lua_State *init_lua_state() noexcept {
  lua_State *const L = luaL_newstate();
  if (L == nullptr) {
    return L;
  }

  luaL_openlibs(L);

  // const int result = luaL_dofile(L, "main.lua");
  // if (result != 0) {
  //   printf("failed to run file\n");
  //   lua_close(L);
  //   return nullptr;
  // }
  //
  // lua_getglobal(L, "print");
  // lua_pushstring(L, "miller");
  // lua_pushstring(L, "north");
  // lua_pushinteger(L, 42);
  //
  // lua_call(L, 3, 0);
  return L;
}

void uninit_lua_state(lua_State *const L) noexcept { lua_close(L); }
