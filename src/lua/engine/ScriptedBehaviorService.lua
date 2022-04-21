-- Engine Lua code to run ScriptedBehavior
local ffi = require("ffi")
local C = ffi.C
ffi.cdef[[
struct ScriptedBehavior {
    uint32_t entity_id;
    uint32_t resource_id;
};
struct EventEnvelope {
    uint32_t type;
    uint32_t target;
    void* data;
};
]]
local core = require('core')
local mm = require('mm_script_api')

local function make_event_obj (ptr)
    local envelope = ffi.cast("struct EventEnvelope*", ptr)
    local ctype = core.event_types[envelope.type]
    local event_obj = ffi.cast(ctype, envelope.data)
    return {
        type=envelope.type,
        target=envelope.target,
        size=ffi.sizeof(ctype) + ffi.sizeof("struct EventEnvelope"),
        event=event_obj
    }
end

function handle_events (num_events, events_buffer)
    local scripted_behaviors = ffi.new('struct ScriptedBehavior*')
    local num_scripted_behaviors = C.get_scripted_behaviors(scripted_behaviors)
    for index = 0, num_scripted_behaviors  do
        local scripted_behavior = scripted_behaviors[index]
        local entity = mm.registry:entity(scripted_behavior.entity_id)
        local event_map = event_maps[scripted_behavior.resource_id]
        if event_map then
            local event_ptr = 0
            for index = 0, num_events do
                local event_obj = make_event_obj(events_buffer[event_ptr]);
                event_ptr = event_ptr + event_obj.size
                if !mm.registry.valid(event_obj.target) or event_obj.target == entity.id then
                    local handler = event_map[event_obj.type]
                    if handler then
                        handler(entity, event_obj.event)
                    end
                end
            end
        end
    end
end