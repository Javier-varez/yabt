#include "yabt/runtime/result.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <source_location>
#include <utility>

using yabt::runtime::Error;
using yabt::runtime::Result;

using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Field;
using ::testing::ResultOf;

namespace {

class MoveableOnly {
public:
  constexpr MoveableOnly(int val) : m_val{val} {};
  MoveableOnly(const MoveableOnly &) = delete;
  constexpr MoveableOnly(MoveableOnly &&) = default;
  MoveableOnly &operator=(const MoveableOnly &) = delete;
  constexpr MoveableOnly &operator=(MoveableOnly &&) = default;

  [[nodiscard]] constexpr int val() const { return m_val; }

private:
  int m_val{};
};

class Tracked {
public:
  explicit Tracked(int *dtor_count) : m_dtor_count{dtor_count} {}
  Tracked(const Tracked &other) : m_dtor_count{other.m_dtor_count} {}
  Tracked(Tracked &&other) noexcept
      : m_dtor_count{std::exchange(other.m_dtor_count, nullptr)} {}
  Tracked &operator=(const Tracked &) = delete;
  Tracked &operator=(Tracked &&) = delete;
  ~Tracked() {
    if (m_dtor_count)
      ++(*m_dtor_count);
  }

private:
  int *m_dtor_count;
};

auto ErrorEntryIs(std::string_view message, std::source_location location) {
  return AllOf(
      Field("message", &Error::Entry::message, Eq(std::string{message})),
      Field("location", &Error::Entry::location,
            ResultOf(
                "line",
                [](const std::source_location &actual_location) {
                  return actual_location.line();
                },
                Eq(location.line()))));
}

consteval bool constexpr_construction_and_access() {
  Result<int, int> ok = Result<int, int>::ok(21);
  ok.ok_value() += 1;

  const Result<int, int> error = Result<int, int>::error(17);
  return ok.is_ok() && !ok.is_error() && ok.ok_value() == 22 &&
         error.is_error() && !error.is_ok() && error.error_value() == 17;
}

consteval bool constexpr_copy_move_and_assignment() {
  const Result<int, int> original = Result<int, int>::ok(42);
  Result<int, int> copy{original};
  if (!copy.is_ok() || copy.ok_value() != 42)
    return false;

  Result<int, int> moved{std::move(copy)};
  if (!moved.is_ok() || moved.ok_value() != 42)
    return false;

  Result<int, int> target = Result<int, int>::error(1);
  target = moved;
  if (!target.is_ok() || target.ok_value() != 42)
    return false;

  Result<int, int> source = Result<int, int>::error(7);
  target = std::move(source);
  return target.is_error() && target.error_value() == 7;
}

consteval bool constexpr_void_results() {
  const Result<void, void> ok = Result<void, void>::ok();
  const Result<void, void> error = Result<void, void>::error();
  const Result<void, int> payload_error = Result<void, int>::error(3);
  const Result<int, void> payload_ok = Result<int, void>::ok(5);

  return ok.is_ok() && error.is_error() && payload_error.is_error() &&
         payload_error.error_value() == 3 && payload_ok.is_ok() &&
         payload_ok.ok_value() == 5;
}

consteval bool constexpr_map_operations() {
  const Result<int, int> ok = Result<int, int>::ok(11);
  const Result<int, int> mapped_ok = ok.map_ok([](int val) { return val * 2; });
  if (!mapped_ok.is_ok() || mapped_ok.ok_value() != 22)
    return false;

  const Result<int, int> error = Result<int, int>::error(4);
  const Result<int, int> mapped_error =
      error.map_err([](int val) { return val + 3; });
  return mapped_error.is_error() && mapped_error.error_value() == 7;
}

consteval bool constexpr_move_only_result() {
  Result<MoveableOnly, void> result = Result<MoveableOnly, void>::ok(9);
  Result<MoveableOnly, void> moved{std::move(result)};
  MoveableOnly value = std::move(moved).ok_value();
  return value.val() == 9;
}

} // namespace

TEST(ResultTest, ConstexprOperations) {
  EXPECT_TRUE(constexpr_construction_and_access());
  EXPECT_TRUE(constexpr_copy_move_and_assignment());
  EXPECT_TRUE(constexpr_void_results());
  EXPECT_TRUE(constexpr_map_operations());
  EXPECT_TRUE(constexpr_move_only_result());
}

TEST(ResultTest, VoidConstruction) {
  static_assert(sizeof(Result<void, void>) == sizeof(bool));
  {
    const Result<void, void> result = Result<void, void>::ok();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_error());
  }

  {
    const Result<void, void> result = Result<void, void>::error();
    EXPECT_TRUE(result.is_error());
    EXPECT_FALSE(result.is_ok());
  }

  static_assert(sizeof(Result<void, int>) == sizeof(int) * 2);
  {
    const Result<void, int> result = Result<void, int>::ok();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_error());
  }

  static_assert(sizeof(Result<int, void>) == sizeof(int) * 2);
  {
    const Result<int, void> result = Result<int, void>::error();
    EXPECT_TRUE(result.is_error());
    EXPECT_FALSE(result.is_ok());
  }
}

TEST(ResultTest, ConstructionFromInnerTrivial) {
  {
    const Result<int, int> result = Result<int, int>::error(123);
    EXPECT_TRUE(result.is_error());
    EXPECT_FALSE(result.is_ok());
    EXPECT_EQ(result.error_value(), 123);
  }

  {
    const Result<int, int> result = Result<int, int>::ok(132);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_error());
    EXPECT_EQ(result.ok_value(), 132);
  }
}

TEST(ResultTest, DefaultErrorLocationPointsToResultErrorCallSite) {
  const auto location = std::source_location::current();
  const Result<int> result = Result<int>::error("root failure");

  ASSERT_TRUE(result.is_error());
  EXPECT_EQ(result.error_value().message(), "root failure");
  EXPECT_EQ(result.error_value().location().file_name(), location.file_name());
  EXPECT_EQ(result.error_value().location().line(), location.line() + 1);
}

TEST(ResultTest, DefaultErrorPreservesExplicitLocation) {
  const auto location = std::source_location::current();
  const Result<int> result = Result<int>::error("root failure", location);

  ASSERT_TRUE(result.is_error());
  EXPECT_EQ(result.error_value().message(), "root failure");
  EXPECT_EQ(result.error_value().location().file_name(), location.file_name());
  EXPECT_EQ(result.error_value().location().line(), location.line());
}

TEST(ResultTest, ExplicitErrorObjectPreservesLocation) {
  const auto location = std::source_location::current();
  const Result<int> result =
      Result<int>::error(Error{"root failure", location});

  ASSERT_TRUE(result.is_error());
  EXPECT_EQ(result.error_value().message(), "root failure");
  EXPECT_EQ(result.error_value().location().file_name(), location.file_name());
  EXPECT_EQ(result.error_value().location().line(), location.line());
}

TEST(ResultTest, WithContextAddsContextToDefaultError) {
  const auto root_location = std::source_location::current();
  const Result<int> original =
      Result<int>::error("root failure", root_location);
  const auto context_location = std::source_location::current();
  const Result<int> result =
      original.with_context("while loading workspace", context_location);

  ASSERT_TRUE(result.is_error());
  EXPECT_THAT(original.error_value().context_chain(),
              ElementsAre(ErrorEntryIs("root failure", root_location)));
  EXPECT_THAT(
      result.error_value().context_chain(),
      ElementsAre(ErrorEntryIs("root failure", root_location),
                  ErrorEntryIs("while loading workspace", context_location)));
}

TEST(ResultTest, WithContextPreservesOkResult) {
  const Result<int> copied = Result<int>::ok(42).with_context("unused context");
  Result<MoveableOnly> moveable = Result<MoveableOnly>::ok(17);
  Result<MoveableOnly> moved =
      std::move(moveable).with_context("unused context");

  ASSERT_TRUE(copied.is_ok());
  EXPECT_EQ(copied.ok_value(), 42);
  ASSERT_TRUE(moved.is_ok());
  EXPECT_EQ(moved.ok_value().val(), 17);
}

TEST(ResultTest, WithContextMovesErrorResult) {
  const auto root_location = std::source_location::current();
  const auto context_location = std::source_location::current();
  const Result<int> result =
      Result<int>::error("root failure", root_location)
          .with_context("while syncing modules", context_location);

  ASSERT_TRUE(result.is_error());
  EXPECT_THAT(
      result.error_value().context_chain(),
      ElementsAre(ErrorEntryIs("root failure", root_location),
                  ErrorEntryIs("while syncing modules", context_location)));
}

TEST(ResultTest, ConstructionFromInnerMovable) {
  static_assert(sizeof(Result<MoveableOnly, void>) == sizeof(int) * 2);
  {
    const Result<void, MoveableOnly> result =
        Result<void, MoveableOnly>::error(MoveableOnly{123});
    EXPECT_TRUE(result.is_error());
    EXPECT_FALSE(result.is_ok());
    EXPECT_EQ(result.error_value().val(), 123);
  }

  {
    const Result<MoveableOnly, void> result =
        Result<MoveableOnly, void>::ok(132);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_error());
    EXPECT_EQ(result.ok_value().val(), 132);
  }
}

TEST(ResultTest, MoveTrivial) {
  // move constructor
  {
    Result<int, int> original = Result<int, int>::ok(42);
    Result<int, int> moved{std::move(original)};
    EXPECT_TRUE(moved.is_ok());
    EXPECT_EQ(moved.ok_value(), 42);
  }
  {
    Result<int, int> original = Result<int, int>::error(99);
    Result<int, int> moved{std::move(original)};
    EXPECT_TRUE(moved.is_error());
    EXPECT_EQ(moved.error_value(), 99);
  }
  // move assignment
  {
    Result<int, int> original = Result<int, int>::ok(42);
    Result<int, int> target = Result<int, int>::error(0);
    target = std::move(original);
    EXPECT_TRUE(target.is_ok());
    EXPECT_EQ(target.ok_value(), 42);
  }
  {
    Result<int, int> original = Result<int, int>::error(99);
    Result<int, int> target = Result<int, int>::ok(0);
    target = std::move(original);
    EXPECT_TRUE(target.is_error());
    EXPECT_EQ(target.error_value(), 99);
  }
}

TEST(ResultTest, MoveNonCopyable) {
  // move constructor
  {
    Result<MoveableOnly, void> original = Result<MoveableOnly, void>::ok(42);
    Result<MoveableOnly, void> moved{std::move(original)};
    EXPECT_TRUE(moved.is_ok());
    EXPECT_EQ(moved.ok_value().val(), 42);
  }
  {
    Result<void, MoveableOnly> original =
        Result<void, MoveableOnly>::error(MoveableOnly{99});
    Result<void, MoveableOnly> moved{std::move(original)};
    EXPECT_TRUE(moved.is_error());
    EXPECT_EQ(moved.error_value().val(), 99);
  }
  // move assignment
  {
    Result<MoveableOnly, void> original = Result<MoveableOnly, void>::ok(42);
    Result<MoveableOnly, void> target = Result<MoveableOnly, void>::error();
    target = std::move(original);
    EXPECT_TRUE(target.is_ok());
    EXPECT_EQ(target.ok_value().val(), 42);
  }
  {
    Result<void, MoveableOnly> original =
        Result<void, MoveableOnly>::error(MoveableOnly{99});
    Result<void, MoveableOnly> target = Result<void, MoveableOnly>::ok();
    target = std::move(original);
    EXPECT_TRUE(target.is_error());
    EXPECT_EQ(target.error_value().val(), 99);
  }
}

TEST(ResultTest, CopyTrivial) {
  // copy constructor
  {
    const Result<int, int> original = Result<int, int>::ok(42);
    Result<int, int> copy{original};
    EXPECT_TRUE(copy.is_ok());
    EXPECT_EQ(copy.ok_value(), 42);
  }
  {
    const Result<int, int> original = Result<int, int>::error(99);
    Result<int, int> copy{original};
    EXPECT_TRUE(copy.is_error());
    EXPECT_EQ(copy.error_value(), 99);
  }
  // copy assignment
  {
    const Result<int, int> original = Result<int, int>::ok(42);
    Result<int, int> target = Result<int, int>::error(0);
    target = original;
    EXPECT_TRUE(target.is_ok());
    EXPECT_EQ(target.ok_value(), 42);
  }
  {
    const Result<int, int> original = Result<int, int>::error(99);
    Result<int, int> target = Result<int, int>::ok(0);
    target = original;
    EXPECT_TRUE(target.is_error());
    EXPECT_EQ(target.error_value(), 99);
  }
}

TEST(ResultTest, AssignmentFreesOldValue) {
  // copy assignment frees old ok value
  {
    int dtor_count = 0;
    Result<Tracked, void> target =
        Result<Tracked, void>::ok(Tracked{&dtor_count});
    const Result<Tracked, void> other =
        Result<Tracked, void>::ok(Tracked{&dtor_count});
    EXPECT_EQ(dtor_count, 0);
    target = other;
    EXPECT_EQ(dtor_count, 1);
  }
  // copy assignment frees old error value
  {
    int dtor_count = 0;
    Result<void, Tracked> target =
        Result<void, Tracked>::error(Tracked{&dtor_count});
    const Result<void, Tracked> other =
        Result<void, Tracked>::error(Tracked{&dtor_count});
    EXPECT_EQ(dtor_count, 0);
    target = other;
    EXPECT_EQ(dtor_count, 1);
  }
  // move assignment frees old ok value
  {
    int dtor_count = 0;
    Result<Tracked, void> target =
        Result<Tracked, void>::ok(Tracked{&dtor_count});
    Result<Tracked, void> other =
        Result<Tracked, void>::ok(Tracked{&dtor_count});
    EXPECT_EQ(dtor_count, 0);
    target = std::move(other);
    EXPECT_EQ(dtor_count, 1);
  }
  // move assignment frees old error value
  {
    int dtor_count = 0;
    Result<void, Tracked> target =
        Result<void, Tracked>::error(Tracked{&dtor_count});
    Result<void, Tracked> other =
        Result<void, Tracked>::error(Tracked{&dtor_count});
    EXPECT_EQ(dtor_count, 0);
    target = std::move(other);
    EXPECT_EQ(dtor_count, 1);
  }
}

TEST(ResultTest, ValueAccessSpecializations) {
  // ok_value const& overload
  {
    const Result<int, int> result = Result<int, int>::ok(42);
    const int &val = result.ok_value();
    EXPECT_EQ(val, 42);
  }
  // ok_value & overload — allows in-place mutation
  {
    Result<int, int> result = Result<int, int>::ok(42);
    result.ok_value() = 99;
    EXPECT_EQ(result.ok_value(), 99);
  }
  // ok_value && overload — moves value out
  {
    Result<MoveableOnly, void> result = Result<MoveableOnly, void>::ok(42);
    MoveableOnly val = std::move(result).ok_value();
    EXPECT_EQ(val.val(), 42);
  }
  // error_value const& overload
  {
    const Result<int, int> result = Result<int, int>::error(99);
    const int &val = result.error_value();
    EXPECT_EQ(val, 99);
  }
  // error_value & overload — allows in-place mutation
  {
    Result<int, int> result = Result<int, int>::error(99);
    result.error_value() = 42;
    EXPECT_EQ(result.error_value(), 42);
  }
  // error_value && overload — moves value out
  {
    Result<void, MoveableOnly> result =
        Result<void, MoveableOnly>::error(MoveableOnly{99});
    MoveableOnly val = std::move(result).error_value();
    EXPECT_EQ(val.val(), 99);
  }
}

TEST(ResultTest, InnerDestructorCalledOnDestruction) {
  // Tracked in ok slot, result holds ok: destructor IS called
  {
    int dtor_count = 0;
    {
      Result<Tracked, int> result =
          Result<Tracked, int>::ok(Tracked{&dtor_count});
      EXPECT_EQ(dtor_count, 0);
    }
    EXPECT_EQ(dtor_count, 1);
  }
  // Tracked in ok slot, result holds error: destructor is NOT called
  {
    int dtor_count = 0;
    {
      Result<Tracked, int> result = Result<Tracked, int>::error(42);
      EXPECT_EQ(dtor_count, 0);
    }
    EXPECT_EQ(dtor_count, 0);
  }
  // Tracked in error slot, result holds error: destructor IS called
  {
    int dtor_count = 0;
    {
      Result<int, Tracked> result =
          Result<int, Tracked>::error(Tracked{&dtor_count});
      EXPECT_EQ(dtor_count, 0);
    }
    EXPECT_EQ(dtor_count, 1);
  }
  // Tracked in error slot, result holds ok: destructor is NOT called
  {
    int dtor_count = 0;
    {
      Result<int, Tracked> result = Result<int, Tracked>::ok(42);
      EXPECT_EQ(dtor_count, 0);
    }
    EXPECT_EQ(dtor_count, 0);
  }
}

TEST(ResultTest, SelfAssignment) {
  // copy self-assignment preserves value and does not double-free
  {
    int dtor_count = 0;
    Result<Tracked, void> result =
        Result<Tracked, void>::ok(Tracked{&dtor_count});
    result = static_cast<const Result<Tracked, void> &>(result);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(dtor_count, 0);
  }
  {
    int dtor_count = 0;
    Result<void, Tracked> result =
        Result<void, Tracked>::error(Tracked{&dtor_count});
    result = static_cast<const Result<void, Tracked> &>(result);
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(dtor_count, 0);
  }
  // move self-assignment preserves value and does not double-free
  {
    int dtor_count = 0;
    Result<Tracked, void> result =
        Result<Tracked, void>::ok(Tracked{&dtor_count});
    auto &alias = result;
    result = std::move(alias);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(dtor_count, 0);
  }
  {
    int dtor_count = 0;
    Result<void, Tracked> result =
        Result<void, Tracked>::error(Tracked{&dtor_count});
    auto &alias = result;
    result = std::move(alias);
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(dtor_count, 0);
  }
}

TEST(ResultTest, MapOk) {
  // callable is applied when ok
  {
    const Result<int, int> result = Result<int, int>::ok(42);
    const Result<int, int> mapped = result.map_ok([](int v) { return v * 2; });
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.ok_value(), 84);
  }
  // callable is NOT applied when error; error value is passed through unchanged
  {
    const Result<int, int> result = Result<int, int>::error(99);
    int call_count = 0;
    const Result<int, int> mapped = result.map_ok([&](int v) {
      ++call_count;
      return v * 2;
    });
    EXPECT_TRUE(mapped.is_error());
    EXPECT_EQ(mapped.error_value(), 99);
    EXPECT_EQ(call_count, 0);
  }
  // callable may transform the ok type
  {
    const Result<int, int> result = Result<int, int>::ok(5);
    const Result<double, int> mapped =
        result.map_ok([](int v) -> double { return v * 1.5; });
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_DOUBLE_EQ(mapped.ok_value(), 7.5);
  }
}

TEST(ResultTest, MapErr) {
  // callable is applied when error
  {
    const Result<int, int> result = Result<int, int>::error(42);
    const Result<int, int> mapped = result.map_err([](int v) { return v * 2; });
    EXPECT_TRUE(mapped.is_error());
    EXPECT_EQ(mapped.error_value(), 84);
  }
  // callable is NOT applied when ok; ok value is passed through unchanged
  {
    const Result<int, int> result = Result<int, int>::ok(42);
    int call_count = 0;
    const Result<int, int> mapped = result.map_err([&](int v) {
      ++call_count;
      return v * 2;
    });
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.ok_value(), 42);
    EXPECT_EQ(call_count, 0);
  }
  // callable may transform the error type
  {
    const Result<int, int> result = Result<int, int>::error(5);
    const Result<int, double> mapped =
        result.map_err([](int v) -> double { return v * 1.5; });
    EXPECT_TRUE(mapped.is_error());
    EXPECT_DOUBLE_EQ(mapped.error_value(), 7.5);
  }
}
