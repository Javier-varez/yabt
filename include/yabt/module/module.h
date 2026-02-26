#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "yabt/runtime/result.h"

namespace yabt::module {

constexpr static std::string_view MODULE_FILE_NAME = "MODULE.lua";
constexpr static std::string_view RULES_DIR_NAME = "rules";
constexpr static std::string_view SRC_DIR_NAME = "src";

class Module {
public:
  [[nodiscard]] virtual std::string name() const noexcept = 0;

  [[nodiscard]] virtual std::filesystem::path disk_path() const noexcept = 0;

  [[nodiscard]] virtual runtime::Result<std::string, std::string>
  head() const noexcept = 0;

  [[nodiscard]] virtual runtime::Result<void, std::string>
  fetch() const noexcept = 0;

  [[nodiscard]] virtual runtime::Result<bool, std::string>
  is_ancestor(std::string_view ancestor,
              std::string_view revision) const noexcept = 0;

  [[nodiscard]] virtual runtime::Result<void, std::string>
  checkout(std::string_view revision) const noexcept = 0;

  [[nodiscard]] std::optional<std::filesystem::path> rules_dir() const noexcept;

  virtual ~Module() = default;
};

[[nodiscard]] runtime::Result<std::unique_ptr<Module>, std::string>
open_or_fetch_module(const std::filesystem::path &mod_dir,
                     std::string_view mod_url, std::string_view mod_type,
                     std::string_view mod_hash);

[[nodiscard]] runtime::Result<std::unique_ptr<Module>, std::string>
open_module(const std::filesystem::path &mod_dir);

} // namespace yabt::module
