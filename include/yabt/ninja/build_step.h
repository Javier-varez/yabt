#pragma once

#include <map>
#include <string>
#include <vector>

#include "yabt/lua/path.h"
#include "yabt/lua/utils.h"

namespace yabt::ninja {

struct BuildStep {
  std::vector<lua::OutPath> outs;
  std::vector<lua::Path> ins;
  std::string cmd;
  std::string descr;
};

[[nodiscard]] inline bool operator==(const BuildStep &lhs,
                                     const BuildStep &rhs) noexcept {
  if (lhs.outs.size() != rhs.outs.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.outs.size(); i++) {
    if (lhs.outs[i] != rhs.outs[i]) {
      return false;
    }
  }

  if (lhs.ins.size() != rhs.ins.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.ins.size(); i++) {
    if (lhs.ins[i] != rhs.ins[i]) {
      return false;
    }
  }

  if (lhs.cmd != rhs.cmd) {
    return false;
  }

  if (lhs.descr != rhs.descr) {
    return false;
  }

  return true;
}

[[nodiscard]] inline bool operator!=(const BuildStep &lhs,
                                     const BuildStep &rhs) noexcept {
  return !(lhs == rhs);
}

using VariableMap = std::map<std::string, std::string>;

struct BuildStepWithRule {
  std::vector<lua::OutPath> outs;
  std::vector<lua::Path> ins;
  std::string rule_name;
  VariableMap variables;
};

[[nodiscard]] inline bool operator==(const BuildStepWithRule &lhs,
                                     const BuildStepWithRule &rhs) noexcept {
  if (lhs.outs.size() != rhs.outs.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.outs.size(); i++) {
    if (lhs.outs[i] != rhs.outs[i]) {
      return false;
    }
  }

  if (lhs.ins.size() != rhs.ins.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.ins.size(); i++) {
    if (lhs.ins[i] != rhs.ins[i]) {
      return false;
    }
  }

  if (lhs.rule_name != rhs.rule_name) {
    return false;
  }

  if (lhs.variables.size() != rhs.variables.size()) {
    return false;
  }

  for (const auto &[k, v] : lhs.variables) {
    const auto it = rhs.variables.find(k);
    if (it == rhs.variables.end()) {
      return false;
    }
    if (v != it->second) {
      return false;
    }
  }

  return true;
}

[[nodiscard]] inline bool operator!=(const BuildStepWithRule &lhs,
                                     const BuildStepWithRule &rhs) noexcept {
  return !(lhs == rhs);
}

} // namespace yabt::ninja

namespace yabt::lua {

LUA_STRUCT_PARSE_SPEC_DEF(             //
    ::yabt::ninja::BuildStep,          //
    (std::vector<lua::OutPath>, outs), //
    (std::vector<lua::Path>, ins),     //
    (std::string, cmd),                //
    (std::string, descr)               //
);

LUA_STRUCT_PARSE_SPEC_DEF(                  //
    ::yabt::ninja::BuildStepWithRule,       //
    (std::vector<lua::OutPath>, outs),      //
    (std::vector<lua::Path>, ins),          //
    (std::string, rule_name),               //
    (::yabt::ninja::VariableMap, variables) //
);

} // namespace yabt::lua
