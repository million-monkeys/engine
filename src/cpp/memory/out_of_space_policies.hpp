#pragma once

#include <stdexcept>
#include <spdlog/spdlog.h>

namespace out_of_space_policies {
    struct Throw {
        template <typename T>
        static T* apply (const std::string& name) {
            throw std::runtime_error(name + " allocated more items than reserved space");
        }
    };
    struct Log {
        template <typename T>
        static T* apply (const std::string& name) {
            spdlog::error("{} allocated more items than reserved space", name);
            return nullptr;
        }
    };
    struct Ignore {
        template <typename T>
        static T* apply (const std::string& name) {
            return nullptr;
        }
    };
}
