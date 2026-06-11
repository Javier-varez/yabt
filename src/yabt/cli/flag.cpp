#include "yabt/cli/flag.h"

namespace yabt::cli {

[[nodiscard]] runtime::Result<void> Flag::validate() const noexcept {
  if (name == "") {
    return runtime::Result<void>::error("Flag name cannot be empty");
  }

  if (name.starts_with("-")) {
    return runtime::Result<void>::error(std::format(
        "Flag names should not contain any leading \"-\": \"{}\"", name));
  }

  if (short_name.has_value() && type != FlagType::BOOL) {
    return runtime::Result<void>::error(
        std::format("Only boolean flags can have short names: \"{}\"", name));
  }

  if (type == FlagType::BOOL && !optional) {
    return runtime::Result<void>::error(std::format(
        "Boolean flags are always inherently optional: \"{}\"", name));
  }

  return runtime::Result<void>::ok();
}

} // namespace yabt::cli
