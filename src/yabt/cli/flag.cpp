#include "yabt/cli/flag.h"

namespace yabt::cli {

[[nodiscard]] runtime::Result<void, std::string>
Flag::validate() const noexcept {
  if (name == "") {
    return runtime::Result<void, std::string>::error(
        "Flag name cannot be empty");
  }

  if (name.starts_with("-")) {
    return runtime::Result<void, std::string>::error(std::format(
        "Flag names should not contain any leading \"-\": \"{}\"", name));
  }

  if (short_name.has_value() && type != FlagType::BOOL) {
    return runtime::Result<void, std::string>::error(
        std::format("Only boolean flags can have short names: \"{}\"", name));
  }

  if (type == FlagType::BOOL && !optional) {
    return runtime::Result<void, std::string>::error(std::format(
        "Boolean flags are always inherently optional: \"{}\"", name));
  }

  return runtime::Result<void, std::string>::ok();
}

} // namespace yabt::cli
