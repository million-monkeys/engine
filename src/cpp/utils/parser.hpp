#pragma once

//#include "gou_engine.hpp"
#include <toml.hpp>
#include <vector>
#include <tsl/ordered_map.h>
#include <sstream>

// Use tsl::ordered_map so order of component attributes is retained
using TomlValue = typename toml::basic_value<toml::discard_comments, tsl::ordered_map, std::vector>;
using TomlTable = typename toml::basic_value<toml::discard_comments, tsl::ordered_map, std::vector>::table_type;
using TomlArray = typename toml::basic_value<toml::discard_comments, tsl::ordered_map, std::vector>::array_type;

namespace parser {
    enum class FileLocation {
        FileSystem = 0,
        PhysicsFS = 1,
    };

    /*
     * Read a file (from PhysicsFS or filesystem) and parse as TOML
     */
    const TomlValue parse_toml (const std::string& filename, FileLocation location=FileLocation::PhysicsFS);

} // parser::
