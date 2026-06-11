#pragma once

#include <format>
#include <iterator>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace yabt::runtime {

class Error final {
public:
  struct Entry final {
    std::string message;
    std::source_location location;
  };

  explicit Error(
      std::string_view message,
      std::source_location location = std::source_location::current()) noexcept
      : m_context_chain{
            Entry{
                .message{std::string{message}},
                .location{std::move(location)},
            },
        } {}

  explicit Error(
      std::string message,
      std::source_location location = std::source_location::current()) noexcept
      : m_context_chain{
            Entry{
                .message{std::move(message)},
                .location{std::move(location)},
            },
        } {}

  explicit Error(
      const char *message,
      std::source_location location = std::source_location::current()) noexcept
      : m_context_chain{
            Entry{
                .message{std::string{message}},
                .location{std::move(location)},
            },
        } {}

  Error(const Error &) noexcept = default;
  Error(Error &&) noexcept = default;
  Error &operator=(const Error &) noexcept = default;
  Error &operator=(Error &&) noexcept = default;

  [[nodiscard]] const std::string &message() const noexcept {
    return m_context_chain.front().message;
  }

  [[nodiscard]] std::source_location location() const noexcept {
    return m_context_chain.front().location;
  }

  [[nodiscard]] const std::vector<Entry> &context_chain() const noexcept {
    return m_context_chain;
  }

  Error &add_context(
      std::string_view context,
      std::source_location location = std::source_location::current()) & {
    m_context_chain.push_back(Entry{std::string{context}, location});
    return *this;
  }

  Error &&add_context(
      std::string_view context,
      std::source_location location = std::source_location::current()) && {
    add_context(context, location);
    return std::move(*this);
  }

  [[nodiscard]] Error with_context(
      std::string_view context,
      std::source_location location = std::source_location::current()) const & {
    Error error{*this};
    error.add_context(context, location);
    return error;
  }

  [[nodiscard]] Error with_context(
      std::string_view context,
      std::source_location location = std::source_location::current()) && {
    add_context(context, location);
    return std::move(*this);
  }

  [[nodiscard]] std::string string() const {
    std::string result{"Error:\n"};
    for (auto iter = m_context_chain.rbegin(); iter != m_context_chain.rend();
         ++iter) {
      const bool is_first = iter == m_context_chain.rbegin();
      const bool is_last = std::next(iter) == m_context_chain.rend();

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
      result.append(std::format("at {}:{}:{}", iter->location.file_name(),
                                iter->location.line(),
                                iter->location.column()));
      if (!is_last) {
        result.push_back('\n');
      }
    }
    return result;
  }

  [[nodiscard]] std::string to_string() const { return string(); }

private:
  std::vector<Entry> m_context_chain;
};

} // namespace yabt::runtime

template <>
struct std::formatter<yabt::runtime::Error> : std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const yabt::runtime::Error &error, FormatContext &ctx) const {
    return std::formatter<std::string>::format(error.string(), ctx);
  }
};
