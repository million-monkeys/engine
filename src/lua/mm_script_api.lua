local core = require('mm_core')
local ffi = require('ffi')
ffi.cdef [[
    uint32_t null_entity_value ();
    uint32_t entity_create (uint32_t which_registry);
    uint32_t entity_create_from_prototype (uint32_t which_registry, const char*);
    void entity_destroy (uint32_t which_registry, uint32_t);
    uint32_t entity_lookup_by_name (uint32_t which_registry, const char*);
    void* component_get_for_entity (uint32_t which_registry, uint32_t, const char*);
    void* component_add_to_entity (uint32_t which_registry, uint32_t, const char*);
    void component_tag_entity (uint32_t which_registry, uint32_t, const char*);
    void component_remove_from_entity (uint32_t which_registry, uint32_t, const char*);
    void output_log (uint32_t, const char*);
    void* allocate_message (const char*, uint32_t, uint8_t);
    void* allocate_command (const char*, uint8_t);
    void* allocate_event (const char*, uint8_t);
    uint32_t load_resource (const char*, const char*, const char*);
    uint32_t find_resource (const char*);
]]
local C = ffi.C

local NULL_ENTITY = C.null_entity_value()

local LOG_LEVELS = {
    CRITICAL = 0,
    ERROR = 1,
    WARNING = 2,
    INFO = 3,
    DEBUG = 4
}


local function log_output(level, fmt, ...)
    local message = string.format(fmt, ...)
    C.output_log(level, message)
end

local function select_regitsry(self, registry_name)
    if registry_name == 'runtime' then
        self._registry = 0
    elseif registry_name == 'background' then
        self._registry = 1
    elseif registry_name == 'prototype' then
        self._registry = 2
    else
        C.output_log(LOG_LEVELS.ERROR, 'Invalid registry: '..registry_name)
    end
end

local function get_component(self, component_name)
    local ptr = C.component_get_for_entity(self._registry, self.id, component_name)
    if ptr ~= 0 then
        return ffi.cast(core.component_types[component_name], ptr)
    else
        return nil
    end
end

local function get_component_and_cache (self, component_name)
    local component = get_component(self, component_name)
    self[component_name] = component -- Cache component for quick multiple lookups
    return component
end

local function add_component(self, component_name)
    local ptr = C.component_add_to_entity(self._registry, self.id, component_name)
    if ptr ~= 0 then
        return ffi.cast(core.component_types[component_name], ptr)
    else
        return nil
    end
end

local function add_tag_component(self, tag_name)
    C.component_tag_entity(self._registry, self.id, tag_name)
end

local function remove_component(self, component_name)
    C.component_remove_from_entity(self._registry, self.id, component_name)
end

local function destroy_entity(self)
    C.entity_destroy(self.id)
end

local function post_message(target, message_name)
    local message_info = core.types_by_name[message_name]
    if message_info then
        if target == nil then
            C.output_log(LOG_LEVELS.WARNING, 'Message "'..message_name..'" sent without target')
        else
            return ffi.cast(message_info.type, C.allocate_message(message_name, target, message_info.size))
        end
    else
        C.output_log(LOG_LEVELS.WARNING, 'Message "'..message_name..'" not found')
    end
end

local function emit_command(command_name)
    local type_info = core.types_by_name[command_name]
    if type_info then
        return ffi.cast(type_info.type, C.allocate_command(command_name, type_info.size))
    else
        C.output_log(LOG_LEVELS.WARNING, 'Command "'..command_name..'" not found')
    end
end

local function emit_event(event_name)
    local type_info = core.types_by_name[event_name]
    if type_info then
        return ffi.cast(type_info.type, C.allocate_event(event_name, type_info.size))
    else
        C.output_log(LOG_LEVELS.WARNING, 'Event "'..event_name..'" not found')
    end
end

local function get_entity_by_id(self, entity_id)
    if entity_id ~= NULL_ENTITY then
        local entity = {
            _registry = self._registry,
            id = entity_id,
            get = get_component,
            add = add_component,
            tag = add_tag_component,
            post = function(entity, message_name) return post_message(entity.id, message_name) end,
            remove = remove_component,
            destroy = destroy_entity
        }
        return setmetatable(entity, {__index = get_component_and_cache})
    end
end

local function get_entity_by_name(self, entity_name)
    local id = C.entity_lookup_by_name(self._registry, entity_name)
    if id ~= NULL_ENTITY then
        return get_entity_by_id(self, id)
    end
end

local function create_entity(self, prototype)
    local id
    if prototype then
        id = C.entity_create_from_prototype(self._registry, prototype)
    else
        id = C.entity_create(self._registry)
    end
    return get_entity_by_id(self, id)
end

return {
    -- Set by engine whenever game state changes
    game_state = '',
    -- Set by engine each frame
    time = {
        delta = 0,
        absolute = 0,
    },
    -- Populated with game attributes by engine
    attrs = {},
    -- Manipulate the ECS registry
    registry = {
        _registry = 0, -- Default to runtime registry
        select_registry = select_regitsry,
        -- Access an entity, given an entity ID
        entity = get_entity_by_id,
        -- Find a named entity
        lookup = get_entity_by_name,
        -- Create a new entity
        create = create_entity,
        -- Check if an ID is valid
        valid  = function(id) return id ~= NULL_ENTITY end
    },
    -- Load and access resources
    resource = {
        -- Load a resource of specified type from a resource file and assign a name to it
        load = function (type, file, name)
            return C.load_resource(type, file, name)
        end,
        -- Find a resource by name
        find = function (name)
            return C.find_resource(name)
        end
    },
    -- Convert a string into a hashed integer reference
    ref = C.get_ref,
    -- Communicate through messages, commands and events
    post=post_message,
    command=emit_command,
    emit=emit_event,
    -- Debug and error logging
    log = {
        debug   = function(...) log_output(LOG_LEVELS.DEBUG,   ...) end,
        info    = function(...) log_output(LOG_LEVELS.INFO,    ...) end,
        warning = function(...) log_output(LOG_LEVELS.WARNING, ...) end,
        error   = function(...) log_output(LOG_LEVELS.ERROR,   ...) end,
        critical= function(...) log_output(LOG_LEVELS.CRITICAL,...) end,
    },
    -- Dump a Lua table, for debugging only
    dump=function (obj, label)
        if label then
            print(label..':')
        end
        for k, v in pairs(obj) do
            print('', k, v)
        end
    end
}
