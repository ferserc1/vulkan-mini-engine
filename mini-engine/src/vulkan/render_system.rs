
use core::f32;
use std::sync::{Arc, RwLock, RwLockReadGuard};

use glam::{Mat3, Mat4};
use vulkano::{ command_buffer::{CommandBuffer, CommandBufferBeginInfo, CommandBufferLevel, CommandBufferUsage, RecordingCommandBuffer, RenderPassBeginInfo, SubpassBeginInfo, SubpassContents}, descriptor_set::{allocator::StandardDescriptorSetAllocator, DescriptorSet, WriteDescriptorSet}, device::Device, format::Format, padded::Padded, pipeline::{graphics::{color_blend::{ColorBlendAttachmentState, ColorBlendState}, depth_stencil::{DepthState, DepthStencilState}, input_assembly::InputAssemblyState, multisample::MultisampleState, rasterization::{CullMode, RasterizationState}, vertex_input::{Vertex, VertexDefinition, VertexInputState}, viewport::ViewportState, GraphicsPipelineCreateInfo}, layout::PipelineDescriptorSetLayoutCreateInfo, DynamicState, GraphicsPipeline, Pipeline, PipelineBindPoint, PipelineLayout, PipelineShaderStageCreateInfo}, render_pass::{RenderPass, Subpass}, swapchain::Swapchain };

use super::{model::{Model, VertexData}, scene::Camera, Context};

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

mod gbuffer_vert {
    vulkano_shaders::shader! {
        ty: "vertex",
        path: "src/shaders/gbuffer.vert.glsl"
    }
}

mod gbuffer_frag {
    vulkano_shaders::shader! {
        ty: "fragment",
        path: "src/shaders/gbuffer.frag.glsl"
    }
}

mod deferred_mix_vert {
    vulkano_shaders::shader! {
        ty: "vertex",
        path: "src/shaders/deferred-mix.vert.glsl"
    }
}

mod deferred_mix_frag {
    vulkano_shaders::shader! {
        ty: "fragment",
        path: "src/shaders/deferred-mix.frag.glsl"
    }
}

pub struct RenderSystem {
    pub layout: Arc<PipelineLayout>,
    pub pipeline: Arc<GraphicsPipeline>,
    pub deferred_mix_pipeline: Arc<GraphicsPipeline>,
    recreate_swapchain: bool,

    // Camera matrixes
    view_matrix: Mat4,
    projection_matrix: Mat4
}

impl RenderSystem {
    pub fn get_render_pass(device: Arc<Device>, swapchain: Arc<Swapchain>) -> Arc<RenderPass> {
        let render_pass = vulkano::ordered_passes_renderpass!(device.clone(),
            attachments: {
                final_color: {
                    format: swapchain.image_format(),
                    samples: 1,
                    load_op: Clear,
                    store_op: Store
                },
                normals: {
                    format: Format::R16G16B16A16_SFLOAT,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare
                },
                positions: {
                    format: Format::R32G32B32A32_SFLOAT,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare
                },
                color: {
                    format: Format::A2B10G10R10_UNORM_PACK32,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare
                },
                depth: {
                    format: Format::D16_UNORM,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare
                }
            },
            passes: [
                {
                    color: [normals, positions, color],
                    depth_stencil: {depth},
                    input: []
                },
                {
                    color: [final_color],
                    depth_stencil: {},
                    input: [normals, positions, color]
                }
            ]
        ).expect("Failed to create render pass");

        render_pass
    }

    pub fn new(context: &Context) -> Self {
        let device = context.device_data.device.clone();
        color_vert::load(device.clone()).unwrap();
        color_frag::load(device.clone()).unwrap();

        let render_pass = context.framebuffer.read().unwrap().render_pass.clone();
        
        // g-buffers pipeline
        let color_pass = Subpass::from(render_pass.clone(), 0).unwrap();

        let color_vertex_entry_point = gbuffer_vert::load(device.clone())
            .expect("Failed to create vertex shader module")
            .entry_point("main")
            .expect("Failed to get color vertex shader entry point");
        let color_fragment_entry_point = gbuffer_frag::load(device.clone())
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
                ..GraphicsPipelineCreateInfo::layout(layout.clone())
            }
        ).expect("Failed to create color graphics pipeline");

        // Deferred mix pipeline
        let deferred_mix_pass = Subpass::from(render_pass.clone(), 1).unwrap();

        let deferred_mix_vertex_entry_point = deferred_mix_vert::load(device.clone())
            .expect("Failed to create vertex shader module")
            .entry_point("main")
            .expect("Failed to get deferred mix vertex shader entry point");
        let deferred_mix_fragment_entry_point = deferred_mix_frag::load(device.clone())
            .expect("Failed to create fragment shader module")
            .entry_point("main")
            .expect("Failed to get deferred mix vertex shader entry point");

        let deferred_mix_shader_stages: Vec<PipelineShaderStageCreateInfo> = vec![
            PipelineShaderStageCreateInfo::new(deferred_mix_vertex_entry_point),
            PipelineShaderStageCreateInfo::new(deferred_mix_fragment_entry_point)
        ].into_iter().collect();

        let deferred_mix_layout = PipelineLayout::new(
            device.clone(),
            PipelineDescriptorSetLayoutCreateInfo::from_stages(&deferred_mix_shader_stages)
                .into_pipeline_layout_create_info(device.clone())
                .unwrap()
        ).expect("Co8uld not create the pipeline layout");

        let deferred_mix_pipeline = GraphicsPipeline::new(
            device.clone(),
            None,
            GraphicsPipelineCreateInfo {
                stages: deferred_mix_shader_stages.into_iter().collect(),
                vertex_input_state: Some(VertexInputState::default()),
                input_assembly_state: Some(InputAssemblyState::default()),
                viewport_state: Some(ViewportState::default()),
                rasterization_state: Some(RasterizationState {
                    cull_mode: CullMode::Back,
                    ..Default::default()
                }),
                multisample_state: Some(MultisampleState::default()),
                color_blend_state: Some(ColorBlendState::with_attachment_states(
                    deferred_mix_pass.num_color_attachments(), 
                    ColorBlendAttachmentState::default())
                ),
                dynamic_state: [DynamicState::Viewport].into_iter().collect(),
                subpass: Some(deferred_mix_pass.into()),
                ..GraphicsPipelineCreateInfo::layout(deferred_mix_layout)
            }
        ).expect("Failed to create deferred mix graphics pipeline");
        
        Self {
            layout,
            pipeline,
            recreate_swapchain: false,
            view_matrix: Mat4::IDENTITY,
            projection_matrix: Mat4::IDENTITY,
            deferred_mix_pipeline
        }
    }

    pub fn resize(&mut self) {
        self.recreate_swapchain = true;
    }

    pub fn update(
        &mut self,
        viewport_extent: [u32; 2],
        camera: &Camera,
        scene: &mut Vec<Arc<RwLock<Model>>>,
        descriptor_set_allocator: Arc<StandardDescriptorSetAllocator>
    ) {
        // Update camera
        let aspect_ratio = viewport_extent[0] as f32 / viewport_extent[1] as f32;
        self.projection_matrix = Mat4::perspective_rh(
            camera.fov * f32::consts::PI / 180.0,
            aspect_ratio,
            camera.near,
            camera.far
        );
        self.view_matrix = camera.transform.inverse();
        
        // Update scene elements
        scene.iter().for_each(|model| {
            {
                // TODO: Implement delta
                model.write().unwrap().update(1.0 / 60.0);
            }
            
            let subbuffer = {
                let transform = &model.read().unwrap().transform;
                let normal_matrix = transform.inverse().transpose();
                let uniform_data = gbuffer_vert::MatrixBuffer {
                    model: transform.to_cols_array_2d(),
                    view: self.view_matrix.to_cols_array_2d(),
                    projection: self.projection_matrix.to_cols_array_2d(),
                    normal: normal_matrix.to_cols_array_2d()
                };

                let subbuffer = model.read().unwrap().matrix_buffer.as_ref().unwrap().allocate_sized().unwrap();
                {
                    *subbuffer.write().unwrap() = uniform_data;
                }
                subbuffer
            };

            let descriptor_set = {
                DescriptorSet::new(
                    descriptor_set_allocator.clone(),
                    self.pipeline.layout().set_layouts()[0].clone(),
                    [
                        WriteDescriptorSet::buffer(0, subbuffer)
                    ],
                    []
                ).unwrap()
            };

            {
                model.write().unwrap().descriptor_set = Some(descriptor_set);
            }
        });
    }

    fn render_scene_elements(&self, scene: &Vec<Arc<RwLock<Model>>>, cmd_buffer: &mut RecordingCommandBuffer) {
        cmd_buffer.bind_pipeline_graphics(self.pipeline.clone()).unwrap();

        scene.iter().for_each(|model| {
            model.read().unwrap()
                .draw(cmd_buffer, self.pipeline.clone());
        });
    }

    pub fn render(
        &self,
        scene: &Vec<Arc<RwLock<Model>>>,
        vulkan_context: RwLockReadGuard<Context>,
        image_index: usize
    ) -> Arc<CommandBuffer> {

        // TODO: Add g-buffer images
        let normal_gbuffer = vulkan_context.framebuffer.read().unwrap()
            .normal_gbuffer.clone().unwrap();
        let position_gbuffer = vulkan_context.framebuffer.read().unwrap()
            .position_gbuffer.clone().unwrap();
        let color_gbuffer = vulkan_context.framebuffer.read().unwrap()
            .color_gbuffer.clone().unwrap();

        let deferred_mix_layout = &self.deferred_mix_pipeline.layout().set_layouts()[0];
        let deferred_mix_set = DescriptorSet::new(
            vulkan_context.descriptor_set_allocator.clone(),
            deferred_mix_layout.clone(),
            [
                WriteDescriptorSet::image_view(0, normal_gbuffer.clone()),
                WriteDescriptorSet::image_view(1, position_gbuffer.clone()),
                WriteDescriptorSet::image_view(2, color_gbuffer.clone())
                // TODO: Add the rest of input g-buffers
            ],
            []
        ).unwrap();

        let clear_values = vec![
            Some([0.0, 0.0, 0.0, 0.0].into()),
            Some([0.0, 0.0, 0.0, 0.0].into()),
            Some([0.0, 0.0, 0.0, 0.0].into()),
            Some([0.0, 0.0, 0.0, 0.0].into()),
            Some(1.0.into())
        ];

        let queue = vulkan_context.device_data.queue.clone();
        let mut cmd_buffer_builder = RecordingCommandBuffer::new(
            vulkan_context.command_buffer_allocator.clone(),
            queue.clone().queue_family_index(),
            CommandBufferLevel::Primary,
            CommandBufferBeginInfo {
                usage: CommandBufferUsage::OneTimeSubmit,
                ..Default::default()
            }
        ).expect("Failed to create command buffer builder");

        let fb = vulkan_context.framebuffer.read().unwrap().get_framebuffer(image_index);
        let viewport = vulkan_context.viewport.read().unwrap().clone();
        cmd_buffer_builder.begin_render_pass(
            RenderPassBeginInfo {
                clear_values,
                ..RenderPassBeginInfo::framebuffer(
                    fb
                )
            },
            Default::default()
        )
        .unwrap()
        .set_viewport(0, [viewport.clone()].into_iter().collect()).unwrap();

        // First render pass: render scene g-buffers
        self.render_scene_elements(&scene, &mut cmd_buffer_builder);

        // Second render pass: mix g-buffers
        let deferred_pipeline = self.deferred_mix_pipeline.clone();
        cmd_buffer_builder
            .next_subpass(
                Default::default(), 
                SubpassBeginInfo {
                    contents: SubpassContents::Inline,
                    ..Default::default()
                }
            ).unwrap()
            .bind_pipeline_graphics(self.deferred_mix_pipeline.clone()).unwrap()
            .bind_descriptor_sets(
                PipelineBindPoint::Graphics,
                deferred_pipeline.layout().clone(),
                0,
                deferred_mix_set
            ).unwrap();

        // Render quad to mix g-buffers
        unsafe {
            // The quad vertices are statically defined in the vertex shader
            cmd_buffer_builder
                .draw(6, 1, 0, 0)
                .unwrap();
        }

        cmd_buffer_builder.end_render_pass(Default::default())
            .unwrap();

        cmd_buffer_builder.end().expect("Failed to build command buffer")
    }
}