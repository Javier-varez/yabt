local targets_per_path = {}

local function get_module_name(path)
    return string.match(path, '^([^/]*).*')
end

local run_sandbox_for_mod = nil

local function import(path)
    local modname = get_module_name(path)
    if targets_per_path[path] == nil then
        -- FIXME: Protect against circular imports
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

local InPath = require 'yabt.core.path'.InPath
local OutPath = require 'yabt.core.path'.OutPath

local function append_path_methods(t, path)
    local relpath = '/src/' .. path .. '/'
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

run_sandbox_for_mod = function(file)
    local modname = get_module_name(file)
    local mod = modules[modname]
    if mod == nil then
        error('Requested unknown module ' .. mod)
    end

    local sandbox = {
        -- TODO: Force this to only be able to ADD to it
        targets = {},
    }
    append_path_methods(sandbox, file)
    setmetatable(sandbox, sandbox_metatable)

    local file_path = modules[modname].path .. '/src/' .. file .. '/BUILD.lua'

    local f = assert(loadfile(file_path))
    setfenv(f, sandbox)

    local ok, err = pcall(f)
    if not ok then
        print('Error running file: ' .. err)
    end

    targets_per_path[file] = sandbox.targets
end

for _, mod in pairs(modules) do
    for _, file in ipairs(mod.build_files) do
        run_sandbox_for_mod(file)
    end
end

local ctx = require 'yabt.core.context'
for path, targets in pairs(targets_per_path) do
    for target_name, target in pairs(targets) do
        print('Registering target ' .. target_name .. ' in path ' .. path)
        target:build(ctx)
    end
end
