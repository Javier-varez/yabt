#pragma once

#include <filesystem>

#include "lua.hpp"

namespace yabt::lua {

// Native utilities to deal with filesystem paths.
struct PathLib {
  PathLib(lua_State *const L, std::filesystem::path source_dir,
          std::filesystem::path output_dir);

  PathLib(const PathLib &) noexcept = delete;
  PathLib &operator=(const PathLib &) noexcept = delete;

  PathLib(PathLib &&) noexcept;
  PathLib &operator=(PathLib &&) noexcept;

  lua_State *m_state;
  std::filesystem::path m_source_dir;
  std::filesystem::path m_output_dir;
};

} // namespace yabt::lua
