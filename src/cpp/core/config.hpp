#pragma once

#include <game.hpp>

namespace core {

    // Read base engine configuration (player editable settings)
    bool readUserConfig (int argc, char* argv[]);
    // Read engine configuration (not meant for player editing)
    bool readEngineConfig (helpers::hashed_string_flat_map<std::uint32_t>& stream_sizes);
    // Read game configuration (not meant for player editing)
    bool readGameConfig (helpers::hashed_string_flat_map<std::string>& game_scripts, std::vector<entt::hashed_string::hash_type>& entity_categories);

} // core::
