#include "graphics.hpp"
#include "context.hpp"

#include "_refactor/world/world.hpp"
#include "_refactor/input/input.hpp"

#include <SDL.h>

SDL_Window* initialize_graphics (int width, int height);
void terminate_graphics (SDL_Window* window);

void graphics::handOff (graphics::Context* context)
{
    EASY_BLOCK("Waiting for renderer to sync", profiler::colors::Red800);
    auto& sync_obj = context->m_sync;
    // First, signal to the renderer that it has exclusive access to the engines state
    {
        std::scoped_lock<std::mutex> lock(sync_obj.state_mutex);
        sync_obj.owner = graphics::Sync::Owner::Renderer;
    }
    sync_obj.sync_cv.notify_one();
    // Now wait for the renderer to relinquish exclusive access back to the engine
    std::unique_lock<std::mutex> lock(sync_obj.state_mutex);
    sync_obj.sync_cv.wait(lock, [&sync_obj]{ return sync_obj.owner == graphics::Sync::Owner::Engine; });
}

void graphics_thread (graphics::Context* context)
{
    EASY_NONSCOPED_BLOCK("Setup Graphics", graphics::COLOR(1));
    const int width = entt::monostate<"graphics/resolution/width"_hs>();
    const int height = entt::monostate<"graphics/resolution/height"_hs>();
    SDL_Window* window = initialize_graphics(width, height);
    if (window == nullptr) {
        context->m_error = true;
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    context->m_initialized = true;

    auto& sync_obj = context->m_sync;
    // Sync with the engine, so that it knows the render thread is set up
    {
        // Wait to grab the lock
        std::unique_lock<std::mutex> lock(sync_obj.state_mutex);
        sync_obj.sync_cv.wait(lock, [&sync_obj](){ return sync_obj.owner == graphics::Sync::Owner::Renderer; });
        // And release it again right away
        sync_obj.owner = graphics::Sync::Owner::Engine;
        lock.unlock();
        sync_obj.sync_cv.notify_one();
    }
    EASY_END_BLOCK

    std::vector<SDL_Rect> rects;

    do {
        EASY_BLOCK("Frame", graphics::COLOR(1));
        input::poll(context->m_input_ctx);

        rects.clear();

        /*********************************************************************/
        /* Wait for engine to hand over exclusive access to engine state.
        * Renderer will then access the ECS registry to gather all components needed for rengering,
        * accumulate a render list and hand exclusive access back to the engine. The renderer will
        * then asynchronously render from its locally owned render list.
        *********************************************************************/
        {
            EASY_NONSCOPED_BLOCK("Wait for Exclusive Access", profiler::colors::Red300);
            // Wait for exclusive access to engine state
            std::unique_lock<std::mutex> lock(sync_obj.state_mutex);
            {
                sync_obj.sync_cv.wait(lock, [&sync_obj](){ return sync_obj.owner == graphics::Sync::Owner::Renderer; });
            }
            EASY_END_BLOCK;

            EASY_BLOCK("Collecting render data", graphics::COLOR(2));
            
//             // Call onPrepareRender during the critical section, before performing any rendering
//             engine.callModuleHook<CM::PREPARE_RENDER>();

            entt::registry& registry = world::registry(context->m_world_ctx);

            registry.view<components::core::graphics::Sprite, components::core::Position>(entt::exclude<components::core::Transform>).each([&rects](const auto, const auto& position) {
                rects.emplace_back(
                    SDL_Rect{
                        int(position.x) - 10,
                        int(position.y) - 10,
                        20,
                        20,
                    });
            });

            // Hand exclusive access back to engine
            sync_obj.owner = graphics::Sync::Owner::Engine;
            lock.unlock();
            sync_obj.sync_cv.notify_one();
        }

        EASY_BLOCK("Rendering", graphics::COLOR(2));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRects(renderer, rects.data(), rects.size());
        SDL_RenderPresent(renderer);
    } while (context->m_running.load());

    SDL_DestroyRenderer(renderer);
    terminate_graphics(window);
}