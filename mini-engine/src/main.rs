
pub mod app;

fn main() {
    let app = app::App::new(
        "Vulkan App", 
        1024, 
        768
    );
    
    app.run();
}
