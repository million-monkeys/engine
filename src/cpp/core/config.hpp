#pragma once

#include <game.hpp>

namespace core {

    // Read base engine configuration (player editable settings)
    bool readUserConfig (int argc, char* argv[]);
    // Read game configuration (not meant for player editing)
    bool readGameConfig (helpers::hashed_string_flat_map<std::uint32_t>& stream_sizes);

} // core::
