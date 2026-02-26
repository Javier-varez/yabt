#include "yabt/build/build.h"
#include "yabt/log/log.h"

namespace yabt::build {

runtime::Result<lua::LuaEngine, std::string> prepare_lua_engine(
    const std::filesystem::path &ws_root,
    std::span<const std::unique_ptr<module::Module>> modules) noexcept {
  lua::LuaEngine engine = RESULT_PROPAGATE(lua::LuaEngine::construct(ws_root));

  std::vector<std::string> paths;
  for (const auto &mod : modules) {
    const std::optional rules_path = mod->rules_dir();
    if (rules_path.has_value()) {
      const std::filesystem::path p = rules_path.value() / "?.lua";
      yabt_debug("Adding rules dir to path: {}", p.native());
      paths.push_back(std::move(p.native()));
    }
  }

  yabt_debug("Setting package.path");
  engine.set_path(paths);

  return runtime::Result<lua::LuaEngine, std::string>::ok(std::move(engine));
}

[[nodiscard]] runtime::Result<void, std::string> invoke_rule_initializers(
    lua::LuaEngine &engine,
    std::span<const std::unique_ptr<module::Module>> modules) noexcept {

  for (const auto &mod : modules) {
    yabt_debug("Handling module: {}", mod->name());
    log::IndentGuard guard{};

    std::optional<std::filesystem::path> rules_dir = mod->rules_dir();
    if (!rules_dir.has_value()) {
      continue;
    }

    std::filesystem::recursive_directory_iterator dir_iter{rules_dir.value()};
    for (const std::filesystem::directory_entry &entry : dir_iter) {
      if (entry.is_regular_file() &&
          entry.path().filename() == INIT_FILE_NAME) {
        yabt_debug("Executing init file: {}", entry.path().native());
        RESULT_PROPAGATE_DISCARD(engine.exec_file(entry.path().native()));
      }
    }
  }

  return engine.exec_file("./runtime.lua");
}

[[nodiscard]] runtime::Result<void, std::string> invoke_build_targets(
    lua::LuaEngine &engine,
    std::span<const std::unique_ptr<module::Module>> modules) noexcept {

  for (const auto &mod : modules) {
    yabt_debug("Handling module: {}", mod->name());
    log::IndentGuard guard{};

    std::filesystem::path mod_dir = mod->disk_path();
    std::vector<std::string> build_files;

    const std::filesystem::path src_dir = mod_dir / module::SRC_DIR_NAME;
    if (!std::filesystem::exists(src_dir)) {
      yabt_debug("Skipping source dir, as it does not exist");
      continue;
    }

    std::filesystem::recursive_directory_iterator dir_iter{src_dir};
    for (const std::filesystem::directory_entry &entry : dir_iter) {
      if (entry.is_regular_file() &&
          entry.path().filename() == BUILD_FILE_NAME) {
        const std::filesystem::path build_file =
            std::filesystem::relative(entry.path().parent_path(), src_dir);
        yabt_debug("Found build file: {}", build_file.native());
        build_files.push_back(build_file);
      }
    }

    // Register modules
    RESULT_PROPAGATE_DISCARD(
        engine.register_module(mod->name(), mod_dir, build_files));
  }

  // FIXME: hardcoding the path here will not work out of this repo. Embed the
  // file in the binary.
  return engine.exec_file("./runtime.lua");
}

} // namespace yabt::build
