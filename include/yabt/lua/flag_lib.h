#pragma once

#include <limits>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "yabt/lua/module.h"
#include "yabt/runtime/result.h"

#include "lua.hpp"

namespace yabt::lua {

enum class FlagType {
  ENUM,
  INTEGER,
  STRING,
};

struct EnumFlagSpec {
  std::string name;
  std::string description;
  std::optional<std::string> default_value;
  std::vector<std::string> allowed_values;

  [[nodiscard]] FlagType type() const noexcept;
};

struct StringFlagSpec {
  std::string name;
  std::string description;
  std::optional<std::string> default_value;

  [[nodiscard]] FlagType type() const noexcept;
};

struct IntFlagRange {
  int min{std::numeric_limits<int>::min()};
  int max{std::numeric_limits<int>::max()};
};

struct IntFlagSpec {
  std::string name;
  std::string description;
  IntFlagRange range;
  std::optional<std::string> default_value;

  [[nodiscard]] FlagType type() const noexcept;
};

using FlagSpec = std::variant<EnumFlagSpec, StringFlagSpec, IntFlagSpec>;

struct RawFlag {
  std::string key;
  std::string value;
};

using FlagValue = std::variant<std::string, int>;

[[nodiscard]] std::string_view get_flag_name(const FlagSpec &) noexcept;
[[nodiscard]] FlagType get_flag_type(const FlagSpec &) noexcept;

// Flag registration library
class FlagLib : public LuaModule {
public:
  FlagLib() = default;

  FlagLib(const FlagLib &) = delete;
  FlagLib &operator=(const FlagLib &) = delete;

  FlagLib(FlagLib &&) = default;
  FlagLib &operator=(FlagLib &&) = default;

  [[nodiscard]] const std::unordered_map<std::string, FlagSpec> &
  flag_specs() const noexcept;

  [[nodiscard]] const std::unordered_map<std::string, FlagValue> &
  flag_values() const noexcept;

  [[nodiscard]] runtime::Result<void>
  resolve_flag_values(const std::span<const RawFlag> flags) noexcept;

  [[nodiscard]] runtime::Result<FlagValue>
  retrieve_value(FlagSpec *spec) const noexcept;

private:
  bool m_resolved{false};
  std::unordered_map<std::string, FlagSpec> m_registered_flags;
  std::unordered_map<std::string, FlagValue> m_resolved_flags;

  void register_in_engine(lua_State *const L) final;

  [[nodiscard]] runtime::Result<FlagSpec> register_flag(lua_State *const L);

  friend int l_register_flag(lua_State *const L);
};

} // namespace yabt::lua
