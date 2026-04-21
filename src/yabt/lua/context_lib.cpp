#include "yabt/lua/context_lib.h"

#include <cstring>

#include "yabt/log/log.h"
#include "yabt/lua/utils.h"

namespace yabt::lua {

namespace {

int registry_key;

void init_registry(lua_State *const L, ContextLib *data) {
  if (L == nullptr) {
    return;
  }
  StackGuard g{L};
  lua_pushlightuserdata(L, &registry_key);
  lua_pushlightuserdata(L, data);
  lua_settable(L, LUA_REGISTRYINDEX);
}

ContextLib *get_lib_from_registry(lua_State *const L) {
  lua_pushlightuserdata(L, &registry_key);
  lua_gettable(L, LUA_REGISTRYINDEX);
  ContextLib *lib = static_cast<ContextLib *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return lib;
}

runtime::Result<void, std::string> add_build_step_impl(ContextLib &lib) {
  if (lua_gettop(lib.state) != 1) {
    return runtime::Result<void, std::string>::error(
        std::format("Expected 1 argument to add_build_step, but got: {}",
                    lua_gettop(lib.state)));
  }

  const ninja::BuildStep step =
      RESULT_PROPAGATE(parse_lua_object<ninja::BuildStep>(lib.state));
  if (step.outs.size() == 0) {
    return runtime::Result<void, std::string>::error(
        std::format("build step does not contain any outputs"));
  }
  if (const auto it = lib.build_step_map.find(step.outs.front().path);
      it != lib.build_step_map.end()) {
    // Check duplicate
    const ninja::BuildStep &other = lib.build_steps[it->second];
    if (other != step) {
      return runtime::Result<void, std::string>::error(std::format(
          "Attempted to register conflicting build step for out: {}",
          step.outs.front().path));
    }
  } else {
    lib.build_steps.push_back(step);
    lib.build_step_map.insert(
        std::pair{step.outs.front().path, lib.build_steps.size() - 1});
    yabt_verbose("Registered build step for: {} with cmd: {}",
                 step.outs[0].path, step.cmd);
  }

  for (const OutPath &out : step.outs) {
    lib.leaf_paths.insert(out.path);
  }
  for (const Path &in : step.ins) {
    lib.leaf_paths.erase(in.path);
  }

  lua_pop(lib.state, 1);
  return runtime::Result<void, std::string>::ok();
}

void add_build_step(ContextLib &lib) {
  {
    const runtime::Result result = add_build_step_impl(lib);
    if (result.is_ok()) {
      return;
    }

    lua_pushstring(lib.state, result.error_value().c_str());
  }

  // This call does longjmp, which breaks destructors of data types, since they
  // do not get executed. That's why the data above is in a different block
  lua_error(lib.state);
}

int l_add_build_step(lua_State *const L) {
  StackGuard g{L, -1}; // 1 input arg, 0 outputs
  ContextLib *const lib = get_lib_from_registry(L);
  runtime::check(lib != nullptr, "Context lib is NULL");
  add_build_step(*lib);
  return 0;
}

runtime::Result<void, std::string>
add_build_step_with_rule_impl(ContextLib &lib) {
  if (lua_gettop(lib.state) != 2) {
    return runtime::Result<void, std::string>::error(std::format(
        "Expected 2 arguments to add_build_step_with_rule, but got: {}",
        lua_gettop(lib.state)));
  }

  const ninja::BuildRule rule =
      RESULT_PROPAGATE(parse_lua_object<ninja::BuildRule>(lib.state));
  if (lib.build_rules.find(rule.name) == lib.build_rules.cend()) {
    lib.build_rules.insert(std::pair{rule.name, rule});
    yabt_verbose("Registered build rule: {}", rule.name);
  }

  lua_pop(lib.state, 1);

  const ninja::BuildStepWithRule step =
      RESULT_PROPAGATE(parse_lua_object<ninja::BuildStepWithRule>(lib.state));
  if (step.outs.size() == 0) {
    return runtime::Result<void, std::string>::error(
        "Attempted to register build_steps_with_rule without an out");
  }

  if (const auto it = lib.build_step_with_rule_map.find(step.outs.front().path);
      it != lib.build_step_with_rule_map.end()) {
    // Check duplicate
    const ninja::BuildStepWithRule &other =
        lib.build_steps_with_rule[it->second];
    if (other != step) {
      return runtime::Result<void, std::string>::error(std::format(
          "Attempted to register conflicting build step with rule for out: {}",
          step.outs.front().path));
    }
  } else {
    lib.build_steps_with_rule.push_back(step);
    lib.build_step_with_rule_map.insert(std::pair{
        step.outs.front().path, lib.build_steps_with_rule.size() - 1});
    yabt_verbose("Registered build step for: {} with rule: {}",
                 step.outs[0].path, step.rule_name);
  }

  for (const OutPath &out : step.outs) {
    lib.leaf_paths.insert(out.path);
  }
  for (const Path &in : step.ins) {
    lib.leaf_paths.erase(in.path);
  }

  lua_pop(lib.state, 1);

  return runtime::Result<void, std::string>::ok();
}

void add_build_step_with_rule(ContextLib &lib) {
  {
    const runtime::Result result = add_build_step_with_rule_impl(lib);
    if (result.is_ok()) {
      return;
    }

    lua_pushstring(lib.state, result.error_value().c_str());
  }

  // This call does longjmp, which breaks destructors of data types, since they
  // do not get executed. That's why the data above is in a different block
  lua_error(lib.state);
}

int l_add_build_step_with_rule(lua_State *const L) {
  StackGuard g{L, -2}; // 2 input args, 0 outputs
  ContextLib *const lib = get_lib_from_registry(L);
  runtime::check(lib != nullptr, "Context lib is NULL");
  add_build_step_with_rule(*lib);
  return 0;
}

int handle_target(ContextLib &lib) {
  if (lua_gettop(lib.state) != 3) {
    lua_pushstring(
        lib.state,
        std::format("Unexpected number of arguments for handle_target: {}",
                    lua_gettop(lib.state))
            .c_str());
    lua_error(lib.state);
  }

  if (!lua_isstring(lib.state, 1) || !lua_isstring(lib.state, 2)) {
    lua_pushstring(
        lib.state,
        std::format("handle_target expects 2 strings, but got {} and {}",
                    lua_typename(lib.state, lua_type(lib.state, 1)),
                    lua_typename(lib.state, lua_type(lib.state, 2)))
            .c_str());
    lua_error(lib.state);
  }

  const char *target_spec_path = lua_tolstring(lib.state, 1, nullptr);
  const char *target_name = lua_tolstring(lib.state, 2, nullptr);
  lib.current_target = std::format("//{}/{}", target_spec_path, target_name);
  lib.leaf_paths = std::set<std::string>{};

  if (lua_pcall(lib.state, 0, 0, 0) != 0 /* LUA_OK */) {
    lua_remove(lib.state, 1);
    lua_pushboolean(lib.state, false);
    lua_replace(lib.state, -3);
    return 2;
  }

  std::vector<Path> leaves{};
  for (const std::string &leaf : lib.leaf_paths) {
    leaves.push_back(Path{leaf});
  }

  // We only expose public targets (start with uppercase)
  const bool public_target =
      strlen(target_name) != 0 && std::isupper(target_name[0]);
  if (public_target) {
    lib.build_steps_with_rule.push_back({
        .outs = std::vector{OutPath{lib.current_target}},
        .ins = leaves,
        .rule_name = "phony",
        .variables{},
    });
    lib.all_targets.push_back(lib.current_target);
  }

  lib.current_target = "";

  lua_pop(lib.state, 2);
  lua_pushboolean(lib.state, true);
  lua_pushnil(lib.state); // Just to keep the return values balanced
  return 2;
}

int l_handle_target(lua_State *const L) {
  StackGuard g{L, -1}; // 3 input args, 2 output args
  ContextLib *const lib = get_lib_from_registry(L);
  runtime::check(lib != nullptr, "Context lib is NULL");
  return handle_target(*lib);
}

int l_register_run_fn(lua_State *const L) {
  StackGuard g{L, -1}; // 1 input arg, 0 outputs (luaL_ref pops the value)
  ContextLib *const lib = get_lib_from_registry(L);
  runtime::check(lib != nullptr, "Context lib is NULL");
  if (lua_gettop(L) != 1 || !lua_isfunction(L, 1)) {
    lua_pushstring(L, "register_run_fn expects a single function argument");
    lua_error(L);
  }
  const int ref = luaL_ref(L, LUA_REGISTRYINDEX); // pops the function
  runtime::check(lib->current_target.size() > 0,
                 "No current target while registering a run function");
  lib->run_fn_refs[lib->current_target] = ref;
  return 0;
}

int l_register_test_fn(lua_State *const L) {
  StackGuard g{L, -1}; // 1 input arg, 0 outputs (luaL_ref pops the value)
  ContextLib *const lib = get_lib_from_registry(L);
  runtime::check(lib != nullptr, "Context lib is NULL");
  if (lua_gettop(L) != 1 || !lua_isfunction(L, 1)) {
    lua_pushstring(L, "register_test_fn expects a single function argument");
    lua_error(L);
  }
  const int ref = luaL_ref(L, LUA_REGISTRYINDEX); // pops the function
  runtime::check(lib->current_target.size() > 0,
                 "No current target while registering a test function");
  lib->test_fn_refs[lib->current_target] = ref;
  return 0;
}

static const luaL_Reg context_functions[]{
    {"add_build_step", l_add_build_step},                     //
    {"add_build_step_with_rule", l_add_build_step_with_rule}, //
    {"handle_target", l_handle_target},                       //
    {"register_run_fn", l_register_run_fn},                   //
    {"register_test_fn", l_register_test_fn},                 //
    {nullptr, nullptr},                                       //
};

static constexpr const char CONTEXT_PACKAGE_NAME[] = "yabt.core.context";

runtime::Result<std::vector<std::string>, std::string>
call_fn_impl(lua_State *const L, const std::map<std::string, int> &fn_refs,
             const std::string_view method_name, const std::string &target,
             std::span<const std::string_view> args) {
  const auto fn_ref_iter = fn_refs.find(target);
  if (fn_ref_iter == fn_refs.cend()) {
    return runtime::Result<std::vector<std::string>, std::string>::error(
        std::format("Target {} has no {}() method", target, method_name));
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, fn_ref_iter->second);

  // Setup a table to pass the args to the function
  lua_newtable(L);
  for (size_t i = 0; i < args.size(); i++) {
    lua_pushstring(L, std::string{args[i]}.c_str());
    lua_rawseti(L, -2, static_cast<int>(i) + 1);
  }

  if (lua_pcall(L, 1, 1, 0) != 0) {
    const std::string err{lua_tostring(L, -1)};
    lua_pop(L, 1);
    return runtime::Result<std::vector<std::string>, std::string>::error(
        std::format("{}() for target {} failed: {}", method_name, target, err));
  }

  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return runtime::Result<std::vector<std::string>, std::string>::error(
        std::format("{}() for target {} must return a table", method_name,
                    target));
  }

  auto result = RESULT_PROPAGATE(parse_lua_object<std::vector<std::string>>(L));
  lua_pop(L, 1);

  if (result.empty()) {
    return runtime::Result<std::vector<std::string>, std::string>::error(
        std::format("{}() for target {} returned empty table", method_name,
                    target));
  }

  return runtime::Result<std::vector<std::string>, std::string>::ok(
      std::move(result));
}

} // namespace

ContextLib::ContextLib(ContextLib &&other)
    : build_steps{std::move(other.build_steps)},
      build_steps_with_rule{std::move(other.build_steps_with_rule)},
      build_rules{std::move(other.build_rules)},
      all_targets{std::move(other.all_targets)},
      run_fn_refs{std::move(other.run_fn_refs)},
      test_fn_refs{std::move(other.test_fn_refs)},

      state{other.state}, current_target{std::move(other.current_target)},
      leaf_paths{std::move(other.leaf_paths)} {
  init_registry(state, this);
}

ContextLib &ContextLib::operator=(ContextLib &&other) {
  if (this != &other) {
    build_steps = std::move(other.build_steps);
    build_steps_with_rule = std::move(other.build_steps_with_rule);
    build_rules = std::move(other.build_rules);
    all_targets = std::move(other.all_targets);
    run_fn_refs = std::move(other.run_fn_refs);
    test_fn_refs = std::move(other.test_fn_refs);

    state = {other.state};
    current_target = std::move(other.current_target);
    leaf_paths = std::move(other.leaf_paths);
    init_registry(state, this);
  }
  return *this;
}

void ContextLib::register_in_engine(lua_State *const L) {
  state = L;
  StackGuard g{L};
  luaL_register(L, CONTEXT_PACKAGE_NAME, context_functions);
  lua_pop(L, 1);
  init_registry(L, this);
}

runtime::Result<std::vector<std::string>, std::string>
ContextLib::call_run_fn(const std::string &target,
                        std::span<const std::string_view> args) {
  return call_fn_impl(state, run_fn_refs, "run", target, args);
}

runtime::Result<std::vector<std::string>, std::string>
ContextLib::call_test_fn(const std::string &target,
                         std::span<const std::string_view> args) {
  return call_fn_impl(state, test_fn_refs, "test", target, args);
}

} // namespace yabt::lua
