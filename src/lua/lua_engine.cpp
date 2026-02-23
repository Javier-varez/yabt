#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "yabt/lua/lua_engine.h"
#include "yabt/ninja/build_rule.h"
#include "yabt/ninja/build_step.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

namespace yabt::lua {

// FIXME: Should not be global, but use data in the LuaEngine
std::vector<ninja::BuildStep> build_steps;
std::vector<ninja::BuildStepWithRule> build_steps_with_rule;
std::map<std::string, ninja::BuildRule> build_rules;

// Table is supposed to be on top of the stack
[[nodiscard]] std::vector<std::string> read_string_array(lua_State *const L,
                                                         const char *field) {
  lua_pushstring(L, field);
  lua_gettable(L, -2);

  const size_t n = lua_objlen(L, -1);
  std::vector<std::string> result(n);

  for (size_t i = 1; i <= n; i++) {
    lua_rawgeti(L, -1, i);
    if (!lua_isstring(L, -1)) {
      // FIXME: handle error
      printf("Is not a string!\n");
    }

    size_t len{};
    const char *str = luaL_checklstring(L, -1, &len);
    result.push_back(std::string(str, len));
    lua_pop(L, 1);
  }

  lua_pop(L, 1);
  return result;
}

// Table is supposed to be on top of the stack
[[nodiscard]] std::string read_string(lua_State *const L, const char *field) {
  lua_pushstring(L, field);
  lua_gettable(L, -2);

  size_t len{};
  const char *str = luaL_checklstring(L, -1, &len);
  const std::string v(str, len);

  lua_pop(L, 1);
  return v;
}

// Table is supposed to be on top of the stack
[[nodiscard]] std::map<std::string, std::string>
read_string_map(lua_State *const L, const char *field) {
  lua_pushstring(L, field);
  lua_gettable(L, -2);

  if (!lua_istable(L, -1)) {
    if (!lua_isnil(L, -1)) {
      printf("Not a map table! Got type: %s\n",
             lua_typename(L, lua_type(L, -1)));
    }
    lua_pop(L, 1);
    return {};
  }

  std::map<std::string, std::string> result;

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    if (!lua_isstring(L, -1) || !lua_isstring(L, -2)) {
      printf("One of the variables is not a string type: ");
      // FIXME: handle error
    }

    size_t len{};
    const char *str = luaL_checklstring(L, -2, &len);
    std::string key{str, len};

    str = luaL_checklstring(L, -1, &len);
    std::string value{str, len};
    result.emplace(std::move(key), std::move(value));

    // drop value, keep key
    lua_pop(L, 1);
  }

  lua_pop(L, 1);
  return result;
}

static int l_add_build_step(lua_State *const L) {
  if (!lua_istable(L, 1)) {
    // FIXME: handle error
    return 0;
  }

  build_steps.push_back(ninja::BuildStep{
      .outs{read_string_array(L, "outs")},
      .ins{read_string_array(L, "ins")},
      .cmd{read_string(L, "cmd")},
      .descr{read_string(L, "descr")},
  });

  lua_pop(L, 1);
  return 0;
}

static int l_add_build_step_with_rule(lua_State *const L) {
  if (!lua_istable(L, 1) || !lua_istable(L, 2)) {
    // FIXME: handle error
    return 0;
  }
  const std::string build_rule_name = read_string(L, "name");
  if (build_rules.find(build_rule_name) == build_rules.cend()) {
    build_rules.insert(std::pair{
        build_rule_name, ninja::BuildRule{
                             .name{build_rule_name},
                             .cmd{read_string(L, "cmd")},
                             .descr{read_string(L, "descr")},
                             .variables{read_string_map(L, "variables")},
                         }});
  }
  lua_pop(L, 1);

  build_steps_with_rule.push_back(ninja::BuildStepWithRule{
      .outs{read_string_array(L, "outs")},
      .ins{read_string_array(L, "ins")},
      .ruleName{read_string(L, "rule_name")},
      .variables{read_string_map(L, "variables")},
  });
  lua_pop(L, 1);

  return 0;
}

static const luaL_Reg yabt_methods[]{
    {"add_build_step", l_add_build_step},
    {"add_build_step_with_rule", l_add_build_step_with_rule},
    {nullptr, nullptr},
};

void luaopen_yabt(lua_State *const L) noexcept {
  luaL_register(L, "yabt_native", yabt_methods);
  lua_pop(L, 1);

  lua_pushstring(L, "source/");
  lua_setglobal(L, "SOURCE_DIR");

  lua_pushstring(L, "output/");
  lua_setglobal(L, "OUTPUT_DIR");
}

[[nodiscard]] yabt::runtime::Result<LuaEngine, std::string>
LuaEngine::construct() noexcept {
  LuaEngine engine;

  engine.m_state = luaL_newstate();
  if (engine.m_state == nullptr) {
    return runtime::Result<LuaEngine, std::string>::error(
        "Unable to create lua state");
  }

  luaL_openlibs(engine.m_state);
  luaopen_yabt(engine.m_state);

  return runtime::Result<LuaEngine, std::string>::ok(std::move(engine));
}

LuaEngine::LuaEngine(LuaEngine &&other) noexcept {
  m_state = other.m_state;
  other.m_state = nullptr;
}

LuaEngine &LuaEngine::operator=(LuaEngine &&other) noexcept {
  if (this != &other) {
    if (m_state)
      lua_close(m_state);
    m_state = other.m_state;
    other.m_state = nullptr;
  }
  return *this;
}

LuaEngine::~LuaEngine() noexcept {
  if (m_state) {
    lua_close(m_state);
  }
}

runtime::Result<void, std::string>
LuaEngine::exec_file(std::string_view file_path) noexcept {
  const int result = luaL_dofile(m_state, std::string{file_path}.c_str());
  if (result != LUA_OK) {
    const char *str = luaL_checklstring(m_state, 1, nullptr);
    return runtime::Result<void, std::string>::error(
        std::format("failed to run file: {}", str));
  }

  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] std::span<const ninja::BuildStep>
LuaEngine::build_steps() const noexcept {
  return yabt::lua::build_steps;
}

[[nodiscard]] std::span<const ninja::BuildStepWithRule>
LuaEngine::build_steps_with_rule() const noexcept {
  return yabt::lua::build_steps_with_rule;
}

[[nodiscard]] const std::map<std::string, ninja::BuildRule> &
LuaEngine::build_rules() const noexcept {
  return yabt::lua::build_rules;
}

} // namespace yabt::lua
