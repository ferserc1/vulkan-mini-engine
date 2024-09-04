use std::sync::Arc;

use vulkano::{command_buffer::allocator::StandardCommandBufferAllocator, descriptor_set::allocator::StandardDescriptorSetAllocator, device::{physical::{PhysicalDevice, PhysicalDeviceType}, Device, DeviceCreateInfo, DeviceExtensions, Queue, QueueCreateInfo, QueueFlags}, format::Format, image::Image, instance::{Instance, InstanceCreateFlags, InstanceCreateInfo}, memory::allocator::StandardMemoryAllocator, pipeline::{graphics::{color_blend::{ColorBlendAttachmentState, ColorBlendState}, depth_stencil::{DepthState, DepthStencilState}, input_assembly::InputAssemblyState, multisample::MultisampleState, rasterization::{CullMode, RasterizationState}, viewport::ViewportState, GraphicsPipelineCreateInfo}, layout::PipelineDescriptorSetLayoutCreateInfo, DynamicState, GraphicsPipeline, PipelineLayout }, render_pass::{RenderPass, Subpass}, swapchain::{Surface, Swapchain, SwapchainCreateInfo}, Version, VulkanLibrary};
use winit::{event_loop::EventLoop, window::Window};

pub mod model;
pub mod scene;
pub mod shaders;

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
    pub memory_allocator: Arc<StandardMemoryAllocator>,
    pub descriptor_set_allocator: Arc<StandardDescriptorSetAllocator>,

    pub gbuffer_pipeline: Arc<GraphicsPipeline>,
    pub simple_mix_pipeline: Arc<GraphicsPipeline>
}

impl VulkanResources {
    pub fn new(event_loop: &EventLoop<()>, window: Arc<Window>) -> Self {
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

        shaders::color::vs::load(device.clone()).unwrap();
        shaders::color::vs::load(device.clone()).unwrap();

        let render_pass = vulkano::ordered_passes_renderpass!(
            device.clone(),
            attachments: {
                final_color: {
                    format: swapchain.image_format(),
                    samples: 1,
                    load_op: Clear,
                    store_op: Store,
                },
                color: {
                    format: Format::A2B10G10R10_UNORM_PACK32,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare,
                },
                normal: {
                    format: Format::R32G32B32A32_SFLOAT,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare,
                },
                position: {
                    format: Format::R32G32B32A32_SFLOAT,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare,
                },
                depth: {
                    format: Format::D16_UNORM,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare,
                }
            },
            passes: [
                {
                    color: [color, normal, position],
                    depth_stencil: {depth},
                    input: []
                },
                {
                    color: [final_color],
                    depth_stencil: {},
                    input: [color, normal, position]
                }
            ]
        ).expect("Couldn't create render pass");

        let gbuffer_pipeline = {
            let subpass = Subpass::from(render_pass.clone(), 0).unwrap();

            let (shader_stages, input_state) = shaders::gbuffers::get_shader(device.clone());

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
                    vertex_input_state: Some(input_state),
                    input_assembly_state: Some(InputAssemblyState::default()),
                    viewport_state: Some(ViewportState::default()),
                    rasterization_state: Some(RasterizationState {
                        // We are inverting the Y axis in the projection matrix to flip the image (see the
                        // command buffer creation for more information). Doing this means that
                        // the winding order is also inverted. For this reason, we are culling the
                        // front face instead of the back face.
                        cull_mode: CullMode::Front,
                        ..Default::default()
                    }),
                    multisample_state: Some(MultisampleState::default()),
                    color_blend_state: Some(ColorBlendState::with_attachment_states(
                        subpass.num_color_attachments(), 
                        ColorBlendAttachmentState::default()
                    )),
                    dynamic_state: [DynamicState::Viewport].into_iter().collect(),
                    subpass: Some(subpass.into()),
                    depth_stencil_state: Some(DepthStencilState {
                        depth: Some(DepthState::simple()),
                        ..Default::default()
                    }),
                    ..GraphicsPipelineCreateInfo::layout(layout)
                }
            ).expect("Failed to create color pipeline")
        };

        let simple_mix_pipeline = {
            let subpass = Subpass::from(render_pass.clone(), 1).unwrap();

            let (shader_stages, input_state) = shaders::simple_mix::get_shader(device.clone());

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
                    vertex_input_state: Some(input_state),
                    input_assembly_state: Some(InputAssemblyState::default()),
                    viewport_state: Some(ViewportState::default()),
                    rasterization_state: Some(RasterizationState {
                        cull_mode: CullMode::Back,
                        ..Default::default()
                    }),
                    multisample_state: Some(MultisampleState::default()),
                    color_blend_state: Some(ColorBlendState::with_attachment_states(
                        subpass.num_color_attachments(), 
                        ColorBlendAttachmentState::default()
                    )),
                    dynamic_state: [DynamicState::Viewport].into_iter().collect(),
                    subpass: Some(subpass.into()),
                    ..GraphicsPipelineCreateInfo::layout(layout)
                }
            ).expect("Failed to create deferred mix pipeline")
        };

        
        let memory_allocator = Arc::new(StandardMemoryAllocator::new_default(device.clone()));

        let descriptor_set_allocator = Arc::new(StandardDescriptorSetAllocator::new(
            device.clone(),
            Default::default()
        ));

        Self {
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
            memory_allocator,
            descriptor_set_allocator,
            gbuffer_pipeline,
            simple_mix_pipeline
        }
    }
}