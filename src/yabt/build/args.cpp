#include "yabt/build/args.h"

namespace yabt::build {

namespace {
[[nodiscard]] std::pair<std::vector<std::regex>, std::vector<BuildFlag>>
parse_build_args(std::span<const std::string_view> arguments) noexcept {
  std::vector<BuildFlag> flags;
  std::vector<std::regex> patterns;
  for (const std::string_view arg : arguments) {
    const size_t delimiter = arg.find_first_of('=');
    if (delimiter != std::string_view::npos) {
      flags.push_back(BuildFlag{.key{arg.substr(0, delimiter)},
                                .value{arg.substr(delimiter + 1)}});
    } else {
      patterns.emplace_back(std::string{arg});
    }
  }

  return std::make_pair(patterns, flags);
}
} // namespace

[[nodiscard]] BuildArgs
parse_args(std::span<const std::string_view> arguments) noexcept {
  using std::string_view_literals::operator""sv;
  std::span<const std::string_view> non_exec_args{};
  std::span<const std::string_view> exec_args{};
  if (const auto iter = std::find(arguments.begin(), arguments.end(), ":"sv);
      iter != arguments.end()) {
    const size_t pos = iter - arguments.begin();
    exec_args = arguments.subspan(pos + 1);
    non_exec_args = arguments.subspan(0, pos);
  } else {
    non_exec_args = arguments;
  }

  auto [target_patterns, flags] = parse_build_args(non_exec_args);
  return BuildArgs{
      .target_patterns{std::move(target_patterns)},
      .flags{std::move(flags)},
      .exec_args{exec_args},
  };
}

} // namespace yabt::build
