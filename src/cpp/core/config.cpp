
#include "config.hpp"
#include "engine.hpp"
#include <game.hpp>
#include "scripting/scripting.hpp"
#include <utils/parser.hpp>
#include <filesystem>

#include <cxxopts.hpp>

// Only set if table contains key and type conversion passes
template <entt::id_type ID, typename T> void maybe_set (const TomlValue& table, const std::string& key) {
    if (table.contains(key)) {
        const auto value = toml::expect<T>(table.at(key));
        if (value.is_ok()) {
            entt::monostate<ID>{} = value.unwrap();
        }   
    }
}

bool core::readUserConfig (int argc, char* argv[])
{
    cxxopts::Options options("Frenzy", "Game Engine");
    options.add_options()
#ifdef DEBUG_BUILD
        ("d,debug", "Enable debug rendering")
        ("p,profiling", "Enable profiling")
        ("export-tasks", "Export task graph", cxxopts::value<std::string>())
#endif
        ("l,loglevel", "Log level", cxxopts::value<std::string>())
        ("x,overridefiles", "Set override path(s) to game files", cxxopts::value<std::vector<std::string>>())
        ("g,gamefiles", "Add override path(s) to game files", cxxopts::value<std::vector<std::string>>())
        ("m,modules", "Modules list file", cxxopts::value<std::string>())
        ("modulepath", "Path to Module files", cxxopts::value<std::string>())
        ("i,init", "Initialisation file", cxxopts::value<std::string>()->default_value("config.toml"));
    auto cli = options.parse(argc, argv);

    //******************************************************//
    //                                                      //
    // Base settings loaded from init TOML file             //
    // These settings are meant to be editable by players   //
    // Loaded from Filesystem (not PhysicsFS)               //
    // Loaded before anything else is set up                //
    //                                                      //
    //******************************************************//
    try {
        std::string config_file = cli["init"].as<std::string>();
        const auto config = parser::parse_toml(config_file, parser::FileLocation::FileSystem);

         std::filesystem::path base_path = std::filesystem::path{config_file}.parent_path();

#ifdef DEBUG_BUILD
         if (cli["export-tasks"].count() != 0) {
             entt::monostate<"dev/export-task-graph"_hs>{} = cli["export-tasks"].as<std::string>();
         } else {
             entt::monostate<"dev/export-task-graph"_hs>{} = std::string{};
         }
#endif

        //******************************************************//
        // TELEMETRY
        //******************************************************//
        if (config.contains("telemetry")) {
            const auto& telemetry = config.at("telemetry");
            maybe_set<"telemetry/log-level"_hs, std::string>(telemetry, "logging");
            maybe_set<"telemetry/profiling"_hs, bool>(telemetry, "profiling");
        } else {
            entt::monostate<"telemetry/log-level"_hs>{} = "info";
        }
        if (cli["loglevel"].count() != 0 ) {
            entt::monostate<"telemetry/log-level"_hs>{} = cli["loglevel"].as<std::string>();
        }
#ifdef DEBUG_BUILD
        if (cli["profiling"].count() != 0) {
            entt::monostate<"telemetry/profiling"_hs>{} = true;
        }
#endif

        //******************************************************//
        // GAME
        //******************************************************//
        // Set default settings for [game] section
        std::vector<std::string> source_files{};
        // Add overrides without removing the configured paths
        if (cli["gamefiles"].count() > 0) {
            source_files = cli["gamefiles"].as<std::vector<std::string>>();
        }

        // Overwrite with settings
        if (config.contains("game")) {
            const auto& game = config.at("game");
            if (game.contains("data")) {
                const auto& sources = game.at("data").as_array();
                for (const auto& source : sources) {
                    source_files.push_back((base_path / source.as_string().str).string());
                }
            }
        }
        
        // Override game sources, if specified on commandline
        if (cli["overridefiles"].count() > 0) {
            source_files = cli["overridefiles"].as<std::vector<std::string>>();
        }
        entt::monostate<"game/sources"_hs>{} = source_files;

        entt::monostate<"game/config-file"_hs>{} = std::string{"game.toml"};
        // Set module path, if specified on commandline (modules loaded relative to this path)
        if (cli["modulepath"].count() > 0) {
            entt::monostate<"game/modules-path"_hs>{} = cli["modulepath"].as<std::string>();;
        } else {
            entt::monostate<"game/modules-path"_hs>{} = "";
        }

        //******************************************************//
        // GRAPHICS
        //******************************************************//
        if (config.contains("graphics")) {
            const auto& graphics = config.at("graphics");
            int width = 640;
            int height = 480;
            if (graphics.contains("resolution")) {
                std::string empty_string;
                const auto& resolution = toml::find_or<std::string>(graphics, "resolution", empty_string);
                if (resolution == "720p") {
                    width = 1280;
                    height = 720;
                } else if (resolution == "1080p") {
                    width = 1920;
                    height = 1080;
                } else if (resolution == "1440p") {
                    width = 2560;
                    height = 1440;
                } else if (resolution == "4k") {
                    width = 4096;
                    height = 2160;
                } else {
                    // parse "<width>x<height>"
                    auto idx = resolution.find('x');
                    if (idx != std::string::npos) {
                        auto w = resolution.substr(0, idx);
                        auto h = resolution.substr(idx+1, resolution.size());
                        width = std::stoi(w);
                        height = std::stoi(h);
                    }
                }
            }
            entt::monostate<"graphics/resolution/width"_hs>{} = width;
            entt::monostate<"graphics/resolution/height"_hs>{} = height;
            entt::monostate<"graphics/fullscreen"_hs>{} = toml::find_or<bool>(graphics, "fullscreen", false);
            entt::monostate<"graphics/v-sync"_hs>{} = toml::find_or<bool>(graphics, "vsync", true);
            entt::monostate<"graphics/renderer/field-of-view"_hs>{} = float(toml::find_or<double>(graphics, "fov", 60.0));
            entt::monostate<"graphics/debug-rendering"_hs>{} = toml::find_or<bool>(graphics, "debug", false);
            entt::monostate<"graphics/resolution/resizable"_hs>{} = toml::find_or<bool>(graphics, "resizable", false);
        } else {
            // No [graphics] section, use default settings
            entt::monostate<"graphics/resolution/width"_hs>{} = int{640};
            entt::monostate<"graphics/resolution/height"_hs>{} = int{480};
            entt::monostate<"graphics/resolution/resizable"_hs>{} = false;
            entt::monostate<"graphics/fullscreen"_hs>{} = false;
            entt::monostate<"graphics/v-sync"_hs>{} = true;
            entt::monostate<"graphics/renderer/field-of-view"_hs>{} = 60.0f;
            entt::monostate<"graphics/debug-rendering"_hs>{} = false;
        }
#ifdef DEBUG_BUILD
        entt::monostate<"graphics/debug-rendering"_hs>{} = bool{cli["debug"].count() > 0};
#endif

        //******************************************************//
        // UI (imgui engine UI, not in-game UI)
        //******************************************************//
        entt::monostate<"ui/theme-file"_hs>{} = std::string{};
        if (config.contains("ui")) {
            const auto& ui = config.at("ui");
            maybe_set<"ui/theme-file"_hs, std::string>(ui, "theme");
        }

        //******************************************************//
        // DEVELOPMENT MODE
        //******************************************************//
#ifdef DEV_MODE
        if (config.contains("dev-mode")) {
            const auto& devmode = config.at("dev-mode");
            entt::monostate<"dev-mode/enabled"_hs>{} = toml::find_or<bool>(devmode, "enabled", false);
            entt::monostate<"dev-mode/reload-interval"_hs>{} = ElapsedTime(toml::find_or<double>(devmode, "reload-interval", 5.0f) * 1000000L);
        } else {
            entt::monostate<"dev-mode/enabled"_hs>{} = bool{false};
        }
#endif

    } catch (const std::exception& e) {
        spdlog::critical("Could not load settings: {}", e.what());
        return false;
    }

    return true;
}


bool core::readEngineConfig (helpers::hashed_string_flat_map<std::uint32_t>& stream_sizes)
{
    //******************************************************//
    //                                                      //
    // Settings loaded from engine config                   //
    // These settings are NOT meant to be edited by players //
    // Loaded from PhysicsFS                                //
    // Loaded after PhysicsFS is setup but before the engine//
    //                                                      //
    //******************************************************//
    try {
        const auto config = parser::parse_toml("engine.toml");

        //******************************************************//
        // GRAPHICS
        //******************************************************//
        // Default settings for [graphics] section
        entt::monostate<"graphics/window/title"_hs>{} = std::string{"Million Monkeys"};
        entt::monostate<"graphics/opengl/minimum-red-bits"_hs>{} = int{8};
        entt::monostate<"graphics/opengl/minimum-green-bits"_hs>{} = int{8};
        entt::monostate<"graphics/opengl/minimum-blue-bits"_hs>{} = int{8};
        entt::monostate<"graphics/opengl/minimum-alpha-bits"_hs>{} = int{8};
        entt::monostate<"graphics/opengl/minimum-framebuffer-bits"_hs>{} = int{32};
        entt::monostate<"graphics/opengl/minimum-depthbuffer-bits"_hs>{} = int{8};
        entt::monostate<"graphics/opengl/double-buffered"_hs>{} = bool{true};
        entt::monostate<"graphics/renderer/near-distance"_hs>{} = 0.01f;
        entt::monostate<"graphics/renderer/far-distance"_hs>{} = 100.0f;

        // Overwrite with settings
        if (config.contains("graphics")) {
            const auto& graphics = config.at("graphics");
            if (graphics.contains("window")) {
                maybe_set<"graphics/window/title"_hs, std::string>(graphics.at("window"), "title");
            }
            if (graphics.contains("opengl")) {
                const auto& opengl = graphics.at("opengl");
                maybe_set<"graphics/opengl/minimum-red-bits"_hs, int>(opengl, "minimum-red-bits");
                maybe_set<"graphics/opengl/minimum-green-bits"_hs, int>(opengl, "minimum-green-bits");
                maybe_set<"graphics/opengl/minimum-blue-bits"_hs, int>(opengl, "minimum-blue-bits");
                maybe_set<"graphics/opengl/minimum-alpha-bits"_hs, int>(opengl, "minimum-alpha-bits");
                maybe_set<"graphics/opengl/minimum-framebuffer-bits"_hs, int>(opengl, "minimum-framebuffer-bits");
                maybe_set<"graphics/opengl/minimum-depthbuffer-bits"_hs, int>(opengl, "minimum-depthbuffer-bits");
                maybe_set<"graphics/opengl/double-buffered"_hs, bool>(opengl, "double-buffered");
            }
            if (graphics.contains("renderer")) {
                const auto& renderer = graphics.at("renderer");
                maybe_set<"graphics/renderer/near-distance"_hs, float>(renderer, "near-distance");
                maybe_set<"graphics/renderer/far-distance"_hs, float>(renderer, "far-distance");
            }
        }

        //******************************************************//
        // MEMORY
        //******************************************************//
        // Default settings for [memory] section
        entt::monostate<"memory/events/pool-size"_hs>{} = std::uint32_t{1024};
        entt::monostate<"memory/events/stream-size"_hs>{} = std::uint32_t{1024};
        entt::monostate<"memory/events/scripts-pool-size"_hs>{} = std::uint32_t{2048};

        // Overwrite with settings
        if (config.contains("memory")) {
            const auto& memory = config.at("memory");
            if (memory.contains("events")) {
                maybe_set<"memory/events/pool-size"_hs, std::uint32_t>(memory.at("events"), "per-thread-pool-size");
                maybe_set<"memory/events/scripts-pool-size"_hs, std::uint32_t>(memory.at("events"), "scripts-pool-size");
                maybe_set<"memory/events/stream-size"_hs, std::uint32_t>(memory.at("events"), "per-stream-pool-size");
            }
            if (memory.contains("streams")) {
                for (const auto& [key, value] : memory.at("streams").as_table()) {
                    stream_sizes[entt::hashed_string::value(key.c_str())] = value.as_integer();
                }
            }
        }
    } catch (const std::exception& e) {
        spdlog::critical("Could not load engine configuration: {}", e.what());
        return false;
    }

    return true;
}


bool core::readGameConfig (helpers::hashed_string_flat_map<std::string>& game_scripts)
{
    //******************************************************//
    //                                                      //
    // Settings loaded from game config                     //
    // These settings are NOT meant to be edited by players //
    // Loaded from PhysicsFS                                //
    // Loaded after the engine and subsystems are setup     //
    //                                                      //
    //******************************************************//
    try {
        const auto config = parser::parse_toml("game.toml");
        
        //******************************************************//
        // GAME
        //******************************************************//
        if (! config.contains("game")) {
            spdlog::error("Game config file did not contain a [game] section.");
            return false;
        }
        const auto& game = config.at("game");
        entt::monostate<"game/user-mods"_hs>{} = toml::find<std::string>(game, "user-mods");
        entt::monostate<"game/initial-state"_hs>{} = toml::find<std::string>(game, "initial-state");
        entt::monostate<"game/script-events"_hs>{} = toml::find<std::string>(game, "script-events");
        if (game.contains("states")) {
            const auto& states = game.at("states");
            if (states.is_table()) {
                for (const auto& [state_name, state] : states.as_table()) {
                    if (state.is_table()) {
                        if (state.contains("script")) {
                            const auto& script = state.at("script");
                            if (script.is_string()) {
                                game_scripts.emplace(entt::hashed_string::value(state_name.c_str()), script.as_string().str);
                            } else {
                                spdlog::error("State '{}'.script must be a string.", state_name);
                                return false;
                            }
                        }
                    } else {
                        spdlog::error("State '{}' value must be a TOML table.", state_name);
                        return false;
                    }
                }
            } else {
                spdlog::error("[game.states] is not a TOML table.");
                return false;
            }
        }


        //******************************************************//
        // SCENES
        //******************************************************//
        if (! config.contains("scenes")) {
            spdlog::error("Game config file did not contain a [scenes] section.");
            return false;
        }
        const auto& scenes = config.at("scenes");
        entt::monostate<"scenes/path"_hs>{} = toml::find<std::string>(scenes, "path");
        entt::monostate<"scenes/initial"_hs>{} = toml::find<std::string>(scenes, "initial");

        //******************************************************//
        // ATTRIBUTES
        //******************************************************//
        if (config.contains("attributes")) {
            const auto& attributes = config.at("attributes");
            if (attributes.is_table()) {
                for (const auto& [key, value] : attributes.as_table()) {
                    if (value.is_floating()) {
                        scripting::call("set_attribute_value", key, value.as_floating());
                    } else if (value.is_integer()) {
                        scripting::call("set_attribute_value", key, value.as_integer());
                    } else if (value.is_string()) {
                        scripting::call("set_attribute_value", key, value.as_string().str);
                    } else if (value.is_boolean()) {
                        scripting::call("set_attribute_value", key, value.as_boolean());
                    } else if (value.is_table()) {
                        for (const auto& [subkey, value] : value.as_table()) {
                            if (value.is_floating()) {
                                scripting::call("set_attribute_value", key, subkey, value.as_floating());
                            } else if (value.is_integer()) {
                                scripting::call("set_attribute_value", key, subkey, value.as_integer());
                            } else if (value.is_string()) {
                                scripting::call("set_attribute_value", key, subkey, value.as_string().str);
                            } else if (value.is_boolean()) {
                                scripting::call("set_attribute_value", key, subkey, value.as_boolean());
                            }
                        }
                    }
                }
            } else {
                spdlog::error("Game config 'attributes' not a TOML table");
                return false;
            }
        }

        //******************************************************//
        // PHYSICS
        //******************************************************//
        if (config.contains("physics")) {
            const auto& physics = config.at("physics");
            entt::monostate<"physics/max-substeps"_hs>{} = toml::find_or<int>(physics, "max-substeps", 5);
            // Gravity
            float gx = 0;
            float gy = 0;
            float gz = 0;
            if (physics.contains("gravity")) {
                const auto& gravity = physics.at("gravity");
                gx = toml::find_or<double>(gravity, "x", 0);
                gy = toml::find_or<double>(gravity, "y", 0);
                gz = toml::find_or<double>(gravity, "z", 0);
            }
            entt::monostate<"physics/gravity"_hs>{} = glm::vec3{gx, gy, gz};
            entt::monostate<"physics/time-step"_hs>{} = float(1.0f / toml::find_or<int>(physics, "target-framerate", 30));
        } else {
            // No [physics] section, use default settings
            entt::monostate<"physics/time-step"_hs>{} = float(1.0f / 30.0f);
            entt::monostate<"physics/max-substeps"_hs>{} = int(5);
            entt::monostate<"physics/gravity"_hs>{} = glm::vec3{0, 0, 0};
        }
    } catch (const std::exception& e) {
        spdlog::critical("Could not load game configuration: {}", e.what());
        return false;
    }

    return true;
}
