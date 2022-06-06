-- Engine Lua code to run ScriptedBehavior
local bit = require("bit")
local ffi = require("ffi")
ffi.cdef[[
struct MessageEnvelope {
    uint32_t type;
    uint32_t target;
    uint32_t metadata;
};
struct BehaviorIterator* setup_scripted_behavior_iterator (void*);
uint32_t get_next_scripted_behavior (void*, struct BehaviorIterator*, const struct Component_Core_ScriptedBehavior**);
bool is_in_group (void*, uint32_t, uint32_t);
uint32_t get_group (void*, uint32_t, const uint32_t**);
uint32_t get_entity_set (void*, uint32_t, const uint32_t**);
uint32_t get_entity_composite (void*, uint32_t, const uint32_t**);
uint32_t get_messages (void*, const char**);
]]
local C = ffi.C
local core = require('mm_core')
local mm = require('mm_script_api')
local registry = mm.registry

local function gather_entities ()
    local scripted_behavior = ffi.new('const struct Component_Core_ScriptedBehavior*[1]')
    local entity_collection = {}
    -- Get a new iterator for ScriptedBehavior's
    local iterator = C.setup_scripted_behavior_iterator(MM_CONTEXT)
    while true do
        -- Get entity and ScriptedBehavior instance and advance iterator
        local entity = registry:entity(C.get_next_scripted_behavior(MM_CONTEXT, iterator, scripted_behavior))
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
            -- Message not registered, assume its a body-less signal
            handler(entity, nil, mm)
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
        -- MessageEnvelope.metadata: 0bTTFxxxxxCCCCCCCCCCCCCCCCSSSSSSSS
        -- T = 2bits flag, Mask: 0xd0000000, Target type. 00 => target entity, 01 => target group, 10 => target entity set, 11 => target composite
        -- F = 1bit flag, Mask: 0x20000000, Filter. 0 => not filtered by category, 1 => filtered by category (only target entities with specified category will receive message)
        -- x = reserved
        -- C = 16bit bitfield, Mask: 0x00ffff00, Category bitfield, each bit represents one of 14 total possible categories. 0 => Category not filtered by, 1 => category filtered by
        -- S = 8bit number, Mask: 0x000000ff, Size of payload in bytes

        local size =  bit.band(envelope.metadata, 0x000000ff)
        index = index + size + ffi.sizeof("struct MessageEnvelope")

        -- Check target flag, 1 = group, 0 = entity
        local target_type = bit.rshift(bit.band(envelope.metadata, 0xd0000000), 30)
        local is_filtered = bit.band(envelope.metadata, 0x20000000)
        local categories = bit.rshift(bit.band(envelope.metadata, 0x00ffff00), 8)
        if target_type == 0 then
            -- Entity target
            -- Get the target entities info, if any
            local entity_info = entities[envelope.target]
            -- If the message is not filtered or the entity has one of the required categories
            if entity_info and (is_filtered == 0 or (entity_info.entity:has('category') and bit.band(entity_info.entity.category.id, categories) ~= 0)) then
                handle_message(envelope.type, entity_info, ptr)
            end
        else
            -- Group target or Entity Set target
            local num_entities
            if target_type == 1 then
                -- Group target
                num_entities = C.get_group(MM_CONTEXT, envelope.target, entity_ids)
            elseif target_type == 2 then
                -- Entity Set target
                num_entities = C.get_entity_set(MM_CONTEXT, envelope.target, entity_ids)
            else
                -- Composite target
                num_entities = C.get_entity_composite(MM_CONTEXT, envelope.target, entity_ids)
            end
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
        local buffer_size = C.get_messages(MM_CONTEXT, message_buffer)
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
