#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "yabt/lua/module.h"

#include "lua.hpp"

namespace yabt::lua {

// Native utilities to deal with filesystem paths.
struct PathLib : public LuaModule {
  PathLib(std::filesystem::path source_dir, std::filesystem::path output_dir,
          std::map<std::string, std::filesystem::path> module_paths) noexcept;

  void register_in_engine(lua_State *const L) noexcept final;

  PathLib(const PathLib &) noexcept = delete;
  PathLib &operator=(const PathLib &) noexcept = delete;

  PathLib(PathLib &&) noexcept;
  PathLib &operator=(PathLib &&) noexcept;

public:
  std::filesystem::path source_dir;
  std::filesystem::path output_dir;
  std::map<std::string, std::filesystem::path> module_paths;

private:
  lua_State *state;
};

} // namespace yabt::lua
