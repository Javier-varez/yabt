local blob = require 'yabt_cc_rules.blob'
local path = require 'yabt.core.path'

targets.Utils = blob.Blob:new {
    out = out('utils.lua.o'),
    inp = inp('yabt/core/utils.lua'),
    base = path.InPath:new_in_module('yabt', 'src'),
}
