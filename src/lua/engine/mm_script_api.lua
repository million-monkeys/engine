local ffi = require("ffi")
ffi.cdef [[
    uint32_t null_entity_value ();
    uint32_t entity_create (uint32_t which_registry);
    void entity_destroy (uint32_t which_registry, uint32_t entity);
    uint32_t entity_lookup_by_name (uint32_t which_registry, const char* name);
    void* component_get_for_entity (uint32_t which_registry, uint32_t entity, const char* component_name);
    void* component_add_to_entity (uint32_t which_registry, uint32_t entity, const char* component_name);
    void component_remove_from_entity (uint32_t which_registry, uint32_t entity, const char* component_name);
    void output_log (uint32_t level, const char* message);
]]
local C = ffi.C
local core = require('mm_core')

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
    elseif registry_name == "background" then
        self._registry = 1
    elseif registry_name == "prototype" then
        self._registry = 2
    else
        log_output(LOG_LEVELS.ERROR, "Invalid registry: %s", registry_name)
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

local function add_component(self, component_name)
    local ptr = C.component_add_to_entity(self._registry, self.id, component_name)
    if ptr ~= 0 then
        return ffi.cast(core.component_types[component_name], ptr)
    else
        return nil
    end
end

local function remove_component(self, component_name)
    C.component_remove_from_entity(self._registry, self.id, component_name)
end

local function destroy_entity(self)
    C.entity_destroy(self.id)
end

local function get_entity_by_id(self, entity_id)
    if entity_id ~= NULL_ENTITY then
        return {
            _registry = self._registry,
            id = entity_id,
            get = get_component,
            add = add_component,
            remove = remove_component,
            destroy = destroy_entity
        }
    else
        return nil
    end
end

local function get_entity_by_name(self, entity_name)
    local id = C.entity_lookup_by_name(self._registry, entity_name)
    if id ~= NULL_ENTITY then
        return get_entity_by_id(self, id)
    else
        return nil
    end
end

local function create_entity(self)
    local id = C.entity_create(self._registry)
    if id ~= NULL_ENTITY then
        return get_entity_by_id(self, id)
    else
        return nil
    end
end

local function emit_event(event)
end

return {
    registry = {
        _registry = 0, -- Default to runtime registry
        select_registry = select_regitsry,
        entity = get_entity_by_id,
        lookup = get_entity_by_name,
        create = create_entity,
        valid = function(id) return id ~= NULL_ENTITY end
    },
    emit = emit_event,
    log = {
        debug = function(...) log_output(LOG_LEVELS.DEBUG, ...) end,
        info = function(...) log_output(LOG_LEVELS.INFO, ...) end,
        warning = function(...) log_output(LOG_LEVELS.WARNING, ...) end,
        error = function(...) log_output(LOG_LEVELS.ERROR, ...) end,
        critical = function(...) log_output(LOG_LEVELS.CRITICAL, ...) end,
    }
}
