#include "modules.hpp"
#include "context.hpp"

modules::Context* modules::init ()
{
    auto context = new modules::Context{};
    return context;
}

void modules::term (modules::Context* context)
{
    delete context;
}
