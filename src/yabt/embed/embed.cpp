#include "yabt/embed/embed.h"

namespace yabt::embed {

namespace {
constexpr static const char runtime_file[] = {
#embed "runtime.lua"
    , '\0'};

constexpr static const char core_utils_file[] = {
#embed "rules/yabt/core/utils.lua"
    , '\0'};

constexpr static const char core_path_file[] = {
#embed "rules/yabt/core/path.lua"
    , '\0'};

constexpr static const char core_context_file[] = {
#embed "rules/yabt/core/context.lua"
    , '\0'};
} // namespace

[[nodiscard]] const char *get_runtime_file() noexcept { return runtime_file; }

[[nodiscard]] std::map<std::string, const char *>
get_embedded_lua_rules() noexcept {
  std::map<std::string, const char *> preloads;
  preloads.insert(std::make_pair("yabt.core.utils", core_utils_file));
  preloads.insert(std::make_pair("yabt.core.path", core_path_file));
  preloads.insert(std::make_pair("yabt.core.context", core_context_file));
  return preloads;
}

} // namespace yabt::embed
