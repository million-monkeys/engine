#pragma once

#include <monkeys.hpp>

namespace config {
    // Read base engine configuration (player editable settings)
    bool readUserConfig (int argc, char* argv[]);
    // Read engine configuration (not meant for player editing)
    bool readEngineConfig ();
    // Read game configuration (not meant for player editing)
    bool readGameConfig (scripting::Context* scripting_ctx, helpers::hashed_string_flat_map<std::string>& game_scripts, std::vector<entt::hashed_string::hash_type>& entity_categories);

    // Access config
    
    helpers::hashed_string_flat_map<std::uint32_t>& stream_sizes ();
}