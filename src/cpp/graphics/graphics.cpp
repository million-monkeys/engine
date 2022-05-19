
#include "graphics.hpp"
#include "core/engine.hpp"
#include <thread>
#include <atomic>

#include <vector>

#include <SDL.h>

std::thread g_graphics_thread;

std::atomic_bool g_running = false;
std::atomic_bool g_initialized = false;
std::atomic_bool g_error = false;

SDL_Window* initializeGraphics (int width, int height);
void terminateGraphics (SDL_Window* window);

void graphicsThread (core::Engine* engine, graphics::Sync* sync_obj)
{
    EASY_NONSCOPED_BLOCK("Setup Graphics", profiler::colors::Pink100);
    const int width = entt::monostate<"graphics/resolution/width"_hs>();
    const int height = entt::monostate<"graphics/resolution/height"_hs>();
    SDL_Window* window = initializeGraphics(width, height);
    if (window == nullptr) {
        g_error = true;
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    g_initialized = true;
    // Sync with the engine, so that it knows the render thread is set up
    {
        // Wait to grab the lock
        std::unique_lock<std::mutex> lock(sync_obj->state_mutex);
        sync_obj->sync_cv.wait(lock, [sync_obj](){ return sync_obj->owner == graphics::Sync::Owner::Renderer; });
        // And release it again right away
        sync_obj->owner = graphics::Sync::Owner::Engine;
        lock.unlock();
        sync_obj->sync_cv.notify_one();
    }
    EASY_END_BLOCK

    std::vector<SDL_Rect> rects;

    do {
        EASY_BLOCK("Frame", profiler::colors::Red100);
        engine->handleInput();

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
            std::unique_lock<std::mutex> lock(sync_obj->state_mutex);
            {
                sync_obj->sync_cv.wait(lock, [sync_obj](){ return sync_obj->owner == graphics::Sync::Owner::Renderer; });
            }
            EASY_END_BLOCK;

            EASY_BLOCK("Collecting render data", profiler::colors::Red800);
            
//             // Call onPrepareRender during the critical section, before performing any rendering
//             engine.callModuleHook<CM::PREPARE_RENDER>();

//             // Gather render data into render list
//             entt::registry& registry = render_api->engine.registry(gou::api::Registry::Runtime);

//             sprites.clear();
//             registry.view<components::graphics::Sprite, components::Position>(entt::exclude<components::Transform>).each([&sprites](const auto, const auto& position) {
//                 sprites.emplace_back(Sprite{position.point, glm::vec3(0.0f), glm::vec3(1.0f)});
//             });
//             registry.view<components::graphics::Sprite, components::Position, components::Transform>().each([&sprites](const auto, const auto& position, const auto& transform) {
//                 sprites.emplace_back(Sprite{position.point, transform.rotation, transform.scale});
//             });

//             // Gather render data, if there is any
//             if (render_api->dirty.exchange(false)) { // Only data set from engine thread context needs to set dirty and be gathered here
//                 if (render_api->window_changed) {
//                     render_api->windowChanged();
//                     render_api->window_changed = false;
// #ifndef WITHOUT_IMGUI
//                     viewport = render_api->viewport();
//                     ImGui::GetIO().DisplaySize = ImVec2(viewport.z, viewport.w);
// #endif
//                 }
//                 viewport = render_api->viewport();
//                 projection_matrix = render_api->projectionMatrix();
//             }

            entt::registry& registry = engine->registries(core::RegistryPair::Registries::Foreground).runtime;

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
            sync_obj->owner = graphics::Sync::Owner::Engine;
            lock.unlock();
            sync_obj->sync_cv.notify_one();
        }

        EASY_BLOCK("Rendering", profiler::colors::Red200);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRects(renderer, rects.data(), rects.size());
        SDL_RenderPresent(renderer);
    } while (g_running.load());

    SDL_DestroyRenderer(renderer);
    terminateGraphics(window);

    return;
}


graphics::Sync* graphics::init (core::Engine& engine)
{
    graphics::Sync* sync_obj = new graphics::Sync();
    g_running = true;
    g_graphics_thread = std::thread(graphicsThread, &engine, sync_obj);
    while (! g_initialized.load()) {}
    if (!g_error.load()) {
        return sync_obj;
    } else {
        return nullptr;
    }
}

void graphics::term (Sync* sync_obj)
{
    g_running = false;
    if (g_initialized.load()) {
        g_graphics_thread.join();
    }
}
