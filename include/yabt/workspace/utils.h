#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "yabt/module/module.h"
#include "yabt/module/module_file.h"
#include "yabt/runtime/result.h"

namespace yabt::workspace {

constexpr static std::string_view BUILD_DIR_NAME = "BUILD";
constexpr static std::string_view NINJA_FILE_PATH = "build.ninja";
constexpr static std::string_view DEPS_DIR_NAME = "DEPS";
constexpr static std::string_view COMPDB_NAME = "compile_commands.json";

enum class SyncMode {
  NORMAL,
  STRICT,
};

[[nodiscard]] std::optional<std::filesystem::path>
get_workspace_root() noexcept;

[[nodiscard]] runtime::Result<void>
sync_workspace(std::filesystem::path ws_root, SyncMode) noexcept;

runtime::Result<std::vector<std::unique_ptr<module::Module>>>
open_workspace(const std::filesystem::path &ws_root) noexcept;

[[nodiscard]] runtime::Result<module::ModuleFile>
load_root_module_file(const std::filesystem::path &ws_root) noexcept;

} // namespace yabt::workspace
