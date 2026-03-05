#include "yabt/lua/log_lib.h"
#include "yabt/log/log.h"
#include "yabt/lua/utils.h"

#include "lua.hpp"

namespace yabt::lua {

namespace {

template <log::LogLevel level> int l_log(lua_State *const L) {
  if (lua_gettop(L) != 1) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, "Unexpected number of arguments given to log function");
    lua_error(L);
  }
  if (!lua_isstring(L, -1)) {
    lua_pop(L, lua_gettop(L));
    lua_pushstring(L, "Invalid argument type given to log function");
    lua_error(L);
  }

  const char *const str = lua_tostring(L, -1);
  if constexpr (level == log::LogLevel::VERBOSE) {
    yabt_verbose("{}", str);
  } else if constexpr (level == log::LogLevel::DEBUG) {
    yabt_debug("{}", str);
  } else if constexpr (level == log::LogLevel::INFO) {
    yabt_info("{}", str);
  } else if constexpr (level == log::LogLevel::WARNING) {
    yabt_warn("{}", str);
  } else if constexpr (level == log::LogLevel::ERROR) {
    yabt_error("{}", str);
  }
  lua_pop(L, 1);
  return 0;
}

static const luaL_Reg logging_functions[]{
    {"verbose", l_log<log::LogLevel::VERBOSE>},
    {"debug", l_log<log::LogLevel::DEBUG>},
    {"info", l_log<log::LogLevel::INFO>},
    {"warn", l_log<log::LogLevel::WARNING>},
    {"error", l_log<log::LogLevel::ERROR>},
    {nullptr, nullptr},
};

static constexpr const char LOG_PACKAGE_NAME[] = "yabt.core.log";

} // namespace

void LogLib::register_in_engine(lua_State *const L) noexcept {
  StackGuard g{L};
  luaL_register(L, LOG_PACKAGE_NAME, logging_functions);
  lua_pop(L, 1);
}

} // namespace yabt::lua
