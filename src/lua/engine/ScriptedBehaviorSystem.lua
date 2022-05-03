-- Engine Lua code to run ScriptedBehavior
local bit = require("bit")
local ffi = require("ffi")
ffi.cdef[[
struct MessageEnvelope {
    uint32_t type;
    uint32_t target;
    uint32_t size;
};
struct BehaviorIterator* setup_scripted_behavior_iterator ();
uint32_t get_next_scripted_behavior (struct BehaviorIterator*, const struct Component_Core_ScriptedBehavior**);
uint32_t get_messages (const char** buffer);
]]
local C = ffi.C
local core = require('mm_core')
local registry = require('mm_script_api').registry
local mm = require('mm_script_api')

local MAX_ITERATIONS = 15

local function make_message_obj (ptr)
    local envelope = ffi.cast("struct MessageEnvelope*", ptr)
    local ctype = core.event_types_by_id[envelope.type]
    if ctype then
        local event_obj = ffi.cast(ctype, ptr + ffi.sizeof("struct MessageEnvelope"))
        return {
            type=envelope.type,
            target=envelope.target,
            size=envelope.size + ffi.sizeof("struct MessageEnvelope"),
            event=event_obj
        }
    else -- Events that were not registered with the script system should be skipped
        return {
            size=envelope.size + ffi.sizeof("struct MessageEnvelope"),
            event=nil,
        }
    end
end

local function process_messages (scripted_behavior, messages_buffer, buffer_size)
    -- Reset iterator
    local iterator = C.setup_scripted_behavior_iterator()
    while true do
        -- Get entity and ScriptedBehavior instance and advance iterator
        local entity = registry:entity(C.get_next_scripted_behavior(iterator, scripted_behavior))
        if not entity then
            -- Entity not valid, no more scripted behaviors, abort processing
            return
        end
        local event_map = core.event_maps[bit.band(scripted_behavior[0].resource, 0xfffff)]
        if event_map then
            local event_ptr = 0
            while event_ptr < buffer_size do
                local event_obj = make_message_obj(messages_buffer + event_ptr);
                event_ptr = event_ptr + event_obj.size
                if event_obj.event and not registry.valid(event_obj.target) or event_obj.target == entity.id then
                    local handler = event_map[event_obj.type]
                    if handler then
                        handler(entity, event_obj.event)
                    end
                end
            end
        end
    end
end

local function process_internal_messages (scripted_behavior, last_buffer_size)
    local events_buffer = ffi.new('const char*[1]')
    local buffer_size = C.get_messages(events_buffer)
    if buffer_size > last_buffer_size then
        process_messages(scripted_behavior, events_buffer[0] + last_buffer_size, buffer_size - last_buffer_size)
    end
    return buffer_size
end

function handle_messages (message_buffer, buffer_size)
    local events_buffer = ffi.cast("char*", message_buffer)
    local scripted_behavior = ffi.new('const struct Component_Core_ScriptedBehavior*[1]')
    -- Process global events
    process_messages(scripted_behavior, events_buffer, buffer_size)

    -- Process script event stream until no new events are received, or MAX_ITERATIONS iterations, whichever happens first
    local count = 0
    local prev_buffer_size = 0
    local buffer_size = 0
    repeat
        count = count + 1
        prev_buffer_size = buffer_size
        print("ITERATION", count)
        buffer_size = process_internal_messages(scripted_behavior, prev_buffer_size)
    until buffer_size == prev_buffer_size or count >= MAX_ITERATIONS
    print("END")
end
