#include <bit>
#include <cstdint>

#include "yabt/embed/embed.h"

extern "C" {
extern uint8_t _binary_yabt_embed_runtime_lua_start[1];
extern uint8_t _binary_yabt_embed_runtime_lua_end[1];

extern uint8_t _binary_yabt_embed_rules_yabt_core_utils_lua_start[1];
extern uint8_t _binary_yabt_embed_rules_yabt_core_utils_lua_end[1];
}

namespace yabt::embed {

[[nodiscard]] std::string_view get_runtime_file() noexcept {
  const size_t size =
      std::bit_cast<uintptr_t>(&_binary_yabt_embed_runtime_lua_end[0]) -
      std::bit_cast<uintptr_t>(&_binary_yabt_embed_runtime_lua_start[0]);
  const char *start =
      std::bit_cast<const char *>(&_binary_yabt_embed_runtime_lua_start[0]);
  return std::string_view{start, size};
}

[[nodiscard]] std::map<std::string, std::string_view>
get_embedded_lua_rules() noexcept {
  std::map<std::string, std::string_view> preloads;
  {
    const size_t size =
        std::bit_cast<uintptr_t>(
            &_binary_yabt_embed_rules_yabt_core_utils_lua_end[0]) -
        std::bit_cast<uintptr_t>(
            &_binary_yabt_embed_rules_yabt_core_utils_lua_start[0]);
    const char *start = std::bit_cast<const char *>(
        &_binary_yabt_embed_rules_yabt_core_utils_lua_start[0]);
    std::string_view sv{start, size};
    preloads.insert(std::make_pair("yabt.core.utils", sv));
  }
  return preloads;
}

} // namespace yabt::embed
