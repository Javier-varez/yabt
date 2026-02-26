#include "yabt/module/module_file.h"
#include "yabt/lua/utils.h"
#include "yabt/utils/defer.h"

namespace yabt::lua {

LUA_STRUCT_PARSE_SPEC_DEF(                //
    ::yabt::module::DependencyDefinition, //
    (std::string, url),                   //
    (std::string, version),               //
    (std::string, hash),                  //
    (std::string, type)                   //
);

LUA_STRUCT_PARSE_SPEC_DEF(                 //
    ::yabt::module::ModuleFile,            //
    (std::string, name),                   //
    (int, version),                        //
    (::yabt::module::DependencyMap, deps), //
    (::yabt::module::FlagMap, flags)       //
);

} // namespace yabt::lua

namespace yabt::module {

namespace {

[[nodiscard]] runtime::Result<void, std::string>
validate_module_file(const ModuleFile &modfile) noexcept {
  if (modfile.name == "") {
    return runtime::Result<void, std::string>::error(
        "Module file has no module name");
  }

  if (modfile.version != 1) {
    return runtime::Result<void, std::string>::error(
        std::format("Unsupported module file version: {}", modfile.version));
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace

[[nodiscard]] runtime::Result<ModuleFile, std::string>
ModuleFile::load_module_file(const std::filesystem::path path) noexcept {
  lua_State *const L = luaL_newstate();
  if (L == nullptr) {
    return runtime::Result<ModuleFile, std::string>::error(
        "Unable to create lua state");
  }
  utils::Defer _deleteL{[L]() { lua_close(L); }};

  // Note that we don't load the libraries intentionally. Here we want a
  // very slim lua runtime, so let's actually restrict it as much as possible.
  // I'm only using lua here because I don't want to add a JSON parser.
  // luaL_openlibs(L);

  const int err = luaL_dofile(L, path.c_str());
  if (err != LUA_OK) {
    const std::string error_msg =
        RESULT_PROPAGATE(lua::parse_lua_object<std::string>(L));
    return runtime::Result<ModuleFile, std::string>::error(
        std::format("Error loading module file: {}", error_msg));
  }

  const ModuleFile mod = RESULT_PROPAGATE(lua::parse_lua_object<ModuleFile>(L));
  RESULT_PROPAGATE_DISCARD(validate_module_file(mod));
  return runtime::Result<ModuleFile, std::string>::ok(mod);
}

} // namespace yabt::module
