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
  inline Subcommand(const std::string_view name, SubcommandHandler &handler,
                    std::string_view short_descr,
                    std::string_view long_descr) noexcept
      : m_name{name}, m_short_descr{short_descr}, m_long_descr{long_descr},
        m_handler{&handler} {}

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

  [[nodiscard]] inline const std::string &name() const noexcept {
    return m_name;
  }

  [[nodiscard]] inline std::span<const Flag> flags() const noexcept {
    return m_flags;
  }

  [[nodiscard]] inline runtime::Result<void, std::string>
  invoke(std::span<const std::string_view> unparsed_args) const noexcept {
    return m_handler->handle_subcommand(unparsed_args);
  }

  [[nodiscard]] inline const std::string &short_description() const noexcept {
    return m_short_descr;
  }

  [[nodiscard]] inline const std::string &long_description() const noexcept {
    if (m_long_descr == "") {
      return m_short_descr;
    }
    return m_long_descr;
  }

private:
  std::string m_name;
  std::string m_short_descr;
  std::string m_long_descr;
  SubcommandHandler *m_handler;
  std::vector<Flag> m_flags;
};

} // namespace yabt::cli
