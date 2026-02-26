local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('build.a'),
    srcs = ins('build.cpp'),
}
