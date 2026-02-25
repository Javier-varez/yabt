#include "yabt/module/module.h"
#include "yabt/module/git_module.h"
#include "yabt/runtime/result.h"

namespace yabt::module {

[[nodiscard]] runtime::Result<std::unique_ptr<Module>, std::string>
open_module(const std::filesystem::path &mod_dir) {
  if (const std::filesystem::path git_dir = mod_dir / ".git";
      std::filesystem::exists(git_dir)) {
    return GitModule::open(mod_dir);
  }

  return runtime::Result<std::unique_ptr<Module>, std::string>::error(
      std::format("Could not detect module type for: {}", mod_dir.native()));
}

[[nodiscard]] runtime::Result<std::unique_ptr<Module>, std::string>
open_or_fetch_module(const std::filesystem::path &mod_dir,
                     std::string_view mod_url, std::string_view mod_type,
                     std::string_view mod_hash) {

  if (mod_type == "git") {
    return GitModule::open_or_fetch_module(mod_dir, mod_url, mod_type,
                                           mod_hash);
  }

  if (mod_url.ends_with(".git")) {
    return GitModule::open_or_fetch_module(mod_dir, mod_url, mod_type,
                                           mod_hash);
  }

  return runtime::Result<std::unique_ptr<Module>, std::string>::error(
      std::format("Could not detect module type for: {}", mod_url));
}

} // namespace yabt::module
