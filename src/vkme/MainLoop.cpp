
#include <vkme/MainLoop.hpp>

#include <thread>

int32_t vkme::MainLoop::run()
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
    _userInterface.init(&_vulkanData);
    _drawLoop.init(&_vulkanData, &_userInterface);
    
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
            
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                _vulkanData.updateSwapchainSize();
            }
            
            if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
            {
                int x, y;
                SDL_GetMouseState(&x, &y);
                
                if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
                {
                    auto button = event.button.button == SDL_BUTTON_LEFT ?
                        0 : event.button.button == SDL_BUTTON_MIDDLE ?
                        1 : 2;
                    if (event.button.state == SDL_PRESSED)
                    {
                        _inputManager.mouseButtonDown(button, x, y);
                    }
                    else
                    {
                        _inputManager.mouseButtonUp(button, x, y);
                    }
                }
                else
                {
                    _inputManager.mouseMove(x, y);
                }
            }
            if (event.type == SDL_MOUSEWHEEL)
            {
                _inputManager.mouseWheel(event.wheel.x, event.wheel.y);
            }
            
            _userInterface.processEvent(&event);
        }
        
        if (stopRendering)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        if (_vulkanData.newFrame())
        {
            _drawLoop.swapchainResized();
        }
        
        _userInterface.newFrame();
        
        _drawLoop.acquireAndPresent();
    }
    
    _userInterface.cleanup();
    _vulkanData.cleanup();
    SDL_DestroyWindow(window);
    
    return 0;
}
