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
runtime::Result<process::Process::ProcessOutput, std::string>
exec_git_command(Args &&...args) noexcept {
  process::Process git_proc{"git", std::forward<Args>(args)...};
  RESULT_PROPAGATE_DISCARD(git_proc.start(true));
  return runtime::Result<process::Process::ProcessOutput, std::string>::ok(
      git_proc.capture_output());
}
} // namespace

runtime::Result<std::unique_ptr<Module>, std::string>
GitModule::open(std::filesystem::path mod_dir) noexcept {
  if (const std::filesystem::path git_dir = mod_dir / ".git";
      !std::filesystem::exists(git_dir)) {
    return runtime::Result<std::unique_ptr<Module>, std::string>::error("");
  }

  return runtime::Result<std::unique_ptr<Module>, std::string>::ok(
      std::unique_ptr<Module>{new GitModule{mod_dir}});
}

runtime::Result<std::unique_ptr<Module>, std::string>
GitModule::open_or_fetch_module(const std::filesystem::path &mod_dir,
                                std::string_view mod_url,
                                std::string_view mod_type,
                                std::string_view mod_hash) noexcept {
  if (const std::filesystem::path git_dir = mod_dir / ".git";
      std::filesystem::exists(git_dir)) {
    // Return existing module
    yabt_debug("Opening already-existing git module at {}", mod_dir.native());
    return GitModule::open(mod_dir);
  }

  const ProcessOutput process_output =
      RESULT_PROPAGATE(exec_git_command("clone", mod_url, mod_dir.native()));
  if (runtime::Result result = process_output.to_result(); result.is_error()) {
    yabt_error("Error cloning git module {}\nstderr: {}", mod_url,
               process_output.stderr);
    RESULT_PROPAGATE_DISCARD(result);
  }

  // FIXME checkout right project version
  static_cast<void>(mod_type);
  static_cast<void>(mod_hash);

  yabt_debug("Successfully fetched git module at {}", mod_dir.native());

  return runtime::Result<std::unique_ptr<Module>, std::string>::ok(
      std::unique_ptr<Module>{new GitModule{mod_dir}});
}

GitModule::GitModule(const std::filesystem::path &path) noexcept
    : m_path{path} {}

[[nodiscard]] std::string GitModule::name() const noexcept {
  return m_path.filename().string();
}

[[nodiscard]] runtime::Result<std::string, std::string>
GitModule::head() const noexcept {
  const ProcessOutput process_output = RESULT_PROPAGATE(exec_git_command(
      "-C", m_path.native(), "rev-list", "--max-count=1", "HEAD"));
  RESULT_PROPAGATE_DISCARD(process_output.to_result());
  return runtime::Result<std::string, std::string>::ok(
      std::string{utils::trim_whitespace(process_output.stdout)});
}

[[nodiscard]] runtime::Result<void, std::string>
GitModule::fetch() const noexcept {
  const ProcessOutput process_output = RESULT_PROPAGATE(
      exec_git_command("-C", m_path.native(), "fetch", "--all", "--tags"));
  RESULT_PROPAGATE_DISCARD(process_output.to_result());
  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<bool, std::string>
GitModule::is_ancestor(std::string_view ancestor,
                       std::string_view revision) const noexcept {
  const ProcessOutput process_output =
      RESULT_PROPAGATE(exec_git_command("-C", m_path.native(), "merge-base",
                                        "--is-ancestor", ancestor, revision));
  return std::visit(
      [&process_output]<typename T>(const T v) {
        if constexpr (std::same_as<T, process::Process::NormalExit>) {
          if (v.exit_code != 0 && v.exit_code != 1) {
            return runtime::Result<bool, std::string>::error(
                std::format("Exited with code {}\nstderr: {}", v.exit_code,
                            process_output.stderr));
          }
          return runtime::Result<bool, std::string>::ok(!v.exit_code);
        } else if constexpr (std::same_as<T,
                                          process::Process::UnhandledSignal>) {
          return runtime::Result<bool, std::string>::error(
              std::format("Exited with signal {}\nstderr: {}", v.signal,
                          process_output.stderr));
        }
        runtime::fatal("Unhandled type");
      },
      process_output.exit_reason);
}

[[nodiscard]] runtime::Result<void, std::string>
GitModule::checkout(std::string_view revision) const noexcept {
  const ProcessOutput process_output = RESULT_PROPAGATE(
      exec_git_command("-C", m_path.native(), "checkout", revision));
  return process_output.to_result();
}

} // namespace yabt::module
