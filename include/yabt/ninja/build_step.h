#pragma once

#include <map>
#include <string>
#include <vector>

#include "yabt/lua/utils.h"

namespace yabt::ninja {

struct BuildStep {
  std::vector<std::string> outs;
  std::vector<std::string> ins;
  std::string cmd;
  std::string descr;
};

using VariableMap = std::map<std::string, std::string>;

struct BuildStepWithRule {
  std::vector<std::string> outs;
  std::vector<std::string> ins;
  std::string ruleName;
  VariableMap variables;
};

} // namespace yabt::ninja

namespace yabt::lua {

LUA_STRUCT_PARSE_SPEC_DEF(            //
    ::yabt::ninja::BuildStep,         //
    (std::vector<std::string>, outs), //
    (std::vector<std::string>, ins),  //
    (std::string, cmd),               //
    (std::string, descr)              //
);

LUA_STRUCT_PARSE_SPEC_DEF(                  //
    ::yabt::ninja::BuildStepWithRule,       //
    (std::vector<std::string>, outs),       //
    (std::vector<std::string>, ins),        //
    (std::string, ruleName),                //
    (::yabt::ninja::VariableMap, variables) //
);

} // namespace yabt::lua
