local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('utils.a'),
    srcs = ins('string.cpp'),
}
