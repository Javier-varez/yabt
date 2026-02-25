#pragma once

namespace yabt::utils {

template <typename T> class Defer final {
public:
  explicit Defer(T runnable) noexcept : m_runnable{runnable} {}

  Defer(const Defer &) = delete;
  Defer(Defer &&) = delete;

  Defer &operator=(const Defer &) = delete;
  Defer &operator=(Defer &&) = delete;

  ~Defer() noexcept { m_runnable(); }

private:
  T m_runnable;
};

} // namespace yabt::utils
