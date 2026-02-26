local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('ninja.a'),
    srcs = ins('ninja.cpp'),
}
