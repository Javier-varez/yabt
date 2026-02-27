local cc = require 'yabt.cc.cc'

local pkg_config = require 'yabt.pkg-config.pkg-config'
local cxxflags = pkg_config.get_compile_flags('luajit')
local ldflags = pkg_config.get_link_flags('luajit')

targets.Lib = cc.Library:new {
    out = out('yabt.a'),
    srcs = ins(
        'build/build.cpp',
        'cli/cli_parser.cpp',
        'cli/flag.cpp',
        'cmd/build.cpp',
        'cmd/help.cpp',
        'cmd/sync.cpp',
        'cmd/clean.cpp',
        'log/log.cpp',
        'lua/lua_engine.cpp',
        'module/git_module.cpp',
        'module/module.cpp',
        'module/module_file.cpp',
        'ninja/ninja.cpp',
        'process/process.cpp',
        'utils/string.cpp',
        'workspace/utils.cpp',
        'embed/embed.cpp'
    ),
    cxxflags = cxxflags,
}

targets.Bin = cc.Binary:new {
    out = out('yabt'),
    srcs = ins(
        'main.cpp'
    ),
    deps = {
        targets.Lib,
    },
    cxxflags = cxxflags,
    ldflags_post = ldflags,
}
