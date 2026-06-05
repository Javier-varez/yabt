#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "yabt/lua/flag_lib.h"
#include "yabt/lua/lua_engine.h"

namespace yabt::lua::tests {
using ::testing::ElementsAre;

class FlagLibTests : public testing::Test {
protected:
  FlagLibTests()
      : m_engine{[]() {
          std::filesystem::path ws_root{"root"};
          runtime::Result<LuaEngine, std::string> engine_result =
              LuaEngine::construct(ws_root);

          runtime::check(engine_result.is_ok(), "Engine construction failed");
          return std::move(engine_result.ok_value());
        }()} {
    m_engine.register_lua_module(m_flag_lib);
  }

  LuaEngine m_engine;
  FlagLib m_flag_lib;
};

TEST_F(FlagLibTests, ImportLib) {
  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'
  )");
  ASSERT_TRUE(result.is_ok());
}

TEST_F(FlagLibTests, RegisterFlagNameMissing) {
  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'

    flags.register {
      description = 'The C/C++ toolchain to use during the build',
      type = flags.StringFlag,
    }
  )");

  ASSERT_TRUE(result.is_error());
  ASSERT_EQ(result.error_value(),
            "failed to run file: Flag does not have a name");
}

TEST_F(FlagLibTests, RegisterStringFlag) {
  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'

    flags.register {
      name = 'cc_toolchain',
      description = 'The C/C++ toolchain to use during the build',
      type = flags.StringFlag,
    }
  )");

  ASSERT_TRUE(result.is_ok());

  auto flag_specs = m_flag_lib.flag_specs();
  ASSERT_TRUE(flag_specs.contains("cc_toolchain"));
  auto val = flag_specs["cc_toolchain"];

  ASSERT_EQ(get_flag_type(val), FlagType::STRING);
  ASSERT_TRUE(std::holds_alternative<StringFlagSpec>(val));
  StringFlagSpec &flag = std::get<StringFlagSpec>(val);
  EXPECT_EQ(flag.description, "The C/C++ toolchain to use during the build");
  EXPECT_EQ(flag.name, "cc_toolchain");
}

TEST_F(FlagLibTests, RegisterDuplicateFlag) {
  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'

    flags.register {
      name = 'cc_toolchain',
      description = 'The C/C++ toolchain to use during the build',
      type = flags.StringFlag,
    }
    flags.register {
      name = 'cc_toolchain',
      type = flags.EnumFlag,
      values = { 't1' },
    }
  )");

  ASSERT_TRUE(result.is_error());
  EXPECT_EQ(result.error_value(),
            "failed to run file: Duplicated flag spec for name: cc_toolchain");
}

TEST_F(FlagLibTests, RegisterIntFlagNoRangeGiven) {
  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'

    flags.register {
      name = 'timeout_ms',
      description = 'A timeout',
      type = flags.IntFlag,
    }
  )");

  ASSERT_TRUE(result.is_ok());

  auto flag_specs = m_flag_lib.flag_specs();
  ASSERT_TRUE(flag_specs.contains("timeout_ms"));
  auto val = flag_specs["timeout_ms"];

  ASSERT_EQ(get_flag_type(val), FlagType::INTEGER);
  ASSERT_TRUE(std::holds_alternative<IntFlagSpec>(val));
  IntFlagSpec &flag = std::get<IntFlagSpec>(val);
  EXPECT_EQ(flag.description, "A timeout");
  EXPECT_EQ(flag.name, "timeout_ms");
  EXPECT_EQ(flag.range.min, std::numeric_limits<int>::min());
  EXPECT_EQ(flag.range.max, std::numeric_limits<int>::max());
}

TEST_F(FlagLibTests, RegisterIntFlagWithRange) {

  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'

    flags.register {
      name = 'timeout_ms',
      description = 'A timeout',
      type = flags.IntFlag,
      range = {
        min = 0,
        max = 10,
      }
    }
  )");

  ASSERT_TRUE(result.is_ok());

  auto flag_specs = m_flag_lib.flag_specs();
  ASSERT_TRUE(flag_specs.contains("timeout_ms"));
  auto val = flag_specs["timeout_ms"];

  ASSERT_EQ(get_flag_type(val), FlagType::INTEGER);
  ASSERT_TRUE(std::holds_alternative<IntFlagSpec>(val));
  IntFlagSpec &flag = std::get<IntFlagSpec>(val);
  EXPECT_EQ(flag.description, "A timeout");
  EXPECT_EQ(flag.name, "timeout_ms");
  EXPECT_EQ(flag.range.min, 0);
  EXPECT_EQ(flag.range.max, 10);
}

TEST_F(FlagLibTests, RegisterIntFlagInvalidRange) {

  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'

    flags.register {
      name = 'timeout_ms',
      description = 'A timeout',
      type = flags.IntFlag,
      range = {
        min = 11,
        max = 10,
      }
    }
  )");

  ASSERT_TRUE(result.is_error());
  EXPECT_EQ(result.error_value(), "failed to run file: Invalid range for yabt "
                                  "flag \"timeout_ms\": [11, 10]");
}

TEST_F(FlagLibTests, RegisterEnumFlag) {
  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'

    flags.register {
      name = 'build_type',
      description = 'Build type',
      type = flags.EnumFlag,
      values = {
        'debug',
        'release',
      }
    }
  )");

  ASSERT_TRUE(result.is_ok());

  auto flag_specs = m_flag_lib.flag_specs();
  ASSERT_TRUE(flag_specs.contains("build_type"));
  auto val = flag_specs["build_type"];

  ASSERT_EQ(get_flag_type(val), FlagType::ENUM);
  ASSERT_TRUE(std::holds_alternative<EnumFlagSpec>(val));
  EnumFlagSpec &flag = std::get<EnumFlagSpec>(val);
  EXPECT_EQ(flag.description, "Build type");
  EXPECT_EQ(flag.name, "build_type");
  EXPECT_THAT(flag.allowed_values, ElementsAre("debug", "release"));
}

TEST_F(FlagLibTests, RegisterEnumFlagWithoutValues) {
  const runtime::Result result = m_engine.exec_string(R"(
    local flags = require 'yabt.core.flags'

    flags.register {
      name = 'build_type',
      description = 'Build type',
      type = flags.EnumFlag,
      values = { }
    }
  )");

  ASSERT_TRUE(result.is_error());
  EXPECT_EQ(
      result.error_value(),
      "failed to run file: Enum flag spec \"build_type\" has no valid values");
}

TEST_F(FlagLibTests, TestResolveFlagValues) {
  {
    const runtime::Result result = m_engine.exec_string(R"(
      local flags = require 'yabt.core.flags'

      flags.register {
        name = 'build_type',
        description = 'Build type',
        type = flags.EnumFlag,
        values = { 'debug', 'release' }
      }

      flags.register {
        name = 'cc_toolchain',
        description = 'C++ toolchain',
        type = flags.StringFlag,
        default_value = 'gcc'
      }
    )");
    ASSERT_TRUE(result.is_ok());
  }

  ASSERT_TRUE(m_flag_lib
                  .resolve_flag_values(std::array{
                      RawFlag{.key{"build_type"}, .value{"release"}}})
                  .is_ok());

  auto flags = m_flag_lib.flag_values();
  ASSERT_TRUE(flags.contains("build_type"));
  EXPECT_EQ(flags["build_type"], FlagValue{std::string{"release"}});

  ASSERT_TRUE(flags.contains("cc_toolchain"));
  EXPECT_EQ(flags["cc_toolchain"], FlagValue{std::string{"gcc"}});
}

TEST_F(FlagLibTests, RequiredFlagNotGiven) {
  {
    const runtime::Result result = m_engine.exec_string(R"(
      local flags = require 'yabt.core.flags'

      flags.register {
        name = 'build_type',
        description = 'Build type',
        type = flags.EnumFlag,
        values = { 'debug', 'release' }
      }

      flags.register {
        name = 'cc_toolchain',
        description = 'C++ toolchain',
        type = flags.StringFlag,
      }
    )");
    ASSERT_TRUE(result.is_ok());
  }

  const auto result = m_flag_lib.resolve_flag_values(
      std::array{RawFlag{.key{"build_type"}, .value{"release"}}});
  ASSERT_TRUE(result.is_error());
  EXPECT_EQ(result.error_value(), "No value given for flag \"cc_toolchain\"");
}

TEST_F(FlagLibTests, TestRetrieveValue) {
  {
    const runtime::Result result = m_engine.exec_string(R"(
      local flags = require 'yabt.core.flags'

      build_type = flags.register {
        name = 'build_type',
        description = 'Build type',
        type = flags.EnumFlag,
        values = { 'debug', 'release' }
      }
    )");
    ASSERT_TRUE(result.is_ok());
  }

  ASSERT_TRUE(m_flag_lib
                  .resolve_flag_values(std::array{
                      RawFlag{.key{"build_type"}, .value{"release"}}})
                  .is_ok());

  {
    const runtime::Result result = m_engine.exec_string(R"(
      local meta = getmetatable(build_type)
      local value = build_type:value()
      assert(value == 'release', "Invalid flag value")
    )");
    ASSERT_TRUE(result.is_ok());
  }
}

} // namespace yabt::lua::tests
