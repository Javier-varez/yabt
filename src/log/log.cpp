#include <type_traits>

#include "yabt/log/log.h"

namespace yabt::log {

namespace {
LogLevel global_level = LogLevel::INFO;
bool enable_color = true;
} // namespace

void set_log_level(const LogLevel level) noexcept { global_level = level; }

[[nodiscard]] runtime::Result<void, std::string>
set_log_level(std::string_view sv) noexcept {
  if (sv == "ERROR") {
    set_log_level(LogLevel::ERROR);
    return runtime::Result<void, std::string>::ok();
  }

  if (sv == "WARNING") {
    set_log_level(LogLevel::WARNING);
    return runtime::Result<void, std::string>::ok();
  }

  if (sv == "INFO") {
    set_log_level(LogLevel::INFO);
    return runtime::Result<void, std::string>::ok();
  }

  if (sv == "DEBUG") {
    set_log_level(LogLevel::DEBUG);
    return runtime::Result<void, std::string>::ok();
  }

  if (sv == "VERBOSE") {
    set_log_level(LogLevel::VERBOSE);
    return runtime::Result<void, std::string>::ok();
  }

  return runtime::Result<void, std::string>::error(std::format(
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

} // namespace yabt::log
