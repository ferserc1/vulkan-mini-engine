
#include <mini_engine/MainLoop.hpp>

#include <thread>

int32_t miniengine::MainLoop::run()
{
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_WindowFlags winFlags = SDL_WindowFlags(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    
    auto window = SDL_CreateWindow(
        _windowTitle.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        _windowWidth,
        _windowHeight,
        winFlags
    );

    _vulkanData.init(window);
    
    SDL_Event event;
    bool quit = false;
    bool stopRendering = false;
    
    while (!quit)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }
            
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_MINIMIZED)
            {
                stopRendering = true;
            }
            
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESTORED)
            {
                stopRendering = false;
            }
        }
        
        if (stopRendering)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
    }
    
    _vulkanData.cleanup();
    SDL_DestroyWindow(window);
    
    return 0;
}
