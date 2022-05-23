#include "parser.hpp"
#include <monkeys.hpp>

const TomlValue parser::parse_toml (const std::string& filename, parser::FileLocation location) {
    if (location == parser::FileLocation::PhysicsFS) {
        std::istringstream iss(std::ios_base::binary | std::ios_base::in);
        iss.str(helpers::readToString(filename));
        return toml::parse<toml::discard_comments, tsl::ordered_map, std::vector>(iss, filename);
    } else {
        return toml::parse<toml::discard_comments, tsl::ordered_map, std::vector>(filename);
    }
}