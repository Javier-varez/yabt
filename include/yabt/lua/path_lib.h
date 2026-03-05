#pragma once

#include <filesystem>

#include "yabt/lua/module.h"

#include "lua.hpp"

namespace yabt::lua {

// Native utilities to deal with filesystem paths.
struct PathLib : public LuaModule {
  PathLib(std::filesystem::path source_dir,
          std::filesystem::path output_dir) noexcept;

  void register_in_engine(lua_State *const L) noexcept final;

  PathLib(const PathLib &) noexcept = delete;
  PathLib &operator=(const PathLib &) noexcept = delete;

  PathLib(PathLib &&) noexcept;
  PathLib &operator=(PathLib &&) noexcept;

public:
  std::filesystem::path source_dir;
  std::filesystem::path output_dir;

private:
  lua_State *state;
};

} // namespace yabt::lua
