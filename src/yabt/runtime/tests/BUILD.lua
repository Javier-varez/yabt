local gtest = require 'yabt.gtest'

targets.Tests = gtest.GtestBinary:new {
    out = out('runtime_tests'),
    srcs = ins('error_tests.cpp', 'result_tests.cpp'),
}
