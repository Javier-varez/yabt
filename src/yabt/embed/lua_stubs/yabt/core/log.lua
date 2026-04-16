---@meta

-- Stub for yabt.core.log, which is a C++-native module.

--- Logs the message as verbose
---@param msg string
local function verbose(msg) end

--- Logs the message as debug
---@param msg string
local function debug(msg) end

--- Logs the message as info
---@param msg string
local function info(msg) end

--- Logs the message as warn
---@param msg string
local function warn(msg) end

--- Logs the message as error
---@param msg string
local function error(msg) end

return {
    verbose = verbose,
    debug = debug,
    info = info,
    warn = warn,
    error = error,
}
