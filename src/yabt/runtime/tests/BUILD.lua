local gtest = require 'yabt.gtest'

targets.Tests = gtest.GtestBinary:new {
    out = out('runtime_tests'),
    srcs = ins('result_tests.cpp'),
}
