-- Engine Lua code to run ScriptedBehavior
local ffi = require("ffi")
ffi.cdef[[
struct EventEnvelope {
    uint32_t type;
    uint32_t target;
    uint32_t size;
};
struct BehaviorIterator* setup_scripted_behavior_iterator ();
uint32_t get_next_scripted_behavior (struct BehaviorIterator*, const struct Component_Core_ScriptedBehavior**);
]]
local C = ffi.C
local core = require('mm_core')
local registry = require('mm_script_api').registry

local function make_event_obj (ptr)
    local envelope = ffi.cast("struct EventEnvelope*", ptr)
    local ctype = core.event_types_by_id[envelope.type]
    local event_obj = ffi.cast(ctype, ptr + ffi.sizeof("struct EventEnvelope"))
    return {
        type=envelope.type,
        target=envelope.target,
        size=envelope.size + ffi.sizeof("struct EventEnvelope"),
        event=event_obj
    }
end

function handle_events (events_buffer, buffer_size)
    local events_buffer = ffi.cast("char*", events_buffer)
    local scripted_behavior = ffi.new('const struct Component_Core_ScriptedBehavior*[1]')
    -- Reset iterator
    local iterator = C.setup_scripted_behavior_iterator()
    while true do
        -- Get entity and ScriptedBehavior instance and advance iterator
        local entity = registry:entity(C.get_next_scripted_behavior(iterator, scripted_behavior))
        if not entity then
            -- Entity not valid, no more scripted behaviors, abort processing
            return
        end
        local event_map = core.event_maps[scripted_behavior[0].resource]
        if event_map then
            local event_ptr = 0
            repeat
                local event_obj = make_event_obj(events_buffer + event_ptr);
                event_ptr = event_ptr + event_obj.size
                if not registry.valid(event_obj.target) or event_obj.target == entity.id then
                    local handler = event_map[event_obj.type]
                    if handler then
                        handler(entity, event_obj.event)
                    end
                end
            until event_ptr >= buffer_size
        end
    end
end
