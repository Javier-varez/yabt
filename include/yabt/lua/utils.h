#pragma once

#include <map>
#include <string>
#include <vector>

#include "yabt/runtime/result.h"
#include "yabt/utils/constexpr_string.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
}

extern "C" {
struct lua_State;
}

namespace yabt::lua {

/// Parsing type identifiers
template <typename T, typename... Args> struct LuaStruct final {};

template <typename T, typename U, utils::ConstexprString Name, U T::*FieldPtr>
struct LuaStructField final {};

template <typename T, typename U> struct LuaKvPairs final {};

template <typename T> struct LuaArray final {};

struct LuaString final {};
struct LuaInteger final {};

// Parsing spec type marker
template <typename T> struct LuaParseSpec;

// Templated parsing specs
template <typename T> struct LuaParseSpec<std::vector<T>> {
  using spec = LuaArray<T>;
};

template <typename T, typename U> struct LuaParseSpec<std::map<T, U>> {
  using spec = LuaKvPairs<T, U>;
};

template <> struct LuaParseSpec<std::string> {
  using spec = LuaString;
};

template <> struct LuaParseSpec<int> {
  using spec = LuaInteger;
};

template <typename T>
[[nodiscard]] runtime::Result<std::vector<T>, std::string>
parse_with_spec(lua_State *const L, LuaArray<T>) noexcept;

template <typename T, typename... Args>
[[nodiscard]] runtime::Result<T, std::string>
parse_with_spec(lua_State *const L, LuaStruct<T, Args...>) noexcept;

template <typename T, typename U>
[[nodiscard]] runtime::Result<std::map<T, U>, std::string>
parse_with_spec(lua_State *const L, LuaKvPairs<T, U>) noexcept;

[[nodiscard]] runtime::Result<std::string, std::string>
parse_with_spec(lua_State *const L, LuaString) noexcept;

[[nodiscard]] runtime::Result<int, std::string>
parse_with_spec(lua_State *const L, LuaInteger) noexcept;

template <typename T>
[[nodiscard]] runtime::Result<T, std::string>
parse_lua_object(lua_State *const L) noexcept {
  return parse_with_spec(L, typename LuaParseSpec<T>::spec{});
}

template <typename T>
[[nodiscard]] runtime::Result<std::vector<T>, std::string>
parse_with_spec(lua_State *const L, LuaArray<T>) noexcept {
  if (lua_isnil(L, -1)) {
    // Treat a missing array as an empty array
    return runtime::Result<std::vector<T>, std::string>::ok(std::vector<T>{});
  }

  if (!lua_istable(L, -1)) {
    return runtime::Result<std::vector<T>, std::string>::error(
        std::format("Deserializing std::vector<T>, but found type: {}",
                    lua_typename(L, lua_type(L, -1))));
  }

  const size_t n = lua_objlen(L, -1);
  std::vector<T> result{};
  result.reserve(n);

  runtime::check(lua_checkstack(L, 1), "Exceeded maximum Lua stack size");
  for (size_t i = 1; i <= n; i++) {
    lua_rawgeti(L, -1, i);
    result.push_back(RESULT_PROPAGATE(parse_lua_object<T>(L)));
    lua_pop(L, 1);
  }

  return runtime::Result<std::vector<T>, std::string>::ok(result);
}

template <typename T, typename U>
[[nodiscard]] runtime::Result<std::map<T, U>, std::string>
parse_with_spec(lua_State *const L, LuaKvPairs<T, U>) noexcept {
  if (lua_isnil(L, -1)) {
    // Treat a missing table as an empty table
    return runtime::Result<std::map<T, U>, std::string>::ok(std::map<T, U>{});
  }

  if (!lua_istable(L, -1)) {
    return runtime::Result<std::map<T, U>, std::string>::error(
        std::format("Deserializing std::map<T, U>, but found type: {}",
                    lua_typename(L, lua_type(L, -1))));
  }

  runtime::check(lua_checkstack(L, 2), "Exceeded maximum Lua stack size");

  std::map<T, U> result{};

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    const U value = RESULT_PROPAGATE(parse_lua_object<U>(L));
    lua_pop(L, 1);

    // keep key for next iter
    const T key = RESULT_PROPAGATE(parse_lua_object<T>(L));
    result.insert(std::make_pair(key, value));
  }
  return runtime::Result<std::map<T, U>, std::string>::ok(result);
}

template <typename T, typename... Args>
[[nodiscard]] runtime::Result<T, std::string>
parse_with_spec(lua_State *const L, LuaStruct<T, Args...>) noexcept {
  if (!lua_istable(L, -1)) {
    return runtime::Result<T, std::string>::error(
        std::format("Deserializing struct, but found type: {}",
                    lua_typename(L, lua_type(L, -1))));
  }

  T result{};

  std::string error_string;
  const auto handle_arg =
      [L, &result,
       &error_string]<typename U, utils::ConstexprString Name, U T::*Field>(
          LuaStructField<T, U, Name, Field>) {
        lua_pushstring(L, Name.data);
        lua_gettable(L, -2);

        const runtime::Result<U, std::string> inner_res =
            parse_lua_object<U>(L);
        if (!inner_res.is_ok()) {
          error_string = inner_res.error_value();
          lua_pop(L, 1);
          return false;
        }

        result.*Field = inner_res.ok_value();
        lua_pop(L, 1);
        return true;
      };

  const bool ok = (handle_arg(Args{}) && ...);
  if (!ok) {
    return runtime::Result<T, std::string>::error(error_string);
  }

  return runtime::Result<T, std::string>::ok(result);
}

[[nodiscard]] inline runtime::Result<std::string, std::string>
parse_with_spec(lua_State *const L, LuaString) noexcept {
  if (lua_isnil(L, -1)) {
    // Empty string
    return runtime::Result<std::string, std::string>::ok("");
  }

  if (!lua_isstring(L, -1)) {
    return runtime::Result<std::string, std::string>::error(
        std::format("Deserializing std::string, but found type {}",
                    lua_typename(L, lua_type(L, -1))));
  }

  return runtime::Result<std::string, std::string>::ok(lua_tostring(L, -1));
}

[[nodiscard]] inline runtime::Result<int, std::string>
parse_with_spec(lua_State *const L, LuaInteger) noexcept {
  if (!lua_isnumber(L, -1)) {
    return runtime::Result<int, std::string>::error(
        std::format("Deserializing int, but found type {}",
                    lua_typename(L, lua_type(L, -1))));
  }

  return runtime::Result<int, std::string>::ok(
      static_cast<int>(lua_tointeger(L, -1)));
}

#define _REMOVE_PARENS_IMPL(...) __VA_ARGS__
#define _REMOVE_PARENS(...) _REMOVE_PARENS_IMPL __VA_ARGS__

#define _ARG16(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,    \
               _15, _16, ...)                                                  \
  _16

#define _NARGS(...)                                                            \
  _ARG16(dummy, ##__VA_ARGS__, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define _JOIN(a, b) a##b

#define _APPLY_OP_VA_ARGS0(op, fixed_arg)
#define _APPLY_OP_VA_ARGS1(op, fixed_arg, arg) op(fixed_arg, arg)
#define _APPLY_OP_VA_ARGS2(op, fixed_arg, first, ...)                          \
  _APPLY_OP_VA_ARGS1(op, fixed_arg, first)                                     \
  _APPLY_OP_VA_ARGS1(op, fixed_arg, __VA_ARGS__)
#define _APPLY_OP_VA_ARGS3(op, fixed_arg, first, ...)                          \
  _APPLY_OP_VA_ARGS1(op, fixed_arg, first)                                     \
  _APPLY_OP_VA_ARGS2(op, fixed_arg, __VA_ARGS__)
#define _APPLY_OP_VA_ARGS4(op, fixed_arg, first, ...)                          \
  _APPLY_OP_VA_ARGS1(op, fixed_arg, first)                                     \
  _APPLY_OP_VA_ARGS3(op, fixed_arg, __VA_ARGS__)

#define _APPLY_OP_VA_ARGS_IMPL(nargs, op, fixed_arg, ...)                      \
  _JOIN(_APPLY_OP_VA_ARGS, nargs)(op, fixed_arg, __VA_ARGS__)

#define _APPLY_OP_VA_ARGS(op, fixed_arg, ...)                                  \
  _APPLY_OP_VA_ARGS_IMPL(_NARGS(__VA_ARGS__), op, fixed_arg, __VA_ARGS__)

#define _LUA_STRUCT_EXPAND_FIELD_DEF3(struct_type, field_type, field_name)     \
  using field_name##_field =                                                   \
      LuaStructField<struct_type, field_type, #field_name##_cs,                \
                     &struct_type::field_name>;

#define _LUA_STRUCT_EXPAND_FIELD_DEF2(struct_type, ...)                        \
  _LUA_STRUCT_EXPAND_FIELD_DEF3(struct_type, __VA_ARGS__)

#define _LUA_STRUCT_EXPAND_FIELD_DEF(struct_type, field_def)                   \
  _LUA_STRUCT_EXPAND_FIELD_DEF2(struct_type, _REMOVE_PARENS(field_def))

#define _LUA_STRUCT_EXPAND_FIELD_NAME3(struct_type, field_type, field_name)    \
  , field_name##_field

#define _LUA_STRUCT_EXPAND_FIELD_NAME2(struct_type, ...)                       \
  _LUA_STRUCT_EXPAND_FIELD_NAME3(struct_type, __VA_ARGS__)

#define _LUA_STRUCT_EXPAND_FIELD_NAME(struct_type, field_def)                  \
  _LUA_STRUCT_EXPAND_FIELD_NAME2(struct_type, _REMOVE_PARENS(field_def))

#define LUA_STRUCT_PARSE_SPEC_DEF(struct_type, ...)                            \
  template <> struct LuaParseSpec<struct_type> {                               \
    _APPLY_OP_VA_ARGS(_LUA_STRUCT_EXPAND_FIELD_DEF, struct_type, __VA_ARGS__)  \
    using spec = LuaStruct<struct_type _APPLY_OP_VA_ARGS(                      \
        _LUA_STRUCT_EXPAND_FIELD_NAME, struct_type, __VA_ARGS__)>;             \
  };

inline void set_package_path(lua_State *const L, const std::string &value) {
  runtime::check(lua_checkstack(L, 2), "Exceeded maximum Lua stack size");
  lua_getglobal(L, "package");
  lua_pushstring(L, value.c_str());
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);
}

inline void set_package_cpath(lua_State *const L, const std::string &value) {
  runtime::check(lua_checkstack(L, 2), "Exceeded maximum Lua stack size");
  lua_getglobal(L, "package");
  lua_pushstring(L, value.c_str());
  lua_setfield(L, -2, "cpath");
  lua_pop(L, 1);
}

} // namespace yabt::lua
