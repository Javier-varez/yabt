local M = {}

local fileExtToLangMap = {
    ['cc'] = 'C++',
    ['cpp'] = 'C++',
    ['hh'] = 'C++',
    ['hpp'] = 'C++',
    ['c'] = 'C',
    ['h'] = 'C',
    ['s'] = 'Asm',
    ['S'] = 'Asm',
}

---@param ext string
local function file_extension_to_language(ext)
    local lang = fileExtToLangMap[ext]
    if lang == nil then
        error('Unknown language for extension ' .. ext)
    end
    return lang
end

---@param toolchain Toolchain
---@return BuildRule # Build rule for the given toolchain
local function cxx_rule_for_toolchain(toolchain)
    return {
        name = toolchain.name .. '-cxx',
        cmd = toolchain.cxxcompiler ..
            ' -c ' .. table.concat(toolchain.cxxflags, ' ') .. ' $flags -o $out ' .. '-MD -MF $out.d -pipe $in',
        descr = 'CXX (toolchain: ' .. toolchain.name .. ') $out',
    }
end

---@param toolchain Toolchain
---@return BuildRule # Build rule for the given toolchain
local function c_rule_for_toolchain(toolchain)
    return {
        name = toolchain.name .. '-c',
        cmd = toolchain.ccompiler ..
            ' -c ' .. table.concat(toolchain.cflags, ' ') .. ' $flags -o $out ' .. '-MD -MF $out.d -pipe $in',
        descr = 'C (toolchain: ' .. toolchain.name .. ') $out',
    }
end

---@param toolchain Toolchain
---@return BuildRule # Build rule for the given toolchain
local function asm_rule_for_toolchain(toolchain)
    return {
        name = toolchain.name .. '-as',
        cmd = toolchain.assembler ..
            ' -c ' .. table.concat(toolchain.asflags, ' ') .. ' $flags -o $out ' .. '-MD -MF $out.d -pipe $in',
        descr = 'ASM (toolchain: ' .. toolchain.name .. ') $out',
    }
end

---@param toolchain Toolchain
---@return BuildRule # Build rule for the given toolchain
local function ar_rule_for_toolchain(toolchain)
    return {
        name = toolchain.name .. '-ar',
        cmd = 'rm -f $out 2> /dev/null; ' .. toolchain.archiver .. ' rcsT $out $in',
        descr = 'AR (toolchain: ' .. toolchain.name .. ') $out',
    }
end

local lang_to_rule_generator = {
    ['C++'] = cxx_rule_for_toolchain,
    ['C'] = c_rule_for_toolchain,
    ['Asm'] = asm_rule_for_toolchain,
}

---@param language string
---@return BuildRule
local function build_rule_for_language_and_toolchain(language, toolchain)
    local rule = lang_to_rule_generator[language]
    if rule == nil then
        error('Unknown language' .. language)
    end
    return rule(toolchain)
end

local lang_to_flag_member = {
    ['C++'] = 'cxxflags',
    ['C'] = 'cflags',
    ['Asm'] = 'asflags',
}

---@class ObjectFile
---@field out OutPath
---@field src Path
---@field includes ?Path[]
---@field cflags ?string[]
---@field cxxflags ?string[]
---@field asflags ?string[]
---@field toolchain ?Toolchain
local ObjectFile = {}

---@param obj ObjectFile
function ObjectFile:new(obj)
    setmetatable(obj, self)
    self.__index = self
    return obj
end

---@param ctx Context
function ObjectFile:build(ctx)
    local selected_toolchain = require 'yabt.cc.toolchain'.selected_toolchain()
    local toolchain = self.toolchain or selected_toolchain

    local language = file_extension_to_language(self.src:ext())
    local build_rule = build_rule_for_language_and_toolchain(language, toolchain)

    local flags = self[lang_to_flag_member[language]] or {}
    if self.includes ~= nil then
        for _, inc in ipairs(self.includes) do
            table.insert(flags, '-I' .. inc)
        end
    end

    local build_step = {
        outs = { self.out },
        ins = { self.src },
        rule_name = build_rule.name,
        variables = {
            flags = table.concat(flags, ' '),
        },
    }

    ctx.add_build_step_with_rule(build_step, build_rule)
end

---@class Dep
---@field cc_library fun(self: Dep): Library

---@class Library
---@field out OutPath
---@field srcs ?Path[]
---@field deps ?Dep[]
---@field includes ?Path[]
---@field cflags ?string[]
---@field cxxflags ?string[]
---@field asflags ?string[]
---@field toolchain ?Toolchain
---@field always_link ?boolean
local Library = {}

---@param obj Library
function Library:new(obj)
    setmetatable(obj, self)
    self.__index = self
    return obj
end

---@param ctx Context
function Library:build(ctx)
    local objs = {}
    for _, src in ipairs(self.srcs) do
        local obj = ObjectFile:new({
            out = src:withExt('o'),
            src = src,
            includes = self.includes,
            cxxflags = self.cxxflags,
            cflags = self.cflags,
            asflags = self.asflags,
            toolchain = self.toolchain,
        })
        obj:build(ctx)
        table.insert(objs, obj.out)
    end
    local selected_toolchain = require 'yabt.cc.toolchain'.selected_toolchain()
    local toolchain = self.toolchain or selected_toolchain

    local build_rule = ar_rule_for_toolchain(toolchain)

    -- FIXME: Handle library dependencies as well
    local build_step = {
        outs = { self.out },
        ins = objs,
        rule_name = build_rule.name,
    }
    ctx.add_build_step_with_rule(build_step, build_rule)
end

M.ObjectFile = ObjectFile
M.Library = Library

return M
