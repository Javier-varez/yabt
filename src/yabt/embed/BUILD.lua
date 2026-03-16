local blob = require 'yabt_cc_rules.blob'
local path = require 'yabt.core.path'

targets.Blob = blob.Blob:new {
    out = out('runtime.lua.o'),
    inp = inp('runtime.lua'),
    base = path.InPath:new_relative('src'),
}
