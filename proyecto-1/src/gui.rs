use egui::{ClippedPrimitive, Context, TexturesDelta, ViewportId};
use egui_wgpu::{Renderer, ScreenDescriptor};
use pixels::{wgpu, PixelsContext};
use winit::event_loop::EventLoopWindowTarget;
use winit::window::Window;

use crate::primitives::core::{Shape, ShapeImpl};
use crate::state::State;

/// Example application state. A real application will need a lot more state than this.
pub(crate) struct TemplateApp {
    state: State,
}

impl TemplateApp {
    /// Called once before the first frame.
    pub fn new() -> Self {
        Self {
            state: State::new(),
        }
    }

    /// Called each time the UI is updated.
    pub fn update(&mut self, ctx: &egui::Context) {
        egui::SidePanel::left("side_panel").show(ctx, |ui| {
            ui.heading("Shapes");

            ui.separator();

            if ui.button("1. Line").clicked() {
                self.state.current = Shape::Line;
            }
            if ui.button("2. Ellipse").clicked() {
                self.state.current = Shape::Ellipse;
            }
            if ui.button("3. Triangle").clicked() {
                self.state.current = Shape::Triangle;
            }
            if ui.button("4. Rectangle").clicked() {
                self.state.current = Shape::Rectangle;
            }
            if ui.button("5. Bezier").clicked() {
                self.state.current = Shape::Bezier;
            }

            ui.separator();

            if self.state.selected.is_some() {
                if ui.button("Degree++").clicked() {
                    self.state.subdivide_selected();
                }
            }

            ui.heading("Color (RGB)");
            ui.horizontal(|ui| {
                ui.label("R:");
                ui.add(
                    egui::DragValue::new(&mut self.state.color[0])
                        .speed(1.0)
                        .clamp_range(0..=255),
                );
            });
            ui.horizontal(|ui| {
                ui.label("G:");
                ui.add(
                    egui::DragValue::new(&mut self.state.color[1])
                        .speed(1.0)
                        .clamp_range(0..=255),
                );
            });
            ui.horizontal(|ui| {
                ui.label("B:");
                ui.add(
                    egui::DragValue::new(&mut self.state.color[2])
                        .speed(1.0)
                        .clamp_range(0..=255),
                );
            });

            ui.with_layout(egui::Layout::bottom_up(egui::Align::LEFT), |ui| {
                ui.add(egui::github_link_file!(
                    "https://github.com/emilk/egui/blob/master/",
                    "Source code."
                ));
            });
        });
    }
}

/// Manages all state required for rendering egui over `Pixels`.
pub(crate) struct Framework {
    // State for egui.
    egui_ctx: Context,
    egui_state: egui_winit::State,
    screen_descriptor: ScreenDescriptor,
    renderer: Renderer,
    paint_jobs: Vec<ClippedPrimitive>,
    textures: TexturesDelta,

    // State for the GUI
    gui: TemplateApp,
}

impl Framework {
    /// Create egui.
    pub(crate) fn new<T>(
        event_loop: &EventLoopWindowTarget<T>,
        width: u32,
        height: u32,
        scale_factor: f32,
        pixels: &pixels::Pixels,
    ) -> Self {
        let max_texture_size = pixels.device().limits().max_texture_dimension_2d as usize;

        let egui_ctx = Context::default();
        let egui_state = egui_winit::State::new(
            egui_ctx.clone(),
            ViewportId::ROOT,
            event_loop,
            Some(scale_factor),
            Some(max_texture_size),
        );
        let screen_descriptor = ScreenDescriptor {
            size_in_pixels: [width, height],
            pixels_per_point: scale_factor,
        };
        let renderer = Renderer::new(pixels.device(), pixels.render_texture_format(), None, 1);
        let textures = TexturesDelta::default();
        let gui = TemplateApp::new();

        Self {
            egui_ctx,
            egui_state,
            screen_descriptor,
            renderer,
            paint_jobs: Vec::new(),
            textures,
            gui,
        }
    }

    /// Handle input events from the window manager.
    pub(crate) fn handle_event(&mut self, window: &Window, event: &winit::event::WindowEvent) {
        let _ = self.egui_state.on_window_event(window, event);
    }

    /// Resize egui.
    pub(crate) fn resize(&mut self, width: u32, height: u32) {
        if width > 0 && height > 0 {
            self.screen_descriptor.size_in_pixels = [width, height];
        }
    }

    /// Update scaling factor.
    pub(crate) fn scale_factor(&mut self, scale_factor: f64) {
        self.screen_descriptor.pixels_per_point = scale_factor as f32;
    }

    /// Prepare egui.
    pub(crate) fn prepare(&mut self, window: &Window) {
        // Run the egui frame and create all paint jobs to prepare for rendering.
        let raw_input = self.egui_state.take_egui_input(window);
        let output = self.egui_ctx.run(raw_input, |egui_ctx| {
            // Draw the demo application.
            self.gui.update(egui_ctx);
        });

        self.textures.append(output.textures_delta);
        self.egui_state
            .handle_platform_output(window, output.platform_output);
        self.paint_jobs = self
            .egui_ctx
            .tessellate(output.shapes, self.screen_descriptor.pixels_per_point);
    }

    /// Render egui.
    pub(crate) fn render(
        &mut self,
        encoder: &mut wgpu::CommandEncoder,
        render_target: &wgpu::TextureView,
        context: &PixelsContext,
    ) {
        // Upload all resources to the GPU.
        for (id, image_delta) in &self.textures.set {
            self.renderer
                .update_texture(&context.device, &context.queue, *id, image_delta);
        }
        self.renderer.update_buffers(
            &context.device,
            &context.queue,
            encoder,
            &self.paint_jobs,
            &self.screen_descriptor,
        );

        // Render egui with WGPU
        {
            let mut rpass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("egui"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: render_target,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Load,
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });

            self.renderer
                .render(&mut rpass, &self.paint_jobs, &self.screen_descriptor);
        }

        // Cleanup
        let textures = std::mem::take(&mut self.textures);
        for id in &textures.free {
            self.renderer.free_texture(id);
        }
    }

    pub(crate) fn get_state(&mut self) -> &mut State {
        &mut self.gui.state
    }

    pub(crate) fn wants_pointer_input(&self) -> bool {
        self.egui_ctx.wants_pointer_input()
    }
}
