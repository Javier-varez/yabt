#pragma once

#include <string>
#include <variant>

namespace yabt::cli {

struct BoolArg {
  std::string name;
};

struct StringArg {
  std::string name;
  std::string value;
};

struct IntegerArg {
  std::string name;
  long long int value;
};

using Arg = std::variant<BoolArg, StringArg, IntegerArg>;

} // namespace yabt::cli
