-- Engine Lua code to run ScriptedBehavior
local ffi = require("ffi")
local C = ffi.C
ffi.cdef[[
typedef Entity uint32_t;
typedef Resource uint32_t;
struct ScriptedBehavior {
    Entity entity_id;
    Resource resource_id;
};
struct Event {
    uint32_t type;
    Entity _sender;
    const char* source;
    void* _data;
};
]]



function load ()
    -- BEGIN USER SCRIPT
    -- $insert user-script
    function handle_foo_event (entity, event)
        -- `entity` is the entity to which this ScriptedBehavior is attached
        -- This is a lightweight wrapper that exposes access to the entities components using the get method
        -- `event` is the instance of the event type that was mapped to this function
        -- `event:sender()` returns the entity that sent this event, if any. This is a function to avoid constructing the entity object unless used
        -- `event.source` returns the name of the system that sent this event, if any
        -- Events are dispatched serially and have exclusive read-write access to the entity component system data
        local position = entity:get('position')
        position.x = event.x
    end
    -- END USER SCRIPT
    
    -- Return event map
    return {
        -- BEGIN EVENT MAP
        -- $insert event-map
        C.event('foo_event')=handle_foo_event
        -- END EVENT MAP
    }
end

function get_component (self, component_name)
    return C.get_component_for_entity(self.id, component_name)
end

function make_event_type (type)
    local type_name = C.get_event_type_name(type)
    local mt = {
        __index = {
            'type'=function(self) return type_name end,
            'sender'=function(self)
                return {
                    id=self._sender,
                    get=get_component
                }
            end
        }
    }
    local event_obj = ffi.metatype(event_types[event.type], mt)
    return event_obj
end

function handle_events ()
    local events = ffi.new('Event*')
    local num_events = C.get_events(events)
    local entity_obj = {
        'get'=get_component
    }

    local scripted_behaviors = ffi.new('ScriptedBehavior*')
    local num_scripted_behaviors = C.get_scripted_behaviors(scripted_behaviors)
    for index = 0, num_scripted_behaviors  do
        local scripted_behavior = scripted_behaviors[index]
        entity_obj.id = scripted_behavior.entity_id
        local event_map = event_maps[scripted_behavior.resource_id]
        if event_map do
            for index = 0, num_events do
                local event = events[index];
                local handler = event_map[event.type]
                if handler do
                    handler(entity_obj, event)
                end
            end
        end
    end
end