local blob = require 'yabt_cc_rules.blob'
local path = require 'yabt.core.path'

targets.ContextBlob = blob.Blob:new {
    out = out('yabt_core_context.lua.o'),
    inp = inp('yabt/core/context.lua'),
    base = path.InPath:new_in_module('yabt', 'src'),
}

targets.LogBlob = blob.Blob:new {
    out = out('yabt_core_log.lua.o'),
    inp = inp('yabt/core/log.lua'),
    base = path.InPath:new_in_module('yabt', 'src'),
}

targets.PathBlob = blob.Blob:new {
    out = out('yabt_core_path.lua.o'),
    inp = inp('yabt/core/path.lua'),
    base = path.InPath:new_in_module('yabt', 'src'),
}

targets.GlobalsBlob = blob.Blob:new {
    out = out('build_globals.lua.o'),
    inp = inp('build_globals.lua'),
    base = path.InPath:new_in_module('yabt', 'src'),
}
