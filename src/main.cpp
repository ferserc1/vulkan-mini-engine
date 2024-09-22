#include <vkme/PlatformTools.hpp>
#include <vkme/MainLoop.hpp>

#include <ClearBackgroundDrawDelegate.hpp>
#include <ComputeShaderBackgroundDelegate.hpp>
#include <PushConstantsComputeShaderDelegate.hpp>
#include <ColorTriangleDelegate.hpp>
#include <MeshBuffersDelegate.hpp>

int main(int argc, char** argv) {
    vkme::MainLoop app;
    
    app.initWindowTitle("Mini Engine Test");
    app.initWindowSize(1400, 700);
    
    // Example 1
    //app.setDrawLoopDelegate(std::shared_ptr<ClearBackgroundDrawDelegate>(new ClearBackgroundDrawDelegate()));
    
    // Example 2
    // This delegate extends the draw and ui delegates, so we can use it for both.
    //auto delegate = std::shared_ptr<ComputeShaderBackgroundDelegate>(new ComputeShaderBackgroundDelegate());
    //auto delegate = std::shared_ptr<PushConstantsComputeShaderDelegate>(new PushConstantsComputeShaderDelegate());
    //auto delegate = std::shared_ptr<ColorTriangleDelegate>(new ColorTriangleDelegate());
    auto delegate = std::shared_ptr<MeshBuffersDelegate>(new MeshBuffersDelegate());
    app.setDrawLoopDelegate(delegate);
    app.setUIDelegate(delegate);
    
    return app.run();
}
