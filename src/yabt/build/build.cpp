#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <string>

#include "yabt/build/build.h"
#include "yabt/embed/embed.h"
#include "yabt/log/log.h"
#include "yabt/lua/lua_engine.h"
#include "yabt/ninja/ninja.h"
#include "yabt/process/process.h"
#include "yabt/workspace/utils.h"

namespace yabt::build {

namespace {

void register_modules(lua::LuaEngine &engine, LuaModules &modules) noexcept {
  engine.register_lua_module(modules.pathlib);
  engine.register_lua_module(modules.contextlib);
  engine.register_lua_module(modules.loglib);
}

} // namespace

[[nodiscard]] std::unique_ptr<LuaModules>
construct_lua_modules(
    const std::filesystem::path &ws_root,
    const std::filesystem::path &build_dir,
    std::span<const std::unique_ptr<module::Module>> yabt_modules) noexcept {
  std::map<std::string, std::filesystem::path> module_paths;
  for (const auto &mod : yabt_modules) {
    module_paths[mod->name()] =
        std::filesystem::relative(mod->disk_path(), ws_root);
  }
  return std::make_unique<LuaModules>(LuaModules{
      .pathlib{ws_root, build_dir, std::move(module_paths)},
      .contextlib{},
      .loglib{},
  });
}

runtime::Result<lua::LuaEngine, std::string> prepare_lua_engine(
    const std::filesystem::path &ws_root, LuaModules &lua_modules,
    std::span<const std::unique_ptr<module::Module>> yabt_modules) noexcept {
  lua::LuaEngine engine = RESULT_PROPAGATE(lua::LuaEngine::construct(ws_root));
  register_modules(engine, lua_modules);

  std::vector<std::string> paths;
  for (const auto &mod : yabt_modules) {
    const std::optional rules_path = mod->rules_dir();
    if (rules_path.has_value()) {
      const std::filesystem::path p = rules_path.value() / "?.lua";
      yabt_debug("Adding rules dir to path: {}", p.native());
      paths.push_back(std::move(p.native()));
    }
  }

  yabt_debug("Setting package.path");
  engine.set_path(paths);
  yabt_debug("Configuring preloads");
  engine.set_preloaded_lua_packages(embed::get_embedded_lua_rules());
  yabt_debug("Configured preloads");

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

  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<void, std::string> invoke_build_targets(
    lua::LuaEngine &engine,
    std::span<const std::unique_ptr<module::Module>> modules) noexcept {

  for (const auto &mod : modules) {
    yabt_debug("Handling module: {}", mod->name());
    log::IndentGuard guard{};

    std::filesystem::path mod_dir = mod->disk_path();
    std::vector<std::string> target_specs;

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
        target_specs.push_back(build_file);
      }
    }

    // Register modules
    RESULT_PROPAGATE_DISCARD(
        engine.register_yabt_module(mod->name(), mod_dir, target_specs));
  }

  return engine.exec_string(std::string{embed::get_runtime_file()}.c_str());
}

[[nodiscard]] runtime::Result<void, std::string>
execute_build(const int threads, const bool compdb,
              const std::optional<std::filesystem::path> &requested_build_dir,
              const std::span<const std::string_view> target_patterns,
              const PostBuildMode mode,
              const std::span<const std::string_view> action_args) {
  const std::optional<std::filesystem::path> ws_root =
      workspace::get_workspace_root();
  if (!ws_root.has_value()) {
    return runtime::Result<void, std::string>::error(
        std::format("Could not find workspace root. Are you sure your "
                    "directory tree contains a {} file?",
                    module::MODULE_FILE_NAME));
  }

  const std::filesystem::path build_dir =
      std::filesystem::absolute(requested_build_dir.value_or(
          ws_root.value() / workspace::BUILD_DIR_NAME));

  auto modules = RESULT_PROPAGATE(workspace::open_workspace(ws_root.value()));
  auto lua_modules = construct_lua_modules(ws_root.value(), build_dir, modules);
  auto lua_engine = RESULT_PROPAGATE(
      prepare_lua_engine(ws_root.value(), *lua_modules, modules));
  RESULT_PROPAGATE_DISCARD(invoke_rule_initializers(lua_engine, modules));
  RESULT_PROPAGATE_DISCARD(invoke_build_targets(lua_engine, modules));

  const std::filesystem::path ninja_file =
      build_dir / workspace::NINJA_FILE_PATH;

  RESULT_PROPAGATE_DISCARD(
      ninja::save_ninja_file(ninja_file, lua_modules->contextlib.build_rules,
                             lua_modules->contextlib.build_steps,
                             lua_modules->contextlib.build_steps_with_rule));

  if (compdb) {
    std::vector<std::string> compdb_rules;
    for (const auto &[name, rule] : lua_modules->contextlib.build_rules) {
      if (rule.compdb) {
        compdb_rules.push_back(name);
      }
    }
    process::Process ninja{"ninja", "-t", "compdb",
                           std::span<const std::string>{compdb_rules}};
    ninja.set_cwd((build_dir).native());
    RESULT_PROPAGATE_DISCARD(ninja.start(true));
    const process::Process::ProcessOutput output = ninja.process_output();
    RESULT_PROPAGATE_DISCARD(output.to_result());

    const std::filesystem::path compdb_path =
        build_dir / workspace::COMPDB_NAME;

    std::ofstream compdb_stream;
    compdb_stream.open(compdb_path.native());
    compdb_stream << output.stdout.value();
    compdb_stream.close();
  }

  // Find all matching targets for a pattern
  std::vector<std::regex> patterns;
  for (const std::string_view target_pattern : target_patterns) {
    patterns.emplace_back(std::string{target_pattern});
  }

  std::vector<std::string> targets{};
  for (const std::string &target : lua_modules->contextlib.all_targets) {
    for (const std::regex &regex : patterns) {
      if (std::regex_match(target, regex)) {
        targets.push_back(target);
        break; // don't add a target more than once
      }
    }
  }

  if (targets.size() == 0) {
    yabt_warn("No matched targets");
    return runtime::Result<void, std::string>::ok();
  }

  // Run build process
  yabt_verbose("Executing build process");
  const std::string threads_str = std::format("{}", threads);
  process::Process ninja{"ninja", "-j", threads_str,
                         std::span<const std::string>{targets}};
  ninja.set_cwd((build_dir).native());
  RESULT_PROPAGATE_DISCARD(ninja.start());
  RESULT_PROPAGATE_DISCARD(ninja.process_output().to_result());

  // If run or test were given, take the time to run/test the corresponding
  // targets.
  if (mode != PostBuildMode::None) {
    for (const std::string &target : targets) {
      std::vector<std::string> exec_argv;
      if (mode == PostBuildMode::Run) {
        yabt_verbose("Collecting run arguments for {}", target);
        exec_argv = RESULT_PROPAGATE(
            lua_modules->contextlib.call_run_fn(target, action_args));
      } else if (mode == PostBuildMode::Test) {
        // FIXME: This should be handled inside call_run_fn and call_test_fn
        if (!lua_modules->contextlib.test_fn_refs.contains(target)) {
          yabt_debug("Skipping {} (no test function registered)", target);
          continue;
        }
        yabt_verbose("Collecting test arguments for {}", target);
        exec_argv = RESULT_PROPAGATE(
            lua_modules->contextlib.call_test_fn(target, action_args));
      }

      std::string arg_string = "";
      for (const std::string_view arg : exec_argv) {
        arg_string.append(arg);
        arg_string.push_back(' ');
      }
      yabt_verbose("Running {}", arg_string);

      process::Process run_process{
          exec_argv[0], std::span<const std::string>{exec_argv}.subspan(1)};
      RESULT_PROPAGATE_DISCARD(run_process.start());
      RESULT_PROPAGATE_DISCARD(run_process.process_output().to_result());
    }
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::build
