#include "utils/helpers.hpp"

#define PHYFSPP_IMPL
#include <physfs.hpp>

#include <exception>

std::string helpers::readToString(const std::string& filename)
{
    if (physfs::exists(filename)) {
        physfs::ifstream stream(filename);
        return std::string(std::istreambuf_iterator<char>(stream),
                        std::istreambuf_iterator<char>());
    } else {
        auto message = std::string{"File could not be read: "} + filename;
        throw std::invalid_argument(message);
    }
}

void helpers::string_replace_inplace (std::string& input, const std::string& search, const std::string& replace)
{
    std::string::size_type n = 0;
    while ( ( n = input.find(search, n ) ) != std::string::npos ) {
        input.replace(n, search.size(), replace);
        n += replace.size();
    }
    if (input.back() == '\n') {
        input.pop_back();
    }   
}