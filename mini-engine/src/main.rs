use vulkan::model::Model;

pub mod app;
pub mod vulkan;

fn main() {
    let mut app = app::App::new(
        "Vulkan App", 
        1024, 
        768
    );

    let model = Model::new(
        vec![
            vulkan::model::VertexData { position: [ 0.5, -0.5, 0.0], color: [1.0, 0.0, 0.0] },
            vulkan::model::VertexData { position: [-0.5, -0.5, 0.0], color: [0.0, 1.0, 0.0] },
            vulkan::model::VertexData { position: [-0.5,  0.5, 0.0], color: [0.0, 0.0, 1.0] },
            vulkan::model::VertexData { position: [ 0.5,  0.5, 0.0], color: [0.0, 0.0, 1.0] },
        ],
        vec![0, 1, 2, 2, 3, 0]
    );
    app.add_model(model);

    app.build();

    app.run();
}
