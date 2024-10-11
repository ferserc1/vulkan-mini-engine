#include <vkme/PlatformTools.hpp>
#include <vkme/MainLoop.hpp>

#include <SimpleTriangle.hpp>
#include <ClearBackgroundDrawDelegate.hpp>
#include <ComputeShaderBackgroundDelegate.hpp>
#include <PushConstantsComputeShaderDelegate.hpp>
#include <ColorTriangleDelegate.hpp>
#include <MeshBuffersDelegate.hpp>
#include <TestModelDelegate.hpp>
#include <VertexBuffersDelegate.hpp>
#include <TexturesTestDelegate.hpp>
#include <GeometryDelegate.hpp>
#include <SkySphereDelegate.hpp>
#include <RenderToTexture.hpp>
#include <RenderToCubemap.hpp>

int main(int argc, char** argv) {
    vkme::MainLoop app;
    
    app.initWindowTitle("Mini Engine Test");
    app.initWindowSize(1400, 700);

    
    //auto delegate = std::shared_ptr<SimpleTriangleDelegate>(new SimpleTriangleDelegate());
    //auto delegate = std::shared_ptr<VertexBuffersDelegate>(new VertexBuffersDelegate());
    
    //auto delegate = std::shared_ptr<ClearBackgroundDrawDelegate>(new ClearBackgroundDrawDelegate());
    //auto delegate = std::shared_ptr<ComputeShaderBackgroundDelegate>(new ComputeShaderBackgroundDelegate());
    //auto delegate = std::shared_ptr<PushConstantsComputeShaderDelegate>(new PushConstantsComputeShaderDelegate());
    //auto delegate = std::shared_ptr<ColorTriangleDelegate>(new ColorTriangleDelegate());
    //auto delegate = std::shared_ptr<MeshBuffersDelegate>(new MeshBuffersDelegate());
    //auto delegate = std::shared_ptr<TestModelDelegate>(new TestModelDelegate());
    //auto delegate = std::shared_ptr<TexturesTestDelegate>(new TexturesTestDelegate());
    //auto delegate = std::shared_ptr<GeometryDelegate>(new GeometryDelegate());
    //auto delegate = std::shared_ptr<SkySphereDelegate>(new SkySphereDelegate());
    //auto delegate = std::shared_ptr<RenderToTexture>(new RenderToTexture());
    auto delegate = std::shared_ptr<RenderToCubemap>(new RenderToCubemap());
    app.setDrawLoopDelegate(delegate);
    app.setUIDelegate(delegate);
    
    return app.run();
}
