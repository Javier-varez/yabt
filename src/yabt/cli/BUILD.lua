local cc = require 'yabt.cc.cc'

targets.Lib = cc.Library:new {
    out = out('cli.a'),
    srcs = ins('cli_parser.cpp', 'flag.cpp')
}
