---@meta

-- Globals injected by yabt into every BUILD.lua sandbox.
-- These are not available in rule files (which use normal require), only in BUILD.lua.

--- Table into which BUILD.lua files assign their named targets.
--- Keys starting with an uppercase letter are "public" targets (visible outside the module).
---@type table<string, any>
targets = {}

--- Creates a single OutPath relative to the current BUILD.lua directory.
---@param path string
---@return OutPath
function out(path) end

--- Creates an array of OutPaths relative to the current BUILD.lua directory.
---@param ... string
---@return OutPath[]
function outs(...) end

--- Creates a single InPath relative to the current BUILD.lua directory.
---@param path string
---@return InPath
function inp(path) end

--- Creates an array of InPaths relative to the current BUILD.lua directory.
---@param ... string
---@return InPath[]
function ins(...) end

--- Imports rules of the BUILD.lua at `path` (a module-relative path like "mymod/subdir")
--- and returns a read-only view of its public targets. Private targets are not reachable
--- in the imported table.
---@param path string  Module-relative path, e.g. "yabt/embed"
---@return table<string, any>
function import(path) end
