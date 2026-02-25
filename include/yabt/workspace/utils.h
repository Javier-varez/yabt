#pragma once

#include <filesystem>
#include <optional>

#include "yabt/runtime/result.h"

namespace yabt::workspace {

constexpr static std::string_view DEPS_DIR_NAME = "DEPS";

enum class SyncMode {
  NORMAL,
  STRICT,
};

[[nodiscard]] std::optional<std::filesystem::path>
get_workspace_root() noexcept;

[[nodiscard]] runtime::Result<void, std::string>
    sync_workspace(SyncMode) noexcept;

} // namespace yabt::workspace
