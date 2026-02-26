#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "yabt/module/module.h"
#include "yabt/runtime/result.h"

namespace yabt::workspace {

constexpr static std::string_view BUILD_DIR_NAME = "BUILD";
constexpr static std::string_view NINJA_FILE_PATH = "BUILD/build.ninja";
constexpr static std::string_view DEPS_DIR_NAME = "DEPS";

enum class SyncMode {
  NORMAL,
  STRICT,
};

[[nodiscard]] std::optional<std::filesystem::path>
get_workspace_root() noexcept;

[[nodiscard]] runtime::Result<void, std::string>
sync_workspace(std::filesystem::path ws_root, SyncMode) noexcept;

runtime::Result<std::vector<std::unique_ptr<module::Module>>, std::string>
open_workspace(const std::filesystem::path &ws_root) noexcept;

} // namespace yabt::workspace
