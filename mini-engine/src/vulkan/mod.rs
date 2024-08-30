use std::sync::Arc;

use vulkano::{instance::{Instance, InstanceCreateFlags, InstanceCreateInfo}, swapchain::Surface, Version, VulkanLibrary};
use crate::app::App;

pub mod device;

pub struct Context {
    pub instance: Arc<Instance>,
    pub surface: Arc<Surface>,
    pub device_data: Arc<device::DeviceData>
    
    // TODO: Swap chain
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

            let device_data = Arc::new(device::DeviceData::new(instance.clone(), surface.clone()));

        Self {
            instance,
            surface,
            device_data
        }
    }
}
