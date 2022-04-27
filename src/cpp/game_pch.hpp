#ifndef GAME_HPP
#define GAME_HPP

// From std::
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <variant>
#include <cassert>

// From third-party dependencies
#include <entt/core/monostate.hpp>
#include <entt/entity/registry.hpp>
#include <entt/core/hashed_string.hpp>
using namespace entt::literals;

#include <parallel_hashmap/phmap.h>
#include <parallel_hashmap/btree.h>

#include <magic_enum.hpp>

// Profiling
#include <easy/profiler.h>
#include <easy/arbitrary_value.h>

// GLM
#ifndef DEBUG_BUILD
#define GLM_FORCE_INLINE
#endif
#define GLM_FORCE_INTRINSICS
#include <glm/glm.hpp>
#include <glm/ext.hpp> 
#include <glm/gtc/type_ptr.hpp>

// Logging
#ifdef DEBUG_BUILD
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>

// From engine
#include <million/million.hpp>
#include "utils/helpers.hpp"
#include "memory/pools.hpp"

#endif