local M = {}

---@param path string
local function dir_and_basename(path)
    return path:match("(.*/)(.*)")
end

---@class Path
---@field protected _relative string
local Path = {}

---@return string # Absolute representation of the path
---@nodiscard
function Path:absolute()
    if SOURCE_DIR:sub(#SOURCE_DIR, #SOURCE_DIR) ~= '/' then
        -- TODO: this code would benefit from a native path representation in C++ using
        -- std::filesystem::path...
        return SOURCE_DIR .. '/' .. self._relative
    end
    return SOURCE_DIR .. self._relative
end

local function newPath()
    local o = {}
    setmetatable(o, Path)
    Path.__index = Path
    return o
end

---@class InPath: Path
local InPath = newPath()

---@class OutPath: Path
local OutPath = newPath()

---@return string # the relative path as a string
---@nodiscard
function Path:relative()
    return self._relative
end

---@return string # the extension of the filepath
---@nodiscard
function Path:ext()
    return self._relative:match(".*%.([^%.]*)")
end

---@param ext string an extension to add
---@return OutPath # a new output path replacing the extension with the given one.
---@nodiscard
function Path:withExt(ext)
    local n, r = self._relative:gsub('%.[^%.]*$', '.' .. ext)
    if r == 0 then
        n = self._relative .. '.' .. ext
    end
    return OutPath:new_relative(n)
end

---@param prefix string a prefix to add
---@return OutPath # a new output path prefixing the basename with the given prefix
---@nodiscard
function Path:withPrefix(prefix)
    local dir, basename = dir_and_basename(self._relative)
    if dir == nil or basename == nil then
        return OutPath:new_relative(prefix .. self._relative)
    end
    return OutPath:new_relative(dir .. prefix .. basename)
end

---@param suffix string a suffix to add
---@return OutPath # a new output path prefixing the basename with the given suffix
---@nodiscard
function Path:withSuffix(suffix)
    return OutPath:new_relative(self._relative .. suffix)
end

---@param path string The path as a string
---@return InPath # An input path at the given location
---@nodiscard
function InPath:new_relative(path)
    local o = { _relative = path }
    setmetatable(o, self)
    self.__index = self
    return o
end

---@param path string The path as a string
---@return OutPath # An input path at the given location
---@nodiscard
function OutPath:new_relative(path)
    local o = { _relative = path }
    setmetatable(o, self)
    self.__index = self
    return o
end

---@return string # Absolute representation of the path
---@nodiscard
function OutPath:absolute()
    if OUTPUT_DIR:sub(#OUTPUT_DIR, #OUTPUT_DIR) ~= '/' then
        -- TODO: this code would benefit from a native path representation in C++ using
        -- std::filesystem::path...
        return OUTPUT_DIR .. '/' .. self._relative
    end
    return OUTPUT_DIR .. self._relative
end

M.InPath = InPath
M.OutPath = OutPath

---@param v any
---@return boolean
---@nodiscard
function M.isOutPath(v)
    return getmetatable(v) == OutPath
end

---@param v any
---@return boolean
---@nodiscard
function M.isPath(v)
    local meta = getmetatable(v)
    return meta == InPath or meta == OutPath
end

return M
