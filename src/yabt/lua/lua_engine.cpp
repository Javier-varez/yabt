#include <format>
#include <map>
#include <string>
#include <vector>

#include "yabt/log/log.h"
#include "yabt/lua/lua_engine.h"
#include "yabt/lua/utils.h"
#include "yabt/ninja/build_rule.h"
#include "yabt/ninja/build_step.h"

#include "lua.hpp"

namespace yabt::lua {

namespace {
int registry_key;

void init_registry(lua_State *const L, LuaEngine *data) {
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

int l_add_build_step(lua_State *const L) noexcept {
  LuaEngine *const engine = get_engine(L);
  engine->add_build_step();
  return 0;
}

int l_add_build_step_with_rule(lua_State *const L) noexcept {
  LuaEngine *const engine = get_engine(L);
  engine->add_build_step_with_rule();
  return 0;
}

int l_do_yabt_preload(lua_State *const L) noexcept {
  LuaEngine *const engine = get_engine(L);
  return engine->do_yabt_preload();
}

namespace {

template <log::LogLevel level> int l_log(lua_State *const L) {
  if (lua_gettop(L) != 1) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, "Unexpected number of arguments given to log function");
    lua_error(L);
  }
  if (!lua_isstring(L, -1)) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, "Invalid argument type given to log function");
    lua_error(L);
  }

  const char *str = lua_tostring(L, -1);
  if constexpr (level == log::LogLevel::VERBOSE) {
    yabt_verbose("{}", str);
  } else if constexpr (level == log::LogLevel::DEBUG) {
    yabt_debug("{}", str);
  } else if constexpr (level == log::LogLevel::INFO) {
    yabt_info("{}", str);
  } else if constexpr (level == log::LogLevel::WARNING) {
    yabt_warn("{}", str);
  } else if constexpr (level == log::LogLevel::ERROR) {
    yabt_error("{}", str);
  }
  lua_pop(L, 1);
  return 0;
}

static const luaL_Reg yabt_methods[]{
    {"add_build_step", l_add_build_step},
    {"add_build_step_with_rule", l_add_build_step_with_rule},
    {"log_verbose", l_log<log::LogLevel::VERBOSE>},
    {"log_debug", l_log<log::LogLevel::DEBUG>},
    {"log_info", l_log<log::LogLevel::INFO>},
    {"log_warn", l_log<log::LogLevel::WARNING>},
    {"log_error", l_log<log::LogLevel::ERROR>},
    {nullptr, nullptr},
};

void luaopen_yabt(lua_State *const L,
                  const std::filesystem::path &workspace_root,
                  const std::filesystem::path &build_dir) noexcept {
  luaL_register(L, "yabt_native", yabt_methods);
  lua_pop(L, 1);

  runtime::check(lua_checkstack(L, 2), "Exceeded maximum Lua stack size");

  lua_pushstring(L, std::filesystem::absolute(workspace_root).c_str());
  lua_setglobal(L, "SOURCE_DIR");

  lua_pushstring(L, std::filesystem::absolute(build_dir).c_str());
  lua_setglobal(L, "OUTPUT_DIR");
}

void init_modules_global(lua_State *const L) noexcept {
  runtime::check(lua_checkstack(L, 1), "Exceeded maximum Lua stack size");
  lua_newtable(L);
  lua_setglobal(L, "modules");
}

} // namespace

[[nodiscard]] yabt::runtime::Result<LuaEngine, std::string>
LuaEngine::construct(const std::filesystem::path &workspace_root,
                     const std::filesystem::path &build_dir) noexcept {
  LuaEngine engine;

  engine.m_state = luaL_newstate();
  engine.m_workspace_root = workspace_root;
  if (engine.m_state == nullptr) {
    return runtime::Result<LuaEngine, std::string>::error(
        "Unable to create lua state");
  }

  luaL_openlibs(engine.m_state);
  luaopen_yabt(engine.m_state, workspace_root, build_dir);

  init_registry(engine.m_state, &engine);
  set_package_path(engine.m_state, "");
  set_package_cpath(engine.m_state, "");

  init_modules_global(engine.m_state);

  return runtime::Result<LuaEngine, std::string>::ok(std::move(engine));
}

LuaEngine::LuaEngine(LuaEngine &&other) noexcept {
  m_state = other.m_state;
  m_workspace_root = std::move(other.m_workspace_root);
  m_preloaded_packages = std::move(other.m_preloaded_packages);
  m_build_steps = std::move(other.m_build_steps);
  m_build_steps_with_rule = std::move(other.m_build_steps_with_rule);
  m_build_rules = std::move(other.m_build_rules);
  other.m_state = nullptr;
  // The object is moved, so we need to update our reference in the lua runtime
  init_registry(m_state, this);
}

LuaEngine &LuaEngine::operator=(LuaEngine &&other) noexcept {
  if (this != &other) {
    if (m_state)
      lua_close(m_state);
    m_state = other.m_state;
    m_workspace_root = std::move(other.m_workspace_root);
    m_preloaded_packages = std::move(other.m_preloaded_packages);
    m_build_steps = std::move(other.m_build_steps);
    m_build_steps_with_rule = std::move(other.m_build_steps_with_rule);
    m_build_rules = std::move(other.m_build_rules);
    other.m_state = nullptr;
    // The object is moved, so we need to update our reference in the lua
    // runtime
    init_registry(m_state, this);
  }
  return *this;
}

LuaEngine::~LuaEngine() noexcept {
  if (m_state) {
    lua_close(m_state);
  }
}

runtime::Result<void, std::string>
LuaEngine::exec_string(const char *string) noexcept {
  const int result = luaL_dostring(m_state, string);
  if (result != 0 /* LUA_OK */) {
    const char *str = luaL_checklstring(m_state, 1, nullptr);
    return runtime::Result<void, std::string>::error(
        std::format("failed to run file: {}", str));
  }

  return runtime::Result<void, std::string>::ok();
}

runtime::Result<void, std::string>
LuaEngine::exec_file(std::string_view file_path) noexcept {
  const int result = luaL_dofile(m_state, std::string{file_path}.c_str());
  if (result != 0 /* LUA_OK */) {
    const char *str = luaL_checklstring(m_state, 1, nullptr);
    return runtime::Result<void, std::string>::error(
        std::format("failed to run file: {}", str));
  }

  return runtime::Result<void, std::string>::ok();
}

void LuaEngine::set_preloaded_lua_packages(
    std::map<std::string, const char *> packages) noexcept {
  m_preloaded_packages = std::move(packages);

  lua_checkstack(m_state, 4);
  lua_getglobal(m_state, "package");
  lua_getfield(m_state, -1, "preload");

  for (const auto &[path, _] : m_preloaded_packages) {
    lua_pushcfunction(m_state, l_do_yabt_preload);
    lua_setfield(m_state, -2, path.c_str());
  }
  lua_pop(m_state, 2);
}

void LuaEngine::set_path(std::span<const std::string> paths) noexcept {
  std::string path{};

  bool needs_delimiter = false;
  for (const std::string &p : paths) {
    if (needs_delimiter)
      path.push_back(';');
    path.append(p);
    needs_delimiter = true;
  }
  set_package_path(m_state, path.c_str());
}

runtime::Result<void, std::string> LuaEngine::add_build_step_impl() noexcept {
  if (lua_gettop(m_state) != 1) {
    return runtime::Result<void, std::string>::error(std::format(
        "Expected 1 argument to add_build_step_with_rule, but got: {}",
        lua_gettop(m_state)));
  }

  const ninja::BuildStep step =
      RESULT_PROPAGATE(parse_lua_object<ninja::BuildStep>(m_state));
  m_build_steps.push_back(step);
  if (step.outs.size() == 0) {
    return runtime::Result<void, std::string>::error(
        "Attempted to register build_steps_with_rule without an out");
  }
  yabt_verbose("Registered build step for: {} with cmd: {}", step.outs[0],
               step.cmd);

  lua_pop(m_state, 1);
  return runtime::Result<void, std::string>::ok();
}

void LuaEngine::add_build_step() noexcept {
  {
    const runtime::Result result = add_build_step_impl();
    if (result.is_ok()) {
      return;
    }

    lua_pushstring(m_state, result.error_value().c_str());
  }

  // This call does longjmp, which breaks destructors of data types, since they
  // do not get executed. That's why the data above is in a different block
  lua_error(m_state);
}

runtime::Result<void, std::string>
LuaEngine::add_build_step_with_rule_impl() noexcept {
  if (lua_gettop(m_state) != 2) {
    return runtime::Result<void, std::string>::error(std::format(
        "Expected 2 arguments to add_build_step_with_rule, but got: {}",
        lua_gettop(m_state)));
  }

  const ninja::BuildRule rule =
      RESULT_PROPAGATE(parse_lua_object<ninja::BuildRule>(m_state));
  if (m_build_rules.find(rule.name) == m_build_rules.cend()) {
    m_build_rules.insert(std::pair{rule.name, rule});
    yabt_verbose("Registered build rule: {}", rule.name);
  }

  lua_pop(m_state, 1);

  const ninja::BuildStepWithRule step =
      RESULT_PROPAGATE(parse_lua_object<ninja::BuildStepWithRule>(m_state));
  if (step.outs.size() == 0) {
    return runtime::Result<void, std::string>::error(
        "Attempted to register build_steps_with_rule without an out");
  }

  m_build_steps_with_rule.push_back(step);
  yabt_verbose("Registered build step for: {} with rule: {}", step.outs[0],
               step.rule_name);

  lua_pop(m_state, 1);

  return runtime::Result<void, std::string>::ok();
}

void LuaEngine::add_build_step_with_rule() noexcept {
  {
    const runtime::Result result = add_build_step_with_rule_impl();
    if (result.is_ok()) {
      return;
    }

    lua_pushstring(m_state, result.error_value().c_str());
  }

  // This call does longjmp, which breaks destructors of data types, since they
  // do not get executed. That's why the data above is in a different block
  lua_error(m_state);
}

int LuaEngine::do_yabt_preload() noexcept {
  const char *req = lua_tostring(m_state, 1);
  yabt_verbose("do_yabt_preload: Loading file: {}", req);

  if (!m_preloaded_packages.contains(req)) {
    yabt_warn("do_yabt_preload: {} Not found", req);
    return 0;
  }
  const std::string &lua_src = m_preloaded_packages[req];
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
LuaEngine::register_module(const std::string &name,
                           const std::filesystem::path &path,
                           std::span<const std::string> target_specs) noexcept {
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

[[nodiscard]] std::span<const ninja::BuildStep>
LuaEngine::build_steps() const noexcept {
  return m_build_steps;
}

[[nodiscard]] std::span<const ninja::BuildStepWithRule>
LuaEngine::build_steps_with_rule() const noexcept {
  return m_build_steps_with_rule;
}

[[nodiscard]] const std::map<std::string, ninja::BuildRule> &
LuaEngine::build_rules() const noexcept {
  return m_build_rules;
}

} // namespace yabt::lua
