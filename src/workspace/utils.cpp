#include <deque>

#include <algorithm>
#include <filesystem>
#include <unistd.h>

#include "yabt/log/log.h"
#include "yabt/module/module.h"
#include "yabt/module/module_file.h"
#include "yabt/workspace/utils.h"

namespace yabt::workspace {

[[nodiscard]] std::optional<std::filesystem::path>
get_workspace_root() noexcept {
  std::filesystem::path cur_dir = std::filesystem::current_path();

  while (true) {
    std::filesystem::directory_iterator dir_iter{cur_dir};
    const auto iter = std::find_if(
        begin(dir_iter), end(dir_iter),
        [](const std::filesystem::directory_entry &entry) {
          //
          return entry.is_regular_file() &&
                 entry.path().filename() == module::MODULE_FILE_NAME;
        });
    if (iter != end(dir_iter)) {
      return cur_dir;
    }

    if (cur_dir == cur_dir.root_directory()) {
      return std::nullopt;
    }
    cur_dir = cur_dir.parent_path();
  }
}

runtime::Result<void, std::string>
sync_workspace(const SyncMode sync_mode) noexcept {
  const std::optional<std::filesystem::path> ws_root = get_workspace_root();
  if (!ws_root.has_value()) {
    return runtime::Result<void, std::string>::error(
        std::format("Could not find workspace root. Are you sure your "
                    "directory tree contains a {} file?",
                    module::MODULE_FILE_NAME));
  }

  const std::filesystem::path deps_dir = ws_root.value() / DEPS_DIR_NAME;

  struct PinnedMod {
    std::string hash;
  };
  std::map<std::string, PinnedMod> pinned_modules;

  auto root_module = RESULT_PROPAGATE(module::open_module(ws_root.value()));
  std::vector<std::unique_ptr<module::Module>> all_modules{};
  all_modules.push_back(std::move(root_module));
  yabt_verbose("Found workspace root at {}", ws_root.value().native());

  std::deque<std::filesystem::path> queue{ws_root.value()};
  while (queue.size() != 0) {
    log::IndentGuard _indent_guard{};

    const std::filesystem::path current_module_dir = queue.front();
    queue.pop_front();

    const std::filesystem::path modfile_path =
        current_module_dir / module::MODULE_FILE_NAME;

    yabt_debug("Reading module file at {}", modfile_path.c_str());
    const module::ModuleFile modfile =
        RESULT_PROPAGATE(module::ModuleFile::load_module_file(modfile_path));

    yabt_debug("Processing dependencies of {}", modfile_path.c_str());
    log::IndentGuard _indent_guard2{};

    for (const auto &[dep_name, dep] : modfile.deps) {
      yabt_debug("Processing {}", dep_name);
      log::IndentGuard _indent_guard{};

      const std::filesystem::path dep_dir = deps_dir / dep_name;
      auto module = RESULT_PROPAGATE(
          module::open_or_fetch_module(dep_dir, dep.url, dep.type, dep.hash));

      if ((dep.hash == "") && (sync_mode == SyncMode::STRICT)) {
        return runtime::Result<void, std::string>::error(std::format(
            "Dependency {} of {} is not pinned. Refusing to sync in strict "
            "mode.",
            dep_name, current_module_dir.filename().native()));
      }

      if (pinned_modules.contains(dep_name)) {
        const PinnedMod &pinned = pinned_modules[dep_name];
        if (pinned.hash != dep.hash) {
          return runtime::Result<void, std::string>::error(std::format(
              "Dependency {} requires hash {}, but it has been pinned to {}",
              dep_name, dep.hash, pinned.hash));
        }
        continue;
      }

      // Check that the given hash is an ancestor of the version string
      // (branch/tag)
      RESULT_PROPAGATE_DISCARD(module->fetch());

      if (dep.hash != "") {
        const auto ancestor_result = module->is_ancestor(dep.hash, dep.version);
        if (ancestor_result.is_error() || !ancestor_result.ok_value()) {
          return runtime::Result<void, std::string>::error(std::format(
              "Dependency {} has a hash: {} that is not an ancestor "
              "of its version: {}",
              dep_name, dep.hash, dep.version));
        }
      }

      auto target_revision = dep.hash;
      if (dep.hash == "") {
        target_revision = dep.version;
      }

      // Checkout the right version of the dependency, if not current
      if (const std::string head = RESULT_PROPAGATE(module->head());
          head != target_revision) {
        yabt_debug("Current head for {} is at {}. Checking out {}", dep_name,
                   head, target_revision);
        RESULT_PROPAGATE_DISCARD(module->checkout(target_revision));
        target_revision = RESULT_PROPAGATE(module->head());
      }

      pinned_modules.insert(
          std::pair{dep_name, PinnedMod{.hash{target_revision}}});
      yabt_debug("Pinning dependency {} to {}", dep_name, target_revision);
      yabt_debug("");

      all_modules.push_back(std::move(module));
      queue.push_back(dep_dir);
    }
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::workspace
