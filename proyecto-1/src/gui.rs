use egui::{ClippedPrimitive, Context, TexturesDelta, ViewportId};
use egui_wgpu::{Renderer, ScreenDescriptor};
use pixels::{wgpu, PixelsContext};
use winit::event_loop::EventLoopWindowTarget;
use winit::window::Window;

use crate::app_state::{AppState, GUIEvent};
use crate::core::Shape;

/// Example application state. A real application will need a lot more state than this.
pub(crate) struct TemplateApp {
    app_state: AppState,
}

impl TemplateApp {
    /// Called once before the first frame.
    pub fn new() -> Self {
        Self {
            app_state: AppState::new(),
        }
    }

    /// Called each time the UI is updated.
    pub fn update(&mut self, ctx: &egui::Context) {
        // 1. Font size
        let mut style = (*ctx.style()).clone();
        style.text_styles = [
            (egui::TextStyle::Heading, egui::FontId::new(22.0, egui::FontFamily::Proportional)),
            (egui::TextStyle::Body, egui::FontId::new(18.0, egui::FontFamily::Proportional)),
            (egui::TextStyle::Button, egui::FontId::new(18.0, egui::FontFamily::Proportional)),
            (egui::TextStyle::Small, egui::FontId::new(14.0, egui::FontFamily::Proportional)),
        ].into();
        ctx.set_style(style);

        egui::SidePanel::left("side_panel")
        .default_width(250.0)
        .show(ctx, |ui| {
            // 2. Top-left controls (File, Undo, Redo)
            ui.horizontal(|ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("Open...").clicked() {
                        self.app_state.gui_update(GUIEvent::Load);
                        ui.close_menu();
                    }
                    if ui.button("Save As...").clicked() {
                        self.app_state.gui_update(GUIEvent::Save);
                        ui.close_menu();
                    }
                });

                if ui.button("↩").on_hover_text("Undo").clicked() {
                    self.app_state.gui_update(GUIEvent::Undo);
                }
                if ui.button("↪").on_hover_text("Redo").clicked() {
                    self.app_state.gui_update(GUIEvent::Redo);
                }
            });

            ui.separator();

            // 3. Shape Selection
            ui.horizontal(|ui| {
                ui.label("Shape:");
                egui::ComboBox::from_id_source("shape_selector")
                    .selected_text(self.app_state.current.to_string())
                    .show_ui(ui, |ui| {
                        let shapes = [Shape::Line, Shape::Ellipse, Shape::Triangle, Shape::Rectangle, Shape::Bezier];
                        for shape in shapes.iter() {
                            if ui.selectable_value(&mut self.app_state.current, *shape, shape.to_string()).changed() {
                                self.app_state.gui_update(GUIEvent::ShapeType(*shape));
                            }
                        }
                    });
            });

            if ui.button("Clear Canvas").clicked() {
                self.app_state.gui_update(GUIEvent::Clear);
            }

            ui.separator();

            // 4. Color Section (Grid layout)
            ui.heading("COLOR");
            let colors = self.app_state.get_colors();
            let (mut border, mut fill, mut points, mut background) = (
                colors.0.into(),
                colors.1.into(),
                colors.2.into(),
                colors.3.into(),
            );

            egui::Grid::new("color_grid")
                .num_columns(2)
                .spacing([10.0, 8.0])
                .show(ui, |ui| {
                    ui.label("Border");
                    if ui.color_edit_button_srgba_unmultiplied(&mut border).changed() {
                        self.app_state.gui_update(GUIEvent::BorderColor(border.into()));
                    }
                    ui.end_row();

                    ui.label("Fill");
                    if ui.color_edit_button_srgba_unmultiplied(&mut fill).changed() {
                        self.app_state.gui_update(GUIEvent::FillColor(fill.into()));
                    }
                    ui.end_row();

                    ui.label("Points");
                    if ui.color_edit_button_srgba_unmultiplied(&mut points).changed() {
                        self.app_state.gui_update(GUIEvent::PointsColor(points.into()));
                    }
                    ui.end_row();

                    ui.label("Background");
                    if ui.color_edit_button_srgba_unmultiplied(&mut background).changed() {
                        self.app_state.gui_update(GUIEvent::BackgroundColor(background.into()));
                    }
                    ui.end_row();
                });


            ui.separator();

            // 5. Depth Section (CollapsingHeader)
            let is_shape_selected = self.app_state.selected.is_some();
            let depth_header = egui::CollapsingHeader::new("Depth")
                .default_open(false)
                .show(ui, |ui| {
                     ui.add_enabled_ui(is_shape_selected, |ui| {
                        ui.horizontal(|ui| {
                            if ui.button("Forward").on_hover_text("Bring forward by one").clicked() {
                                self.app_state.gui_update(GUIEvent::ToFront(false));
                            }
                            if ui.button("Backward").on_hover_text("Send backward by one").clicked() {
                                self.app_state.gui_update(GUIEvent::ToBack(false));
                            }
                        });
                        ui.horizontal(|ui| {
                            if ui.button("To Front").on_hover_text("Bring to front").clicked() {
                                self.app_state.gui_update(GUIEvent::ToFront(true));
                            }
                            if ui.button("To Back").on_hover_text("Send to back").clicked() {
                                self.app_state.gui_update(GUIEvent::ToBack(true));
                            }
                        });
                    });
                });
            if !is_shape_selected && depth_header.header_response.hovered() {
                egui::show_tooltip(ctx, egui::Id::new("depth_tooltip"), |ui| {
                   ui.label("Select a shape to enable these options.");
               });
           }


            // 6. Bezier Settings
            let is_bezier_selected = matches!(self.app_state.get_selected_shape_type(), Some(Shape::Bezier));
            let bezier_header = egui::CollapsingHeader::new("Bezier Settings")
                .default_open(false)
                .show(ui, |ui| {
                    ui.add_enabled_ui(is_bezier_selected, |ui| {
                        if ui.button("Degree Elevate").clicked() {
                            self.app_state.gui_update(GUIEvent::DegreeElevate);
                        }
                        if ui.button("Subdivide").clicked() {
                            self.app_state.gui_update(GUIEvent::Subdivide);
                        }
                        let mut subdivision_t = self.app_state.ui_subdivision_t;
                        if ui.add(egui::Slider::new(&mut subdivision_t, 0.0..=1.0).text("Subdivision")).changed() {
                            self.app_state.gui_update(GUIEvent::SubdivisionValue(subdivision_t));
                        }
                    });
                });

            if !is_bezier_selected && bezier_header.header_response.hovered() {
                 egui::show_tooltip(ctx, egui::Id::new("bezier_tooltip"), |ui| {
                    ui.label("Select a Bezier curve to enable these options.");
                });
            }


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

    pub(crate) fn get_state(&mut self) -> &mut AppState {
        &mut self.gui.app_state
    }

    pub(crate) fn wants_pointer_input(&self) -> bool {
        self.egui_ctx.wants_pointer_input() || self.egui_ctx.is_pointer_over_area()
    }
}
