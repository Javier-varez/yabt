#include "yabt/lua/flag_lib.h"

#include <cstring>
#include <format>
#include <iterator>
#include <span>
#include <string>
#include <variant>

#include "yabt/lua/utils.h"

#include "lua.hpp"

namespace yabt::lua {

namespace {

struct FlatFlagSpec {
  std::optional<std::string> name;
  void *type;
  std::optional<std::string> description;
  std::optional<std::vector<std::string>> values;
  std::optional<IntFlagRange> range;
  std::optional<std::string> default_value;
};

} // namespace

LUA_STRUCT_PARSE_SPEC_DEF(                             //
    ::yabt::lua::FlatFlagSpec,                         //
    (std::optional<std::string>, name),                //
    (void *, type),                                    //
    (std::optional<std::string>, description),         //
    (std::optional<std::vector<std::string>>, values), //
    (std::optional<IntFlagRange>, range),              //
    (std::optional<std::string>, default_value)        //
);

LUA_STRUCT_PARSE_SPEC_DEF(     //
    ::yabt::lua::IntFlagRange, //
    (int, min),                //
    (int, max)                 //
);

namespace {

int registry_key;

void init_registry(lua_State *const L, FlagLib *data) {
  if (L == nullptr) {
    return;
  }

  StackGuard g{L};
  lua_pushlightuserdata(L, &registry_key);
  lua_pushlightuserdata(L, data);
  lua_settable(L, LUA_REGISTRYINDEX);
}

[[nodiscard]] FlagLib *get_lib_from_registry(lua_State *const L) {
  lua_pushlightuserdata(L, &registry_key);
  lua_gettable(L, LUA_REGISTRYINDEX);
  FlagLib *lib = static_cast<FlagLib *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  runtime::check(
      lib != nullptr,
      "BUG: Invalid pointer to FlagLib. Are you sure it is registered?");
  return lib;
}

const char *FLAG_HANDLE = "FLAG_HANDLE";

struct PushFlag {
  lua_State *L;

  void operator()(int value) { lua_pushinteger(L, value); }

  void operator()(const std::string &value) {
    lua_pushstring(L, value.c_str());
  }
};

int l_flag_handle_value(lua_State *L) {
  if (lua_gettop(L) != 1 || !lua_isuserdata(L, -1)) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, "Expected flag handle");
    lua_error(L);
  }

  FlagSpec *udata =
      static_cast<FlagSpec *>(luaL_checkudata(L, -1, FLAG_HANDLE));
  if (udata == nullptr) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, "Invalid flag handle");
    lua_error(L);
  }

  FlagLib *const flag_lib = get_lib_from_registry(L);
  runtime::Result<FlagValue, std::string> value =
      flag_lib->retrieve_value(udata);
  if (value.is_error()) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, value.error_value().c_str());
    lua_error(L);
  }

  std::visit(PushFlag{L}, value.ok_value());
  return 1;
}
int l_flag_handle_newindex(lua_State *L) {
  lua_pop(L, lua_gettop(L));
  lua_pushstring(L, "Cannot modify flag handle");
  lua_error(L);
  return 0;
}

[[nodiscard]] int l_gc(lua_State *const L) {
  FlagSpec *ud = static_cast<FlagSpec *>(luaL_checkudata(L, -1, FLAG_HANDLE));
  luaL_argcheck(L, ud != nullptr, 1, "flag expected");
  ud->~FlagSpec();
  lua_pop(L, 1);
  return 0;
}

static const luaL_Reg flag_handle_meta_functions[]{
    {"__newindex", l_flag_handle_newindex},
    {"__gc", l_gc}, //
    {nullptr, nullptr},
};

static const luaL_Reg flag_handle_functions[]{
    {"value", l_flag_handle_value},
    {nullptr, nullptr},
};

void init_flag_handle_metatable(lua_State *const L) {
  StackGuard g{L};
  luaL_newmetatable(L, FLAG_HANDLE);
  luaL_register(L, nullptr, flag_handle_meta_functions);

  // Make __index table
  lua_pushstring(L, "__index");
  lua_newtable(L);
  luaL_register(L, nullptr, flag_handle_functions);
  lua_settable(L, -3);
  lua_pop(L, 1);
}

} // namespace

int l_register_flag(lua_State *const L) {
  StackGuard g{L};
  if (lua_gettop(L) != 1) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, "Unexpected number of arguments given to register_flag");
    lua_error(L);
  }

  FlagLib *const flag_lib = get_lib_from_registry(L);
  const auto result = flag_lib->register_flag(L);
  if (result.is_error()) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, result.error_value().c_str());
    lua_error(L);
  }
  lua_pop(L, 1);

  void *const userdata = lua_newuserdata(L, sizeof(FlagSpec));
  new (userdata) FlagSpec{result.ok_value()};
  luaL_setmetatable(L, FLAG_HANDLE);
  return 1;
}

namespace {

int string_flag_key;
int int_flag_key;
int enum_flag_key;

[[nodiscard]] runtime::Result<FlagType, std::string>
lua_flag_key_to_type(void *key) noexcept {
  if (key == &int_flag_key) {
    return runtime::Result<FlagType, std::string>::ok(FlagType::INTEGER);
  } else if (key == &string_flag_key) {
    return runtime::Result<FlagType, std::string>::ok(FlagType::STRING);
  } else if (key == &enum_flag_key) {
    return runtime::Result<FlagType, std::string>::ok(FlagType::ENUM);
  }
  return runtime::Result<FlagType, std::string>::error(
      "Invalid flag type! Expected one of `flags.StringFlag`, `flags.EnumFlag` "
      "or `flags.IntFlag`");
}

[[nodiscard]] runtime::Result<FlagValue, std::string>
resolve_flag(const StringFlagSpec &, const RawFlag &raw_flag) noexcept {
  return runtime::Result<FlagValue, std::string>::ok(raw_flag.value);
}

[[nodiscard]] runtime::Result<FlagValue, std::string>
resolve_flag(const EnumFlagSpec &spec, const RawFlag &raw_flag) noexcept {
  bool found = false;
  for (const std::string &allowed_val : spec.allowed_values) {
    if (allowed_val == raw_flag.value) {
      found = true;
      break;
    }
  }
  if (!found) {
    std::string error_msg{};
    std::format_to(
        std::back_inserter(error_msg),
        "Invalid value \"{}\" for flag \"{}\". Accepted values are: ",
        raw_flag.value, spec.name);
    for (size_t i = 0; i < spec.allowed_values.size(); i++) {
      if (i + 1 < spec.allowed_values.size()) {
        std::format_to(std::back_inserter(error_msg), "\"{}\", ",
                       spec.allowed_values[i]);
      } else {
        std::format_to(std::back_inserter(error_msg), "\"{}\"",
                       spec.allowed_values[i]);
      }
    }
    return runtime::Result<FlagValue, std::string>::error(error_msg);
  }
  return runtime::Result<FlagValue, std::string>::ok(raw_flag.value);
}

[[nodiscard]] runtime::Result<FlagValue, std::string>
resolve_flag(const IntFlagSpec &spec, const RawFlag &raw_flag) noexcept {
  char *end{nullptr};
  const long parsed = strtol(raw_flag.value.c_str(), &end, 0);
  if ((end - raw_flag.value.c_str()) !=
      static_cast<long>(raw_flag.value.size())) {
    return runtime::Result<FlagValue, std::string>::error(
        std::format("Invalid integer value of \"{}\" for flag \"{}\"",
                    raw_flag.value, spec.name));
  }

  if (parsed > spec.range.max || parsed < spec.range.min) {
    return runtime::Result<FlagValue, std::string>::error(
        std::format("Out of range value of \"{}\" for flag \"{}\". Expected a "
                    "value in range [{}, {}]",
                    parsed, spec.name, spec.range.min, spec.range.max));
  }

  return runtime::Result<FlagValue, std::string>::ok(
      FlagValue{static_cast<int>(parsed)});
}

[[nodiscard]] runtime::Result<FlagValue, std::string>
resolve_flag(const FlagSpec &spec, const RawFlag &raw_flag) noexcept {
  return std::visit(
      [&raw_flag](const auto &spec) { return resolve_flag(spec, raw_flag); },
      spec);
}

static const luaL_Reg flag_functions[]{
    {"register", l_register_flag},
    {nullptr, nullptr},
};

static constexpr const char FLAG_PACKAGE_NAME[] = "yabt.core.flags";

} // namespace

[[nodiscard]] FlagType get_flag_type(const FlagSpec &spec) noexcept {
  return std::visit([](const auto &v) { return v.type(); }, spec);
}

[[nodiscard]] std::string_view get_flag_name(const FlagSpec &spec) noexcept {
  return std::visit([](const auto &v) { return std::string_view{v.name}; },
                    spec);
}

[[nodiscard]] FlagType IntFlagSpec::type() const noexcept {
  return FlagType::INTEGER;
}

[[nodiscard]] FlagType StringFlagSpec::type() const noexcept {
  return FlagType::STRING;
}

[[nodiscard]] FlagType EnumFlagSpec::type() const noexcept {
  return FlagType::ENUM;
}

void FlagLib::register_in_engine(lua_State *const L) {
  StackGuard g{L};
  luaL_register(L, FLAG_PACKAGE_NAME, flag_functions);

  // Register type flags

  lua_pushstring(L, "StringFlag");
  lua_pushlightuserdata(L, &string_flag_key);
  lua_settable(L, -3);

  lua_pushstring(L, "EnumFlag");
  lua_pushlightuserdata(L, &enum_flag_key);
  lua_settable(L, -3);

  lua_pushstring(L, "IntFlag");
  lua_pushlightuserdata(L, &int_flag_key);
  lua_settable(L, -3);

  lua_pop(L, 1);
  init_registry(L, this);
  init_flag_handle_metatable(L);
}

runtime::Result<FlagSpec, std::string>
FlagLib::register_flag(lua_State *const L) {
  if (m_resolved) {
    return runtime::Result<FlagSpec, std::string>::error(
        "Flags have already been resolved! Cannot register any more flags at "
        "this point. Ensure that flag registration is called in an INIT.lua "
        "file");
  }

  FlatFlagSpec flat = RESULT_PROPAGATE(parse_lua_object<FlatFlagSpec>(L));

  if (!flat.name.has_value()) {
    return runtime::Result<FlagSpec, std::string>::error(
        "Flag does not have a name");
  }
  const std::string name = flat.name.value();
  const std::string description = flat.description.value_or("");

  if (m_registered_flags.contains(name)) {
    return runtime::Result<FlagSpec, std::string>::error(
        std::format("Duplicated flag spec for name: {}", name));
  }

  FlagSpec flag{};

  switch (RESULT_PROPAGATE(lua_flag_key_to_type(flat.type))) {
  case FlagType::ENUM: {
    if (!flat.values.has_value() || flat.values.value().size() == 0) {
      return runtime::Result<FlagSpec, std::string>::error(
          std::format("Enum flag spec \"{}\" has no valid values", name));
    }
    if (flat.range.has_value()) {
      return runtime::Result<FlagSpec, std::string>::error(std::format(
          "Enum flag spec \"{}\" was given invalid \"range\" property", name));
    }
    flag = EnumFlagSpec{
        .name{name},
        .description{description},
        .default_value{flat.default_value},
        .allowed_values{flat.values.value()},
    };
    break;
  }
  case FlagType::INTEGER: {
    if (flat.values.has_value()) {
      return runtime::Result<FlagSpec, std::string>::error(std::format(
          "Int flag spec \"{}\" was given invalid \"values\" property", name));
    }
    const IntFlagRange range = flat.range.value_or(IntFlagRange{});
    if (range.min > range.max) {
      return runtime::Result<FlagSpec, std::string>::error(
          std::format("Invalid range for yabt flag \"{}\": [{}, {}]", name,
                      range.min, range.max));
    }
    flag = IntFlagSpec{
        .name{name},
        .description{description},
        .range{range},
        .default_value{flat.default_value},
    };
    break;
  }
  case FlagType::STRING: {
    if (flat.values.has_value()) {
      return runtime::Result<FlagSpec, std::string>::error(std::format(
          "String flag spec \"{}\" was given invalid \"values\" property",
          name));
    }
    if (flat.range.has_value()) {
      return runtime::Result<FlagSpec, std::string>::error(std::format(
          "String flag spec \"{}\" was given invalid \"range\" property",
          name));
    }
    flag = StringFlagSpec{
        .name{name},
        .description{description},
        .default_value{flat.default_value},
    };
    break;
  }
  };

  m_registered_flags.insert(std::pair(name, flag));
  return runtime::Result<FlagSpec, std::string>::ok(flag);
}

const std::unordered_map<std::string, FlagSpec> &
FlagLib::flag_specs() const noexcept {
  return m_registered_flags;
}

[[nodiscard]] const std::unordered_map<std::string, FlagValue> &
FlagLib::flag_values() const noexcept {
  runtime::check(
      m_resolved,
      "BUG: Called flag_values, but flags have not been resolved yet");
  return m_resolved_flags;
}

runtime::Result<void, std::string>
FlagLib::resolve_flag_values(const std::span<const RawFlag> flags) noexcept {
  runtime::check(!m_resolved, "BUG: Flags have already been resolved");
  m_resolved = true;
  for (const RawFlag &flag : flags) {
    if (!m_registered_flags.contains(flag.key)) {
      return runtime::Result<void, std::string>::error(
          std::format("Unknown yabt flag \"{}\"", flag.key));
    }

    const FlagSpec &flag_spec = m_registered_flags[flag.key];
    const FlagValue resolved_flag =
        RESULT_PROPAGATE(resolve_flag(flag_spec, flag));

    if (m_resolved_flags.contains(flag.key)) {
      return runtime::Result<void, std::string>::error(
          std::format("Flag \"{}\" was given more than once", flag.key));
    }

    m_resolved_flags.insert(std::make_pair(flag.key, resolved_flag));
  }

  // Validate that all flags are present, otherwise inject defaults
  for (const auto &[name, spec] : m_registered_flags) {
    if (m_resolved_flags.contains(name)) {
      continue;
    }

    const std::optional<std::string> &default_value =
        std::visit([](const auto &v) { return v.default_value; }, spec);
    if (default_value.has_value()) {
      const FlagValue resolved_flag = RESULT_PROPAGATE(resolve_flag(
          spec, RawFlag{.key{name}, .value{default_value.value()}}));
      m_resolved_flags[name] = resolved_flag;
      continue;
    }

    return runtime::Result<void, std::string>::error(
        std::format("No value given for flag \"{}\"", name));
  }

  return runtime::Result<void, std::string>::ok();
}

[[nodiscard]] runtime::Result<FlagValue, std::string>
FlagLib::retrieve_value(FlagSpec *spec) const noexcept {
  const std::string flag_name{get_flag_name(*spec)};
  auto iter = m_resolved_flags.find(flag_name);
  if (iter == m_resolved_flags.cend()) {
    return runtime::Result<FlagValue, std::string>::error(
        std::format("Unable to obtain value for flag \"{}\"", flag_name));
  }
  return runtime::Result<FlagValue, std::string>::ok(iter->second);
}

} // namespace yabt::lua
