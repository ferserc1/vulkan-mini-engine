use std::sync::{Arc, RwLock};

use vulkano::{device::Device, format::Format, image::{ view::ImageView, Image, ImageCreateInfo, ImageType, ImageUsage }, memory::allocator::{AllocationCreateInfo, StandardMemoryAllocator}, pipeline::graphics::viewport::Viewport, render_pass::{Framebuffer, FramebufferCreateInfo, RenderPass}, swapchain::{self, Surface, Swapchain, SwapchainAcquireFuture, SwapchainCreateInfo}, Validated, VulkanError};
use winit::window::Window;


pub struct FramebufferData {
    pub swapchain: Arc<Swapchain>,
    pub images: Arc<RwLock<Vec<Arc<Image>>>>,
    pub render_pass: Arc<RenderPass>,
    pub framebuffers: Option<Vec<Arc<Framebuffer>>>

    // TODO: Add the rest of the elements here, as they are needed
    // depth_image
    // color_gbuffer
    // normal_gbuffer
    // ...
}

impl FramebufferData {
    pub fn new(device: Arc<Device>, surface: Arc<Surface>) -> Self {
        let caps = device
            .physical_device()
            .surface_capabilities(&surface.clone(), Default::default())
            .unwrap();

        let usage = caps.supported_usage_flags;
        let alpha = caps.supported_composite_alpha
            .into_iter()
            .next()
            .unwrap();

        let image_format = device
            .physical_device()
            .surface_formats(&surface.clone(), Default::default())
            .unwrap()[0].0;

        let window = surface.object()
            .unwrap()
            .downcast_ref::<Window>()
            .unwrap();
    
        let image_extent: [u32; 2] = window.inner_size().into();

        let (swapchain, images) = Swapchain::new(
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
        ).unwrap();

        let render_pass = vulkano::single_pass_renderpass!(
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
        ).expect("Failed to create render pass");

        Self {
            swapchain,
            images: Arc::new(RwLock::new(images)),
            render_pass,
            framebuffers: None
        }
    }

    pub fn extent(&self) -> [u32; 2] {
        let images = self.images.read().unwrap();
        let extent = images[0].extent();
        [extent[0], extent[1]]
    }

    pub fn get_framebuffer(&self, index: usize) -> Arc<Framebuffer> {
        self.framebuffers.as_ref().unwrap()[index].clone()
    }
    
    pub fn recreate_swapchain(
        &mut self,
        image_extent: [u32; 2]
    ) {
        let (swapchain, images) = self.swapchain.recreate(SwapchainCreateInfo {
            image_extent,
            ..self.swapchain.create_info()
        }).unwrap();

        self.swapchain = swapchain;
        self.images = Arc::new(RwLock::new(images));
    }

    pub fn update_sizes(
        &mut self,
        allocator: Arc<StandardMemoryAllocator>,
        viewport: &mut Viewport
    ) {
        let images = self.images.read().unwrap();
        let extent = images[0].extent();
        viewport.extent = [extent[0] as f32, extent[1] as f32];

        let depth_buffer_image = Image::new(
            allocator.clone(),
            ImageCreateInfo { 
                image_type: ImageType::Dim2d,
                format: Format::D16_UNORM,
                extent: extent,
                usage: ImageUsage::DEPTH_STENCIL_ATTACHMENT |
                       ImageUsage::TRANSIENT_ATTACHMENT,
                ..Default::default()
            },
            AllocationCreateInfo::default()
        ).expect("Could not create depth buffer image");

        let depth_buffer = ImageView::new_default(depth_buffer_image)
            .expect("Could not create depth buffer image view");

        let framebuffers = self.images.read().unwrap()
            .iter()
            .map(|image| {
                let view = ImageView::new_default(image.clone()).unwrap();
                Framebuffer::new(
                    self.render_pass.clone(),
                    FramebufferCreateInfo {
                        attachments: vec![
                            view,
                            depth_buffer.clone()
                        ],
                        ..Default::default()
                    }
                ).unwrap()
            })
            .collect::<Vec<_>>();

        self.framebuffers = Some(framebuffers);
        // TODO: Update the rest of the image resources ase they are added
    }

    pub fn acquire_next_image(&self) -> (u32, Option<SwapchainAcquireFuture>) {
        match swapchain::acquire_next_image(self.swapchain.clone(), None).map_err(Validated::unwrap) {
            Ok(r) => {
                let (image_index, suboptimal, acquire_future) = r;
                if suboptimal {
                    return (0, None);
                }
                return (image_index, Some(acquire_future));
            },
            Err(VulkanError::OutOfDate) => {
                return (0, None);
            }
            Err(e) => panic!("Failed to acquire next image: {:?}", e)
        }
    }
}
