#include <vkme/InputManager.hpp>


namespace vkme {

void InputManager::keyDown(int key)
{

}

void InputManager::keyUp(int key)
{

}

void InputManager::mouseMove(int x, int y)
{
    if (_delegate.get())
    {
        _delegate->mouseMove(x, y);
    }
}

void InputManager::mouseButtonDown(int button, int x, int y)
{
    if (_delegate.get())
    {
        _delegate->mouseButtonDown(button, x, y);
    }
}

void InputManager::mouseButtonUp(int button, int x, int y)
{
    if (_delegate.get())
    {
        _delegate->mouseButtonUp(button, x, y);
    }
}

void InputManager::mouseWheel(int deltaX, int deltaY)
{
    if (_delegate.get())
    {
        _delegate->mouseWheel(deltaX, deltaY);
    }
}


}
