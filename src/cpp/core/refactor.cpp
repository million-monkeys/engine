
#include <optional>
#include <string>
#include <game.hpp>

struct Config {

};

struct InitError {
    bool fatal;
    std::string message;
};

struct CommandFn {
    
};

struct InitResult {
    std::optional<InitError> error_state;
    helpers::hashed_string_flat_map<CommandFn> commands;

    static InitResult success () {
        return {
            std::nullopt
        };
    }
    static InitResult error (const std::string& message) {
        return {
            std::optional<InitError>{{false, message}}
        };
    }
};



class Subsystem {
public:
    virtual InitResult init (Config) = 0;
    virtual void term () = 0;
};
