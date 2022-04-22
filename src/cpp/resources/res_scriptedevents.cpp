
#include "resources.hpp"
#include "utils/parser.hpp"
#include "scripting/core.hpp"


bool ScriptedEvents::load (entt::hashed_string::hash_type id, const std::string& filename)
{
    try {
        auto config = parser::parse_toml(filename);

        if (!config.contains("event-map")) {
            spdlog::warn("[resource:ScriptedEvents] In '{}', missing 'event-map' field", filename);
            return false;
        }

        auto events = config.at("event-map");
        if (! events.is_table()) {
            spdlog::warn("[resource:ScriptedEvents] In '{}', invalid 'event-map' field: must be a table", filename);
        }

        std::ostringstream oss;

        if (config.contains("script") && config.contains("script-file")) {
            spdlog::warn("[resource:ScriptedEvents] In '{}', contains both 'script' and 'script-file' fields, should only contain one", filename);
            return false;
        } else if (config.contains("script")) {
            auto script = config.at("script");
            if (! script.is_string()) {
                spdlog::warn("[resource:ScriptedEvents] In '{}', invalid 'script' field: must be a string", filename);
                return false;
            }
            oss << script.as_string().str << "\n";
        } else if (config.contains("script-file")) {
            auto script_file = config.at("script-file");
            if (! script_file.is_string()) {
                spdlog::warn("[resource:ScriptedEvents] In '{}', invalid 'script-file' field: must be a string", filename);
                return false;
            }
            try {
                oss << helpers::readToString(script_file.as_string()) << "\n";
            } catch (const std::invalid_argument& e) {
                spdlog::warn("[resource:ScriptedEvents] In '{}', could not load 'script-file': '{}' file not found", filename);
            }
        } else {
            spdlog::warn("[resource:ScriptedEvents] In '{}', missing 'script' or 'script-file' fields", filename);
            return false;
        }

        oss << "local core = require('mm_core')\n";
        oss << "core:register_script({\n";
        oss << "  resource_id=" << id << ",\n";
        oss << "  event_map={\n";
        for (const auto& [key, value] : events.as_table()) {
            if (!value.is_string()) {
                spdlog::warn("[resource:ScriptedEvents] In '{}', invalid event mapping: {} value is not a string", filename, key);
                return false;
            }
            oss << "    [" << entt::hashed_string::value(key.c_str()) << "]=" << value.as_string().str << ",\n";
        }
        oss << "}})";

        if (! scripting::evaluate(filename, oss.str())) {
            spdlog::warn("[resource:ScriptedEvents] '{}' script was invalid", filename);
            return false;
        }
        SPDLOG_TRACE("[resource:ScriptedEvents] '{}' loaded as resource: {}", filename, id);

    } catch (const std::invalid_argument& e) {
        spdlog::warn("[resource:ScriptedEvents] '{}' file not found", filename);
    }

    return true;
}

void ScriptedEvents::unload (entt::hashed_string::hash_type id)
{
    std::ostringstream oss;
    oss << "local core = require('mm_core')\n";
    oss << "core:unregister_script(" << id << ")";
    if (! scripting::evaluate("resource:ScriptedEvents:unload", oss.str())) {
        spdlog::warn("[resource:ScriptedEvents] Resoruce '{}' could not be unloaded, script error", id);
    }
}
