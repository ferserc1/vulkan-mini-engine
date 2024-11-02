#include <vkme/InputManager.hpp>
#include "imgui.h"

namespace vkme {

void InputManager::keyDown(int key)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureKeyboard)
    {
        _delegate->keyDown(key);
    }
}

void InputManager::keyUp(int key)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureKeyboard)
    {
        _delegate->keyUp(key);
    }
}

void InputManager::mouseMove(int x, int y)
{
	ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureMouse)
    {
        _delegate->mouseMove(x, y);
    }
}

void InputManager::mouseButtonDown(int button, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureMouse)
    {
        _delegate->mouseButtonDown(button, x, y);
    }
}

void InputManager::mouseButtonUp(int button, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureMouse)
    {
        _delegate->mouseButtonUp(button, x, y);
    }
}

void InputManager::mouseWheel(int deltaX, int deltaY)
{
    ImGuiIO& io = ImGui::GetIO();
    if (_delegate.get() && !io.WantCaptureMouse )
    {
        _delegate->mouseWheel(deltaX, deltaY);
    }
}


}
