local core = require('mm_core')
local ffi = require('ffi')
ffi.cdef [[
    uint32_t null_entity_value ();
    uint32_t entity_create (uint32_t which_registry);
    uint32_t entity_create_from_prototype (uint32_t which_registry, const char* prototype);
    void entity_destroy (uint32_t which_registry, uint32_t entity);
    uint32_t entity_lookup_by_name (uint32_t which_registry, const char* name);
    void* component_get_for_entity (uint32_t which_registry, uint32_t entity, const char* component_name);
    void* component_add_to_entity (uint32_t which_registry, uint32_t entity, const char* component_name);
    void component_tag_entity (uint32_t which_registry, uint32_t entity, const char* tag_name);
    void component_remove_from_entity (uint32_t which_registry, uint32_t entity, const char* component_name);
    void output_log (uint32_t level, const char* message);
    void* allocate_event (const char* event_name, uint32_t target, uint8_t size, bool emit_later);
    void* allocate_command (const char* event_name, uint8_t size);
    uint32_t load_resource (const char* type, const char* filename, const char* name);
    uint32_t find_resource (const char* name);
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

-- Populated with game attributes by engine
local ATTRS_TABLE = {}

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

local function get_entity_by_id(self, entity_id)
    if entity_id ~= NULL_ENTITY then
        local entity = {
            _registry = self._registry,
            id = entity_id,
            get = get_component,
            add = add_component,
            tag = add_tag_component,
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

local function emit_message(message_name, target, later)
    local message_info = core.event_types_by_name[message_name]
    if message_info then
        if target == nil then
            C.output_log(LOG_LEVELS.WARNING, 'Message "'..message_name..'" sent without target')
        else
            if type(target) == 'table' then
                target = target.id
            end
            return ffi.cast(message_info.type, C.allocate_event(message_name, target, message_info.size, later))
        end
    else
        C.output_log(LOG_LEVELS.WARNING, 'Message "'..message_name..'" not found')
    end
end

local function emit_command(command_name)
    local event_info = core.event_types_by_name[command_name]
    if event_info then
        return ffi.cast(event_info.type, C.allocate_command(command_name, event_info.size))
    else
        C.output_log(LOG_LEVELS.WARNING, 'Command "'..command_name..'" not found')
    end
end

local function get_signal (signal_id)
    return {
        id=signal_id,
        send=function (self, event_name, event_data)
            log_output(LOG_LEVELS.DEBUG, "Sending %s to signal %s", event_name, self.id)
        end
    }
end

return {
    time = {
        delta = 0
    },
    attrs = ATTRS_TABLE,
    registry = {
        _registry = 0, -- Default to runtime registry
        select_registry = select_regitsry,
        entity = get_entity_by_id,
        lookup = get_entity_by_name,
        create = create_entity,
        valid  = function(id) return id ~= NULL_ENTITY end
    },
    resource = {
        load = function (type, file, name)
            return C.load_resource(type, file, name)
        end,
        find = function (name)
            return C.find_resource(name)
        end
    },
    ref = C.get_ref,
    signal = get_signal,
    emit = function (name, target) return emit_message(name, target, false) end,
    emit_later = function (name, target) return emit_message(name, target, true) end,
    command=emit_command,
    log = {
        debug   = function(...) log_output(LOG_LEVELS.DEBUG,   ...) end,
        info    = function(...) log_output(LOG_LEVELS.INFO,    ...) end,
        warning = function(...) log_output(LOG_LEVELS.WARNING, ...) end,
        error   = function(...) log_output(LOG_LEVELS.ERROR,   ...) end,
        critical= function(...) log_output(LOG_LEVELS.CRITICAL,...) end,
    },
    dump=function (obj, label)
        if label then
            print(label..':')
        end
        for k, v in pairs(obj) do
            print('', k, v)
        end
    end
}
