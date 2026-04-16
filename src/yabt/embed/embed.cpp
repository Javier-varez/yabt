#include <bit>
#include <cstdint>

#include "yabt/embed/embed.h"

#define EMBED_FILE(resource_name, var)                                         \
  extern "C" {                                                                 \
  extern uint8_t _binary_##var##_start[1];                                     \
  extern uint8_t _binary_##var##_end[1];                                       \
  }                                                                            \
                                                                               \
  namespace {                                                                  \
                                                                               \
  [[nodiscard, maybe_unused]] constexpr std::string_view                       \
  resource_name##_as_string_view() noexcept {                                  \
    const size_t size = std::bit_cast<uintptr_t>(&_binary_##var##_end[0]) -    \
                        std::bit_cast<uintptr_t>(&_binary_##var##_start[0]);   \
    const char *start =                                                        \
        std::bit_cast<const char *>(&_binary_##var##_start[0]);                \
    return std::string_view{start, size};                                      \
  }                                                                            \
  }

// Lua runtime
EMBED_FILE(runtime_lua, yabt_embed_runtime_lua)

// Extra internal rules
EMBED_FILE(rules_yabt_core_utils_lua, yabt_embed_rules_yabt_core_utils_lua)

// Lua stubs for the LSP
EMBED_FILE(stub_yabt_core_context_lua,
           yabt_embed_lua_stubs_yabt_core_context_lua)
EMBED_FILE(stub_yabt_core_log_lua, yabt_embed_lua_stubs_yabt_core_log_lua)
EMBED_FILE(stub_yabt_core_path_lua, yabt_embed_lua_stubs_yabt_core_path_lua)
EMBED_FILE(stub_build_globals_lua, yabt_embed_lua_stubs_build_globals_lua)

namespace yabt::embed {

[[nodiscard]] std::string_view get_runtime_file() noexcept {
  return runtime_lua_as_string_view();
}

[[nodiscard]] std::map<std::string, std::string_view>
get_embedded_lua_rules() noexcept {
  std::map<std::string, std::string_view> preloads;
  preloads.insert(std::make_pair("yabt.core.utils",
                                 rules_yabt_core_utils_lua_as_string_view()));
  return preloads;
}

[[nodiscard]] std::map<std::string, std::string_view>
get_embedded_lua_stubs() noexcept {
  std::map<std::string, std::string_view> preloads;
  preloads.insert(std::make_pair("yabt/core/context.lua",
                                 stub_yabt_core_context_lua_as_string_view()));
  preloads.insert(std::make_pair("yabt/core/log.lua",
                                 stub_yabt_core_log_lua_as_string_view()));
  preloads.insert(std::make_pair("yabt/core/path.lua",
                                 stub_yabt_core_path_lua_as_string_view()));
  preloads.insert(std::make_pair("build_globals.lua",
                                 stub_build_globals_lua_as_string_view()));
  return preloads;
}

} // namespace yabt::embed
