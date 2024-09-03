use std::{error::Error, sync::{Arc, RwLock}};

use glam::Mat4;
use vulkano::{buffer::{allocator::{SubbufferAllocator, SubbufferAllocatorCreateInfo}, Buffer, BufferContents, BufferCreateInfo, BufferUsage, Subbuffer}, command_buffer::RecordingCommandBuffer, descriptor_set::DescriptorSet, memory::allocator::{AllocationCreateInfo, MemoryTypeFilter, StandardMemoryAllocator}, pipeline::{graphics::vertex_input, GraphicsPipeline, Pipeline, PipelineBindPoint}};



#[derive(BufferContents, vertex_input::Vertex, Copy, Clone)]
#[repr(C)]
pub struct VertexData {
    #[format(R32G32B32_SFLOAT)]
    pub position: [f32; 3],

    #[format(R32G32B32_SFLOAT)]
    pub normal: [f32; 3]
}

pub struct Model {
    pub vertices: Vec<VertexData>,

    pub indices: Vec<u32>,

    pub vertex_buffer: Option<Subbuffer<[VertexData]>>,

    pub index_buffer: Option<Subbuffer<[u32]>>,

    pub matrix_buffer: Option<SubbufferAllocator>,

    pub transform: Mat4,

    pub descriptor_set: Option<Arc<DescriptorSet>>,

    on_update_callback: Option<Box<dyn Fn(f32, Mat4) -> Mat4>>
}

impl Model {
    pub fn new(vertices: Arc<Vec<VertexData>>, indices: Arc<Vec<u32>>) -> Self {
        Self {
            vertices: vertices.iter().cloned().collect(),
            indices: indices.iter().cloned().collect(),
            vertex_buffer: None,
            index_buffer: None,
            matrix_buffer: None,
            transform: Mat4::IDENTITY,
            descriptor_set: None,
            on_update_callback: None
        }
    }

    pub fn on_update(&mut self, callback: Box<dyn Fn(f32, Mat4) -> Mat4>) {
        self.on_update_callback = Some(callback);
    }

    pub fn build(&mut self, allocator: Arc<StandardMemoryAllocator>) -> Result<&Self, Box<dyn Error>>{
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

        let index_buffer = Buffer::from_iter(
            allocator.clone(),
            BufferCreateInfo {
                usage: BufferUsage::INDEX_BUFFER,
                ..Default::default()
            },
            AllocationCreateInfo {
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            },
            self.indices.iter().cloned()
        )?;

        self.index_buffer = Some(index_buffer);

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

    pub fn update(&mut self, delta: f32) {
        if let Some(callback) = self.on_update_callback.as_ref() {
            self.transform = callback(delta, self.transform);
        }
    }

    pub fn draw(&self, cmd_buffer: &mut RecordingCommandBuffer, pipeline: Arc<GraphicsPipeline>) {
        let vertex_buffer = self.vertex_buffer.as_ref().unwrap();
        let index_buffer = self.index_buffer.as_ref().unwrap();

        if let Some(descriptor_set) = self.descriptor_set.as_ref() {
            cmd_buffer.bind_descriptor_sets(
                PipelineBindPoint::Graphics,
                pipeline.clone().layout().clone(),
                0,
                descriptor_set.clone()
            ).unwrap();
        }

        cmd_buffer.bind_vertex_buffers(0, vertex_buffer.clone()).unwrap();
        cmd_buffer.bind_index_buffer(index_buffer.clone()).unwrap();
        unsafe {
            cmd_buffer.draw_indexed(
                self.indices.len() as u32,
                1,
                0,
                0,
                0
            ).unwrap();
        }
    }
}