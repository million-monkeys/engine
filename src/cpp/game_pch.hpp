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
#include <million/monkeys.hpp>
#include "utils/helpers.hpp"
#include "memory/pools.hpp"

// Convenience types
namespace helpers {

    template <typename KeyT, typename ValueT, typename HashT = std::hash<KeyT>>
    using thread_safe_flat_map = phmap::parallel_flat_hash_map<KeyT, ValueT, HashT, std::equal_to<size_t>, std::allocator<std::pair<const size_t, size_t>>,  4,  std::mutex>;

    template <typename KeyT, typename ValueT, typename HashT = std::hash<KeyT>>
    using thread_safe_node_map = phmap::parallel_node_hash_map<KeyT, ValueT, HashT, std::equal_to<size_t>, std::allocator<std::pair<const size_t, size_t>>,  4,  std::mutex>;

    template <typename ValueT, template<typename, typename, typename> typename BaseType>
    using hashed_string_map = BaseType<entt::hashed_string::hash_type, ValueT, helpers::Identity>;

    template <typename ValueT>
    using hashed_string_flat_map = phmap::flat_hash_map<entt::hashed_string::hash_type, ValueT, helpers::Identity>;

    template <typename ValueT>
    using hashed_string_node_map = phmap::node_hash_map<entt::hashed_string::hash_type, ValueT, helpers::Identity>;
}

#endif