
#include "resources/builtintypes.hpp"
#include "scripting/scripting.hpp"
#include "utils/parser.hpp"

bool resources::types::EntityScripts::cached (const std::string& filename, std::uint32_t* id)
{
    return m_cached_ids.if_contains(
        entt::hashed_string::value(filename.c_str()),
        [&id](auto& element){
        *id = element.second;
    });
}

bool resources::types::EntityScripts::load (million::resources::Handle handle, const std::string& filename)
{
    try {
        auto config = parser::parse_toml(filename);

        if (!config.contains("message-map")) {
            spdlog::warn("[resource:entity-scripts] In '{}', missing 'message-map' field", filename);
            return false;
        }

        auto messages = config.at("message-map");
        if (! messages.is_table()) {
            spdlog::warn("[resource:entity-scripts] In '{}', invalid 'message-map' field: must be a table", filename);
        }

        std::ostringstream oss;

        if (config.contains("script") && config.contains("script-file")) {
            spdlog::warn("[resource:entity-scripts] In '{}', contains both 'script' and 'script-file' fields, should only contain one", filename);
            return false;
        } else if (config.contains("script")) {
            auto script = config.at("script");
            if (! script.is_string()) {
                spdlog::warn("[resource:entity-scripts] In '{}', invalid 'script' field: must be a string", filename);
                return false;
            }
            oss << script.as_string().str << "\n";
        } else if (config.contains("script-file")) {
            auto script_file = config.at("script-file");
            if (! script_file.is_string()) {
                spdlog::warn("[resource:entity-scripts] In '{}', invalid 'script-file' field: must be a string", filename);
                return false;
            }
            try {
                oss << helpers::readToString(script_file.as_string()) << "\n";
            } catch (const std::invalid_argument& e) {
                spdlog::warn("[resource:entity-scripts] In '{}', could not load 'script-file': '{}' file not found", filename);
            }
        } else {
            spdlog::warn("[resource:entity-scripts] In '{}', missing 'script' or 'script-file' fields", filename);
            return false;
        }

        oss << "local core = require('mm_core')\n";
        oss << "core:register_entity_script({\n";
        oss << "  resource_id=" << handle.id() << ",\n";
        oss << "  message_map={\n";
        for (const auto& [key, value] : messages.as_table()) {
            if (!value.is_string()) {
                spdlog::warn("[resource:entity-scripts] In '{}', invalid event mapping: {} value is not a string", filename, key);
                return false;
            }
            oss << "    [" << entt::hashed_string::value(key.c_str()) << "]=" << value.as_string().str << ",\n";
        }
        oss << "}})";

        if (! scripting::evaluate(filename, oss.str())) {
            spdlog::warn("[resource:entity-scripts] '{}' script was invalid", filename);
            return false;
        }

    } catch (const std::invalid_argument& e) {
        spdlog::warn("[resource:entity-scripts] '{}' file not found", filename);
        return false;
    }

    // Cache the handle
    m_cached_ids.insert(std::make_pair(entt::hashed_string::value(filename.c_str()), handle.id()));

    // Signal success
    return true;
}

void resources::types::EntityScripts::unload (million::resources::Handle handle)
{
    std::ostringstream oss;
    oss << "local core = require('mm_core')\n";
    oss << "core:unregister_entity_script(" << handle.id() << ")";
    if (! scripting::evaluate("resource:entity-scripts:unload", oss.str())) {
        spdlog::warn("[resource:entity-scripts] Resoruce '{}' could not be unloaded, script error", handle.id());
    }
}
