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

---@class TargetsTable

local function create_targets_table()
    local targets_metatable = {
    }
    function targets_metatable.__index(_, k)
        return targets_metatable[k]
    end

    function targets_metatable.__newindex(_, k, v)
        if targets_metatable[k] ~= nil then
            error('Target '.. k.. ' already exists in this BUILD.lua')
        end
        targets_metatable[k] = v
    end


    ---@type TargetsTable
    local targets = {
        unwrap = function(self)
            local self_meta = getmetatable(self)
            local new = {}
            for k, v in pairs(self_meta) do
                if not string.match(k, '__.*') then
                    new[k] = v
                end
            end
            return new
        end
    }
    setmetatable(targets, targets_metatable)
    return targets
end

local yabt_native = require 'yabt_native'
local utils = require 'yabt.core.utils'
local in_progress_builds = {}
local build_failed = false
run_sandbox_for_mod = function(target_spec_path)
    local modname = get_module_name(target_spec_path)
    local mod = modules[modname]
    if mod == nil then
        error('Requested unknown module ' .. modname)
    end

    if utils.table_contains(in_progress_builds, target_spec_path) then
        local cycle = table.concat(in_progress_builds, ' -> ') .. ' -> ' .. target_spec_path
        error('Detected circular import cycle: ' .. cycle)
    end

    local sandbox = {
        targets = create_targets_table(),
    }
    append_path_methods(sandbox, mod.relative_path .. '/src/' .. target_spec_path .. '/')
    setmetatable(sandbox, sandbox_metatable)

    local file_path = modules[modname].path .. '/src/' .. target_spec_path .. '/BUILD.lua'

    local f = assert(loadfile(file_path))
    setfenv(f, sandbox)

    local saved_module_path = MODULE_PATH
    MODULE_PATH = mod.relative_path
    table.insert(in_progress_builds, target_spec_path)
    local ok, err = pcall(f)
    if not ok then
        yabt_native.log_error('Error running file ' .. file_path .. ': ' .. err)
        build_failed = true
    end
    table.remove(in_progress_builds, #in_progress_builds)
    MODULE_PATH = saved_module_path

    targets_per_path[target_spec_path] = sandbox.targets:unwrap()
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
