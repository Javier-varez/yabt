local gtest = require 'yabt.gtest'
local pkg_config = require 'yabt_cc_rules.pkg-config'

local yabt = import 'yabt'

local cxxflags = pkg_config.get_compile_flags('luajit')
local ldflags = pkg_config.get_link_flags('luajit')

targets.Tests = gtest.GtestBinary:new {
    out = out('lua_tests'),
    srcs = ins('flag_lib_tests.cpp'),
    deps = {
        -- FIXME: Should use a more granular library dependency here...
        yabt.Lib
    },
    cxxflags = cxxflags,
    ldflags = ldflags,
}
