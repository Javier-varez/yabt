-- gtest.lua: Google Test binary rules for yabt

local M = {}

local cc = require 'yabt_cc_rules.cc'
local path = require 'yabt.core.path'

---@param p string the file path inside googletest
local function gtest_path(p)
    return path.InPath:new_in_module('googletest', p)
end

-- These path objects are created once at module load time.
-- PathLib is registered before BUILD.lua files run, so this is safe.
local gtest_include = gtest_path('googletest/include')
local gtest_src_include = gtest_path('googletest')
local gtest_out = gtest_src_include:with_ext('/libgtest.a')
local gtest_main_out = gtest_src_include:with_ext('/libgtest_main.a')

local gmock_include = gtest_path('googlemock/include')
local gmock_src_include = gtest_path('googlemock')
local gmock_out = gmock_src_include:with_ext('/libgmock.a')

-- Singletons shared across all GtestBinary targets in one build.
-- Created lazily inside GtestBinary:new (where MODULE_PATH is set).

---@type CcLibrary
local gtest_lib = nil

---@type CcLibrary
local gtest_main_lib = nil

---@type CcLibrary
local gmock_lib = nil

---@type boolean
local gtest_libs_built = false

-- Create the gtest, gtest_main, and gmock library objects if not done yet.
-- Must be called when MODULE_PATH is available (i.e. during BUILD.lua execution).
local function ensure_gtest_libs_exist()
    if gtest_lib ~= nil then return end

    gtest_lib = cc.Library:new {
        out = gtest_out,
        srcs = { gtest_path('googletest/src/gtest-all.cc') },
        includes = { gtest_include, gtest_src_include },
    }

    gtest_main_lib = cc.Library:new {
        out = gtest_main_out,
        srcs = { gtest_path('googletest/src/gtest_main.cc') },
        includes = { gtest_include },
    }

    gmock_lib = cc.Library:new {
        out = gmock_out,
        srcs = { gtest_path('googlemock/src/gmock-all.cc') },
        includes = { gtest_include, gmock_include, gmock_src_include },
    }
end

---@class GtestBinary
---@field out OutPath
---@field srcs Path[]
---@field deps ?Path[]
---@field includes ?string[]
---@field cflags ?string[]
---@field cxxflags ?string[]
---@field asflags ?string[]
---@field ldflags ?string[]
---@field ldflags_post ?string[]
---@field toolchain ?Toolchain
---@field private binary ?CcBinary
local GtestBinary = {}

---@param bin GtestBinary
function GtestBinary:new(bin)
    ensure_gtest_libs_exist()

    bin.deps = bin.deps or {}
    table.insert(bin.deps, gtest_lib)
    table.insert(bin.deps, gtest_main_lib)
    table.insert(bin.deps, gmock_lib)

    bin.ldflags_post = bin.ldflags_post or {}
    table.insert(bin.ldflags_post, '-lpthread')

    -- Build the underlying CcBinary, which depends on gtest + gtest_main.
    bin.binary = cc.Binary:new {
        out = bin.out,
        srcs = bin.srcs,
        deps = bin.deps,
        includes = bin.includes,
        cflags = bin.cflags,
        cxxflags = bin.cxxflags,
        asflags = bin.asflags,
        ldflags = bin.ldflags,
        ldflags_post = bin.ldflags_post,
    }

    setmetatable(bin, self)
    self.__index = self
    return bin
end

---@param ctx Context
function GtestBinary:build(ctx)
    -- Build the shared gtest libraries exactly once across all test targets.
    if not gtest_libs_built then
        ensure_gtest_libs_exist()
        gtest_lib:build(ctx)
        gtest_main_lib:build(ctx)
        gmock_lib:build(ctx)
        gtest_libs_built = true
    end
    self.binary:build(ctx)
end

---@param args string[]
function GtestBinary:test(args)
    return self.binary:run(args)
end

M.GtestBinary = GtestBinary

return M
