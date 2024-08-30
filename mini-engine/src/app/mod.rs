use std::{error::Error, sync::Arc};

use vulkano::{instance::InstanceExtensions, swapchain::Surface};
use winit::{event::{ElementState, Event, KeyEvent, WindowEvent}, event_loop::{ControlFlow, EventLoop}, keyboard::{KeyCode, PhysicalKey}, window::Window};

pub struct App {
    window_title: String,
    window_size: (u32, u32),

    event_loop: Option<EventLoop<()>>,
    window: Option<Arc<Window>>,
    vulkan_context: Option<Arc<crate::vulkan::Context>>
}

impl App {
    pub fn new(title: &str, width: u32, height: u32) -> Self {
        Self {
            window_title: title.to_string(),
            window_size: (width, height),
            event_loop: None,
            window: None,
            vulkan_context: None
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

        let vulkan_context = Arc::new(
            crate::vulkan::Context::new(self)
        );
        
        self.vulkan_context = Some(vulkan_context);

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

    pub fn get_vulkan_context(&self) -> Result<Arc<crate::vulkan::Context>, Box<dyn Error>> {
        match &self.vulkan_context {
            Some(context) => Ok(context.clone()),
            None => Err("The application is not initialized".into())
        }
    }

    pub fn run(&mut self) {
        let event_loop = self.event_loop.take().unwrap();
        let window = self.get_window().unwrap().clone();

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

                }

                Event::WindowEvent {
                    event: WindowEvent::RedrawRequested,
                    ..
                } => {

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