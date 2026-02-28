#pragma once

#include <map>
#include <string>

#include "yabt/lua/utils.h"

namespace yabt::ninja {

using VariableMap = std::map<std::string, std::string>;

struct BuildRule {
  std::string name;
  std::string cmd;
  std::string descr;
  std::map<std::string, std::string> variables;
  bool compdb; // whether the rule should be part of the compilation database
};

} // namespace yabt::ninja

namespace yabt::lua {

LUA_STRUCT_PARSE_SPEC_DEF(                   //
    ::yabt::ninja::BuildRule,                //
    (std::string, name),                     //
    (std::string, cmd),                      //
    (std::string, descr),                    //
    (::yabt::ninja::VariableMap, variables), //
    (bool, compdb)                           //
);

} // namespace yabt::lua
