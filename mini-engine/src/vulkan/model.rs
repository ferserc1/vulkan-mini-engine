use std::{error::Error, sync::{Arc, RwLock}};

use glam::Mat4;
use vulkano::{buffer::{allocator::{SubbufferAllocator, SubbufferAllocatorCreateInfo}, Buffer, BufferContents, BufferCreateInfo, BufferUsage, Subbuffer}, descriptor_set::DescriptorSet, memory::allocator::{AllocationCreateInfo, MemoryTypeFilter}, pipeline::graphics::vertex_input};

use super::Context;


#[derive(BufferContents, vertex_input::Vertex, Copy, Clone)]
#[repr(C)]
pub struct VertexData {
    #[format(R32G32B32_SFLOAT)]
    pub position: [f32; 3],

    #[format(R32G32B32_SFLOAT)]
    pub color: [f32; 3]
}

pub struct Model {
    pub vertices: Vec<VertexData>,

    pub vertex_buffer: Option<Subbuffer<[VertexData]>>,

    pub matrix_buffer: Option<SubbufferAllocator>,

    pub transform: Mat4,

    pub descriptor_set: Option<Arc<DescriptorSet>>
}

impl Model {
    pub fn new(vertices: Vec<VertexData>) -> Self {
        Self {
            vertices,
            vertex_buffer: None,
            matrix_buffer: None,
            transform: Mat4::IDENTITY,
            descriptor_set: None
        }
    }

    pub fn build(&mut self, vulkan_context: Arc<RwLock<Context>>) -> Result<&Self, Box<dyn Error>>{
        let context = vulkan_context.read().unwrap();
        let allocator = context.memory_allocator.clone();
        let vertex_buffer = Buffer::from_iter(
            allocator.clone(),
            BufferCreateInfo {
                usage: BufferUsage::VERTEX_BUFFER,
                ..Default::default()
            },
            AllocationCreateInfo {
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            },
            self.vertices.iter().cloned()
        )?;

        self.vertex_buffer = Some(vertex_buffer);

        let matrix_buffer = SubbufferAllocator::new(
            allocator.clone(),
            SubbufferAllocatorCreateInfo {
                buffer_usage: BufferUsage::UNIFORM_BUFFER,
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE |
                    MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            }
        );

        self.matrix_buffer = Some(matrix_buffer);

        Ok(self)
    }

    pub fn is_built(&self) -> bool {
        self.vertex_buffer.is_some()
    }

    pub fn update(&mut self) {
        //println!("Updating model");
    }
}