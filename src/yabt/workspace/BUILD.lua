local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('workspace.a'),
    srcs = ins('utils.cpp'),
}
