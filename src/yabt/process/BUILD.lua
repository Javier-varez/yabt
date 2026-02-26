local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('process.a'),
    srcs = ins('process.cpp'),
}
