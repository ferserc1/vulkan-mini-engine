use std::{error::Error, sync::{Arc, RwLock}};

use vulkano::{command_buffer::{CommandBufferBeginInfo, CommandBufferLevel, CommandBufferUsage, RecordingCommandBuffer, RenderPassBeginInfo}, instance::InstanceExtensions, swapchain::{self, Surface, SwapchainPresentInfo}, sync::GpuFuture, Validated, VulkanError};
use winit::{event::{ElementState, Event, KeyEvent, WindowEvent}, event_loop::{ControlFlow, EventLoop}, keyboard::{KeyCode, PhysicalKey}, window::Window};

use crate::vulkan::{model::Model, render_system::RenderSystem, Context};

pub struct App {
    window_title: String,
    window_size: (u32, u32),

    event_loop: Option<EventLoop<()>>,
    window: Option<Arc<Window>>,

    vulkan_context: Option<Arc<RwLock<crate::vulkan::Context>>>,
    render_system: Option<Arc<RwLock<RenderSystem>>>,

    pub scene: Vec<Arc<RwLock<Model>>>
}

impl App {
    pub fn new(title: &str, width: u32, height: u32) -> Self {
        Self {
            window_title: title.to_string(),
            window_size: (width, height),
            event_loop: None,
            window: None,
            vulkan_context: None,
            render_system: None,
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

        self.window = Some(window.clone());
        self.event_loop = Some(event_loop);

        let vulkan_context = Context::new(self);
        
        self.render_system = Some(Arc::new(RwLock::new(
            RenderSystem::new(&vulkan_context)
        )));

        self.vulkan_context = Some(Arc::new(RwLock::new(vulkan_context)));

        self
    }

    pub fn get_instance_extensions(&self) -> Result<InstanceExtensions, Box<dyn Error>> {
        match &self.event_loop {
            Some(event_loop) => Ok(Surface::required_extensions(event_loop).expect("Failed to get required instance extensions")),
            None => Err("The application is not initialized".into())
        }
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
        let vulkan_context = self.vulkan_context.take().unwrap();
        let window = self.get_window().unwrap().clone();
        let command_buffer_allocator = vulkan_context.read().unwrap().command_buffer_allocator.clone();
        let render_system = self.render_system.as_ref().unwrap();

        // Initialize scene elements
        self.scene
            .iter()
            .for_each(|model| {
                let mut model = model.write().unwrap();
                model.build(vulkan_context.clone())
                    .expect("Failed to build model");
            });

        let mut previous_frame_end = Some(
            Box::new(vulkano::sync::now(vulkan_context.read().unwrap().device_data.device.clone())) as Box<dyn GpuFuture>
        );
        let mut recreate_swapchain = false;

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
                    if recreate_swapchain {
                        let image_extent: [u32; 2] = window.inner_size().into();
                        vulkan_context.write().unwrap().recreate_swapchain(image_extent);
                    }

                    let framebuffer = &vulkan_context.read().unwrap().framebuffer;
                    let swapchain = vulkan_context.read().unwrap().framebuffer.read().unwrap().swapchain.clone();
                    

                    let (image_index, suboptimal, acquire_future) = {
                        match swapchain::acquire_next_image(swapchain.clone(), None).map_err(Validated::unwrap) {
                            Ok(r) => r,
                            Err(VulkanError::OutOfDate) => {
                                recreate_swapchain = true;
                                return;
                            }
                            Err(e) => panic!("Failed to acquire next image: {:?}", e)
                        }
                    };

                    if suboptimal {
                        recreate_swapchain = true;
                        return;
                    }

                    let clear_values = vec![
                        Some([0.0, 0.0, 0.0, 1.0].into()),
                        Some(1.0.into())
                    ];

                    let device = vulkan_context.read().unwrap().device_data.device.clone();
                    let queue = vulkan_context.read().unwrap().device_data.queue.clone();
                    let mut cmd_buffer_builder = RecordingCommandBuffer::new(
                        command_buffer_allocator.clone(),
                        queue.queue_family_index(),
                        CommandBufferLevel::Primary,
                        CommandBufferBeginInfo {
                            usage: CommandBufferUsage::OneTimeSubmit,
                            ..Default::default()
                        }
                    ).expect("Failed to create command buffer builder");

                    let fb = framebuffer.read().unwrap().get_framebuffer(image_index as usize);
                    let viewport = vulkan_context.read().unwrap().viewport.read().unwrap().clone();
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

                    render_system.read().unwrap()
                        .render(&self.scene, &mut cmd_buffer_builder);

                    cmd_buffer_builder.end_render_pass(Default::default())
                        .unwrap();

                    let command_buffer = cmd_buffer_builder.end().expect("Failed to build command buffer");

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
                            previous_frame_end = Some(Box::new(vulkano::sync::now(device.clone())) as Box<_>);
                        }
                        Err(e) => {
                            println!("Failed to flush future: {:?}", e);
                            previous_frame_end = Some(Box::new(vulkano::sync::now(device.clone())) as Box<_>);
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