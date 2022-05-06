local ffi = require('ffi')
ffi.cdef[[
    struct Vec2 {float x, y;};
    struct Vec3 {float x, y, z;};
    struct Vec4 {float x, y, z, w;};
    struct RGB {float r, g, b;};
    struct RGBA {float r, g, b, a;};
    uint32_t get_ref (const char* name);

    struct NameOnlyEvent {};
]]
local C = ffi.C
require('jit').opt.start('hotloop=28', 'maxmcode=2048', 'sizemcode=64', 'maxtrace=2000', 'maxrecord=8000')

local function table_merge(...)
    local tables_to_merge = { ... }
    assert(#tables_to_merge > 1, 'There should be at least two tables to merge them')

    for k, t in ipairs(tables_to_merge) do
        assert(type(t) == 'table', string.format('Expected a table as function parameter %d', k))
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

local function register_events(self, type_list)
    for _, info in ipairs(type_list) do
        local type_struct = 'struct ' .. info.type
        local ctype = type_struct .. '*'
        self.types_by_name[info.name] = {
            type = ctype,
            size = ffi.sizeof(type_struct),
        }
        self.types_by_id[C.get_ref(info.name)] = ctype
    end
end

local function register_entity_script(self, script)
    self.message_maps[script.resource_id] = script.message_map
end

local function register_scene_script(self, script)
    self.scene_event_maps[script.resource_id] = script.event_map
end

local function unregister_entity_script(self, resource_id)
    table.remove(self.message_maps, resource_id)
end

local obj = {
    -- Components
    component_types = {},
    -- Event/message types
    types_by_name = {},
    types_by_id = {},
    -- ScriptedBehavior events
    message_maps = {},
    -- Scene events
    scene_event_maps = {},
    -- API
    register_components = register_components,
    register_events = register_events,
    register_entity_script = register_entity_script,
    register_scene_script = register_scene_script,
    unregister_entity_script = unregister_entity_script,
    table_merge = table_merge
}

obj:register_events({
    {name="engine/exit", type="NameOnlyEvent"},
})

return obj