local ffi = require("ffi")
ffi.cdef[[
struct EventEnvelope {
    uint32_t type;
    uint32_t size;
};
uint32_t get_stream_events (void*, uint32_t, const char**);
]]
local C = ffi.C
local core = require('mm_core')
local mm = require('mm_script_api')

local function process_events (events_buffer, buffer_size, event_map)
    local ptr = 0
    local index = 0
    while index < buffer_size do
        ptr = events_buffer + index
        -- Get envelope for next message
        local envelope = ffi.cast("struct EventEnvelope*", ptr)
        index = index + envelope.size + ffi.sizeof("struct EventEnvelope")
        -- Get the handler for this event, if there is one
        local handler = event_map[envelope.type]
        -- If the entity has a handler for this type of event, get the event data type
        if handler then
            local ctype = core.types_by_id[envelope.type]
            -- If the event type is registered, then extract the message data and call the handler
            if ctype then
                local event = ffi.cast(ctype, ptr + ffi.sizeof("struct EventEnvelope"))
                handler(event, mm)
            else
                mm.log.warning("Scene registered for unknown event type: %d", envelope.type)
            end
        end
    end
end

return {
    handle_game_events = function ()
        local event_buffer = ffi.new('const char*[1]')
        for stream, event_map in pairs(core.game_event_map) do
            local buffer_size = C.get_stream_events(MM_CONTEXT, stream, event_buffer)
            if buffer_size > 0 then
                process_events(event_buffer[0], buffer_size, event_map)
            end
        end
    end,
    handle_scene_events = function (current_scene)
        local scene_events = core.scene_event_maps[current_scene]
        if scene_events then
            local event_buffer = ffi.new('const char*[1]')
            for stream, event_map in pairs(scene_events) do
                local buffer_size = C.get_stream_events(MM_CONTEXT, stream, event_buffer)
                if buffer_size > 0 then
                    process_events(event_buffer[0], buffer_size, event_map)
                end
            end
        end
    end,
}