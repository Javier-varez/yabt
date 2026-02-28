---@param unsanitized Path[]
---@return string[]
local function validateInputPaths(unsanitized)
    local path = require 'yabt.core.path'

    local ins = {}
    for _, input in ipairs(unsanitized) do
        if not path.isPath(input) then
            error('input is not a Path: ' .. input)
        end

        table.insert(ins, input:absolute())
    end
    return ins
end

---@param unsanitized Path[]
---@return string[]
local function validateOutputPaths(unsanitized)
    local path = require 'yabt.core.path'

    if unsanitized == nil or #unsanitized == 0 then
        error('No output provided for the build target')
    end

    local outs = {}
    for _, out in ipairs(unsanitized) do
        if not path.isOutPath(out) then
            error('output is not an OutPath: ' .. out)
        end

        table.insert(outs, out:absolute())
    end
    return outs
end

---@class Context
---@field add_build_step fun(BuildStep: BuildStep): nil
---@field add_build_step_with_rule fun(BuildStepWithRule: BuildStepWithRule, BuildRule: BuildRule): nil
local M = {
    ---@param build_step BuildStep
    add_build_step = function(build_step)
        local outs = validateOutputPaths(build_step.outs)
        local ins = validateInputPaths(build_step.ins)

        local native = require 'yabt_native'
        native.add_build_step {
            outs = outs,
            ins = ins,
            cmd = build_step.cmd,
            descr = build_step.descr,
        }
    end,

    ---@param build_step BuildStepWithRule
    ---@param rule BuildRule
    add_build_step_with_rule = function(build_step, rule)
        local outs = validateOutputPaths(build_step.outs)
        local ins = validateInputPaths(build_step.ins)

        local native = require 'yabt_native'
        native.add_build_step_with_rule({
            outs = outs,
            ins = ins,
            rule_name = build_step.rule_name,
            variables = build_step.variables,
        }, rule)
    end,

    ---@param target_spec_path string
    ---@param target_name string
    ---@param fn fun(): nil
    handle_target = function(target_spec_path, target_name, fn)
        local native = require 'yabt_native'
        native.handle_target(target_spec_path, target_name, fn)
    end
}

---@alias Pool string A Ninja pool

---@class BuildStep
---@field outs OutPath[]
---@field ins Path[]
---@field cmd string
---@field descr string

---@class BuildStepWithRule
---@field outs OutPath[]
---@field ins Path[]
---@field rule_name string
---@field variables? { [string]: string }

---@class BuildRule
---@field name string
---@field cmd string
---@field descr string
---@field variables? { [string]: string }

return M
