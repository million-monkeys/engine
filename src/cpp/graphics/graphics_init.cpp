
#include <SDL.h>

// #include <bgfx/bgfx.h>
// #include <bgfx/platform.h>

 SDL_Window* createWindow (int width, int height, std::uint32_t flags)
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

// SDL_Window* initializeGraphics (int width, int height)
// {
//     EASY_FUNCTION(profiler::colors::Pink50);

//     if (SDL_Init(SDL_INIT_VIDEO) < 0) {
//         spdlog::error("SDL could not initialize. SDL_Error: {}", SDL_GetError());
//         return nullptr;
//     }

//     SDL_Window* window = createWindow(width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

//     if (window == nullptr) {
//         spdlog::error("Window could not be created. SDL_Error: {}", SDL_GetError());
//         return nullptr;
//     }

//     SDL_SysWMinfo wmi;
//     SDL_VERSION(&wmi.version);
//     if (!SDL_GetWindowWMInfo(window, &wmi)) {
//         spdlog::error("SDL_SysWMinfo could not be retrieved. SDL_Error: {}", SDL_GetError());
//         return nullptr;
//     }

//     bgfx::renderFrame(); // single threaded mode

//     bgfx::PlatformData pd{};
//     #if BX_PLATFORM_WINDOWS
//         pd.nwh = wmi.info.win.window;
//     #elif BX_PLATFORM_OSX
//         pd.nwh = wmi.info.cocoa.window;
//     #elif BX_PLATFORM_LINUX
//         const std::string& force_backend = entt::monostate<"graphics/force-wm-backend"_hs>();
//         bool wayland_detected = !getEnvVar("WAYLAND_DISPLAY").empty();
//         if (force_backend == "x11" || (!wayland_detected && force_backend != "wayland")) {
//             spdlog::info("Using X11 backend");
//             // X11
//             pd.ndt = wmi.info.x11.display;
//             pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
//         } else if (force_backend == "wayland" || wayland_detected) {
//             spdlog::info("Using Wayland backend");
//             // WAYLAND
//             g_use_wayland = true;
//             pd.ndt = wmi.info.wl.display;
//             wl_egl_window* win_impl = (wl_egl_window*)SDL_GetWindowData(window, "wl_egl_window");
//             if(!win_impl)
//             {
//                 int width, height;
//                 SDL_GetWindowSize(window, &width, &height);
//                 struct wl_surface* surface = wmi.info.wl.surface;
//                 if(!surface) {
//                     spdlog::error("Failed to create Wayland surface");
//                     return nullptr;
//                 }
//                 win_impl = wl_egl_window_create(surface, width, height);
//                 SDL_SetWindowData(window, "wl_egl_window", win_impl);
//             }
//             pd.nwh = (void*)(uintptr_t)win_impl;
//         } else {
//             spdlog::error("Unknown WM backend: {}", force_backend);
//             return nullptr;
//         }
//     #endif
//     bgfx::Init bgfx_init;
//     #if BX_PLATFORM_LINUX
//         if (g_use_wayland) {
//             bgfx_init.type = bgfx::RendererType::Vulkan;
//         } else {
//     #endif
//     #if BX_PLATFORM_LINUX
//             bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
//         }
//     #endif
//     bgfx_init.resolution.width = width;
//     bgfx_init.resolution.height = height;
//     bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
//     bgfx_init.platformData = pd;
//     spdlog::warn("Init bgfx");
//     bgfx::init(bgfx_init);
//     spdlog::warn("Inited bgfx");

//     return window;
// }

SDL_Window* initializeGraphics (int width, int height)
{
    EASY_FUNCTION(profiler::colors::Pink50);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        spdlog::error("SDL could not initialize. SDL_Error: {}", SDL_GetError());
        return nullptr;
    }

    SDL_Window* window = createWindow(width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (window == nullptr) {
        spdlog::error("Window could not be created. SDL_Error: {}", SDL_GetError());
        return nullptr;
    }

    return window;
}

void terminateGraphics (SDL_Window* window)
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}
