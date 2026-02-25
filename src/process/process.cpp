#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

#include "yabt/process/process.h"
#include "yabt/runtime/check.h"

namespace yabt::process {

runtime::Result<void, std::string>
Process::ProcessOutput::to_result() const noexcept {
  return std::visit(
      [this]<typename T>(const T v) {
        if constexpr (std::same_as<T, NormalExit>) {
          if (v.exit_code == 0) {
            return runtime::Result<void, std::string>::ok();
          }
          return runtime::Result<void, std::string>::error(std::format(
              "Exited with code {}\nstderr: {}", v.exit_code, stderr));
        } else if constexpr (std::same_as<T, UnhandledSignal>) {
          return runtime::Result<void, std::string>::error(std::format(
              "Exited with signal {}\nstderr: {}", v.signal, stderr));
        }
        runtime::fatal("Unhandled type");
      },
      exit_reason);
}

Process::Process(std::string_view executable,
                 std::span<const std::string> arguments) noexcept
    : m_exe{executable}, m_args{[arguments]() {
        std::vector<std::string> r{};
        r.reserve(arguments.size());
        for (const std::string &s : arguments) {
          r.push_back(s);
        }
        return r;
      }()} {}

void Process::set_cwd(const std::string_view path) {
  yabt::runtime::check(
      m_status == Status::CREATED,
      "Attempted to set cwd on a Process that is not in the CREATED status");
  m_cwd = path;
}

[[nodiscard]] runtime::Result<void, std::string>
Process::start(const bool capture_stdout) noexcept {
  yabt::runtime::check(m_status == Status::CREATED,
                       "Process is not in the created status");

  std::array<int, 2> stdout_pipes{-1, -1};
  std::array<int, 2> stderr_pipes{-1, -1};

  if (capture_stdout) {
    if (pipe(stdout_pipes.data()) != 0) {
      runtime::fatal("Unable to create pipe: {}", strerror(errno));
    }

    if (pipe(stderr_pipes.data()) != 0) {
      runtime::fatal("Unable to create pipe: {}", strerror(errno));
    }

    m_stdout_read_pipe = stdout_pipes[0];
    m_stderr_read_pipe = stderr_pipes[0];
  }

  const pid_t pid = fork();
  if (pid == -1) {
    return yabt::runtime::Result<void, std::string>::error(
        std::format("Failed to fork process: {}", strerror(errno)));
  }

  if (pid == 0) {
    std::vector<const char *> args{m_exe.c_str()};
    args.reserve(1 + m_args.size());
    for (const std::string &arg : m_args) {
      args.push_back(arg.c_str());
    }
    args.push_back(nullptr);

    if (capture_stdout) {
      dup2(stdout_pipes[1], STDOUT_FILENO);
      dup2(stderr_pipes[1], STDERR_FILENO);

      close(stdout_pipes[0]);
      close(stdout_pipes[1]);
      close(stderr_pipes[0]);
      close(stderr_pipes[1]);
    }

    if (m_cwd.has_value()) {
      runtime::check(chdir(m_cwd.value().c_str()) == 0,
                     "Chdir to {} failed with: {}", m_cwd.value(),
                     strerror(errno));
    }

    execvp(m_exe.c_str(), const_cast<char *const *>(args.data()));
    runtime::fatal("Child failed to start program: {}", strerror(errno));
  }

  m_status = Status::RUNNING;
  m_child_pid = pid;

  if (capture_stdout) {
    close(stdout_pipes[1]);
    close(stderr_pipes[1]);
  }

  return yabt::runtime::Result<void, std::string>::ok();
}

[[nodiscard]] Process::ExitReason Process::wait_completion() noexcept {
  yabt::runtime::check(m_status == Status::RUNNING,
                       "Process is not in the running status");

  while (true) {
    int result = waitpid(m_child_pid, &m_exit_code, 0);
    if (result != -1) {
      break;
    }

    result = errno;
    runtime::check(result == EINTR, "Unexpected return value from waitpid: {}",
                   strerror(result));
  }
  m_status = Status::FINISHED;

  Process::ExitReason exit_reason{};

  if (WIFEXITED(m_exit_code)) {
    exit_reason = NormalExit{.exit_code = WEXITSTATUS(m_exit_code)};
  } else if (WIFSIGNALED(m_exit_code)) {
    exit_reason = UnhandledSignal{
        .core_dumped = WCOREDUMP(m_exit_code),
        .signal = WTERMSIG(m_exit_code),
    };
  } else {
    runtime::fatal("Unexpected exit code from subprocess: {}", m_exit_code);
  }

  return exit_reason;
}

[[nodiscard]] Process::ProcessOutput Process::capture_output() noexcept {
  yabt::runtime::check(m_status == Status::RUNNING,
                       "Process is not in the running status");
  yabt::runtime::check(m_stdout_read_pipe != -1,
                       "Stdout is not being captured");
  ProcessOutput output;

  const auto read_stream = [](int fd, std::string &stream) {
    while (true) {
      const size_t offset = stream.size();

      constexpr static size_t BUF_SIZE = 1024;
      stream.resize(offset + BUF_SIZE);

      const ssize_t n_read = read(fd, stream.data() + offset, BUF_SIZE);
      if (n_read < 0) {
        runtime::fatal("Unexpected error reading from pipe: {}",
                       strerror(errno));
      }
      if (n_read == 0) {
        stream.resize(offset);
        break;
      }

      stream.resize(offset + n_read);
    }
  };

  read_stream(m_stdout_read_pipe, output.stdout);
  read_stream(m_stderr_read_pipe, output.stderr);
  output.exit_reason = wait_completion();

  return output;
}

Process::~Process() noexcept {
  if (m_status == Status::RUNNING) {
    static_cast<void>(wait_completion());
  }
}

} // namespace yabt::process
