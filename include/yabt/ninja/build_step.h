#pragma once

#include <map>
#include <string>
#include <vector>

namespace yabt::ninja {

struct BuildStep {
  std::vector<std::string> outs;
  std::vector<std::string> ins;
  std::string cmd;
  std::string descr;
};

struct BuildStepWithRule {
  std::vector<std::string> outs;
  std::vector<std::string> ins;
  std::string ruleName;
  std::map<std::string, std::string> variables;
};

} // namespace yabt::ninja
