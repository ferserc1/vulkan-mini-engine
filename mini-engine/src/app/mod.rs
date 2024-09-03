use core::f32;
use std::{error::Error, sync::{Arc, RwLock}, vec};

use glam::Mat4;
use vulkano::{command_buffer::{allocator::StandardCommandBufferAllocator, CommandBufferBeginInfo, CommandBufferLevel, CommandBufferUsage, RecordingCommandBuffer, RenderPassBeginInfo}, descriptor_set::{allocator::StandardDescriptorSetAllocator, DescriptorSet, WriteDescriptorSet}, device::{physical::{PhysicalDevice, PhysicalDeviceType}, Device, DeviceCreateInfo, DeviceExtensions, Queue, QueueCreateInfo, QueueFlags}, format::Format, image::{view::ImageView, Image, ImageCreateInfo, ImageType, ImageUsage}, instance::{Instance, InstanceCreateFlags, InstanceCreateInfo}, memory::allocator::{AllocationCreateInfo, StandardMemoryAllocator}, pipeline::{graphics::{color_blend::{ColorBlendAttachmentState, ColorBlendState}, depth_stencil::{DepthState, DepthStencilState}, input_assembly::InputAssemblyState, multisample::MultisampleState, rasterization::{CullMode, RasterizationState}, vertex_input::{Vertex, VertexDefinition}, viewport::{Viewport, ViewportState}, GraphicsPipelineCreateInfo}, layout::PipelineDescriptorSetLayoutCreateInfo, DynamicState, GraphicsPipeline, Pipeline, PipelineLayout, PipelineShaderStageCreateInfo}, render_pass::{Framebuffer, FramebufferCreateInfo, RenderPass, Subpass}, swapchain::{self, Surface, Swapchain, SwapchainCreateInfo, SwapchainPresentInfo}, sync::{self, GpuFuture}, Validated, Version, VulkanError, VulkanLibrary};
use winit::{event::{ElementState, Event, KeyEvent, WindowEvent}, event_loop::{ControlFlow, EventLoop}, keyboard::{KeyCode, PhysicalKey}, window::Window};

use crate::vulkan::{model::{Model, VertexData}, scene::Camera};

pub struct VulkanResources {
    pub instance: Arc<Instance>,
    pub surface: Arc<Surface>,
    pub physical_device: Arc<PhysicalDevice>,
    pub queue_family_index: u32,
    pub device: Arc<Device>,
    pub queue: Arc<Queue>,
    pub swapchain: Arc<Swapchain>,
    pub swapchain_images: Vec<Arc<Image>>,
    pub command_buffer_allocator: Arc<StandardCommandBufferAllocator>,
    pub render_pass: Arc<RenderPass>,
    pub color_pipeline: Arc<GraphicsPipeline>,
    pub memory_allocator: Arc<StandardMemoryAllocator>,
    pub descriptor_set_allocator: Arc<StandardDescriptorSetAllocator>
}

pub struct App {
    // Window resources
    window_title: String,
    window_size: (u32, u32),
    event_loop: Option<EventLoop<()>>,
    window: Option<Arc<Window>>,

    // Vulkan resources
    vulkan_resources: Option<VulkanResources>,
    
    // Scene resources
    pub camera: Arc<RwLock<Camera>>,
    pub scene: Vec<Arc<RwLock<Model>>>
}

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

impl App {
    pub fn new(title: &str, width: u32, height: u32) -> Self {
        Self {
            // Window resources
            window_title: title.to_string(),
            window_size: (width, height),
            event_loop: None,
            window: None,

            vulkan_resources: None,
            
            // Scene resources
            camera: Arc::new(RwLock::new(Camera::new(65.0, 0.1, 100.0))),
            scene: Vec::new()
        }
    }

    pub fn build(&mut self) -> &mut Self {
        let event_loop = winit::event_loop::EventLoop::new()
            .expect("Failed to create event loop");

        let window = std::sync::Arc::new(winit::window::WindowBuilder::new()
            .with_title(self.window_title.clone())
            .with_inner_size(winit::dpi::LogicalSize::new(
                self.window_size.0,
                self.window_size.1
            ))
            .build(&event_loop)
            .expect("Failed to create window"));
        

        // Vulkan resources
        let instance = {
            let library = VulkanLibrary::new().unwrap();
            let extensions = Surface::required_extensions(&event_loop).unwrap();
        
            Instance::new(
                library,
                InstanceCreateInfo {
                    enabled_extensions: extensions,
                    flags: InstanceCreateFlags::ENUMERATE_PORTABILITY,
                    max_api_version: Some(Version::V1_3),
                    ..Default::default()
                }
            ).unwrap()
        };

        self.window = Some(window.clone());
        self.event_loop = Some(event_loop);
    
        let surface = Surface::from_window(instance.clone(), window.clone())
            .unwrap();
    
        let device_extensions = DeviceExtensions {
            khr_swapchain: true,
            ..DeviceExtensions::empty()
        };
    
        let (physical_device, queue_family_index) = instance
            .enumerate_physical_devices()
            .unwrap()
            .filter(|p| p.supported_extensions().contains(&device_extensions))
            .filter_map(|p| {
                p.queue_family_properties()
                    .iter()
                    .enumerate()
                    .position(|(i, q)| {
                        q.queue_flags.contains(QueueFlags::GRAPHICS) && p.surface_support(i as u32, &surface).unwrap_or(false)
                    })
                    .map(|i| (p, i as u32))
            })
            .min_by_key(|(p, _)| {
                match p.properties().device_type {
                    PhysicalDeviceType::DiscreteGpu => 0,
                    PhysicalDeviceType::IntegratedGpu => 1,
                    PhysicalDeviceType::VirtualGpu => 2,
                    PhysicalDeviceType::Cpu => 3,
                    PhysicalDeviceType::Other => 4,
                    _ => 5
                }
            })
            .expect("No suitable physical device found.");
    
        let (device, mut queues) = Device::new(
            physical_device.clone(),
            DeviceCreateInfo {
                enabled_extensions: device_extensions,
                queue_create_infos: vec![QueueCreateInfo {
                    queue_family_index,
                    ..Default::default()
                }],
                ..Default::default()
            }
        ).unwrap();
    
        let queue = queues.next().unwrap();
    
        let (swapchain, swapchain_images) = {
            let caps = device.physical_device()
                .surface_capabilities(&surface, Default::default())
                .unwrap();
    
            let usage = caps.supported_usage_flags;
            let alpha = caps.supported_composite_alpha.into_iter().next().unwrap();
    
            let image_format = device.physical_device()
                    .surface_formats(&surface, Default::default())
                    .unwrap()[0]
                    .0;
    
            let window = surface.object().unwrap().downcast_ref::<Window>().unwrap();
            let image_extent: [u32; 2] = window.inner_size().into();
    
            Swapchain::new(
                device.clone(),
                surface.clone(),
                SwapchainCreateInfo {
                    min_image_count: caps.min_image_count,
                    image_format,
                    image_extent,
                    image_usage: usage,
                    composite_alpha: alpha,
                    ..Default::default()
                }
            ).unwrap()
        };
    
        let command_buffer_allocator = Arc::new(
            StandardCommandBufferAllocator::new(device.clone(), Default::default())
        );

        color_vert::load(device.clone()).unwrap();
        color_frag::load(device.clone()).unwrap();
        
        let render_pass = vulkano::single_pass_renderpass! {
            device.clone(),
            attachments: {
                color: {
                    format: swapchain.image_format(),
                    samples: 1,
                    load_op: Clear,
                    store_op: Store
                },
                depth: {
                    format: Format::D16_UNORM,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare
                }
            },
            pass: {
                color: [color],
                depth_stencil: {depth}
            }
        }.expect("Couldn't create render pass");

        let color_pipeline = {
            let color_pass = Subpass::from(render_pass.clone(), 0).unwrap();
    
            let color_vs = color_vert::load(device.clone())
                .expect("Failed to load vertex shader module")
                .entry_point("main")
                .expect("Failed to set entry point");
            let color_fs = color_frag::load(device.clone())
                .expect("Failed to load fragment shader module")
                .entry_point("main")
                .expect("Failed to set entry point");
            
            let color_input_state = VertexData::per_vertex().definition(&color_vs).unwrap();

            let shader_stages: Vec<PipelineShaderStageCreateInfo> = vec![
                PipelineShaderStageCreateInfo::new(color_vs),
                PipelineShaderStageCreateInfo::new(color_fs)
            ].into_iter().collect();

            let layout = PipelineLayout::new(
                device.clone(),
                PipelineDescriptorSetLayoutCreateInfo::from_stages(&shader_stages)
                    .into_pipeline_layout_create_info(device.clone())
                    .unwrap()
            ).expect("Failed to create pipeline layout");

            GraphicsPipeline::new(
                device.clone(),
                None,
                GraphicsPipelineCreateInfo {
                    stages: shader_stages.into_iter().collect(),
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
            ).expect("Failed to create color pipeline")
        };
        
        let memory_allocator = Arc::new(StandardMemoryAllocator::new_default(device.clone()));

        let descriptor_set_allocator = Arc::new(StandardDescriptorSetAllocator::new(
            device.clone(),
            Default::default()
        ));


        self.vulkan_resources = Some(VulkanResources {
            instance,
            surface,
            physical_device,
            queue_family_index,
            device,
            queue,
            swapchain,
            swapchain_images,
            command_buffer_allocator,
            render_pass,
            color_pipeline,
            memory_allocator,
            descriptor_set_allocator
        });

        self
    }

    pub fn get_window(&self) -> Result<Arc<Window>, Box<dyn Error>> {
        match &self.window {
            Some(window) => Ok(window.clone()),
            None => Err("The application is not initialized".into())
        }
    }

    pub fn add_model(&mut self, model: Model) {
        self.scene.push(Arc::new(RwLock::new(model)));
    }

    pub fn run(&mut self) {
        let event_loop = self.event_loop.take().unwrap();
        let window = self.get_window().unwrap().clone();

        let vulkan_resources = match self.vulkan_resources {
            Some(ref mut resources) => resources,
            None => panic!("Vulkan resources not initialized")
        };
        
        let memory_allocator = vulkan_resources.memory_allocator.clone();
        let device = vulkan_resources.device.clone();
        let render_pass = vulkan_resources.render_pass.clone();
        let color_pipeline = vulkan_resources.color_pipeline.clone();
        let descriptor_set_allocator = vulkan_resources.descriptor_set_allocator.clone();
        let command_buffer_allocator = vulkan_resources.command_buffer_allocator.clone();
        let queue = vulkan_resources.queue.clone();

        // Initialize scene elements
        self.scene
            .iter()
            .for_each(|model| {
                let mut model = model.write().unwrap();
                model.build(memory_allocator.clone()) // Aquí habrá que pasar el dispositivo
                    .expect("Failed to build model");
            });

        let mut previous_frame_end = Some(Box::new(vulkano::sync::now(device.clone())) as Box<dyn GpuFuture>);
        let mut recreate_swapchain = false;

        // Framebuffer resources: stored locally in run function
        let mut viewport = Viewport {
            offset: [0.0, 0.0],
            extent: [0.0, 0.0],
            depth_range: 0.0..=1.0
        };
    
        let swapchain_images = vulkan_resources.swapchain_images.clone();
        let mut framebuffer = resize_framebuffer(
            memory_allocator.clone(),
            &swapchain_images, 
            render_pass.clone(), 
            &mut viewport
        ).0;
    

        event_loop.run(|event, target_window| {
            target_window.set_control_flow(ControlFlow::Poll);

            match event {
                Event::WindowEvent {
                    event: WindowEvent::CloseRequested,
                    ..
                } => {
                    target_window.exit();
                }

                Event::WindowEvent {
                    event: WindowEvent::Resized(_),
                    ..
                } => {
                    recreate_swapchain = true;
                }

                Event::WindowEvent {
                    event: WindowEvent::RedrawRequested,
                    ..
                } => {
                    

                    previous_frame_end
                        .as_mut()
                        .take()
                        .unwrap()
                        .cleanup_finished();

                    let image_extent: [u32; 2] = window.inner_size().into();

                    if image_extent.contains(&0) {
                        return;
                    }

                    if recreate_swapchain {
                        let swapchain = vulkan_resources.swapchain.clone();
                        let (new_swapchain, new_images) = swapchain.recreate(
                            SwapchainCreateInfo {
                                image_extent,
                                ..swapchain.create_info()
                            }
                        ).expect("Failed to recreate swap chain");

                        vulkan_resources.swapchain = new_swapchain;
                        vulkan_resources.swapchain_images = new_images.clone();

                        framebuffer = resize_framebuffer(
                            memory_allocator.clone(), 
                            &new_images.clone(), 
                            render_pass.clone(), 
                            &mut viewport
                        ).0;
                        recreate_swapchain = false;
                    }

                    let (image_index, suboptimal, acquire_future) = {
                        let swapchain = vulkan_resources.swapchain.clone();
                        match swapchain::acquire_next_image(swapchain.clone(), None).map_err(Validated::unwrap) {
                            Ok(r) => r,
                            Err(VulkanError::OutOfDate) => {
                                recreate_swapchain = true;
                                return;
                            }
                            Err(e) => panic!("Failed to acquire next image: {:?}", e),
                        }
                    };
    
                    if suboptimal {
                        recreate_swapchain = true;
                    }
    
                    let layout = &color_pipeline.layout().set_layouts()[0];
                    let view_matrix = self.camera.read().unwrap().transform.inverse();
                    let projection = Mat4::perspective_rh(
                        self.camera.read().unwrap().fov, 
                        viewport.extent[0] as f32 / viewport.extent[1] as f32,
                        self.camera.read().unwrap().near,
                        self.camera.read().unwrap().far
                    );
                    self.scene.iter().for_each(|model| {
                        {
                            let mut model = model.write().unwrap();
                            model.update(1.0 / 60.0);
                        }

                        let normal_matrix = model.read().unwrap().transform.inverse().transpose();
                        let uniform_data = color_vert::MatrixBuffer {
                            model: model.read().unwrap().transform.to_cols_array_2d().into(),
                            view: view_matrix.to_cols_array_2d().into(),
                            projection: projection.to_cols_array_2d().into(),
                            normal: normal_matrix.to_cols_array_2d().into()
                        };

                        let subbuffer = model.read().unwrap().matrix_buffer.as_ref().unwrap().allocate_sized().unwrap();
                        *subbuffer.write().unwrap() = uniform_data;
                        
                        let descriptor_set = DescriptorSet::new(
                            descriptor_set_allocator.clone(), 
                            layout.clone(), 
                            [
                                WriteDescriptorSet::buffer(0, subbuffer)
                            ], 
                            []
                        ).unwrap();
                        {
                            let mut model = model.write().unwrap();
                            model.descriptor_set = Some(descriptor_set);
                        }
                    });

                    // Create command buffer
                    let clear_values = vec![
                        Some([0.0, 0.0, 0.0, 1.0].into()),
                        Some(1.0.into())
                    ];

                    let mut cmd_buffer_builder = RecordingCommandBuffer::new(
                        command_buffer_allocator.clone(),
                        queue.queue_family_index(),
                        CommandBufferLevel::Primary,
                        CommandBufferBeginInfo {
                            usage: CommandBufferUsage::OneTimeSubmit,
                            ..Default::default()
                        }
                    ).expect("Failed to create command buffer");
                    
                    cmd_buffer_builder
                        .begin_render_pass(
                            RenderPassBeginInfo {
                                clear_values,
                                ..RenderPassBeginInfo::framebuffer(
                                    framebuffer[image_index as usize].clone(),
                                )
                            },
                            Default::default()
                        )
                        .unwrap()

                        .set_viewport(0, [viewport.clone()].into_iter().collect()).unwrap()

                        // Start render pass
                        .bind_pipeline_graphics(color_pipeline.clone()).unwrap();

                    self.scene.iter().for_each(|model| {
                        let model = model.read().unwrap();
                        model.draw(&mut cmd_buffer_builder, color_pipeline.clone());
                    });

                    // TODO: Other render passes

                    cmd_buffer_builder
                        .end_render_pass(Default::default())
                        .unwrap();

                    let command_buffer = cmd_buffer_builder.end().expect("Failed to build command buffer");

                    let swapchain = vulkan_resources.swapchain.clone();
                    let future = previous_frame_end
                        .take()
                        .unwrap()
                        .join(acquire_future)
                        .then_execute(queue.clone(), command_buffer)
                        .unwrap()
                        .then_swapchain_present(
                            queue.clone(), 
                            SwapchainPresentInfo::swapchain_image_index(swapchain.clone(), image_index)
                        )
                        .then_signal_fence_and_flush();

                    match future.map_err(Validated::unwrap) {
                        Ok(future) => {
                            previous_frame_end = Some(Box::new(future) as Box<_>);
                        }
                        Err(VulkanError::OutOfDate) => {
                            recreate_swapchain = true;
                            previous_frame_end = Some(Box::new(sync::now(device.clone())) as Box<_>);
                        }
                        Err(e) => {
                            println!("Failed to flush future: {:?}", e);
                            previous_frame_end = Some(Box::new(sync::now(device.clone()))  as Box<_>);
                        }
                    }
                }

                Event::WindowEvent {
                    event: WindowEvent::KeyboardInput {
                        event: KeyEvent {
                            state,
                            physical_key: key,
                            repeat: false,
                            ..
                        },
                        ..
                    },
                    ..
                } => {
                    if key == PhysicalKey::Code(KeyCode::Enter) && state == ElementState::Released {
                        println!("Enter released");
                    } else if key == PhysicalKey::Code(KeyCode::Enter) && state == ElementState::Pressed {
                        println!("Enter pressed");
                    }
                }

                Event::WindowEvent {
                    event: WindowEvent::CursorMoved { position, .. },
                    ..
                } => {
                    println!("Cursor moved to: {:?}", position);
                }

                Event::WindowEvent {
                    event: WindowEvent::MouseWheel { delta, .. },
                    ..
                } => {
                    println!("Mouse wheel delta: {:?}", delta);
                }

                Event::WindowEvent {
                    event: WindowEvent::MouseInput { device_id, state, button },
                    ..
                } => {
                    println!("Mouse input: {:?}, {:?}, {:?}", device_id, state, button);
                }

                // TODO: Esto puede ser un loop, o bien se podría invocar de forma manual para que la ventana no se
                // redibuje constantemente si no es necesario
                Event::AboutToWait => window.request_redraw(),
                _ => {}
            }
        }).unwrap();
    }
}

fn resize_framebuffer(
    allocator: Arc<StandardMemoryAllocator>,
    images: &[Arc<Image>],
    render_pass: Arc<RenderPass>,
    viewport: &mut Viewport
) -> (
    Vec<Arc<Framebuffer>>,
    // Arc<ImageView>,
    // Arc<ImageView>,
    // Arc<ImageView>
 ) {
    let dimensions = images[0].extent();
    viewport.extent = [dimensions[0] as f32, dimensions[1] as f32];

    // let color_buffer_image = Image::new(
    //     allocator.clone(),
    //     ImageCreateInfo {
    //         image_type: ImageType::Dim2d,
    //         format: Format::A2B10G10R10_UNORM_PACK32,
    //         extent: images[0].extent(),
    //         usage: ImageUsage::COLOR_ATTACHMENT |
    //                ImageUsage::INPUT_ATTACHMENT |
    //                ImageUsage::TRANSIENT_ATTACHMENT,
    //         ..Default::default()
    //     },
    //     AllocationCreateInfo::default()
    // ).expect("Could not create color g-buffer");

    // let color_buffer = ImageView::new_default(color_buffer_image)
    //     .expect("Could not create color buffer image view");

    // let normal_buffer_image = Image::new(
    //     allocator.clone(),
    //     ImageCreateInfo {
    //         image_type: ImageType::Dim2d,
    //         format: Format::R16G16B16A16_SFLOAT,
    //         extent: images[0].extent(),
    //         usage: ImageUsage::COLOR_ATTACHMENT |
    //                ImageUsage::INPUT_ATTACHMENT |
    //                ImageUsage::TRANSIENT_ATTACHMENT,
    //         ..Default::default()
    //     },
    //     AllocationCreateInfo::default()
    // ).expect("Could not create normal g-buffer");

    // let normal_buffer = ImageView::new_default(normal_buffer_image)
    //     .expect("Could not create normal buffer image view");

    // let position_buffer_image = Image::new(
    //     allocator.clone(),
    //     ImageCreateInfo {
    //         image_type: ImageType::Dim2d,
    //         format: Format::R32G32B32A32_SFLOAT,
    //         extent: images[0].extent(),
    //         usage: ImageUsage::COLOR_ATTACHMENT |
    //                ImageUsage::INPUT_ATTACHMENT |
    //                ImageUsage::TRANSIENT_ATTACHMENT,
    //         ..Default::default()
    //     },
    //     AllocationCreateInfo::default()
    // ).expect("Failed to create position g-buffer");

    // let position_buffer = ImageView::new_default(position_buffer_image)
    //     .expect("Could not create position buffer image view");

    let depth_buffer_image = Image::new(
        allocator.clone(),
        ImageCreateInfo {
            image_type: ImageType::Dim2d,
            format: Format::D16_UNORM,
            extent: images[0].extent(),
            usage: ImageUsage::DEPTH_STENCIL_ATTACHMENT |
                   ImageUsage::TRANSIENT_ATTACHMENT,
            ..Default::default()
        },
        AllocationCreateInfo::default()
    ).expect("Could not create depth buffer image");

    let depth_buffer= ImageView::new_default(depth_buffer_image)
        .expect("Could not create depth buffer image view");

    let framebuffers = images
        .iter()
        .map(|image| {
            let view = ImageView::new_default(image.clone()).unwrap();
            Framebuffer::new(
                render_pass.clone(),
                FramebufferCreateInfo {
                    attachments: vec![
                        view,
                        //color_buffer.clone(),
                        //normal_buffer.clone(),
                        //position_buffer.clone(),
                        depth_buffer.clone()
                    ],
                    ..Default::default()
                }
            )
            .unwrap()
        })
        .collect::<Vec<_>>();

    (
        framebuffers,
        // color_buffer,
        // normal_buffer,
        // position_buffer
    )
}
