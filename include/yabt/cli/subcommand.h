#pragma once

#include <algorithm>
#include <span>
#include <string_view>
#include <vector>

#include "yabt/cli/flag.h"
#include "yabt/runtime/result.h"

namespace yabt::cli {

class SubcommandHandler {
public:
  [[nodiscard]] virtual runtime::Result<void, std::string> handle_subcommand(
      std::span<const std::string_view> unparsed_args) noexcept = 0;

  virtual ~SubcommandHandler() noexcept = default;
};

class Subcommand {
public:
  inline Subcommand(const std::string_view name,
                    SubcommandHandler &handler) noexcept
      : m_name{name}, m_handler{&handler} {}

  [[nodiscard]] inline runtime::Result<void, std::string>
  register_flag(Flag flag) noexcept {
    RESULT_PROPAGATE_DISCARD(flag.validate());
    const auto iter = std::find_if(m_flags.begin(), m_flags.end(),
                                   [&flag](const Flag &other) noexcept -> bool {
                                     return other.name == flag.name;
                                   });
    if (iter != m_flags.end()) {
      *iter = std::move(flag);
      return runtime::Result<void, std::string>::ok();
    }
    m_flags.push_back(std::move(flag));
    return runtime::Result<void, std::string>::ok();
  }

  inline void set_handler(SubcommandHandler &handler) noexcept {
    m_handler = &handler;
  }

  [[nodiscard]] inline std::string_view name() const noexcept { return m_name; }

  [[nodiscard]] inline std::span<const Flag> flags() const noexcept {
    return m_flags;
  }

  [[nodiscard]] inline runtime::Result<void, std::string>
  invoke(std::span<const std::string_view> unparsed_args) const noexcept {
    return m_handler->handle_subcommand(unparsed_args);
  }

private:
  std::string m_name;
  SubcommandHandler *m_handler;
  std::vector<Flag> m_flags;
};

} // namespace yabt::cli
