local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('lua.a'),
    srcs = ins('lua_engine.cpp'),
}
