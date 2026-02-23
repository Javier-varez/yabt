local ctx = require 'yabt.core.context'
local InPath = require 'yabt.core.path'.InPath
local OutPath = require 'yabt.core.path'.OutPath
local cc = require 'yabt.cc.cc'

local function ins(...)
    local arg = { ... }
    local r = {}
    for _, i in ipairs(arg) do
        table.insert(r, InPath:new_relative(i))
    end
    return r
end

local function out(o)
    return OutPath:new_relative(o)
end

-- Right now we require manually registering toolchains
require 'yabt.cc.gcc_toolchain'

local obj = cc.ObjectFile:new {
    out = out('main'),
    src = InPath:new_relative('main.cpp'),
}

obj:build(ctx)

local lib = cc.Library:new {
    out = out('lib.a'),
    srcs = ins('lib1.cc', 'lib2.c', 'lib3.S'),
}
lib:build(ctx)
