use winit::{event::{ElementState, Event, KeyEvent, WindowEvent}, event_loop::ControlFlow, keyboard::{KeyCode, PhysicalKey}};

pub struct App {
    window_title: String,
    window_size: (u32, u32)
}

impl App {
    pub fn new(title: &str, width: u32, height: u32) -> Self {
        Self {
            window_title: title.to_string(),
            window_size: (width, height)
        }
    }

    pub fn run(&self) {
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
        
        event_loop.run(move |event, target_window| {
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