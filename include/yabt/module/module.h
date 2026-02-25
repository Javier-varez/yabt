#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "yabt/runtime/result.h"

namespace yabt::module {

constexpr static std::string_view MODULE_FILE_NAME = "MODULE.lua";

class Module {
public:
  [[nodiscard]] virtual std::string name() const noexcept = 0;

  [[nodiscard]] virtual runtime::Result<std::string, std::string>
  head() const noexcept = 0;

  [[nodiscard]] virtual runtime::Result<void, std::string>
  fetch() const noexcept = 0;

  [[nodiscard]] virtual runtime::Result<bool, std::string>
  is_ancestor(std::string_view ancestor,
              std::string_view revision) const noexcept = 0;

  [[nodiscard]] virtual runtime::Result<void, std::string>
  checkout(std::string_view revision) const noexcept = 0;

  virtual ~Module() = default;
};

[[nodiscard]] runtime::Result<std::unique_ptr<Module>, std::string>
open_or_fetch_module(const std::filesystem::path &mod_dir,
                     std::string_view mod_url, std::string_view mod_type,
                     std::string_view mod_hash);

[[nodiscard]] runtime::Result<std::unique_ptr<Module>, std::string>
open_module(const std::filesystem::path &mod_dir);

} // namespace yabt::module
