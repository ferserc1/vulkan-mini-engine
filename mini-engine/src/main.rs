
pub mod app;
pub mod vulkan;

fn main() {
    let mut app = app::App::new(
        "Vulkan App", 
        1024, 
        768
    );

    app.build();

    app.run();
}
