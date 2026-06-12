#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "yabt/log/log.h"
#include "yabt/module/git_module.h"
#include "yabt/module/module.h"
#include "yabt/process/process.h"
#include "yabt/runtime/result.h"
#include "yabt/utils/string.h"

namespace yabt::module {

using ProcessOutput = process::Process::ProcessOutput;

namespace {
template <typename... Args>
runtime::Result<process::Process::ProcessOutput>
exec_git_command(Args &&...args) noexcept {
  process::Process git_proc{"git", std::forward<Args>(args)...};
  RESULT_PROPAGATE_DISCARD(
      git_proc.start(true).with_context("while starting git process"));
  return runtime::Result<process::Process::ProcessOutput>::ok(
      git_proc.process_output());
}

// Validates a dependency URL before it is handed to `git clone`. This guards
// against two distinct injection vectors:
//
//   1. Option injection: because the URL is passed as a positional argument,
//      a value beginning with '-' would be parsed by git as a flag (e.g.
//      `--upload-pack=<cmd>`). We also pass a `--` separator at the call site
//      as defense in depth.
//   2. Remote-helper transports: git's `ext::`/`fd::` (and similar
//      `transport::address`) syntax can execute arbitrary commands. Standard
//      URLs use `scheme://` or scp-like `user@host:path` and never contain a
//      bare `::`, so we reject any URL that does.
[[nodiscard]] runtime::Result<void>
validate_git_url(std::string_view mod_url) noexcept {
  if (mod_url.empty()) {
    return runtime::Result<void>::error("Dependency URL is empty");
  }
  if (mod_url.front() == '-') {
    return runtime::Result<void>::error(std::format(
        "Refusing to clone dependency URL that looks like a command-line "
        "option: {}",
        mod_url));
  }
  if (mod_url.find("::") != std::string_view::npos) {
    return runtime::Result<void>::error(std::format(
        "Refusing to clone dependency URL using a git remote-helper transport "
        "(contains \"::\"): {}",
        mod_url));
  }
  return runtime::Result<void>::ok();
}
} // namespace

runtime::Result<std::unique_ptr<Module>>
GitModule::open(std::filesystem::path mod_dir) noexcept {
  if (const std::filesystem::path git_dir = mod_dir / ".git";
      !std::filesystem::exists(git_dir)) {
    return runtime::Result<std::unique_ptr<Module>>::error("");
  }

  return runtime::Result<std::unique_ptr<Module>>::ok(
      std::unique_ptr<Module>{new GitModule{mod_dir}});
}

runtime::Result<std::unique_ptr<Module>> GitModule::open_or_fetch_module(
    const std::filesystem::path &mod_dir, std::string_view mod_url,
    std::string_view mod_type, std::string_view mod_hash) noexcept {
  if (const std::filesystem::path git_dir = mod_dir / ".git";
      std::filesystem::exists(git_dir)) {
    // Return existing module
    yabt_debug("Opening already-existing git module at {}", mod_dir.native());
    return GitModule::open(mod_dir).with_context(
        std::format("while opening git module at {}", mod_dir.native()));
  }

  RESULT_PROPAGATE_DISCARD(validate_git_url(mod_url));

  const ProcessOutput process_output = RESULT_PROPAGATE(
      exec_git_command("clone", "--", mod_url, mod_dir.native()));
  if (runtime::Result result = process_output.to_result(); result.is_error()) {
    yabt_error("Error cloning git module {}\nstderr: {}", mod_url,
               process_output.stderr.value_or(""));
    RESULT_PROPAGATE_DISCARD(std::move(result).with_context(
        std::format("while cloning git module into {}", mod_dir.native())));
  }

  // FIXME checkout right project version
  static_cast<void>(mod_type);
  static_cast<void>(mod_hash);

  yabt_debug("Successfully fetched git module at {}", mod_dir.native());

  return runtime::Result<std::unique_ptr<Module>>::ok(
      std::unique_ptr<Module>{new GitModule{mod_dir}});
}

GitModule::GitModule(const std::filesystem::path &path) noexcept
    : m_path{path} {}

[[nodiscard]] std::string GitModule::name() const noexcept {
  return m_path.filename().string();
}

[[nodiscard]] std::filesystem::path GitModule::disk_path() const noexcept {
  return m_path;
}

[[nodiscard]] runtime::Result<std::string> GitModule::head() const noexcept {
  const ProcessOutput process_output = RESULT_PROPAGATE(exec_git_command(
      "-C", m_path.native(), "rev-list", "--max-count=1", "HEAD"));
  RESULT_PROPAGATE_DISCARD(process_output.to_result().with_context(
      std::format("while reading git HEAD in {}", m_path.native())));
  return runtime::Result<std::string>::ok(
      std::string{utils::trim_whitespace(process_output.stdout.value_or(""))});
}

[[nodiscard]] runtime::Result<void> GitModule::fetch() const noexcept {
  const ProcessOutput process_output = RESULT_PROPAGATE(
      exec_git_command("-C", m_path.native(), "fetch", "--all", "--tags"));
  RESULT_PROPAGATE_DISCARD(process_output.to_result().with_context(
      std::format("while fetching git refs in {}", m_path.native())));
  return runtime::Result<void>::ok();
}

[[nodiscard]] runtime::Result<bool>
GitModule::is_ancestor(std::string_view ancestor,
                       std::string_view revision) const noexcept {
  const ProcessOutput process_output =
      RESULT_PROPAGATE(exec_git_command("-C", m_path.native(), "merge-base",
                                        "--is-ancestor", ancestor, revision));
  return std::visit(
      [&process_output]<typename T>(const T v) {
        if constexpr (std::same_as<T, process::Process::NormalExit>) {
          if (v.exit_code != 0 && v.exit_code != 1) {
            return runtime::Result<bool>::error(
                std::format("Exited with code {}\nstderr: {}", v.exit_code,
                            process_output.stderr.value_or("")));
          }
          return runtime::Result<bool>::ok(!v.exit_code);
        } else if constexpr (std::same_as<T,
                                          process::Process::UnhandledSignal>) {
          return runtime::Result<bool>::error(
              std::format("Exited with signal {}\nstderr: {}", v.signal,
                          process_output.stderr.value_or("")));
        }
        runtime::fatal("Unhandled type");
      },
      process_output.exit_reason);
}

[[nodiscard]] runtime::Result<void>
GitModule::checkout(std::string_view revision) const noexcept {
  const ProcessOutput process_output = RESULT_PROPAGATE(
      exec_git_command("-C", m_path.native(), "checkout", revision));
  return process_output.to_result().with_context(std::format(
      "while checking out revision {} in {}", revision, m_path.native()));
}

} // namespace yabt::module
