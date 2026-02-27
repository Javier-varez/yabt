local targets_per_path = {}

local function get_module_name(path)
    return string.match(path, '^([^/]*).*')
end

local run_sandbox_for_mod = nil

local function import(path)
    local modname = get_module_name(path)
    if targets_per_path[path] == nil then
        run_sandbox_for_mod(path)
        return targets_per_path[path]
    end
    return targets_per_path[path]
end

local allowed_globals = {
    print = print,
    require = require,
    import = import,
}

local sandbox_metatable = {
    __index = allowed_globals,
    __newindex = function(_, _)
        error('Creating new global variables is not supported in BUILD.lua files')
    end
}

local InPath = require('yabt.core.path').InPath
local OutPath = require('yabt.core.path').OutPath

local function append_path_methods(t, relpath)
    t.inp = function(v)
        return InPath:new_relative(relpath .. v)
    end

    t.ins = function(...)
        local arg = { ... }
        local r = {}
        for _, v in ipairs(arg) do
            table.insert(r, InPath:new_relative(relpath .. v))
        end
        return r
    end

    t.out = function(v)
        return OutPath:new_relative(relpath .. v)
    end

    t.outs = function(...)
        local arg = { ... }
        local r = {}
        for _, v in ipairs(arg) do
            table.insert(r, OutPath:new_relative(relpath .. v))
        end
        return r
    end
end

local yabt_native = require 'yabt_native'
local utils = require 'yabt.core.utils'
local in_progress_builds = {}
local build_failed = false
run_sandbox_for_mod = function(build_spec)
    local modname = get_module_name(build_spec)
    local mod = modules[modname]
    if mod == nil then
        error('Requested unknown module ' .. modname)
    end

    if utils.table_contains(in_progress_builds, build_spec) then
        local cycle = table.concat(in_progress_builds, ' -> ') .. ' -> ' .. build_spec
        error('Detected circular import cycle: ' .. cycle)
    end

    local sandbox = {
        -- TODO: Force this to only be able to ADD to it
        targets = {},
    }
    append_path_methods(sandbox, mod.relative_path .. '/src/' .. build_spec .. '/')
    setmetatable(sandbox, sandbox_metatable)

    local file_path = modules[modname].path .. '/src/' .. build_spec .. '/BUILD.lua'

    local f = assert(loadfile(file_path))
    setfenv(f, sandbox)

    local saved_module_path = MODULE_PATH
    MODULE_PATH = mod.relative_path
    table.insert(in_progress_builds, build_spec)
    local ok, err = pcall(f)
    if not ok then

        -- FIXME: Use logging system here
        yabt_native.log_error('Error running file ' .. file_path .. ': ' .. err)
        build_failed = true
    end
    table.remove(in_progress_builds, #in_progress_builds)
    MODULE_PATH = saved_module_path

    targets_per_path[build_spec] = sandbox.targets
end

for _, mod in pairs(modules) do
    for _, file in ipairs(mod.build_files) do
        run_sandbox_for_mod(file)
    end
end

if build_failed then
    error('Failed to process some build specs')
end

local ctx = require 'yabt.core.context'
for _, targets in pairs(targets_per_path) do
    for _, target in pairs(targets) do
        target:build(ctx)
    end
end
