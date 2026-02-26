local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('log.a'),
    srcs = ins('log.cpp'),
}
