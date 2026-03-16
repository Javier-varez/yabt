#include <map>
#include <string>
#include <string_view>

namespace yabt::embed {

[[nodiscard]] std::string_view get_runtime_file() noexcept;

[[nodiscard]] std::map<std::string, std::string_view>
get_embedded_lua_rules() noexcept;

} // namespace yabt::embed
