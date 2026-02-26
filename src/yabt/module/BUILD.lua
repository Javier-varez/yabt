local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('module.a'),
    srcs = ins('module.cpp', 'git_module.cpp', 'module_file.cpp'),
}
