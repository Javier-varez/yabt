#pragma once

#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "yabt/runtime/result.h"

namespace yabt::process {

namespace detail {

template <typename... Args>
[[nodiscard]] std::vector<std::string> to_str_vec(Args &&...args) noexcept {
  std::vector<std::string> result;
  result.reserve(sizeof...(Args));

  const auto process_one = [&result](const std::string_view s) {
    result.push_back(std::string{s});
  };

  (process_one(std::forward<Args>(args)), ...);
  return result;
}

} // namespace detail

class Process {
public:
  struct NormalExit final {
    int exit_code;
  };

  struct UnhandledSignal final {
    bool core_dumped;
    int signal;
  };

  using ExitReason = std::variant<NormalExit, UnhandledSignal>;

  struct ProcessOutput final {
    std::string stdout;
    std::string stderr;
    ExitReason exit_reason;

    [[nodiscard]] runtime::Result<void, std::string> to_result() const noexcept;
  };

  template <typename... Args>
  explicit Process(std::string_view executable, Args &&...args) noexcept
      : Process{executable, std::span<const std::string>{detail::to_str_vec(
                                std::forward<Args>(args)...)}} {}

  explicit Process(std::string_view executable,
                   std::span<const std::string> arguments = {}) noexcept;

  Process(const Process &) noexcept = delete;
  Process &operator=(const Process &) noexcept = delete;

  Process(Process &&) noexcept = delete;
  Process &operator=(Process &&) noexcept = delete;

  ~Process() noexcept;

  void set_cwd(const std::string_view path);

  [[nodiscard]] runtime::Result<void, std::string>
  start(bool capture_stdout = false) noexcept;

  [[nodiscard]] ExitReason wait_completion() noexcept;

  [[nodiscard]] ProcessOutput capture_output() noexcept;

private:
  enum class Status {
    CREATED,
    RUNNING,
    FINISHED,
  };

  std::string m_exe;
  std::vector<std::string> m_args;
  std::optional<std::string> m_cwd{};
  Status m_status{Status::CREATED};
  int m_child_pid{};
  int m_stdout_read_pipe{-1};
  int m_stderr_read_pipe{-1};
  int m_exit_code{};
};

} // namespace yabt::process
