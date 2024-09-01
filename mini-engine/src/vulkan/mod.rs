use std::sync::{Arc, RwLock};

use vulkano::{command_buffer::allocator::StandardCommandBufferAllocator, descriptor_set::allocator::StandardDescriptorSetAllocator, instance::{Instance, InstanceCreateFlags, InstanceCreateInfo}, memory::allocator::{FreeListAllocator, GenericMemoryAllocator, StandardMemoryAllocator}, pipeline::graphics::viewport::Viewport, swapchain::Surface, Version, VulkanLibrary};
use crate::app::App;

pub mod device;
pub mod framebuffer;
pub mod model;
pub mod render_system;
pub mod scene;

pub struct Context {
    pub instance: Arc<Instance>,
    pub surface: Arc<Surface>,
    pub device_data: device::DeviceData,
    pub framebuffer: Arc<RwLock<framebuffer::FramebufferData>>,
    pub command_buffer_allocator: Arc<StandardCommandBufferAllocator>,
    pub memory_allocator: Arc<GenericMemoryAllocator<FreeListAllocator>>,
    pub descriptor_set_allocator: Arc<StandardDescriptorSetAllocator>,
    pub viewport: Arc<RwLock<Viewport>>
}

impl Context {
    pub fn new(app: &App) -> Self {
        let window = app.get_window().unwrap();

        let library = VulkanLibrary::new().unwrap();
        let extensions = app.get_instance_extensions().unwrap();
    
        let instance = Instance::new(
            library.clone(),
            InstanceCreateInfo {
                enabled_extensions: extensions,
                flags: InstanceCreateFlags::ENUMERATE_PORTABILITY,
                max_api_version: Some(Version::V1_3),
                ..Default::default()
            }).expect("Could not create Vulkan instance");

        let surface = Surface::from_window(instance.clone(), window.clone())
            .expect("Failed to create window surface");

        let device_data = device::DeviceData::new(instance.clone(), surface.clone());

        let mut framebuffer = framebuffer::FramebufferData::new(
            device_data.device.clone(),
            surface.clone()
        );

        let command_buffer_allocator = Arc::new(
            StandardCommandBufferAllocator::new(device_data.device.clone(), Default::default())
        );

        let memory_allocator = Arc::new(
            StandardMemoryAllocator::new_default(device_data.device.clone())
        );

        let descriptor_set_allocator = Arc::new(
            StandardDescriptorSetAllocator::new(
                device_data.device.clone(),
                Default::default()
            )
        );

        let mut viewport = Viewport {
            offset: [0.0, 0.0],
            extent: [0.0, 0.0],
            depth_range: 0.0..=1.0
        };

        framebuffer.update_sizes(memory_allocator.clone(), &mut viewport);

        Self {
            instance,
            surface,
            device_data,
            framebuffer: Arc::new(RwLock::new(framebuffer)),
            command_buffer_allocator,
            memory_allocator,
            descriptor_set_allocator,
            viewport: Arc::new(RwLock::new(viewport))
        }
    }

    pub fn recreate_swapchain(&self, image_extent: [u32; 2]) {
        let mut framebuffer = self.framebuffer.write().unwrap();
        let mut viewport = self.viewport.write().unwrap();

        framebuffer.recreate_swapchain(image_extent);

        framebuffer.update_sizes(self.memory_allocator.clone(), &mut viewport);
    }
}
