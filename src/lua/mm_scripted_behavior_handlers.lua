-- Engine Lua code to run ScriptedBehavior
local bit = require("bit")
local ffi = require("ffi")
ffi.cdef[[
struct MessageEnvelope {
    uint32_t type;
    uint32_t target;
    uint32_t metadata;
};
struct BehaviorIterator* setup_scripted_behavior_iterator ();
uint32_t get_next_scripted_behavior (struct BehaviorIterator*, const struct Component_Core_ScriptedBehavior**);
bool is_in_group (uint32_t, uint32_t);
uint32_t get_group (uint32_t, const uint32_t**);
uint32_t get_messages (const char**);
]]
local C = ffi.C
local core = require('mm_core')
local mm = require('mm_script_api')
local registry = mm.registry

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
        local message_map = core.message_maps[bit.band(scripted_behavior[0].behavior, 0x003fffff)]
        -- If it has one, add the entity and message map to collection
        if message_map then
            entity_collection[entity.id] = {
                entity=entity,
                message_map=message_map,
            }
        end
    end
end

local function handle_message (message_type, entity_info, ptr)
    -- Get the entities message map and the handler for this message, if any
    local message_map = entity_info.message_map
    local handler = message_map[message_type]
    -- If the entity has a handler for this type of message, get the message data type
    if handler then
        local entity = entity_info.entity
        local ctype = core.types_by_id[message_type]
        -- If the message type is registered, then extract the message data and call the handler
        if ctype then
            local msg = ffi.cast(ctype, ptr + ffi.sizeof("struct MessageEnvelope"))
            handler(entity, msg, mm)
        else
            mm.log.warning("Entity %d registered for unknown message type: %d", entity.id, message_type)
        end
    end
end

local function process_messages (entities, message_buffer, buffer_size)
    local entity_ids = ffi.new('const uint32_t*[1]')
    local ptr = 0
    local index = 0
    while index < buffer_size do
        ptr = message_buffer + index
        -- Get envelope for next message
        local envelope = ffi.cast("struct MessageEnvelope*", ptr)
        local size =  bit.band(envelope.metadata, 0xffff)
        index = index + size + ffi.sizeof("struct MessageEnvelope")

        -- MessageEnvelope.metadata: 0bTFCCCCCCCCCCCCCCSSSSSSSSSSSSSSSS
        -- Check target flag, 1 = group, 0 = entity
        local target_type = bit.band(envelope.metadata, 0x80000000)
        local is_filtered = bit.band(envelope.metadata, 0x40000000)
        local categories = bit.rshift(bit.band(envelope.metadata, 0x3fff0000), 16)
        if target_type == 0 then
            -- Entity target
            -- Get the target entities info, if any
            local entity_info = entities[envelope.target]
            -- If the message is not filtered or the entity has one of the required categories
            if entity_info and (is_filtered == 0 or (entity_info.entity:has('category') and bit.band(entity_info.entity.category.id, categories) ~= 0)) then
                handle_message(envelope.type, entity_info, ptr)
            end
        else
            -- Group target
            local num_entities = C.get_group(envelope.target, entity_ids)
            local entity_id_ptr = entity_ids[0]
            if is_filtered == 0 then
                -- Every entity that is in the group
                repeat
                    num_entities = num_entities - 1
                    local entity_info = entities[entity_id_ptr[num_entities]]
                    if entity_info then
                        handle_message(envelope.type, entity_info, ptr)
                    end
                until num_entities == 0
            else
                -- Every entity that is in the group and has one of the required categories
                repeat
                    num_entities = num_entities - 1
                    local entity_info = entities[entity_id_ptr[num_entities]]
                    if entity_info and entity_info.entity:has('category') and bit.band(entity_info.entity.category.id, categories) ~= 0 then
                        handle_message(envelope.type, entity_info, ptr)
                    end
                until num_entities == 0
            end
        end
    end
end

return function ()
    local max_iterations = core.config.max_iterations
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
    until iteration >= max_iterations
end
