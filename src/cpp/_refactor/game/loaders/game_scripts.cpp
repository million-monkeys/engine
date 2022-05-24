#include "game_scripts.hpp"
#include "../game.hpp"
#include "../context.hpp"

#include "_refactor/scripting/scripting.hpp"
#include "_refactor/utils/parser.hpp"

#include "_refactor/utils/parser.hpp"

bool loaders::GameScripts::cached (const std::string& filename, std::uint32_t* id)
{
    EASY_FUNCTION(profiler::colors::Teal300);
    return m_cached_ids.if_contains(
        entt::hashed_string::value(filename.c_str()),
        [&id](auto& element){
        *id = element.second;
    });
}

bool loaders::GameScripts::load (million::resources::Handle handle, const std::string& filename)
{
    EASY_FUNCTION(profiler::colors::Teal300);
    try {
        auto config = parser::parse_toml(filename);

        if (!config.contains("event-map")) {
            spdlog::warn("[resource:game-scripts] In '{}', missing 'event-map' field", filename);
            return false;
        }

        auto event_categories = config.at("event-map");
        if (! event_categories.is_table()) {
            spdlog::warn("[resource:game-scripts] In '{}', invalid 'event-map' field: must be a table", filename);
        }

        std::ostringstream oss;

        if (config.contains("script") && config.contains("script-file")) {
            spdlog::warn("[resource:game-scripts] In '{}', contains both 'script' and 'script-file' fields, should only contain one", filename);
            return false;
        } else if (config.contains("script")) {
            auto script = config.at("script");
            if (! script.is_string()) {
                spdlog::warn("[resource:game-scripts] In '{}', invalid 'script' field: must be a string", filename);
                return false;
            }
            oss << script.as_string().str << "\n";
        } else if (config.contains("script-file")) {
            auto script_file = config.at("script-file");
            if (! script_file.is_string()) {
                spdlog::warn("[resource:game-scripts] In '{}', invalid 'script-file' field: must be a string", filename);
                return false;
            }
            try {
                oss << helpers::readToString(script_file.as_string()) << "\n";
            } catch (const std::invalid_argument& e) {
                spdlog::warn("[resource:game-scripts] In '{}', could not load 'script-file': '{}' file not found", filename);
            }
        } else {
            spdlog::warn("[resource:game-scripts] In '{}', missing 'script' or 'script-file' fields", filename);
            return false;
        }

        oss << "local core = require('mm_core')\n";
        oss << "core:register_game_script({\n";
        for (const auto& [category, event_map] : event_categories.as_table()) {
            if (! event_map.is_table()) {
                spdlog::warn("[resource:game-scripts] In '{}', invalid event mapping: {} value is not a table", filename, category);
                return false;
            }
            oss << "    [" << entt::hashed_string::value(category.c_str()) << "]={\n";
            for (const auto& [event_name, value] : event_map.as_table()) {
                if (!value.is_string()) {
                    spdlog::warn("[resource:game-scripts] In '{}', invalid event mapping: {} value is not a string", filename, event_name);
                    return false;
                }
                oss << "        [" << entt::hashed_string::value(event_name.c_str()) << "]=" << value.as_string().str << ",\n";
            }
            oss << "    },\n";
        }
        oss << "})";

        if (! scripting::evaluate(m_context->m_scripting_ctx, filename, oss.str())) {
            spdlog::warn("[resource:game-scripts] '{}' script was invalid", filename);
            return false;
        }

    } catch (const std::invalid_argument& e) {
        spdlog::warn("[resource:game-scripts] '{}' file not found", filename);
        return false;
    }

    // Cache the handle
    m_cached_ids.insert(std::make_pair(entt::hashed_string::value(filename.c_str()), handle.id()));

    return true;
}

void loaders::GameScripts::unload (million::resources::Handle handle)
{
    std::ostringstream oss;
    oss << "local core = require('mm_core')\n";
    oss << "core:unregister_game_script()";
    if (! scripting::evaluate(m_context->m_scripting_ctx, "resource:game-scripts:unload", oss.str())) {
        spdlog::warn("[resource:game-scripts] Resoruce '{}' could not be unloaded, script error", handle.id());
    }
}
