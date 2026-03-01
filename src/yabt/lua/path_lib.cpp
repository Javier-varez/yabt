#include "yabt/lua/path_lib.h"
#include "lauxlib.h"
#include "yabt/lua/utils.h"

#include <filesystem>

namespace yabt::lua {

namespace {

int registry_key;

void init_registry(lua_State *const L, PathLib *data) {
  StackGuard g{L};
  lua_pushlightuserdata(L, &registry_key);
  lua_pushlightuserdata(L, data);
  lua_settable(L, LUA_REGISTRYINDEX);
}

PathLib *get_lib_from_registry(lua_State *const L) {
  lua_pushlightuserdata(L, &registry_key);
  lua_gettable(L, LUA_REGISTRYINDEX);
  PathLib *lib = static_cast<PathLib *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return lib;
}

constexpr static const char IN_PATH_META[] = "Yabt.PathLib.InPath";
constexpr static const char OUT_PATH_META[] = "Yabt.PathLib.OutPath";

static constexpr const char PATH_PACKAGE_NAME[] = "yabt.core.path";

struct InPath final {
  constexpr static const char *const METATABLE = IN_PATH_META;
  constexpr static const char *const PACKAGE_NAME = "yabt.core.in_path";
  constexpr static const char *const TYPE_NAME = "InPath";

  constexpr static std::filesystem::path PathLib::*BASE_DIR =
      &PathLib::m_source_dir;
};

struct OutPath final {
  constexpr static const char *const METATABLE = OUT_PATH_META;
  constexpr static const char *const PACKAGE_NAME = "yabt.core.out_path";
  constexpr static const char *const TYPE_NAME = "OutPath";

  constexpr static std::filesystem::path PathLib::*BASE_DIR =
      &PathLib::m_output_dir;
};

template <typename PathParams> struct Path final {

  template <typename T>
  static void new_path(lua_State *const L, T val) noexcept {
    void *data = lua_newuserdata(L, sizeof(std::filesystem::path));
    luaL_getmetatable(L, PathParams::METATABLE);
    lua_setmetatable(L, -2);
    auto *ud = new (data) std::filesystem::path{val};
    *ud = std::filesystem::weakly_canonical(*ud);
  }

  [[nodiscard]] static int l_new_relative_path(lua_State *const L) noexcept {
    const PathLib *const pathlib = get_lib_from_registry(L);
    runtime::check(pathlib != nullptr,
                   "get_lib_from_registry returned nullptr!");

    StackGuard g{L};
    size_t length{};
    const char *path = luaL_checklstring(L, -1, &length);

    const std::filesystem::path rel{std::string_view{path, length}};
    new_path(L, pathlib->*PathParams::BASE_DIR / rel);

    lua_remove(L, -2);
    return 1;
  }

  [[nodiscard]] static int l_absolute(lua_State *const L) noexcept {
    StackGuard g{L};
    std::filesystem::path *ud = static_cast<std::filesystem::path *>(
        luaL_checkudata(L, 1, PathParams::METATABLE));
    luaL_argcheck(L, ud != nullptr, 1, "path expected");
    lua_pushstring(L, ud->c_str());
    lua_remove(L, 1);
    return 1;
  }

  [[nodiscard]] static int l_relative(lua_State *const L) noexcept {
    const PathLib *const pathlib = get_lib_from_registry(L);
    runtime::check(pathlib != nullptr,
                   "get_lib_from_registry returned nullptr!");

    StackGuard g{L};
    std::filesystem::path *ud = static_cast<std::filesystem::path *>(
        luaL_checkudata(L, 1, PathParams::METATABLE));
    luaL_argcheck(L, ud != nullptr, 1, "path expected");

    const std::filesystem::path relative =
        std::filesystem::relative(*ud, pathlib->*PathParams::BASE_DIR);
    lua_pop(L, 1);
    lua_pushstring(L, relative.c_str());
    return 1;
  }

  [[nodiscard]] static int l_ext(lua_State *const L) noexcept {
    StackGuard g{L};
    std::filesystem::path *ud = static_cast<std::filesystem::path *>(
        luaL_checkudata(L, 1, PathParams::METATABLE));
    luaL_argcheck(L, ud != nullptr, 1, "path expected");
    lua_pushstring(L, ud->extension().c_str());
    lua_remove(L, 1);
    return 1;
  }

  [[nodiscard]] static int l_with_ext(lua_State *const L) noexcept {
    StackGuard g{L, -1};
    std::filesystem::path *ud = static_cast<std::filesystem::path *>(
        luaL_checkudata(L, 1, PathParams::METATABLE));
    luaL_argcheck(L, ud != nullptr, 1, "path expected");

    size_t length{};
    const char *ext = luaL_checklstring(L, -1, &length);

    std::filesystem::path newp = *ud;
    newp.replace_extension(ext);

    lua_pop(L, 2);
    Path<OutPath>::new_path(L, std::move(newp));
    return 1;
  }

  [[nodiscard]] static int l_gc(lua_State *const L) noexcept {
    std::filesystem::path *ud = static_cast<std::filesystem::path *>(
        luaL_checkudata(L, 1, PathParams::METATABLE));
    luaL_argcheck(L, ud != nullptr, 1, "path expected");
    ud->std::filesystem::path::~path();
    lua_pop(L, 1);
    return 0;
  }

  constexpr static struct luaL_Reg constructor_table[] = {
      {"new_relative", l_new_relative_path}, //
      {nullptr, nullptr},                    //
  };

  constexpr static struct luaL_Reg instance_table[] = {
      {"absolute", l_absolute}, //
      {"relative", l_relative}, //
      {"ext", l_ext},           //
      {"with_ext", l_with_ext}, //
      {"__gc", l_gc},           //
      {nullptr, nullptr},       //
  };

  static void register_lib(lua_State *const L) noexcept {
    luaL_newmetatable(L, PathParams::METATABLE);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    luaL_register(L, nullptr, instance_table);
    lua_pop(L, 1);

    luaL_register(L, PathParams::PACKAGE_NAME, constructor_table);
    lua_setfield(L, -2, PathParams::TYPE_NAME);
  }
};

[[nodiscard]] static int l_is_path(lua_State *const L) noexcept {
  StackGuard g{L};
  const auto *in = luaL_testudata(L, 1, IN_PATH_META);
  const auto *out = luaL_testudata(L, 1, OUT_PATH_META);
  const bool res = in != nullptr || out != nullptr;
  lua_pop(L, 1);
  lua_pushboolean(L, res);
  return 1;
}

[[nodiscard]] static int l_is_out_path(lua_State *const L) noexcept {
  StackGuard g{L};
  const auto *out = luaL_testudata(L, 1, OUT_PATH_META);
  const bool res = out != nullptr;
  lua_pop(L, 1);
  lua_pushboolean(L, res);
  return 1;
}

constexpr static struct luaL_Reg free_functions_table[] = {
    {"is_path", l_is_path},
    {"is_out_path", l_is_out_path},
    {nullptr, nullptr},
};

} // namespace

PathLib::PathLib(lua_State *const L, std::filesystem::path source_dir,
                 std::filesystem::path output_dir)
    : m_state{L}, m_source_dir{source_dir}, m_output_dir{output_dir} {
  StackGuard g{L};
  luaL_register(L, PATH_PACKAGE_NAME, free_functions_table);
  Path<InPath>::register_lib(L);
  Path<OutPath>::register_lib(L);
  lua_pop(L, 1);
  init_registry(L, this);
}

PathLib::PathLib(PathLib &&other) noexcept
    : m_state{other.m_state}, m_source_dir{std::move(other.m_source_dir)},
      m_output_dir{std::move(other.m_output_dir)} {
  init_registry(m_state, this);
}

PathLib &PathLib::operator=(PathLib &&other) noexcept {
  if (&other != this) {
    m_state = other.m_state;
    m_source_dir = std::move(other.m_source_dir);
    m_output_dir = std::move(other.m_output_dir);
    init_registry(m_state, this);
  }
  return *this;
}

} // namespace yabt::lua
