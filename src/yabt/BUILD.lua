local cc = require 'yabt_cc_rules.cc'
local pkg_config = require 'yabt_cc_rules.pkg-config'

local cxxflags = pkg_config.get_compile_flags('luajit')
local ldflags = pkg_config.get_link_flags('luajit')

local embed = import 'yabt/embed'
local rules = import 'yabt/embed/rules'

targets.lib = cc.Library:new {
    out = out('yabt.a'),
    srcs = ins(
        'build/build.cpp',
        'cli/cli_parser.cpp',
        'cli/flag.cpp',
        'cmd/build.cpp',
        'cmd/help.cpp',
        'cmd/lsp.cpp',
        'cmd/run.cpp',
        'cmd/sync.cpp',
        'cmd/test.cpp',
        'cmd/clean.cpp',
        'cmd/list.cpp',
        'log/log.cpp',
        'lua/lua_engine.cpp',
        'lua/path_lib.cpp',
        'lua/context_lib.cpp',
        'lua/log_lib.cpp',
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
        targets.lib,
        embed.Blob,
        rules.Utils,
    },
    cxxflags = cxxflags,
    ldflags_post = ldflags,
}
