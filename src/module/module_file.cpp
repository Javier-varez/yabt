#include "yabt/module/module_file.h"
#include "yabt/lua/utils.h"
#include "yabt/utils/defer.h"

namespace yabt::lua {

LUA_STRUCT_PARSE_SPEC_DEF(                //
    ::yabt::module::DependencyDefinition, //
    (std::string, url),                   //
    (std::string, version),               //
    (std::string, sha256),                //
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
    return runtime::Result<ModuleFile, std::string>::error(
        std::format("Error loading module file: {}", path.string()));
  }

  const ModuleFile mod = RESULT_PROPAGATE(lua::parse_lua_object<ModuleFile>(L));
  return runtime::Result<ModuleFile, std::string>::ok(mod);
}

} // namespace yabt::module
