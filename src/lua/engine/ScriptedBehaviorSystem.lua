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

local function gather_entities ()
    local scripted_behavior = ffi.new('const struct Component_Core_ScriptedBehavior*[1]')
    local entity_collection = {}
    -- Get a new iterator for ScriptedBehavior's
    local iterator = C.setup_scripted_behavior_iterator()
    while true do
        -- Get entity and ScriptedBehavior instance and advance iterator
        local entity = registry:entity(C.get_next_scripted_behavior(iterator, scripted_behavior))
        if not entity then
            -- Entity not valid, no more scripted behaviors, return the collection
            return entity_collection
        end
        -- Otherwise, get the entities message map
        local message_map = core.message_maps[bit.band(scripted_behavior[0].resource, 0xfffff)]
        -- If it has one, add the entity and message map to collection
        if message_map then
            entity_collection[entity.id] = {
                entity=entity,
                message_map=message_map,
            }
        end
    end
end

local function process_messages (entities, message_buffer, buffer_size)
    local ptr = 0
    local index = 0
    while index < buffer_size do
        ptr = message_buffer + index
        -- Get envelope for next message
        local envelope = ffi.cast("struct MessageEnvelope*", ptr)
        index = index + envelope.size + ffi.sizeof("struct MessageEnvelope")
        -- Get the target entities info, if any
        local entity_info = entities[envelope.target]
        if entity_info then
            -- Get the entities message map and the handler for this message, if any
            local message_map = entity_info.message_map
            local handler = message_map[envelope.type]
            -- If the entity has a handler for this type of message, get the message data type
            if handler then
                local entity = entity_info.entity
                local ctype = core.event_types_by_id[envelope.type]
                -- If the message type is registered, then extract the message data and call the handler
                if ctype then
                    local msg = ffi.cast(ctype, ptr + ffi.sizeof("struct MessageEnvelope"))
                    handler(entity, msg, mm)
                else
                    mm.log.warning("Entity %d registered for unknown event type: %d", entity.id, envelope.type)
                end
            end
        end
    end
end

function handle_messages ()
    local message_buffer = ffi.new('const char*[1]')
    -- Collect entities that have a ScriptedBehavior component and valid message map resource
    local entity_message_maps = gather_entities()
    -- Process messages until no new messages are received, or MAX_ITERATIONS iterations, whichever happens first
    local iteration = 0
    repeat
        local buffer_size = C.get_messages(message_buffer)
        -- If there are no more messages, then abort
        if buffer_size == 0 then
            break
        end
        -- Process current buffer of messages
        process_messages(entity_message_maps, message_buffer[0], buffer_size)
        -- Increment iteration count and stop if MAX_ITERATIONS has been reached
        iteration = iteration + 1
    until iteration >= MAX_ITERATIONS
end
