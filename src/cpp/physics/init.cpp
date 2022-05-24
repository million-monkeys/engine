#include "physics.hpp"
#include "context.hpp"

physics::Context* physics::init ()
{
    auto context = new physics::Context{};
    return context;
}

void physics::term (physics::Context* context)
{

}