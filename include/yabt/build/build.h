#pragma once

#include <memory>
#include <span>

#include "yabt/lua/lua_engine.h"
#include "yabt/module/module.h"
#include "yabt/runtime/result.h"

namespace yabt::build {

constexpr static std::string_view BUILD_FILE_NAME = "BUILD.lua";
constexpr static std::string_view INIT_FILE_NAME = "INIT.lua";

[[nodiscard]] runtime::Result<lua::LuaEngine, std::string>
prepare_lua_engine(const std::filesystem::path &ws_root,
                   const std::filesystem::path &build_dir,
                   std::span<const std::unique_ptr<module::Module>>) noexcept;

[[nodiscard]] runtime::Result<void, std::string> invoke_rule_initializers(
    lua::LuaEngine &engine,
    std::span<const std::unique_ptr<module::Module>> modules) noexcept;

[[nodiscard]] runtime::Result<void, std::string>
invoke_build_targets(lua::LuaEngine &engine,
                     std::span<const std::unique_ptr<module::Module>>) noexcept;

} // namespace yabt::build
