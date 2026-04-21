#include <cstring>
#include <format>
#include <map>
#include <string>

#include "yabt/log/log.h"
#include "yabt/lua/lua_engine.h"
#include "yabt/lua/utils.h"

#include "lua.hpp"

namespace yabt::lua {

namespace {

int registry_key;

void init_registry(lua_State *const L, LuaEngine *data) {
  StackGuard g{L};
  lua_pushlightuserdata(L, &registry_key);
  lua_pushlightuserdata(L, data);
  lua_settable(L, LUA_REGISTRYINDEX);
}

LuaEngine *get_engine(lua_State *const L) {
  lua_pushlightuserdata(L, &registry_key);
  lua_gettable(L, LUA_REGISTRYINDEX);
  LuaEngine *engine = static_cast<LuaEngine *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return engine;
}

} // namespace

int l_do_yabt_preload(lua_State *const L) {
  StackGuard g{L};
  LuaEngine *const engine = get_engine(L);
  return engine->do_yabt_preload();
}

namespace {

void init_modules_global(lua_State *const L) {
  runtime::check(lua_checkstack(L, 1), "Exceeded maximum Lua stack size");
  lua_newtable(L);
  lua_setglobal(L, "modules");
}

} // namespace

[[nodiscard]] yabt::runtime::Result<LuaEngine, std::string>
LuaEngine::construct(const std::filesystem::path &workspace_root) {
  LuaEngine engine;

  engine.m_state = luaL_newstate();
  StackGuard g{engine.m_state};

  engine.m_workspace_root = workspace_root;
  if (engine.m_state == nullptr) {
    return runtime::Result<LuaEngine, std::string>::error(
        "Unable to create lua state");
  }

  luaL_openlibs(engine.m_state);

  init_registry(engine.m_state, &engine);
  set_package_path(engine.m_state, "");
  set_package_cpath(engine.m_state, "");

  init_modules_global(engine.m_state);

  return runtime::Result<LuaEngine, std::string>::ok(std::move(engine));
}

void LuaEngine::register_lua_module(LuaModule &module) {
  module.register_in_engine(m_state);
}

LuaEngine::LuaEngine(LuaEngine &&other) {
  m_state = other.m_state;
  m_workspace_root = std::move(other.m_workspace_root);
  m_preloaded_packages = std::move(other.m_preloaded_packages);
  other.m_state = nullptr;
  // The object is moved, so we need to update our reference in the lua runtime
  init_registry(m_state, this);
}

LuaEngine &LuaEngine::operator=(LuaEngine &&other) {
  if (this != &other) {
    if (m_state)
      lua_close(m_state);
    m_state = other.m_state;
    m_workspace_root = std::move(other.m_workspace_root);
    m_preloaded_packages = std::move(other.m_preloaded_packages);
    other.m_state = nullptr;
    // The object is moved, so we need to update our reference in the lua
    // runtime
    init_registry(m_state, this);
  }
  return *this;
}

LuaEngine::~LuaEngine() {
  if (m_state) {
    lua_close(m_state);
  }
}

runtime::Result<void, std::string> LuaEngine::exec_string(const char *string) {
  const int result = luaL_dostring(m_state, string);
  if (result != 0 /* LUA_OK */) {
    const char *str = luaL_checklstring(m_state, 1, nullptr);
    return runtime::Result<void, std::string>::error(
        std::format("failed to run file: {}", str));
  }

  return runtime::Result<void, std::string>::ok();
}

runtime::Result<void, std::string>
LuaEngine::exec_file(std::string_view file_path) {
  StackGuard g{m_state};
  const int result = luaL_dofile(m_state, std::string{file_path}.c_str());
  if (result != 0 /* LUA_OK */) {
    const char *str = luaL_checklstring(m_state, 1, nullptr);
    return runtime::Result<void, std::string>::error(
        std::format("failed to run file: {}", str));
  }

  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string>
LuaEngine::set_args(std::span<const std::string> args) {
  lua_newtable(m_state);
  for (size_t i = 0; i < args.size(); i++) {
    lua_pushstring(m_state, args[i].c_str());
    lua_rawseti(m_state, -2, i + 1);
  }
  lua_setglobal(m_state, "arg");

  return runtime::Result<void, std::string>::ok();
}

void LuaEngine::set_preloaded_lua_packages(
    std::map<std::string, std::string_view> packages) {
  m_preloaded_packages = std::move(packages);
  StackGuard g{m_state};

  lua_checkstack(m_state, 4);
  lua_getglobal(m_state, "package");
  lua_getfield(m_state, -1, "preload");

  for (const auto &[path, _] : m_preloaded_packages) {
    lua_pushcfunction(m_state, l_do_yabt_preload);
    lua_setfield(m_state, -2, path.c_str());
  }
  lua_pop(m_state, 2);
}

void LuaEngine::set_path(std::span<const std::string> paths) {
  StackGuard g{m_state};
  std::string path{};

  bool needs_delimiter = false;
  for (const std::string &p : paths) {
    if (needs_delimiter)
      path.push_back(';');
    path.append(p);
    needs_delimiter = true;
  }
  yabt_debug("Setting package.path: {}", path);
  set_package_path(m_state, path.c_str());
}

void LuaEngine::set_cpath(std::span<const std::string> paths) {
  StackGuard g{m_state};
  std::string path{};

  bool needs_delimiter = false;
  for (const std::string &p : paths) {
    if (needs_delimiter)
      path.push_back(';');
    path.append(p);
    needs_delimiter = true;
  }
  yabt_debug("Setting package.cpath: {}", path);
  set_package_cpath(m_state, path.c_str());
}

int LuaEngine::do_yabt_preload() {
  const char *req = lua_tostring(m_state, 1);
  yabt_verbose("do_yabt_preload: Loading file: {}", req);

  if (!m_preloaded_packages.contains(req)) {
    yabt_warn("do_yabt_preload: {} Not found", req);
    return 0;
  }
  const std::string lua_src{m_preloaded_packages[req]};
  lua_pop(m_state, 1);

  if (luaL_loadstring(m_state, lua_src.c_str())) {
    lua_pushstring(m_state,
                   std::format("Unable to load file: {}", req).c_str());
    lua_error(m_state);
  }
  lua_call(m_state, 0, LUA_MULTRET);
  const int retvals = lua_gettop(m_state);
  return retvals;
}

[[nodiscard]] runtime::Result<void, std::string>
LuaEngine::register_yabt_module(const std::string &name,
                                const std::filesystem::path &path,
                                std::span<const std::string> target_specs) {
  StackGuard g{m_state};
  runtime::check(lua_checkstack(m_state, 4), "Exceeded maximum Lua stack size");

  lua_getglobal(m_state, "modules");
  lua_newtable(m_state);

  // Fill in module table

  // Path
  lua_pushstring(m_state, path.c_str());
  lua_setfield(m_state, -2, "path");

  const std::filesystem::path relative_path =
      std::filesystem::relative(path, m_workspace_root);
  lua_pushstring(m_state, relative_path.c_str());
  lua_setfield(m_state, -2, "relative_path");

  // build files
  lua_newtable(m_state);
  for (size_t i = 0; i < target_specs.size(); i++) {
    lua_pushstring(m_state, target_specs[i].c_str());
    lua_rawseti(m_state, -2, i + 1);
  }
  lua_setfield(m_state, -2, "build_files");

  // Save module
  lua_setfield(m_state, -2, name.c_str());

  lua_pop(m_state, 1); // the modules global
  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::lua
