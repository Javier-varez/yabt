#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include "yabt/runtime/check.h"

namespace yabt::runtime {

namespace detail {

template <class T> struct [[nodiscard]] ErrSentinel {
  template <class... Args>
  explicit constexpr ErrSentinel(Args &&...args) noexcept
      : value{std::forward<Args>(args)...} {}
  T value;
};

template <class T> ErrSentinel(T) -> ErrSentinel<std::decay_t<T>>;

template <class T> struct [[nodiscard]] OkSentinel {
  template <class... Args>
  explicit constexpr OkSentinel(Args &&...args) noexcept
      : value{std::forward<Args>(args)...} {}
  T value;
};

template <class T> OkSentinel(T) -> OkSentinel<std::decay_t<T>>;

template <typename T>
concept NonVoid = !std::same_as<T, void>;

struct OkTag final {};
struct ErrTag final {};

template <class Ok, class Err> struct ResultPayload {
  constexpr ResultPayload() noexcept {}
  constexpr ~ResultPayload() noexcept {}

  union {
    Ok ok;
    Err err;
  };
};

template <class Ok> struct ResultPayload<Ok, void> {
  constexpr ResultPayload() noexcept {}
  constexpr ~ResultPayload() noexcept {}

  union {
    Ok ok;
  };
};

template <class Err> struct ResultPayload<void, Err> {
  constexpr ResultPayload() noexcept {}
  constexpr ~ResultPayload() noexcept {}

  union {
    Err err;
  };
};

template <> struct ResultPayload<void, void> {};

} // namespace detail

// NOTE: Probably worth adding a `context` method to Result<T, std::string> to
// add context to errors.
template <class Ok, class Err> class [[nodiscard]] Result final {
public:
  // Constructors used for propagating errors or values
  template <detail::NonVoid T = Ok>
  constexpr Result(detail::OkSentinel<T> val) noexcept
      : Result{detail::OkTag{}, std::move(val.value)} {}

  template <detail::NonVoid T = Err>
  constexpr Result(detail::ErrSentinel<T> val) noexcept
      : Result{detail::ErrTag{}, std::move(val.value)} {}

  /// Construct an Ok object with perfect forwarding
  template <class... Args>
  [[nodiscard]] static constexpr Result ok(Args &&...args) noexcept {
    if constexpr (std::is_same_v<void, Ok>) {
      static_assert(sizeof...(Args) == 0,
                    "Result::ok() does not take arguments for 'void'");
      Result res{};
      res.m_is_ok = true;
      return res;
    } else {
      return Result{detail::OkTag{}, std::forward<Args>(args)...};
    }
  }

  /// Construct an Err object with perfect forwarding
  template <class... Args>
  [[nodiscard]] static constexpr Result error(Args &&...args) noexcept {
    if constexpr (std::is_same_v<void, Err>) {
      static_assert(sizeof...(Args) == 0,
                    "Result::error() does not take arguments for 'void'");
      Result res{};
      res.m_is_ok = false;
      return res;
    } else {
      return Result{detail::ErrTag{}, std::forward<Args>(args)...};
    }
  }

  [[nodiscard]] constexpr bool is_error() const noexcept { return !m_is_ok; }
  [[nodiscard]] constexpr bool is_ok() const noexcept { return m_is_ok; }

  template <detail::NonVoid T = Ok>
  [[nodiscard]] constexpr const T &ok_value() const & noexcept {
    check(m_is_ok, "Attempted to unwrap ok from erroneous result");
    return m_payload.ok;
  }

  template <detail::NonVoid T = Ok>
  [[nodiscard]] constexpr T &ok_value() & noexcept {
    check(m_is_ok, "Attempted to unwrap ok from erroneous result");
    return m_payload.ok;
  }

  template <detail::NonVoid T = Ok>
  [[nodiscard]] constexpr T &&ok_value() && noexcept {
    check(m_is_ok, "Attempted to unwrap ok from erroneous result");
    return std::move(m_payload.ok);
  }

  template <detail::NonVoid T = Err>
  [[nodiscard]] constexpr const T &error_value() const & noexcept {
    check(!m_is_ok, "Attempted to unwrap error from successful result");
    return m_payload.err;
  }

  template <detail::NonVoid T = Err>
  [[nodiscard]] constexpr T &error_value() & noexcept {
    check(!m_is_ok, "Attempted to unwrap error from successful result");
    return m_payload.err;
  }

  template <detail::NonVoid T = Err>
  [[nodiscard]] constexpr T &&error_value() && noexcept {
    check(!m_is_ok, "Attempted to unwrap error from successful result");
    return std::move(m_payload.err);
  }

  template <typename Callable, detail::NonVoid T = Ok>
  [[nodiscard]] constexpr auto map_ok(Callable callable) const noexcept
      -> Result<decltype(std::declval<Callable>()(std::declval<Ok>())), Err> {
    using MappedOk = decltype(std::declval<Callable>()(std::declval<Ok>()));
    if (!is_ok()) {
      if constexpr (std::same_as<Err, void>) {
        return Result<MappedOk, Err>::error();
      } else {
        return Result<MappedOk, Err>::error(error_value());
      }
    }
    return Result<MappedOk, Err>::ok(callable(ok_value()));
  }

  template <typename Callable, detail::NonVoid T = Err>
  [[nodiscard]] constexpr auto map_err(Callable callable) const noexcept
      -> Result<Ok, decltype(std::declval<Callable>()(std::declval<Err>()))> {
    using MappedErr = decltype(std::declval<Callable>()(std::declval<Err>()));
    if (!is_error()) {
      if constexpr (std::same_as<Ok, void>) {
        return Result<Ok, MappedErr>::ok();
      } else {
        return Result<Ok, MappedErr>::ok(ok_value());
      }
    }
    return Result<Ok, MappedErr>::error(callable(error_value()));
  }

  constexpr ~Result() noexcept { release(); }

  constexpr Result(const Result &other) noexcept {
    m_is_ok = other.m_is_ok;
    if (m_is_ok) {
      if constexpr (!std::same_as<Ok, void>) {
        construct_ok(other.ok_value());
      }
    } else {
      if constexpr (!std::same_as<Err, void>) {
        construct_err(other.error_value());
      }
    }
  }

  constexpr Result(Result &&other) noexcept {
    m_is_ok = other.m_is_ok;
    if (m_is_ok) {
      if constexpr (!std::same_as<Ok, void>) {
        construct_ok(std::move(other).ok_value());
      }
    } else {
      if constexpr (!std::same_as<Err, void>)
        construct_err(std::move(other).error_value());
    }
  }

  constexpr Result &operator=(const Result &other) noexcept {
    if (&other == this)
      return *this;

    release();

    m_is_ok = other.m_is_ok;

    if (m_is_ok) {
      if constexpr (!std::same_as<Ok, void>)
        construct_ok(other.ok_value());
    } else {
      if constexpr (!std::same_as<Err, void>)
        construct_err(other.error_value());
    }
    return *this;
  }

  constexpr Result &operator=(Result &&other) noexcept {
    if (&other == this)
      return *this;

    release();

    m_is_ok = other.m_is_ok;
    if (m_is_ok) {
      if constexpr (!std::same_as<Ok, void>)
        construct_ok(std::move(other).ok_value());
    } else {
      if constexpr (!std::same_as<Err, void>)
        construct_err(std::move(other).error_value());
    }

    return *this;
  }

private:
  bool m_is_ok = false;
  [[no_unique_address]] detail::ResultPayload<Ok, Err> m_payload;

  constexpr Result() noexcept = default;

  template <class... Args>
  constexpr Result(detail::OkTag, Args &&...args) noexcept : m_is_ok{true} {
    construct_ok(std::forward<Args>(args)...);
  }

  template <class... Args>
  constexpr Result(detail::ErrTag, Args &&...args) noexcept : m_is_ok{false} {
    construct_err(std::forward<Args>(args)...);
  }

  template <class... Args> constexpr void construct_ok(Args &&...args) {
    if constexpr (!std::same_as<Ok, void>) {
      std::construct_at(std::addressof(m_payload.ok),
                        std::forward<Args>(args)...);
    }
  }

  template <class... Args> constexpr void construct_err(Args &&...args) {
    if constexpr (!std::same_as<Err, void>) {
      std::construct_at(std::addressof(m_payload.err),
                        std::forward<Args>(args)...);
    }
  }

  constexpr void release() {
    if (m_is_ok) {
      if constexpr (!std::is_same_v<void, Ok>) {
        std::destroy_at(std::addressof(m_payload.ok));
      }
    } else {
      if constexpr (!std::is_same_v<void, Err>) {
        std::destroy_at(std::addressof(m_payload.err));
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
