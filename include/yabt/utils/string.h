#pragma once

#include <string_view>

namespace yabt::utils {

[[nodiscard]] std::string_view trim_whitespace(std::string_view sv) noexcept;

[[nodiscard]] std::string_view
trim_left_charset(std::string_view sv, std::string_view chars) noexcept;

[[nodiscard]] std::string_view
trim_right_charset(std::string_view sv, std::string_view chars) noexcept;

} // namespace yabt::utils
