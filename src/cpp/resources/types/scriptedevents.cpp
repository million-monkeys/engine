
#include "resources/builtintypes.hpp"
#include "scripting/scripting.hpp"
#include "utils/parser.hpp"

bool resources::types::ScriptedEvents::load (monkeys::resources::Handle handle, const std::string& filename)
{
    try {
        auto config = parser::parse_toml(filename);

        if (!config.contains("event-map")) {
            spdlog::warn("[resource:scripted-events] In '{}', missing 'event-map' field", filename);
            return false;
        }

        auto events = config.at("event-map");
        if (! events.is_table()) {
            spdlog::warn("[resource:scripted-events] In '{}', invalid 'event-map' field: must be a table", filename);
        }

        std::ostringstream oss;

        if (config.contains("script") && config.contains("script-file")) {
            spdlog::warn("[resource:scripted-events] In '{}', contains both 'script' and 'script-file' fields, should only contain one", filename);
            return false;
        } else if (config.contains("script")) {
            auto script = config.at("script");
            if (! script.is_string()) {
                spdlog::warn("[resource:scripted-events] In '{}', invalid 'script' field: must be a string", filename);
                return false;
            }
            oss << script.as_string().str << "\n";
        } else if (config.contains("script-file")) {
            auto script_file = config.at("script-file");
            if (! script_file.is_string()) {
                spdlog::warn("[resource:scripted-events] In '{}', invalid 'script-file' field: must be a string", filename);
                return false;
            }
            try {
                oss << helpers::readToString(script_file.as_string()) << "\n";
            } catch (const std::invalid_argument& e) {
                spdlog::warn("[resource:scripted-events] In '{}', could not load 'script-file': '{}' file not found", filename);
            }
        } else {
            spdlog::warn("[resource:scripted-events] In '{}', missing 'script' or 'script-file' fields", filename);
            return false;
        }

        oss << "local core = require('mm_core')\n";
        oss << "core:register_script({\n";
        oss << "  resource_id=" << handle.id() << ",\n";
        oss << "  event_map={\n";
        for (const auto& [key, value] : events.as_table()) {
            if (!value.is_string()) {
                spdlog::warn("[resource:scripted-events] In '{}', invalid event mapping: {} value is not a string", filename, key);
                return false;
            }
            oss << "    [" << entt::hashed_string::value(key.c_str()) << "]=" << value.as_string().str << ",\n";
        }
        oss << "}})";

        if (! scripting::evaluate(filename, oss.str())) {
            spdlog::warn("[resource:scripted-events] '{}' script was invalid", filename);
            return false;
        }

    } catch (const std::invalid_argument& e) {
        spdlog::warn("[resource:scripted-events] '{}' file not found", filename);
        return false;
    }

    return true;
}

void resources::types::ScriptedEvents::unload (monkeys::resources::Handle handle)
{
    std::ostringstream oss;
    oss << "local core = require('mm_core')\n";
    oss << "core:unregister_script(" << handle.id() << ")";
    if (! scripting::evaluate("resource:scripted-events:unload", oss.str())) {
        spdlog::warn("[resource:scripted-events] Resoruce '{}' could not be unloaded, script error", handle.id());
    }
}
