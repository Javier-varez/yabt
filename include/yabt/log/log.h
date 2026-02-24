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

template <typename... Args>
void log(std::format_string<const std::string_view &, const std::string_view &,
                            Args...>
             fstr,
         LogLevel level, Color color, Args &&...args) noexcept {
  if (is_level_enabled(level)) {
    const std::string_view set_color = get_set_color_code(color);
    const std::string_view reset_color = get_reset_color_code();
    puts(std::format(fstr, set_color, reset_color, std::forward<Args>(args)...)
             .c_str());
  }
}

#define _yabt_log(level_str, color, level, fstring, ...)                       \
  ::yabt::log::log("{}" level_str "{}" fstring, level, color, ##__VA_ARGS__)

#define yabt_error(fstring, ...)                                               \
  _yabt_log("Error: ", ::yabt::log::Color::RED, ::yabt::log::LogLevel::ERROR,  \
            fstring, ##__VA_ARGS__)

#define yabt_warn(fstring, ...)                                                \
  _yabt_log("Warning: ", ::yabt::log::Color::MAGENTA,                          \
            ::yabt::log::LogLevel::WARNING, fstring, ##__VA_ARGS__)

#define yabt_info(fstring, ...)                                                \
  _yabt_log("Info: ", ::yabt::log::Color::YELLOW, ::yabt::log::LogLevel::INFO, \
            fstring, ##__VA_ARGS__)

#define yabt_debug(fstring, ...)                                               \
  _yabt_log("Debug: ", ::yabt::log::Color::GREEN,                              \
            ::yabt::log::LogLevel::DEBUG, fstring, ##__VA_ARGS__)

#define yabt_verbose(fstring, ...)                                             \
  _yabt_log("Verbose: ", ::yabt::log::Color::BLUE,                             \
            ::yabt::log::LogLevel::VERBOSE, fstring, ##__VA_ARGS__)

} // namespace yabt::log
