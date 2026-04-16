#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

#include "yabt/cli/args.h"
#include "yabt/cmd/lsp.h"
#include "yabt/embed/embed.h"
#include "yabt/log/log.h"
#include "yabt/module/module.h"
#include "yabt/process/process.h"
#include "yabt/runtime/result.h"
#include "yabt/workspace/utils.h"

namespace yabt::cmd {

namespace {

const std::string_view SHORT_DESCRIPTION =
    "Starts a language server for Yabt lua files in the current workspace.";
const std::string_view LONG_DESCRIPTION =
    "Starts a language server for Yabt lua files in the current workspace.\n"
    "Any extra arguments are forwarded to lua-language-server.";

// TODO: Probably should use a more comprehensive json escape implementation.
[[nodiscard]] std::string
json_escape(const std::string_view unscaped) noexcept {
  std::string escaped;

  escaped.reserve(unscaped.size());
  for (const char c : unscaped) {
    if (c == '"') {
      escaped += "\\\"";
    } else if (c == '\\') {
      escaped += "\\\\";
    } else if (c == '\b') {
      escaped += "\\b";
    } else if (c == '\r') {
      escaped += "\\r";
    } else if (c == '\n') {
      escaped += "\\n";
    } else if (c == '\f') {
      escaped += "\\f";
    } else if (c == '\t') {
      escaped += "\\t";
    } else {
      escaped += c;
    }
  }

  return escaped;
}

[[nodiscard]] std::string generate_luarc_json(
    const std::span<const std::filesystem::path> library_paths) noexcept {
  std::stringstream luarc_content;
  luarc_content << "{\n"
                   "    \"runtime\": {\n"
                   "        \"version\": \"Lua 5.1\"\n"
                   "    },\n"
                   "    \"workspace\": {\n"
                   "        \"library\": [\n";

  for (size_t i = 0; i < library_paths.size(); ++i) {
    luarc_content << "        \"";
    luarc_content << json_escape(library_paths[i].string());
    luarc_content << '"';
    if (i < library_paths.size() - 1) {
      luarc_content << ',';
    }
    luarc_content << '\n';
  }

  luarc_content
      << "        ],\n"
         "        \"checkThirdParty\": false\n"
         "    },\n"
         "    \"diagnostics\": {\n"
         "        \"globals\": [\n"
         "            \"targets\", \"out\", \"outs\", \"inp\", \"ins\",\n"
         "            \"import\", \"MODULE_PATH\", \"modules\"\n"
         "        ]\n"
         "    }\n"
         "}\n";

  return std::move(luarc_content).str();
}

} // namespace

[[nodiscard]] runtime::Result<void, std::string>
LspCommand::register_command(cli::CliParser &cli_parser) noexcept {
  yabt::cli::Subcommand &subcommand = cli_parser.register_subcommand(
      "lsp", *this, SHORT_DESCRIPTION, LONG_DESCRIPTION);

  return subcommand.register_flag({
      .name{"lsp-binary"},
      .short_name{},
      .optional = true,
      .type = yabt::cli::FlagType::STRING,
      .description{"Path to the lua-language-server binary "
                   "(default: lua-language-server)."},
      .handler{[this](const cli::Arg &a) {
        const cli::StringArg arg = std::get<cli::StringArg>(a);
        this->m_lsp_binary = arg.value;
        return runtime::Result<void, std::string>::ok();
      }},
  });
}

[[nodiscard]] runtime::Result<void, std::string> LspCommand::handle_subcommand(
    const std::span<const std::string_view> unparsed_args) noexcept {
  const std::optional<std::filesystem::path> ws_root =
      workspace::get_workspace_root();
  if (!ws_root.has_value()) {
    return runtime::Result<void, std::string>::error(
        std::format("Could not find workspace root. Are you sure your "
                    "directory tree contains a {} file?",
                    module::MODULE_FILE_NAME));
  }

  const auto modules =
      RESULT_PROPAGATE(workspace::open_workspace(ws_root.value()));

  std::vector<std::filesystem::path> library_paths;
  for (const auto &mod : modules) {
    if (const auto rules = mod->rules_dir(); rules.has_value()) {
      yabt_verbose("Adding library path: {}", rules.value().native());
      library_paths.push_back(rules.value());
    }
  }

  std::filesystem::path lua_config_dir_template{
      std::filesystem::temp_directory_path() / "yabt_lsp_XXXXXX"};
  std::string lua_config_dir_string = lua_config_dir_template.string();
  runtime::check(mkdtemp(lua_config_dir_string.data()) != nullptr,
                 "Error obtaining temporary directory");
  const std::filesystem::path lua_config_dir{lua_config_dir_string};

  std::error_code error_code;
  std::filesystem::create_directories(lua_config_dir_string, error_code);
  if (error_code) {
    return runtime::Result<void, std::string>::error(
        std::format("Failed to create build directory {}: {}",
                    lua_config_dir.native(), error_code.message()));
  }

  const std::filesystem::path stubs_dir = lua_config_dir / ".lua-stubs";
  const auto stubs = embed::get_embedded_lua_stubs();
  for (const auto &[name, content] : stubs) {
    const std::filesystem::path filepath = stubs_dir / name;
    std::error_code error_code;
    std::filesystem::create_directories(filepath.parent_path(), error_code);
    if (error_code) {
      return runtime::Result<void, std::string>::error(
          std::format("Failed to create parent directory for {}: {}",
                      filepath.native(), error_code.message()));
    }

    std::ofstream file{filepath};
    if (!file) {
      return runtime::Result<void, std::string>::error(
          std::format("Failed to write file {}", filepath.native()));
    }
    file << content;
  }

  yabt_verbose("Adding library path: {}", stubs_dir.native());
  library_paths.push_back(stubs_dir);

  const std::string json = generate_luarc_json(library_paths);

  const std::filesystem::path config_path = lua_config_dir / ".luarc.json";
  {
    std::ofstream config_file{config_path};
    if (!config_file) {
      return runtime::Result<void, std::string>::error(
          std::format("Failed to write config to {}", config_path.native()));
    }
    config_file << json;
  }
  yabt_verbose("Wrote LSP config to {}", config_path.native());

  // Build the argument list for lua-language-server.
  std::vector<std::string> args;
  args.push_back("--configpath");
  args.push_back(config_path.string());
  args.push_back("--workspace");
  args.push_back(ws_root.value().string());
  for (const std::string_view arg : unparsed_args) {
    args.emplace_back(arg);
  }

  process::Process proc{m_lsp_binary, std::span<const std::string>{args}};
  RESULT_PROPAGATE_DISCARD(proc.start());

  const auto output = proc.process_output();

  // Cleanup
  std::filesystem::remove_all(lua_config_dir);

  return output.to_result();
}

} // namespace yabt::cmd
