#pragma once

#include <filesystem>
#include <map>
#include <vector>

#include "yabt/runtime/result.h"

namespace yabt::module {

enum class ModuleType {
  Git,
};

struct DependencyDefinition final {
  std::string url;
  std::string version;
  std::string hash;
  std::string type;
};

using FlagMap = std::map<std::string, std::string>;

using DependencyMap = std::map<std::string, DependencyDefinition>;

struct ModuleFile {
  std::string name;
  int version;
  DependencyMap deps;
  FlagMap flags;

  [[nodiscard]] static runtime::Result<ModuleFile, std::string>
  load_module_file(std::filesystem::path path) noexcept;
};

} // namespace yabt::module
