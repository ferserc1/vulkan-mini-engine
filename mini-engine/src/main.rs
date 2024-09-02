use glam::vec3;
use vulkan::model::{ Model, VertexData };

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
            // front face
            VertexData { position: [-1.000000, -1.000000,  1.000000], normal: [0.0000, 0.0000,  1.0000] }, // 0
            VertexData { position: [-1.000000,  1.000000,  1.000000], normal: [0.0000, 0.0000,  1.0000] }, // 1
            VertexData { position: [ 1.000000,  1.000000,  1.000000], normal: [0.0000, 0.0000,  1.0000] }, // 2
            VertexData { position: [ 1.000000, -1.000000,  1.000000], normal: [0.0000, 0.0000,  1.0000] }, // 3

            // back face
            VertexData { position: [ 1.000000, -1.000000, -1.000000], normal: [0.0000, 0.0000, -1.0000] }, // 4
            VertexData { position: [ 1.000000,  1.000000, -1.000000], normal: [0.0000, 0.0000, -1.0000] }, // 5
            VertexData { position: [-1.000000,  1.000000, -1.000000], normal: [0.0000, 0.0000, -1.0000] }, // 6
            VertexData { position: [-1.000000, -1.000000, -1.000000], normal: [0.0000, 0.0000, -1.0000] }, // 7

            // top face
            VertexData { position: [-1.000000, -1.000000,  1.000000], normal: [0.0000, -1.0000, 0.0000] }, // 8
            VertexData { position: [ 1.000000, -1.000000,  1.000000], normal: [0.0000, -1.0000, 0.0000] }, // 9
            VertexData { position: [ 1.000000, -1.000000, -1.000000], normal: [0.0000, -1.0000, 0.0000] }, // 10
            VertexData { position: [-1.000000, -1.000000, -1.000000], normal: [0.0000, -1.0000, 0.0000] }, // 11

            // bottom face
            VertexData { position: [ 1.000000,  1.000000,  1.000000], normal: [0.0000, 1.0000, 0.0000] }, // 12
            VertexData { position: [-1.000000,  1.000000,  1.000000], normal: [0.0000, 1.0000, 0.0000] }, // 13
            VertexData { position: [-1.000000,  1.000000, -1.000000], normal: [0.0000, 1.0000, 0.0000] }, // 14
            VertexData { position: [ 1.000000,  1.000000, -1.000000], normal: [0.0000, 1.0000, 0.0000] }, // 15


            // left face
            VertexData { position: [-1.000000, -1.000000, -1.000000], normal: [-1.0000, 0.0000, 0.0000] }, // 16
            VertexData { position: [-1.000000,  1.000000, -1.000000], normal: [-1.0000, 0.0000, 0.0000] }, // 17
            VertexData { position: [-1.000000,  1.000000,  1.000000], normal: [-1.0000, 0.0000, 0.0000] }, // 18
            VertexData { position: [-1.000000, -1.000000,  1.000000], normal: [-1.0000, 0.0000, 0.0000] }, // 19


            // right face
            VertexData { position: [ 1.000000, -1.000000,  1.000000], normal: [1.0000, 0.0000, 0.0000] }, // 20
            VertexData { position: [ 1.000000,  1.000000,  1.000000], normal: [1.0000, 0.0000, 0.0000] }, // 21
            VertexData { position: [ 1.000000,  1.000000, -1.000000], normal: [1.0000, 0.0000, 0.0000] }, // 22
            VertexData { position: [ 1.000000, -1.000000, -1.000000], normal: [1.0000, 0.0000, 0.0000] }  // 23
        ],
        vec![
            0, 1, 2, 2, 3, 0,       // Front face
            4, 5, 6, 6, 7, 4,       // Back face
            8, 9, 10, 10, 11, 8,    // Top face
            12, 13, 14, 14, 15, 12, // Bottom face
            16, 17, 18, 18, 19, 16, // Left face
            20, 21, 22, 22, 23, 20  // Right face
        ]
    );
    app.add_model(model);

    {
        app.camera.write().unwrap().transform = glam::Mat4::from_translation(vec3(0.0, 0.0, 5.0));
    }

    app.build();

    app.run();
}
