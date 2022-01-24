#include "renderer.hpp"

#include <SDL.h>
#include <glad/glad.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

using namespace renderer::opengl;

#ifdef DEBUG_BUILD
void GLAPIENTRY opengl_messageCallback (GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*);
#endif

Renderer::Renderer ()
{

}

Renderer::~Renderer ()
{

}

void Renderer::create (CreateWindow createWindow)
{
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on OS X
    #define OGL_MAJOR_VERSION 4
    #define OGL_MINOR_VERSION 1
#else
    #define OGL_MAJOR_VERSION 4
    #define OGL_MINOR_VERSION 3
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OGL_MAJOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OGL_MINOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, int(entt::monostate<"graphics/opengl/minimum-red-bits"_hs>()));
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, int(entt::monostate<"graphics/opengl/minimum-green-bits"_hs>()));
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, int(entt::monostate<"graphics/opengl/minimum-blue-bits"_hs>()));
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, int(entt::monostate<"graphics/opengl/minimum-alpha-bits"_hs>()));
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, int(entt::monostate<"graphics/opengl/minimum-framebuffer-bits"_hs>()));
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, int(entt::monostate<"graphics/opengl/minimum-depthbuffer-bits"_hs>()));
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, bool(entt::monostate<"graphics/opengl/double-buffered"_hs>()) ? 1 : 0);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    SDL_Window* window = createWindow(SDL_WINDOW_OPENGL);

    renderer->gl_render_context = SDL_GL_CreateContext(window);
    renderer->gl_init_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(renderer->window, renderer->gl_init_context);

    SDL_GL_SetSwapInterval(bool(entt::monostate<"graphics/v-sync"_hs>()) ? 1 : 0);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        spdlog::critical("Failed to initialize GLAD");
        return nullptr;
    }

    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &m_info.textures.max_layers);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_info.textures.max_combined_units);
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &m_info.textures.max_vertex_units);
    glGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &m_info.textures.max_geometry_units);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_info.textures.max_fragment_units);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_info.textures.max_size);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &m_info.shaders.max_vertex_uniform_vectors);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &m_info.shaders.max_fragment_uniform_vectors);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &m_info.shaders.max_varying_vectors);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_info.shaders.max_vertex_attributes);
    
#ifdef DEBUG_BUILD
    spdlog::debug("Texture limits: {} combined units, {}/{}/{} vs/gs/fs units, {} array layers, {}x{} max size", m_info.textures.max_combined_units, m_info.textures.max_vertex_units, m_info.textures.max_geometry_units, m_info.textures.max_fragment_units, m_info.textures.max_layers, m_info.textures.max_size, m_info.textures.max_size);
    spdlog::debug("Shader limits: {} vertex attributes, {} varying vectors, {} vertex vectors, {} fragment vectors", m_info.shaders.max_vertex_attributes, m_info.shaders.max_varying_vectors, m_info.shaders.max_vertex_uniform_vectors, m_info.shaders.max_fragment_uniform_vectors);
#endif
}

void Renderer::setup ()
{
#ifdef DEBUG_BUILD
        spdlog::info("Setting error callback");
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(opengl_messageCallback, 0);
        GLuint unused_ids = 0;
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused_ids, true);
#endif

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glClearDepth(1.0);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::gatherRenderables (entt::registry& registry)
{
    
}

void Renderer::render ()
{
    // Clear viewport
    {
        EASY_BLOCK("Clearing viewport", profiler::colors::Orange200);
        glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // Scene rendering here
    {
        EASY_BLOCK("Rendering scene", profiler::colors::Orange200);
        
    }

    // End frame
    {
        EASY_BLOCK("Swap Backbuffer", profiler::colors::DeepOrange500);
        SDL_GL_SwapWindow(render_api->window);
    }
}

void Renderer::cleanup ()
{

}

const graphics::hardware::Info& Renderer::info () const {
    return m_info;
}
