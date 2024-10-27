#pragma once

#include <memory>

namespace vkme {

class InputManager;

class InputDelegate {
public:
    virtual ~InputDelegate() {}

    virtual void keyDown(int key) {}
    virtual void keyUp(int key) {}
    virtual void mouseMove(int x, int y) {}
    virtual void mouseButtonDown(int button, int x, int y) {}
    virtual void mouseButtonUp(int button, int x, int y) {}
    virtual void mouseWheel(int deltaX, int deltaY) {}
};

class InputManager {
public:
    void keyDown(int key);
    void keyUp(int key);
    void mouseMove(int x, int y);
    void mouseButtonDown(int button, int x, int y);
    void mouseButtonUp(int button, int x, int y);
    void mouseWheel(int deltaX, int deltaY);

    inline void setDelegate(std::shared_ptr<InputDelegate> delegate) { _delegate = delegate; }
    
protected:
    std::shared_ptr<InputDelegate> _delegate;
};


}
