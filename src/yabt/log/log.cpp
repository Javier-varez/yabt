#include <type_traits>

#include "yabt/log/log.h"

namespace yabt::log {

namespace {
LogLevel global_level = LogLevel::INFO;
bool enable_color = true;
size_t indent_level = 0;
} // namespace

void set_log_level(const LogLevel level) noexcept { global_level = level; }

[[nodiscard]] runtime::Result<void>
set_log_level(std::string_view sv) noexcept {
  if (sv == "ERROR") {
    set_log_level(LogLevel::ERROR);
    return runtime::Result<void>::ok();
  }

  if (sv == "WARNING") {
    set_log_level(LogLevel::WARNING);
    return runtime::Result<void>::ok();
  }

  if (sv == "INFO") {
    set_log_level(LogLevel::INFO);
    return runtime::Result<void>::ok();
  }

  if (sv == "DEBUG") {
    set_log_level(LogLevel::DEBUG);
    return runtime::Result<void>::ok();
  }

  if (sv == "VERBOSE") {
    set_log_level(LogLevel::VERBOSE);
    return runtime::Result<void>::ok();
  }

  return runtime::Result<void>::error(std::format(
      "Unknown log level {}. Available log levels: \"ERROR\", \"WARNING\", "
      "\"INFO\", \"DEBUG\", \"VERBOSE\"",
      sv));
}

[[nodiscard]] bool is_level_enabled(const LogLevel level) noexcept {
  auto level_int = static_cast<std::underlying_type_t<LogLevel>>(level);
  auto global_level_int =
      static_cast<std::underlying_type_t<LogLevel>>(global_level);
  return level_int <= global_level_int;
}

using std::string_view_literals::operator""sv;

[[nodiscard]] std::string_view get_set_color_code(Color color) noexcept {
  if (!enable_color) {
    return ""sv;
  }
  switch (color) {
  case Color::RED: {
    return "\x1b[1;31m"sv;
  }
  case Color::MAGENTA: {
    return "\x1b[1;35m"sv;
  }
  case Color::YELLOW: {
    return "\x1b[1;33m"sv;
  }
  case Color::GREEN: {
    return "\x1b[1;32m"sv;
  }
  case Color::BLUE: {
    return "\x1b[1;34m"sv;
  }
  }
  return "";
}

[[nodiscard]] std::string_view get_reset_color_code() noexcept {
  if (enable_color) {
    return "\x1b[0m"sv;
  }
  return ""sv;
}

void disable_color() noexcept { enable_color = false; }

namespace detail {

[[nodiscard]] std::string indent_continuation_lines(std::string message,
                                                    const size_t indentation) {
  size_t pos = 0;
  while ((pos = message.find('\n', pos)) != std::string::npos) {
    ++pos;
    if (pos == message.size()) {
      break;
    }
    message.insert(pos, indentation, ' ');
    pos += indentation;
  }
  return message;
}

[[nodiscard]] std::string format_log_arg(const runtime::Error &error) {
  constexpr static std::string_view DIM_TEXT{"\x1b[2m"};

  const std::string_view dim_text = enable_color ? DIM_TEXT : ""sv;
  const std::string_view reset_text = get_reset_color_code();

  std::string result{"Error:\n"};
  for (auto iter = error.context_chain().rbegin();
       iter != error.context_chain().rend(); ++iter) {
    const bool is_first = iter == error.context_chain().rbegin();
    const bool is_last = std::next(iter) == error.context_chain().rend();

    if (is_first) {
      result.append("  ×─▶ ");
    } else if (is_last) {
      result.append("  ╰─▶ ");
    } else {
      result.append("  ├─▶ ");
    }
    result.append(iter->message);
    result.push_back('\n');
    result.append(is_last ? "      " : "  │   ");
    result.append(std::format("{}at {}:{}:{}{}", dim_text,
                              iter->location.file_name(), iter->location.line(),
                              iter->location.column(), reset_text));
    if (!is_last) {
      result.push_back('\n');
    }
  }
  return result;
}

} // namespace detail

[[nodiscard]] size_t current_indent_level() { return indent_level; }

IndentGuard::IndentGuard() { indent_level++; }
IndentGuard::~IndentGuard() { indent_level--; };

} // namespace yabt::log
