#include <mini_engine/PlatformTools.hpp>
#include <mini_engine/MainLoop.hpp>



int main(int argc, char** argv) {
    miniengine::MainLoop app;
    
    app.initWindowTitle("Mini Engine Test");
    app.initWindowSize(1400, 700);
    
    return app.run();
}