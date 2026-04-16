---@meta

-- Stub for yabt.core.context, which is a C++-native module.
-- The module itself is passed as `ctx` to each target's build() method.

---@class BuildRule
---@field name string        Unique rule name used by ninja.
---@field cmd string         The command template (uses ninja variables like $in, $out, $flags, etc.).
---@field descr? string      Human-readable description shown during build.
---@field variables? table<string, string>  Extra ninja default rule variables (e.g. depfile).
---@field compdb? boolean    Whether this rule should appear in compile_commands.json.

---@class BuildStep
---@field outs OutPath[]     Resulting paths out of the compilation process.
---@field ins Path[]         Input paths used for the compilation process. These are used to generate build dependency links.
---@field cmd string         The command to build the outputs.
---@field descr? string      Human-readable description shown during build.
---@field variables? table<string, string> Extra ninja variables used in the generated rule.

---@class BuildStepWithRule
---@field outs OutPath[]     Resulting paths out of the compilation process.
---@field ins Path[]         Input paths used for the compilation process. These are used to generate build dependency links.
---@field rule_name string   The name of the rule used to convert the inputs to outputs.
---@field variables? table<string, string> Overrides the default variables that are defined in the build rule.

---@class Context
---@field add_build_step fun(step: BuildStep)    Registers a build step in the global context.
---@field add_build_step_with_rule fun(step: BuildStepWithRule, rule: BuildRule)    Registers a build step with a generic rule in the global context.
---@field register_run_fn fun(fn: fun(args: string[]): string[])    Registers a runnable for a given target
---@field register_test_fn fun(fn: fun(args: string[]): string[])   Registers a testable for a given target.

---@type Context
local M

return M
