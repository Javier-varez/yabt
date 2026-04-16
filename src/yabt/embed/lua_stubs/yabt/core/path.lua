---@meta

-- Stub for yabt.core.path, which is a C++-native module.
-- InPath and OutPath are filesystem path objects backed by the C++ implementation of std::filesystem::path.

--- Parent class used to abstract common InPath and OutPath methods.
---@class Path
---@field absolute fun(self: Path): string                 Returns the absolute path string.
---@field relative fun(self: Path): string                 Returns the path relative to its base dir.
---@field relative_to fun(self: Path, base: Path): string  Returns this path relative to `base`.
---@field ext fun(self: Path): string?                     Returns the file extension (e.g. ".cpp"), or nil.
---@field with_ext fun(self: Path, ext: string): OutPath   Returns a copy with the extension replaced.
---@field join fun(self: Path, chunk: string): OutPath     Appends `chunk` and returns an OutPath.

---@class InPath : Path
---@field new_relative fun(self: InPath, path: string): InPath     Constructs from a source-relative path.
---@field new_in_module fun(self: InPath, module: string, path?: string): InPath  Constructs from a module-relative path.
local InPath = {}

---@class OutPath : Path
---@field new_relative fun(self: OutPath, path: string): OutPath   Constructs from an output-relative path.
local OutPath = {}

---@param p any
---@return boolean -- True if `p` is an InPath or OutPath.
local function is_path(p) end

---@param p any
---@return boolean -- True if `p` is an OutPath.
local function is_out_path(p) end

return {
    InPath = InPath,
    OutPath = OutPath,
    is_path = is_path,
    is_out_path = is_out_path,
}
