#pragma once

#include <format>

#include "yabt/runtime/result.h"

namespace yabt::log {

enum class LogLevel {
  ERROR,
  WARNING,
  INFO,
  DEBUG,
  VERBOSE,
};

enum class Color { RED, MAGENTA, YELLOW, GREEN, BLUE };

void set_log_level(LogLevel) noexcept;

[[nodiscard]] runtime::Result<void, std::string>
set_log_level(std::string_view sv) noexcept;

[[nodiscard]] bool is_level_enabled(LogLevel) noexcept;

[[nodiscard]] std::string_view get_set_color_code(Color) noexcept;
[[nodiscard]] std::string_view get_reset_color_code() noexcept;

void disable_color() noexcept;

class IndentGuard {
public:
  IndentGuard();
  ~IndentGuard();

  IndentGuard(const IndentGuard &) = delete;
  IndentGuard(IndentGuard &&) = delete;
  IndentGuard &operator=(const IndentGuard &) = delete;
  IndentGuard &operator=(IndentGuard &&) = delete;

private:
};

[[nodiscard]] size_t current_indent_level();

template <typename... Args>
void log(const std::format_string<
             const std::string_view &, const std::string_view &,
             const std::string_view &, std::string_view, size_t, Args...>
             fstr,
         const LogLevel level, const std::string_view level_str,
         const Color color, Args &&...args) noexcept {
  if (is_level_enabled(level)) {
    using std::string_view_literals::operator""sv;
    const std::string_view set_color = get_set_color_code(color);
    const std::string_view reset_color = get_reset_color_code();

    constexpr static size_t INDENT_SPACES = 2;
    const size_t indent_level = current_indent_level();

    puts(std::format(fstr, set_color, level_str, reset_color, ""sv,
                     indent_level * INDENT_SPACES, std::forward<Args>(args)...)
             .c_str());
  }
}

#define _yabt_log(color, level, level_str, fstring, ...)                       \
  ::yabt::log::log("{}{:<10}{}{:{}}" fstring, level, level_str, color,         \
                   ##__VA_ARGS__)

#define yabt_error(fstring, ...)                                               \
  _yabt_log(::yabt::log::Color::RED, ::yabt::log::LogLevel::ERROR,             \
            "Error: ", fstring, ##__VA_ARGS__)

#define yabt_warn(fstring, ...)                                                \
  _yabt_log(::yabt::log::Color::MAGENTA, ::yabt::log::LogLevel::WARNING,       \
            "Warning: ", fstring, ##__VA_ARGS__)

#define yabt_info(fstring, ...)                                                \
  _yabt_log(::yabt::log::Color::YELLOW, ::yabt::log::LogLevel::INFO,           \
            "Info: ", fstring, ##__VA_ARGS__)

#define yabt_debug(fstring, ...)                                               \
  _yabt_log(::yabt::log::Color::GREEN, ::yabt::log::LogLevel::DEBUG,           \
            "Debug: ", fstring, ##__VA_ARGS__)

#define yabt_verbose(fstring, ...)                                             \
  _yabt_log(::yabt::log::Color::BLUE, ::yabt::log::LogLevel::VERBOSE,          \
            "Verbose: ", fstring, ##__VA_ARGS__)

} // namespace yabt::log
