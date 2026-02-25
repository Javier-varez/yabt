#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "yabt/runtime/check.h"

namespace yabt::runtime {

namespace detail {

template <typename T> struct Instantiable {
  using type = T;
};

template <> struct Instantiable<void> {
  using type = std::uint8_t;
};

template <class T> struct [[nodiscard]] ErrSentinel {
  explicit ErrSentinel(T val) noexcept : value{std::move(val)} {}
  T value;
};

template <class T> struct [[nodiscard]] OkSentinel {
  explicit OkSentinel(T val) noexcept : value{std::move(val)} {}
  T value;
};

template <typename T>
concept NonVoid = !std::same_as<T, void>;

} // namespace detail

// FIXME: make this class constexpr. Need to refactor to use std::variant
// instead of placement new and reinterpret_cast (the latter cannot be made
// constexpr).
//
// NOTE: Probably worth adding a `context` method to Result<T, std::string> to
// add context to errors.
template <class Ok, class Err> class [[nodiscard]] Result final {
public:
  // Constructors used for propagating errors or values
  template <detail::NonVoid T = Ok>
  Result(detail::OkSentinel<T> val) noexcept : m_is_ok{true} {
    new (&m_memory_buffer) T{std::move(val.value)};
  }

  template <detail::NonVoid T = Err>
  Result(detail::ErrSentinel<T> val) noexcept : m_is_ok{false} {
    new (&m_memory_buffer) T{std::move(val.value)};
  }

  /// Construct an Ok object with perfect forwarding
  template <class... Args>
  [[nodiscard]] static Result ok(Args &&...args) noexcept {
    if constexpr (std::is_same_v<void, Ok>) {
      static_assert(sizeof...(Args) == 0,
                    "Result::ok() does not take arguments for 'void'");
      Result res{};
      res.m_is_ok = true;
      return res;
    } else {
      return Result{detail::OkSentinel<Ok>{std::forward<Args>(args)...}};
    }
  }

  /// Construct an Err object with perfect forwarding
  template <class... Args>
  [[nodiscard]] static Result error(Args &&...args) noexcept {
    if constexpr (std::is_same_v<void, Err>) {
      static_assert(sizeof...(Args) == 0,
                    "Result::error() does not take arguments for 'void'");
      Result res{};
      res.m_is_ok = false;
      return res;
    } else {
      return Result{detail::ErrSentinel<Err>{std::forward<Args>(args)...}};
    }
  }

  [[nodiscard]] bool is_error() const noexcept { return !m_is_ok; }
  [[nodiscard]] bool is_ok() const noexcept { return m_is_ok; }

  template <detail::NonVoid T = Ok>
  [[nodiscard]] const T &ok_value() const & noexcept {
    check(m_is_ok, "Attempted to unwrap ok from erroneous result");
    return *reinterpret_cast<const Ok *>(&m_memory_buffer);
  }

  template <detail::NonVoid T = Ok> [[nodiscard]] T &ok_value() & noexcept {
    check(m_is_ok, "Attempted to unwrap ok from erroneous result");
    return *reinterpret_cast<Ok *>(&m_memory_buffer);
  }

  template <detail::NonVoid T = Ok> [[nodiscard]] T &&ok_value() && noexcept {
    check(m_is_ok, "Attempted to unwrap ok from erroneous result");
    return std::move(*reinterpret_cast<Ok *>(&m_memory_buffer));
  }

  template <detail::NonVoid T = Err>
  [[nodiscard]] const T &error_value() const & noexcept {
    check(!m_is_ok, "Attempted to unwrap error from successful result");
    return *reinterpret_cast<const Err *>(&m_memory_buffer);
  }

  template <detail::NonVoid T = Err> [[nodiscard]] T &error_value() & noexcept {
    check(!m_is_ok, "Attempted to unwrap error from successful result");
    return *reinterpret_cast<Err *>(&m_memory_buffer);
  }

  template <detail::NonVoid T = Err>
  [[nodiscard]] T &&error_value() && noexcept {
    check(!m_is_ok, "Attempted to unwrap error from successful result");
    return std::move(*reinterpret_cast<Err *>(&m_memory_buffer));
  }

  template <typename Callable, detail::NonVoid T = Ok>
  [[nodiscard]] auto map_ok(Callable callable) noexcept
      -> Result<decltype(std::declval<Callable>()(std::declval<Ok>())), Err> {
    if (!is_ok()) {
      return {detail::ErrSentinel<Err>{error_value()}};
    }
    return {detail::OkSentinel{callable(ok_value())}};
  }

  template <typename Callable, detail::NonVoid T = Err>
  [[nodiscard]] auto map_err(Callable callable) noexcept
      -> Result<Ok, decltype(std::declval<Callable>()(std::declval<Err>()))> {
    if (!is_error()) {
      return {detail::OkSentinel<Ok>{ok_value()}};
    }
    return {detail::ErrSentinel{callable(error_value())}};
  }

  ~Result() noexcept { release(); }

  Result(const Result &other) noexcept {
    m_is_ok = other.m_is_ok;
    if (m_is_ok) {
      if constexpr (!std::same_as<Ok, void>) {
        new (&m_memory_buffer) Ok{other.ok_value()};
      }
    } else {
      if constexpr (!std::same_as<Err, void>) {
        new (&m_memory_buffer) Err{other.error_value()};
      }
    }
  }

  Result(Result &&other) noexcept {
    m_is_ok = other.m_is_ok;
    if (m_is_ok) {
      if constexpr (!std::same_as<Ok, void>) {
        new (&m_memory_buffer) Ok{std::move(other.ok_value())};
      }
    } else {
      if constexpr (!std::same_as<Err, void>)
        new (&m_memory_buffer) Err{std::move(other.error_value())};
    }
  }

  Result &operator=(const Result &other) noexcept {
    if (&other == this)
      return *this;

    release();

    m_is_ok = other.m_is_ok;

    if (m_is_ok) {
      if constexpr (!std::same_as<Ok, void>)
        new (&m_memory_buffer) Ok{other.ok_value()};
    } else {
      if constexpr (!std::same_as<Err, void>)
        new (&m_memory_buffer) Err{other.error_value()};
    }
    return *this;
  }

  Result &operator=(Result &&other) noexcept {
    if (&other == this)
      return *this;

    release();

    m_is_ok = other.m_is_ok;
    if (m_is_ok) {
      if constexpr (!std::same_as<Ok, void>)
        new (&m_memory_buffer) Ok{std::move(other.ok_value())};
    } else {
      if constexpr (!std::same_as<Err, void>)
        new (&m_memory_buffer) Err{std::move(other.error_value())};
    }

    return *this;
  }

private:
  using InstantiableOk = typename detail::Instantiable<Ok>::type;
  using InstantiableErr = typename detail::Instantiable<Err>::type;
  constexpr static std::size_t ALIGNMENT =
      std::max(alignof(InstantiableOk), alignof(InstantiableErr));
  constexpr static std::size_t BUFFER_SIZE =
      std::max(sizeof(InstantiableOk), sizeof(InstantiableErr));
  bool m_is_ok = false;
  typename std::aligned_storage<BUFFER_SIZE, ALIGNMENT>::type m_memory_buffer;

  Result() noexcept = default;

  void release() {
    if (m_is_ok) {
      if constexpr (!std::is_same_v<void, Ok>) {
        auto *ok = std::launder(reinterpret_cast<Ok *>(&m_memory_buffer));
        ok->~Ok();
      }
    } else {
      if constexpr (!std::is_same_v<void, Err>) {
        auto *err = std::launder(reinterpret_cast<Err *>(&m_memory_buffer));
        err->~Err();
      }
    }
  }
};

#define RESULT_VERIFY(expr)                                                    \
  {                                                                            \
    yabt::runtime::check(expr, #expr " does not evaluate to true");            \
  }

#define RESULT_VERIFY_OK(expression)                                           \
  {                                                                            \
    yabt::runtime::Result _evaluated_result = expression;                      \
    RESULT_VERIFY(_evaluated_result.is_ok());                                  \
  }

#define RESULT_VERIFY_ERR(expression)                                          \
  {                                                                            \
    yabt::runtime::Result _evaluated_result = expression;                      \
    RESULT_VERIFY(_evaluated_result.is_error());                               \
  }

#define RESULT_PROPAGATE(expression)                                           \
  ({                                                                           \
    yabt::runtime::Result _result = expression;                                \
    if (_result.is_error()) {                                                  \
      return {yabt::runtime::detail::ErrSentinel{                              \
          std::move(_result).error_value()}};                                  \
    }                                                                          \
    std::move(_result).ok_value();                                             \
  })

#define RESULT_PROPAGATE_DISCARD(expression)                                   \
  {                                                                            \
    yabt::runtime::Result _result = expression;                                \
    if (_result.is_error()) {                                                  \
      return {yabt::runtime::detail::ErrSentinel{                              \
          std::move(_result).error_value()}};                                  \
    }                                                                          \
  }

#define RESULT_PROPAGATE_OK(expression)                                        \
  ({                                                                           \
    yabt::runtime::Result _result = expression;                                \
    if (_result.is_ok()) {                                                     \
      return {                                                                 \
          yabt::runtime::detail::OkSentinel{std::move(_result).ok_value()}};   \
    }                                                                          \
    std::move(_result).error_value();                                          \
  })

#define RESULT_UNWRAP(expression)                                              \
  ({                                                                           \
    yabt::runtime::Result _result = expression;                                \
    RESULT_VERIFY(_result.is_ok())                                             \
    std::move(_result).ok_value();                                             \
  })

#define RESULT_UNWRAP_OR(expression, alternative)                              \
  ({                                                                           \
    yabt::runtime::Result _result = expression;                                \
    auto unwrap_or = [&]() {                                                   \
      if (_result.is_ok()) {                                                   \
        return std::move(_result).ok_value();                                  \
      } else {                                                                 \
        return alternative;                                                    \
      }                                                                        \
    };                                                                         \
    unwrap_or();                                                               \
  })

#define RESULT_UNWRAP_OR_ELSE(expression, functor)                             \
  ({                                                                           \
    yabt::runtime::Result _result = expression;                                \
    auto unwrap_or = [&]() {                                                   \
      if (_result.is_ok()) {                                                   \
        return std::move(_result).ok_value();                                  \
      } else {                                                                 \
        return functor();                                                      \
      }                                                                        \
    };                                                                         \
    unwrap_or();                                                               \
  })

#define RESULT_UNWRAP_ERR(expression)                                          \
  ({                                                                           \
    yabt::runtime::Result _result = expression;                                \
    RESULT_VERIFY(_result.is_error())                                          \
    std::move(_result).error_value();                                          \
  })

#define RESULT_UNWRAP_ERR_OR(expression, alternative)                          \
  ({                                                                           \
    yabt::runtime::Result _result = expression;                                \
    auto unwrap_or = [&]() {                                                   \
      if (_result.is_error()) {                                                \
        return std::move(_result).error_value();                               \
      } else {                                                                 \
        return alternative;                                                    \
      }                                                                        \
    };                                                                         \
    unwrap_or();                                                               \
  })

#define RESULT_UNWRAP_ERR_OR_ELSE(expression, functor)                         \
  ({                                                                           \
    yabt::runtime::Result _result = expression;                                \
    auto unwrap_or = [&]() {                                                   \
      if (_result.is_error()) {                                                \
        return std::move(_result).error_value();                               \
      } else {                                                                 \
        return functor();                                                      \
      }                                                                        \
    };                                                                         \
    unwrap_or();                                                               \
  })

} // namespace yabt::runtime
