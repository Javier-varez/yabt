#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "yabt/lua/context_lib.h"
#include "yabt/lua/log_lib.h"
#include "yabt/lua/lua_engine.h"
#include "yabt/lua/path_lib.h"
#include "yabt/module/module.h"
#include "yabt/runtime/result.h"

namespace yabt::build {

constexpr static std::string_view BUILD_FILE_NAME = "BUILD.lua";
constexpr static std::string_view INIT_FILE_NAME = "INIT.lua";

enum class PostBuildMode { None, Run, Test };

[[nodiscard]] runtime::Result<void, std::string>
execute_build(int threads, bool compdb,
              const std::optional<std::filesystem::path> &requested_build_dir,
              std::span<const std::string_view> target_patterns,
              PostBuildMode mode = PostBuildMode::None,
              std::span<const std::string_view> action_args = {});

struct LuaModules final {
  lua::PathLib pathlib;
  lua::ContextLib contextlib;
  lua::LogLib loglib;
};

struct LuaPath {
  std::string path;
  std::string cpath;
};

[[nodiscard]] std::unique_ptr<LuaModules> construct_lua_modules(
    const std::filesystem::path &ws_root,
    const std::filesystem::path &build_dir,
    std::span<const std::unique_ptr<module::Module>> yabt_modules) noexcept;

runtime::Result<lua::LuaEngine, std::string> prepare_lua_engine(
    const std::filesystem::path &ws_root, LuaModules &lua_modules,
    const std::span<const std::unique_ptr<module::Module>> yabt_modules,
    const std::span<const LuaPath> extra_paths) noexcept;

[[nodiscard]] runtime::Result<void, std::string> invoke_rule_initializers(
    lua::LuaEngine &engine,
    std::span<const std::unique_ptr<module::Module>> modules) noexcept;

[[nodiscard]] runtime::Result<void, std::string>
invoke_build_targets(lua::LuaEngine &engine,
                     std::span<const std::unique_ptr<module::Module>>) noexcept;

} // namespace yabt::build
