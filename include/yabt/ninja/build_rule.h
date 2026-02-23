#pragma once

#include <map>
#include <string>

namespace yabt::ninja {

struct BuildRule {
  std::string name;
  std::string cmd;
  std::string descr;
  std::map<std::string, std::string> variables;
};

} // namespace yabt::ninja
