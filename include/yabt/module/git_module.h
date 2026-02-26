#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include "yabt/module/module.h"
#include "yabt/runtime/result.h"

namespace yabt::module {

class GitModule final : public Module {
public:
  [[nodiscard]] static runtime::Result<std::unique_ptr<Module>, std::string>
  open_or_fetch_module(const std::filesystem::path &mod_dir,
                       std::string_view mod_url, std::string_view mod_type,
                       std::string_view mod_hash) noexcept;

  [[nodiscard]] static runtime::Result<std::unique_ptr<Module>, std::string>
  open(std::filesystem::path mod_dir) noexcept;

  [[nodiscard]] std::string name() const noexcept final;

  [[nodiscard]] std::filesystem::path disk_path() const noexcept final;

  [[nodiscard]] runtime::Result<std::string, std::string>
  head() const noexcept final;

  [[nodiscard]] runtime::Result<void, std::string> fetch() const noexcept final;

  [[nodiscard]] runtime::Result<bool, std::string>
  is_ancestor(std::string_view ancestor,
              std::string_view revision) const noexcept final;

  [[nodiscard]] runtime::Result<void, std::string>
  checkout(std::string_view revision) const noexcept final;

  GitModule(const GitModule &) noexcept = default;
  GitModule(GitModule &&) noexcept = default;
  GitModule &operator=(const GitModule &) noexcept = default;
  GitModule &operator=(GitModule &&) noexcept = default;

  ~GitModule() noexcept final = default;

private:
  GitModule(const std::filesystem::path &) noexcept;

  std::filesystem::path m_path;
};

} // namespace yabt::module
