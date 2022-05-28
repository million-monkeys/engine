#include "graphics.hpp"
#include "context.hpp"

#include <SDL.h>

void graphics_thread (graphics::Context* context);

graphics::Context* graphics::init (world::Context* world_ctx, input::Context* input_ctx, modules::Context* modules_ctx)
{
    EASY_BLOCK("graphics::init", graphics::COLOR(1));
    SPDLOG_DEBUG("[graphics] Init");
    auto context = new graphics::Context{};
    context->m_world_ctx = world_ctx;
    context->m_input_ctx = input_ctx;
    context->m_modules_ctx = modules_ctx;

    context->m_running = true;
    context->m_graphics_thread = std::thread(graphics_thread, context);

    // Sync to make sure render thread is set up before continuing
    graphics::handOff(context);

    if (!context->m_error.load()) {
        return context;   
    } else {
        return nullptr;
    }
}

void graphics::term (graphics::Context* context)
{
    EASY_BLOCK("graphics::term", graphics::COLOR(1));
    SPDLOG_DEBUG("[graphics] Term");
    if (context->m_initialized.load()) {
        context->m_running = false;
        SPDLOG_DEBUG("[graphics] Waiting for graphics to terminate");
        // Make sure graphics thread can unblock from its critical section, if its waiting
        auto& sync_obj = context->m_sync;
        {
            std::scoped_lock<std::mutex> lock(sync_obj.state_mutex);
            sync_obj.owner = graphics::Sync::Owner::Renderer;
        }
        sync_obj.sync_cv.notify_one();
        context->m_graphics_thread.join();
    }
}

 SDL_Window* create_window (int width, int height, std::uint32_t flags)
 {
    const bool fullscreen = entt::monostate<"graphics/fullscreen"_hs>();
    const bool resizable = entt::monostate<"graphics/resolution/resizable"_hs>();

    return SDL_CreateWindow(
        std::string(entt::monostate<"graphics/window/title"_hs>()).c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        flags | (fullscreen ? SDL_WINDOW_FULLSCREEN : (resizable ? SDL_WINDOW_RESIZABLE : 0)));
 }

 SDL_Window* initialize_graphics (int width, int height)
{
    EASY_FUNCTION(graphics::COLOR(3));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        spdlog::error("SDL could not initialize. SDL_Error: {}", SDL_GetError());
        return nullptr;
    }

    SDL_Window* window = create_window(width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (window == nullptr) {
        spdlog::error("Window could not be created. SDL_Error: {}", SDL_GetError());
        return nullptr;
    }

    return window;
}

void terminate_graphics (SDL_Window* window)
{
    EASY_FUNCTION(graphics::COLOR(3));
    SDL_DestroyWindow(window);
    SDL_Quit();
}