
use std::sync::{Arc, RwLock};

use vulkano::{ command_buffer::RecordingCommandBuffer, pipeline::{graphics::{color_blend::{ColorBlendAttachmentState, ColorBlendState}, depth_stencil::{DepthState, DepthStencilState}, input_assembly::InputAssemblyState, multisample::MultisampleState, rasterization::{CullMode, RasterizationState}, vertex_input::{Vertex, VertexDefinition}, viewport::ViewportState, GraphicsPipelineCreateInfo}, layout::PipelineDescriptorSetLayoutCreateInfo, DynamicState, GraphicsPipeline, PipelineLayout, PipelineShaderStageCreateInfo}, render_pass::Subpass };

use super::{model::{Model, VertexData}, Context};

mod color_vert {
    vulkano_shaders::shader! {
        ty: "vertex",
        path: "src/shaders/color.vert.glsl"
    }
}

mod color_frag {
    vulkano_shaders::shader! {
        ty: "fragment",
        path: "src/shaders/color.frag.glsl"
    }
}

pub struct RenderSystem {
    pub pipeline: Arc<GraphicsPipeline>,
    recreate_swapchain: bool
}

impl RenderSystem {
    pub fn new(context: &Context) -> Self {
        let device = context.device_data.device.clone();
        color_vert::load(device.clone()).unwrap();
        color_frag::load(device.clone()).unwrap();

        let render_pass = context.framebuffer.read().unwrap().render_pass.clone();
        let color_pass = Subpass::from(render_pass, 0).unwrap();

        let color_vertex_entry_point = color_vert::load(device.clone())
            .expect("Failed to create vertex shader module")
            .entry_point("main")
            .expect("Failed to get color vertex shader entry point");
        let color_fragment_entry_point = color_frag::load(device.clone())
            .expect("Failed to create fragment shader module")
            .entry_point("main")
            .expect("Failed to get color fragment shader entry point");

        let color_input_state = VertexData::per_vertex().definition(&color_vertex_entry_point).unwrap();

        let color_shader_stages: Vec<PipelineShaderStageCreateInfo> = vec![
            PipelineShaderStageCreateInfo::new(color_vertex_entry_point),
            PipelineShaderStageCreateInfo::new(color_fragment_entry_point)
        ].into_iter().collect();

        let layout = PipelineLayout::new(
            device.clone(),
            PipelineDescriptorSetLayoutCreateInfo::from_stages(&color_shader_stages)
                .into_pipeline_layout_create_info(device.clone())
                .unwrap()
        ).expect("Could not create color pipeline layout");

        let pipeline = GraphicsPipeline::new(
            device.clone(),
            None,
            GraphicsPipelineCreateInfo {
                stages: color_shader_stages.into_iter().collect(),
                vertex_input_state: Some(color_input_state),
                input_assembly_state: Some(InputAssemblyState::default()),
                viewport_state: Some(ViewportState::default()),
                rasterization_state: Some(RasterizationState {
                    cull_mode: CullMode::Back,
                    ..Default::default()
                }),
                multisample_state: Some(MultisampleState::default()),
                color_blend_state: Some(ColorBlendState::with_attachment_states(
                    color_pass.num_color_attachments(), 
                    ColorBlendAttachmentState::default()
                )),
                dynamic_state: [DynamicState::Viewport].into_iter().collect(),
                subpass: Some(color_pass.into()),
                depth_stencil_state: Some(DepthStencilState {
                    depth: Some(DepthState::simple()),
                    ..Default::default()
                }),
                ..GraphicsPipelineCreateInfo::layout(layout)
            }
        ).expect("Failed to create color graphics pipeline");
        
        Self {
            pipeline,
            recreate_swapchain: false
        }
    }

    pub fn resize(&mut self) {
        self.recreate_swapchain = true;
    }

    pub fn render(&self, scene: &Vec<Arc<RwLock<Model>>>, cmd_buffer: &mut RecordingCommandBuffer) {
        cmd_buffer.bind_pipeline_graphics(self.pipeline.clone()).unwrap();

        scene.iter().for_each(|model| {
            let model = model.read().unwrap();
            let vertex_buffer = model.vertex_buffer.as_ref().unwrap();

            cmd_buffer.bind_vertex_buffers(0, vertex_buffer.clone()).unwrap();

            unsafe {
                cmd_buffer.draw(
                    model.vertices.len() as u32,
                    1,
                    0,
                    0
                ).unwrap();
            }
        });
    }
}