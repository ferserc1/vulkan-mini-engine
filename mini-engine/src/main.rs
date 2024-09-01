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
            // Front face vertices
            vulkan::model::VertexData { position: [ 0.5, -0.5,  0.5], color: [1.0, 0.0, 0.0] },
            vulkan::model::VertexData { position: [-0.5, -0.5,  0.5], color: [0.0, 1.0, 0.0] },
            vulkan::model::VertexData { position: [-0.5,  0.5,  0.5], color: [0.0, 0.0, 1.0] },
            vulkan::model::VertexData { position: [ 0.5,  0.5,  0.5], color: [0.0, 0.0, 1.0] },

            // Back face vertices
            vulkan::model::VertexData { position: [ 0.5, -0.5, -0.5], color: [1.0, 0.0, 0.0] },
            vulkan::model::VertexData { position: [-0.5, -0.5, -0.5], color: [0.0, 1.0, 0.0] },
            vulkan::model::VertexData { position: [-0.5,  0.5, -0.5], color: [0.0, 0.0, 1.0] },
            vulkan::model::VertexData { position: [ 0.5,  0.5, -0.5], color: [0.0, 0.0, 1.0] },
        ],
        vec![
            0, 1, 2, 2, 3, 0, // Front face
            5, 4, 7, 7, 6, 5, // Back face
            4, 0, 3, 3, 7, 4, // Right face
            1, 5, 6, 6, 2, 1, // Left face
            4, 5, 1, 1, 0, 4, // Top face
            6, 7, 3, 3, 2, 6  // Bottom face
        ]
    );
    app.add_model(model);

    app.build();

    app.run();
}
