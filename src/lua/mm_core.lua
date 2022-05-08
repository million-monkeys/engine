local ffi = require('ffi')
ffi.cdef[[
    struct Vec2 {float x, y;};
    struct Vec3 {float x, y, z;};
    struct Vec4 {float x, y, z, w;};
    struct RGB {float r, g, b;};
    struct RGBA {float r, g, b, a;};
    uint32_t get_ref (const char* name);

    struct NameOnlyEvent {};
    struct SceneActivatedEvent {uint32_t id;};
]]
local C = ffi.C

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

local obj = {
    -- Various script settings
    config = {
        max_iterations = 15,
    },
    -- Components
    component_types = {},
    -- Event/message types
    types_by_name = {},
    types_by_id = {},
    -- ScriptedBehavior messages
    message_maps = {},
    -- Game events
    game_event_map = {},
    -- Scene events
    scene_event_maps = {},
    -- API, used internally by the engine to register information with Lua
    register_components = register_components,
    register_events = register_events,
    register_game_script = function(self, script)
        self.game_event_map = script
    end,
    register_scene_script = function(self, script)
        self.scene_event_maps[script.resource_id] = script.event_map
    end,
    unregister_scene_script = function(self, resource_id)
        table.remove(self.scene_event_maps, resource_id)
    end,
    register_entity_script = function(self, script)
        self.message_maps[script.resource_id] = script.message_map
    end,
    unregister_entity_script = function(self, resource_id)
        table.remove(self.message_maps, resource_id)
    end,
    table_merge = table_merge
}

obj:register_events({
    {name="engine/exit", type="NameOnlyEvent"},
    {name="scene/activated", type="SceneActivatedEvent"},
})

return obj