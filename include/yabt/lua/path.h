#pragma once

#include <string>

namespace yabt::lua {

struct Path {
  std::string path;
};

struct OutPath {
  std::string path;
};

[[nodiscard]] inline bool operator==(const OutPath &lhs,
                                     const OutPath &rhs) noexcept {
  return lhs.path == rhs.path;
}

[[nodiscard]] inline bool operator!=(const OutPath &lhs,
                                     const OutPath &rhs) noexcept {
  return !(lhs.path == rhs.path);
}

[[nodiscard]] inline bool operator==(const Path &lhs,
                                     const Path &rhs) noexcept {
  return lhs.path == rhs.path;
}

[[nodiscard]] inline bool operator!=(const Path &lhs,
                                     const Path &rhs) noexcept {
  return !(lhs.path == rhs.path);
}

} // namespace yabt::lua
