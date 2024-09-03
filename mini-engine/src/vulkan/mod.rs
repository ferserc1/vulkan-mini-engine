use std::sync::{Arc, RwLock};

use vulkano::{command_buffer::allocator::StandardCommandBufferAllocator, descriptor_set::allocator::StandardDescriptorSetAllocator, instance::{Instance, InstanceCreateFlags, InstanceCreateInfo}, memory::allocator::{FreeListAllocator, GenericMemoryAllocator, StandardMemoryAllocator}, pipeline::graphics::viewport::Viewport, swapchain::Surface, Version, VulkanLibrary};
use crate::app::App;

pub mod model;
pub mod scene;


