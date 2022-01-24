#pragma once

#include <graphics/renderer.hpp>

namespace renderer::opengl {

    class Renderer : public graphics::Renderer {
    public:
        Renderer ();
        virtual ~Renderer ();
        void create (CreateWindow) final;
        void setup () final;
        void gatherRenderables (entt::registry&) final;
        void render () final;
        void cleanup () final;

        const graphics::hardware::Info& info () const final;

    private:
        graphics::hardware::Info m_info;
    };

}