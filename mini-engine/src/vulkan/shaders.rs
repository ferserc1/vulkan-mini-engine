
pub mod color {
    pub mod vs {
        vulkano_shaders::shader! {
            ty: "vertex",
            path: "src/shaders/color.vert.glsl"
        }
    }

    pub mod fs {
        vulkano_shaders::shader! {
            ty: "fragment",
            path: "src/shaders/color.frag.glsl"
        }
    }

}

pub mod gbuffers {
    pub mod vs {
        vulkano_shaders::shader! {
            ty: "vertex",
            path: "src/shaders/gbuffer.vert.glsl"
        }
    }

    pub mod fs {
        vulkano_shaders::shader! {
            ty: "fragment",
            path: "src/shaders/gbuffer.frag.glsl"
        }
    }
}

pub mod simple_mix {
    pub mod vs {
        vulkano_shaders::shader! {
            ty: "vertex",
            path: "src/shaders/simple_mix.vert.glsl"
        }
    }

    pub mod fs {
        vulkano_shaders::shader! {
            ty: "fragment",
            path: "src/shaders/simple_mix.frag.glsl"
        }
    }
}