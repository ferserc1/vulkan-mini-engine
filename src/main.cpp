#include <vkme/PlatformTools.hpp>
#include <vkme/MainLoop.hpp>

#include <ClearBackgroundDrawDelegate.hpp>
#include <ComputeShaderBackgroundDrawDelegate.hpp>

// A function that print hello world
void sayHello() {
    
}

int main(int argc, char** argv) {
    vkme::MainLoop app;
    
    app.initWindowTitle("Mini Engine Test");
    app.initWindowSize(1400, 700);
    
    // Example 1
    //app.setDrawLoopDelegate(new ClearBackgroundDrawDelegate());
    
    // Example 2
    app.setDrawLoopDelegate(new ComputeShaderBackgroundDrawDelegate());
    
    return app.run();
}
