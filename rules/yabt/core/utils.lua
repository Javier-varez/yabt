local M = {}

---@param table any[]
---@param elem any
function M.table_contains(table, elem)
    for _, cur in ipairs(table) do
        if elem == cur then
            return true
        end
    end
    return false
end

return M
