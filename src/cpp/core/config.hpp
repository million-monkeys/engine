#pragma once

namespace core {

    // Read base engine configuration (player editable settings)
    bool readUserConfig (int argc, char* argv[]);
    // Read game configuration (not meant for player editing)
    bool readGameConfig ();

} // core::
