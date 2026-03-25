local gtest = require 'yabt.gtest'

targets.BasicTest = gtest.GtestBinary:new {
    out = out('basic_test'),
    srcs = ins('basic_test.cpp'),
}
