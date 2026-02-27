#include <map>
#include <string>

namespace yabt::embed {

[[nodiscard]] const char *get_runtime_file() noexcept;

[[nodiscard]] std::map<std::string, const char *>
get_embedded_lua_rules() noexcept;

} // namespace yabt::embed
