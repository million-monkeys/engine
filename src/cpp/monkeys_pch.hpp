#ifndef GAME_HPP
#define GAME_HPP

// From std::
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <variant>
#include <array>
#include <cassert>
#include <optional>

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
#include <million/monkeys.hpp>
#include "utils/helpers.hpp"
#include "memory/pools.hpp"

// Subsystem contexts
namespace audio { struct Context; }
namespace events { struct Context; }
namespace game { struct Context; }
namespace graphics { struct Context; }
namespace input { struct Context; }
namespace messages { struct Context; }
namespace modules { struct Context; }
namespace physics { struct Context; }
namespace resources { struct Context; }
namespace scheduler { struct Context; }
namespace scripting { struct Context; }
namespace world { struct Context; }


// Macros
#if defined(__GNUC__) || defined(__clang__) || defined(__GNUG__)
#   define EXPECT_NOT_TAKEN(cond) (__builtin_expect(int(cond), 0))
#   define EXPECT_TAKEN(cond) (__builtin_expect(int(cond), 1))
#else
#   define EXPECT_NOT_TAKEN(cond) (cond)
#   define EXPECT_TAKEN(cond) (cond)
#endif

#endif