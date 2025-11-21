//! A simple paint application that uses `pixels` for the canvas and `egui` for the UI.

use egui_wgpu::ScreenDescriptor;
use log::error;
use pixels::{wgpu, Error, Pixels, SurfaceTexture};
use rand::Rng;
use winit::{
    dpi::LogicalSize,
    event::Event,
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

const WIDTH: u32 = 400;
const HEIGHT: u32 = 300;

/// Representation of the application state. In this example, a single button state.
struct World {
    should_draw: bool,
}

fn main() -> Result<(), Error> {
    env_logger::init();
    let event_loop = EventLoop::new().unwrap();
    let mut input = WinitInputHelper::new();
    let window = {
        let size = LogicalSize::new(WIDTH as f64, HEIGHT as f64);
        WindowBuilder::new()
            .with_title("Paint App")
            .with_inner_size(size)
            .with_min_inner_size(size)
            .build(&event_loop)
            .unwrap()
    };

    let (mut pixels, mut framework) = {
        let window_size = window.inner_size();
        let scale_factor = window.scale_factor() as f32;
        let surface_texture = SurfaceTexture::new(window_size.width, window_size.height, &window);
        let pixels = Pixels::new(WIDTH, HEIGHT, surface_texture)?;
        let framework = Framework::new(
            &event_loop,
            window_size.width,
            window_size.height,
            scale_factor,
            &pixels,
        );

        (pixels, framework)
    };

    let mut world = World { should_draw: false };

    event_loop.run(move |event, _, control_flow| {
        // Handle input events
        if input.update(&event) {
            // Close events
            if input.key_pressed(VirtualKeyCode::Escape) || input.close_requested() {
                *control_flow = ControlFlow::Exit;
                return;
            }

            // Update the scale factor
            if let Some(scale_factor) = input.scale_factor() {
                framework.scale_factor(scale_factor);
            }

            // Resize the window
            if let Some(size) = input.window_resized() {
                if size.width > 0 && size.height > 0 {
                    pixels.resize_surface(size.width, size.height).unwrap();
                    framework.resize(size.width, size.height);
                }
            }

            // Update internal state and request a redraw
            world.update();
            window.request_redraw();
        }

        match event {
            Event::WindowEvent { event, .. } => {
                // Update egui inputs
                framework.handle_event(&event);
            }
            // Redraw the application
            Event::RedrawRequested(_) => {
                // Draw the world
                world.draw(pixels.frame_mut());

                // Prepare egui
                framework.prepare(&window, &mut world);

                // Render everything
                let render_result = pixels.render_with(|encoder, render_target, context| {
                    // Render the world texture
                    context.scaling_renderer.render(encoder, render_target);

                    // Render egui
                    framework.render(encoder, render_target, &context.device, &context.queue);

                    Ok(())
                });

                // Handle rendering errors
                if let Err(err) = render_result {
                    error!("pixels.render_with() failed: {}", err);
                    *control_flow = ControlFlow::Exit;
                }
            }
            _ => (),
        }
    });
}

impl World {
    /// Update the world state.
    fn update(&mut self) {
        // This is where you would update your application logic.
        // For this example, the state is updated in the UI.
    }

    /// Draw the world to the frame buffer.
    fn draw(&self, frame: &mut [u8]) {
        if self.should_draw {
            let mut rng = rand::thread_rng();
            for _ in 0..10 {
                let x = rng.gen_range(0..WIDTH) as usize;
                let y = rng.gen_range(0..HEIGHT) as usize;
                let i = (y * WIDTH as usize + x) * 4;
                let color: [u8; 3] = rng.gen();

                if i + 3 < frame.len() {
                    frame[i] = color[0]; // R
                    frame[i + 1] = color[1]; // G
                    frame[i + 2] = color[2]; // B
                    frame[i + 3] = 255; // A
                }
            }
        }
    }
}

// --- Framework boilerplate ---

/// A wrapper for all the framework state.
struct Framework {
    egui_ctx: egui::Context,
    egui_state: egui_winit::State,
    screen_descriptor: ScreenDescriptor,
    renderer: egui_wgpu::Renderer,
    paint_jobs: Vec<egui::ClippedPrimitive>,
    textures: egui::TexturesDelta,
}

impl Framework {
    /// Create egui.
    fn new<T>(
        event_loop: &EventLoop<T>,
        width: u32,
        height: u32,
        scale_factor: f32,
        pixels: &Pixels,
    ) -> Self {
        let max_texture_size = pixels.device().limits().max_texture_dimension_2d as usize;

        let egui_ctx = egui::Context::default();
        let mut egui_state = egui_winit::State::new(event_loop);
        egui_state.set_max_texture_side(max_texture_size);
        egui_state.set_pixels_per_point(scale_factor);
        let screen_descriptor = ScreenDescriptor {
            size_in_pixels: [width, height],
            pixels_per_point: scale_factor,
        };
        let renderer = egui_wgpu::Renderer::new(pixels.device(), pixels.render_texture_format(), None, 1);
        let textures = egui::TexturesDelta::default();

        Self {
            egui_ctx,
            egui_state,
            screen_descriptor,
            renderer,
            paint_jobs: Vec::new(),
            textures,
        }
    }

    /// Handle input events from winit.
    fn handle_event(&mut self, event: &winit::event::WindowEvent) {
        let _ = self.egui_state.on_event(&self.egui_ctx, event);
    }

    /// Resize egui.
    fn resize(&mut self, width: u32, height: u32) {
        if width > 0 && height > 0 {
            self.screen_descriptor.size_in_pixels = [width, height];
        }
    }

    /// Update scaling factor.
    fn scale_factor(&mut self, scale_factor: f64) {
        self.screen_descriptor.pixels_per_point = scale_factor as f32;
    }

    /// Prepare egui.
    fn prepare(&mut self, window: &winit::window::Window, world: &mut World) {
        // Run the egui frame and create the UI.
        let raw_input = self.egui_state.take_egui_input(window);
        let output = self.egui_ctx.run(raw_input, |egui_ctx| {
            // Create a simple window.
            egui::Window::new("Controls").show(egui_ctx, |ui| {
                if ui.button("Draw 10 Random Pixels").clicked() {
                    world.should_draw = true;
                } else {
                    world.should_draw = false;
                }
            });
        });

        self.textures.append(output.textures_delta);
        self.egui_state
            .handle_platform_output(window, &self.egui_ctx, output.platform_output);
        self.paint_jobs = self.egui_ctx.tessellate(output.shapes);
    }

    /// Render egui.
    fn render(
        &mut self,
        encoder: &mut wgpu::CommandEncoder,
        render_target: &wgpu::TextureView,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
    ) {
        // Upload all resources to the GPU.
        self.renderer
            .update_buffers(device, queue, encoder, &self.paint_jobs, &self.screen_descriptor);
        
        // Handle texture updates
        for (id, image_delta) in &self.textures.set {
            self.renderer.update_texture(device, queue, *id, image_delta);
        }
        for id in &self.textures.free {
            self.renderer.free_texture(id);
        }

        // Render egui with a render pass.
        {
            let mut rpass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("egui"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: render_target,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Load,
                        store: true,
                    },
                })],
                depth_stencil_attachment: None,
            });

            self.renderer.render(&mut rpass, &self.paint_jobs, &self.screen_descriptor);
        }

        // Cleanup
        self.textures = egui::TexturesDelta::default();
    }
}