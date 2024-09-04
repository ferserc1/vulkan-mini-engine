use std::sync::Arc;
use vulkano::{pipeline::{graphics::vertex_input::{Vertex, VertexDefinition, VertexInputState}, PipelineShaderStageCreateInfo}, shader::ShaderModule, Validated};
use crate::vulkan::model::VertexData;

pub fn create_shader_stages(
    vs_module: Result<Arc<ShaderModule>, Validated<vulkano::VulkanError>>,
    fs_module: Result<Arc<ShaderModule>, Validated<vulkano::VulkanError>>
) -> (
    Vec<PipelineShaderStageCreateInfo>,
    VertexInputState
) {
    let vs = vs_module
        .expect("Could not load vertex shader module")
        .entry_point("main")
        .expect("Failed to load color vertex shader entry point");

    let fs = fs_module
        .expect("Could not load fragment shader module")
        .entry_point("main")
        .expect("Failed to load color fragment shader entry point");

    let input_state = VertexData::per_vertex().definition(&vs).unwrap();

    let shader_stages: Vec<PipelineShaderStageCreateInfo> = vec![
        PipelineShaderStageCreateInfo::new(vs),
        PipelineShaderStageCreateInfo::new(fs)
    ].into_iter().collect();

    (
        shader_stages, 
        input_state
    )
}

pub mod color {
    use std::sync::Arc;
    use vulkano::{device::Device, pipeline::{graphics::vertex_input::VertexInputState, PipelineShaderStageCreateInfo}};

    pub mod vs {
        vulkano_shaders::shader! {
            ty: "vertex",
            path: "src/shaders/color.vert.glsl"
        }
    }

    pub mod fs {
        vulkano_shaders::shader! {
            ty: "fragment",
            path: "src/shaders/color.frag.glsl"
        }
    }

    pub fn get_shader(device: Arc<Device>) -> (Vec<PipelineShaderStageCreateInfo>, VertexInputState) {
        super::create_shader_stages(
            vs::load(device.clone()),
            fs::load(device.clone())
        )
    }
}

pub mod gbuffers {
    use std::sync::Arc;
    use vulkano::{device::Device, pipeline::{graphics::vertex_input::VertexInputState, PipelineShaderStageCreateInfo}};

    pub mod vs {
        vulkano_shaders::shader! {
            ty: "vertex",
            path: "src/shaders/gbuffer.vert.glsl"
        }
    }

    pub mod fs {
        vulkano_shaders::shader! {
            ty: "fragment",
            path: "src/shaders/gbuffer.frag.glsl"
        }
    }

    pub fn get_shader(device: Arc<Device>) -> (Vec<PipelineShaderStageCreateInfo>, VertexInputState) {
        super::create_shader_stages(
            vs::load(device.clone()),
            fs::load(device.clone())
        )
    }
}

pub mod simple_mix {
    use std::sync::Arc;
    use vulkano::{device::Device, pipeline::{graphics::vertex_input::VertexInputState, PipelineShaderStageCreateInfo}};

    pub mod vs {
        vulkano_shaders::shader! {
            ty: "vertex",
            path: "src/shaders/simple_mix.vert.glsl"
        }
    }

    pub mod fs {
        vulkano_shaders::shader! {
            ty: "fragment",
            path: "src/shaders/simple_mix.frag.glsl"
        }
    }

    pub fn get_shader(device: Arc<Device>) -> (Vec<PipelineShaderStageCreateInfo>, VertexInputState) {
        super::create_shader_stages(
            vs::load(device.clone()),
            fs::load(device.clone())
        )
    }
}
