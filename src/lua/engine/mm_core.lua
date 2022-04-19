local ffi = require("ffi")

local function table_merge(...)
    local tables_to_merge = { ... }
    assert(#tables_to_merge > 1, "There should be at least two tables to merge them")

    for k, t in ipairs(tables_to_merge) do
        assert(type(t) == "table", string.format("Expected a table as function parameter %d", k))
    end

    local result = tables_to_merge[1]

    for i = 2, #tables_to_merge do
        local from = tables_to_merge[i]
        for k, v in pairs(from) do
            result[k] = v
        end
    end

    return result
end

local function register_components(self, components_table)
    self.component_types = table_merge(self.component_types, components_table)
end

local function register_events(self, events_table)
    self.event_types = table_merge(self.event_types, events_table)
end

return {
    component_types = {},
    event_types = {},
    register_components = register_components,
    register_events = register_events,
    table_merge = table_merge
}
