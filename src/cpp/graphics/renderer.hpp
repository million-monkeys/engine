#pragma once

#include <cstdint>
#include <entt/entity/fwd.hpp>

struct SDL_Window;

namespace graphics {
    using CreateWindow = SDL_Window* (*) (std::uint32_t);

    namespace hardware {
        struct Info {
            struct Texture {
                int max_layers;
                int max_combined_units;
                int max_vertex_units;
                int max_geometry_units;
                int max_fragment_units;
                int max_size;
            };
            struct Shader {
                int max_vertex_uniform_vectors;
                int max_fragment_uniform_vectors;
                int max_varying_vectors;
                int max_vertex_attributes;
            };

            Texture textures;
            Shader shaders;
        };
    }

    class Renderer {
    public:
        virtual ~Renderer () {}
        // Create context
        virtual void create (CreateWindow) = 0;

        // Setup rendering
        virtual void setup () = 0;
        // Gather renderables from world
        virtual void gatherRenderables (entt::registry&) = 0;
        // Render gathered renderables
        virtual void render () = 0;
        // Cleanup rendering
        virtual void cleanup () = 0;

        virtual const hardware::Info& info () const = 0;
    };
}