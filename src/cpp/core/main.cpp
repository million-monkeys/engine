
#include <monkeys.hpp>
#include "config/config.hpp"
#include "events/events.hpp"

#include "engine.hpp"

#include <map>

#define SPDLOG_HEADER_ONLY
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <physfs.hpp>

void setupPhysFS (const char* argv0)
{
    const std::vector<std::string>& sourcePaths = entt::monostate<"game/sources"_hs>{};
    physfs::init(argv0);
    // Mount game sources to search path
    for (auto path : sourcePaths) {
        SPDLOG_DEBUG("Adding to path: {}", path);
        physfs::mount(path, "/", 1);
    }
}

std::shared_ptr<spdlog::logger> setupLogging ()
{
    std::map<std::string,spdlog::level::level_enum> log_levels{
        {"trace", spdlog::level::trace},
        {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},
        {"off", spdlog::level::off},
    };
    const std::string& raw_log_level = entt::monostate<"telemetry/log-level"_hs>{};
    std::string log_level = raw_log_level;
#ifndef DEBUG_BUILD
    // Only debug builds have debug or trace log levels
    if (log_level == "debug" || log_level == "trace") {
        log_level = "info";
    }
#endif
    // TODO: Error checking for invalid values of log_level
    spdlog::level::level_enum level = log_levels[log_level];
    std::vector<spdlog::sink_ptr> sinks;
    if (level != spdlog::level::off) {
        spdlog::init_thread_pool(8192, 1);
        auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
        sinks = {stdout_sink};
    }
    auto logger = std::make_shared<spdlog::async_logger>("game", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);
    spdlog::set_default_logger(logger);
    if (level != spdlog::level::off) {
        spdlog::set_pattern("[%D %T.%f] [Thread: %t] [%-8l] %^%v%$");
        spdlog::info("Setting log level to: {}", log_level);
#ifdef DEBUG_BUILD
        // Set global profiling on or off
        // Profile::profiling_enabled = profiling;
#endif
    }
    spdlog::set_level(level);
    return logger;
}

int game_main (int argc, char** argv)
{
    // Load base engine settings
    if (! config::readUserConfig(argc, argv)) {
        return -1;
    }
    // Initialise logger and filesystem
    auto logger = setupLogging();
    setupPhysFS(argv[0]);

    const bool& profiling_enabled = entt::monostate<"telemetry/profiling"_hs>{};
    if (profiling_enabled) {
        EASY_PROFILER_ENABLE;
        spdlog::info("Starting easy_profiler");
        profiler::startListen();
    }
    if (profiler::isListening()) {
        spdlog::info("easy_profiler is listening");
    } else {
        spdlog::info("easy_profiler is not listening");
    }
    EASY_MAIN_THREAD;

    bool clean_exit = true;
    try {
        Engine engine;
        if (engine.init()) {
            engine.execute();
        } else {
            spdlog::critical("Could not start engine");
            spdlog::critical("Terminating.");
            clean_exit = false;
        }
        // Clear data before unloading modules, to avoid referencing memory owned by modules after they are unloaded
        engine.shutdown();
        // moduleManager.unload();
        logger->flush();

    } catch (const std::exception& e) {
        spdlog::error("Uncaught exception: {}", e.what());
        spdlog::critical("Terminating.");
        clean_exit = false;
    }

#ifdef BUILD_WITH_EASY_PROFILER
    if (profiling_enabled) {
        const std::string& filename = entt::monostate<"telemetry/profiling-dump-file"_hs>();
        if (! filename.empty()) {
            spdlog::info("Writing easy_profiler data to file: {}", filename);
            profiler::dumpBlocksToFile(filename.c_str());
        }
    }
#endif

    physfs::deinit();
    if (clean_exit) {
        spdlog::info("Goodbye, until next time.");
    }
    return 0;
}