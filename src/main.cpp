#include <mini_engine/PlatformTools.hpp>
#include <mini_engine/MainLoop.hpp>

#include <ClearBackgroundDrawDelegate.hpp>

int main(int argc, char** argv) {
    miniengine::MainLoop app;
    
    app.initWindowTitle("Mini Engine Test");
    app.initWindowSize(1400, 700);
    app.setDrawLoopDelegate(new ClearBackgroundDrawDelegate());
    
    return app.run();
}
