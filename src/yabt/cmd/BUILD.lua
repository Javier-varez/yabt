local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('cmd.a'),
    srcs = ins('build.cpp', 'help.cpp', 'sync.cpp'),
}
