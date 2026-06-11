#include <yabt/runtime/error.h>
#include <yabt/runtime/result.h>

#include <format>
#include <gtest/gtest.h>
#include <source_location>
#include <string>
#include <string_view>

using yabt::runtime::Error;
using yabt::runtime::Result;

TEST(ErrorTest, ConstructionAndFormatting) {
  const auto location = std::source_location::current();
  const Error error{"root failure", location};

  EXPECT_EQ(error.message(), "root failure");
  ASSERT_EQ(error.context_chain().size(), 1U);
  EXPECT_EQ(error.context_chain()[0].message, "root failure");
  EXPECT_EQ(error.context_chain()[0].location.line(), location.line());
  EXPECT_EQ(error.location().line(), location.line());

  const std::string expected = std::format(
      R"(Error:
  ×─▶ root failure
      at {}:{}:{})",
      location.file_name(), location.line(), location.column());
  EXPECT_EQ(error.string(), expected);
  EXPECT_EQ(error.to_string(), expected);
  EXPECT_EQ(std::format("{}", error), expected);
}

TEST(ErrorTest, ContextChain) {
  const auto root_location = std::source_location::current();
  Error error{"root failure", root_location};
  const auto inner_location = std::source_location::current();
  error.add_context("while parsing module", inner_location);
  const auto outer_location = std::source_location::current();
  error.add_context("while loading workspace", outer_location);

  ASSERT_EQ(error.context_chain().size(), 3U);
  EXPECT_EQ(error.context_chain()[0].message, "root failure");
  EXPECT_EQ(error.context_chain()[0].location.line(), root_location.line());
  EXPECT_EQ(error.context_chain()[1].message, "while parsing module");
  EXPECT_EQ(error.context_chain()[1].location.line(), inner_location.line());
  EXPECT_EQ(error.context_chain()[2].message, "while loading workspace");
  EXPECT_EQ(error.context_chain()[2].location.line(), outer_location.line());

  const std::string expected = std::format(
      R"(Error:
  ×─▶ while loading workspace
  │   at {}:{}:{}
  ├─▶ while parsing module
  │   at {}:{}:{}
  ╰─▶ root failure
      at {}:{}:{})",
      outer_location.file_name(), outer_location.line(),
      outer_location.column(), inner_location.file_name(),
      inner_location.line(), inner_location.column(), root_location.file_name(),
      root_location.line(), root_location.column());
  EXPECT_EQ(error.string(), expected);
  EXPECT_EQ(std::format("{}", error), expected);
}

TEST(ErrorTest, WithContext) {
  const Error original{"root failure"};
  const Error copied = original.with_context("copied context");
  const Error moved = Error{"root failure"}.with_context("moved context");

  ASSERT_EQ(original.context_chain().size(), 1U);
  EXPECT_EQ(original.context_chain()[0].message, "root failure");
  ASSERT_EQ(copied.context_chain().size(), 2U);
  EXPECT_EQ(copied.context_chain()[0].message, "root failure");
  EXPECT_EQ(copied.context_chain()[1].message, "copied context");
  ASSERT_EQ(moved.context_chain().size(), 2U);
  EXPECT_EQ(moved.context_chain()[0].message, "root failure");
  EXPECT_EQ(moved.context_chain()[1].message, "moved context");
}

TEST(ErrorTest, DefaultResultErrorType) {
  const Result<int> result = Result<int>::error("root failure");

  ASSERT_TRUE(result.is_error());
  EXPECT_EQ(result.error_value().message(), "root failure");
}
