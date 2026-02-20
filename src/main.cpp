#include <stdio.h>

#include "lua_engine.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

typedef struct Rule {
  const char *name;
  const char *statement;
  const char *description;
} Rule;

typedef struct InPath {
  const char *path;
} InPath;

typedef struct OutPath {
  const char *path;
} OutPath;

typedef struct Flag {
  const char *name;
  const char *value;
} Flag;

typedef struct Build {
  const char *rule_name;
  const OutPath *out_paths;
  const InPath *in_paths;
  const Flag *flags;
} Build;

// int add_rule(lua_State *const L) { return 0; }

int main() {
  lua_State *L = init_lua_state();
  if (L == nullptr) {
    printf("failed to create lua state\n");
  }

  const int result = luaL_dofile(L, "script.lua");
  if (result != 0) {
    printf("failed to run file\n");
  }

  lua_getglobal(L, "print");
  lua_pushstring(L, "miller");
  lua_pushstring(L, "north");
  lua_pushinteger(L, 42);

  lua_call(L, 3, 0);

  uninit_lua_state(L);

  return 0;
}
