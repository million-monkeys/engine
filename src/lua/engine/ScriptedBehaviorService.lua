-- Engine Lua code to run ScriptedBehavior
local ffi = require("ffi")
local C = ffi.C
ffi.cdef[[
struct ScriptedBehavior {
    uint32_t resource_id;
};
struct EventEnvelope {
    uint32_t type;
    uint32_t target;
    void* data;
};
void setup_scripted_behavior_iterator ();
uint32_t get_next_scripted_behavior (const struct ScriptedBehavior**);
]]
local core = require('mm_core')
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
    local scripted_behavior = ffi.new('const struct ScriptedBehavior*[1]')
    -- Reset iterator
    C.setup_scripted_behavior_iterator()
    while true do
        -- Get entity and ScriptedBehavior instance and advance iterator
        local entity = mm.registry:entity(C.get_next_scripted_behavior(scripted_behavior))
        if not entity then
            -- Entity not valid, no more scripted behaviors, abort processing
            return
        end
        local event_map = core.event_maps[scripted_behavior[0].resource_id]
        if event_map then
            local event_ptr = 0
            for _2 = 0, num_events do
                local event_obj = make_event_obj(events_buffer[event_ptr]);
                event_ptr = event_ptr + event_obj.size
                if not mm.registry.valid(event_obj.target) or event_obj.target == entity.id then
                    local handler = event_map[event_obj.type]
                    if handler then
                        handler(entity, event_obj.event)
                    end
                end
            end
        end
    end
end
