use glam::Mat4;


pub struct Camera {
    pub fov: f32,
    pub near: f32,
    pub far: f32,
    pub transform: Mat4
}

impl Camera {
    pub fn new(fov: f32, near: f32, far: f32) -> Self {
        Self {
            fov,
            near,
            far,
            transform: Mat4::IDENTITY
        }
    }
}

