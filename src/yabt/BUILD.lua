local cc = require 'yabt.cc.cc'

local pkg_config = require 'yabt.pkg-config.pkg-config'

targets.Bin = cc.Binary:new {
    out = out('yabt'),
    srcs = ins(
        'main.cpp',
        'build/build.cpp',
        'cli/cli_parser.cpp',
        'cli/flag.cpp',
        'cmd/build.cpp',
        'cmd/help.cpp',
        'cmd/sync.cpp',
        'log/log.cpp',
        'lua/lua_engine.cpp',
        'module/git_module.cpp',
        'module/module.cpp',
        'module/module_file.cpp',
        'ninja/ninja.cpp',
        'process/process.cpp',
        'utils/string.cpp',
        'workspace/utils.cpp'
    ),
    cxxflags = pkg_config.get_compile_flags('luajit'),
    ldflags_post = pkg_config.get_link_flags('luajit'),
}
