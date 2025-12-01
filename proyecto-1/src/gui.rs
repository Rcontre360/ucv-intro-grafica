use egui::{ClippedPrimitive, Context, TexturesDelta, ViewportId};
use egui_wgpu::{Renderer, ScreenDescriptor};
use pixels::{wgpu, PixelsContext};
use winit::event_loop::EventLoopWindowTarget;
use winit::window::Window;

use crate::app_state::{AppState, GUIEvent};
use crate::core::Shape;

/// we have different panels that are drawn, this is a trait (interface) for all of them
trait UiPanel {
    /// draws the panel
    fn draw(&mut self, ui: &mut egui::Ui, ctx: &egui::Context, app_state: &mut AppState);
}

/// top panel which holds undo/redo and file functionality
struct TopControlsPanel;
impl UiPanel for TopControlsPanel {
    fn draw(&mut self, ui: &mut egui::Ui, _ctx: &egui::Context, app_state: &mut AppState) {
        ui.horizontal(|ui| {
            ui.menu_button("File", |ui| {
                if ui.button("Open...").clicked() {
                    app_state.gui_update(GUIEvent::Load);
                    ui.close_menu();
                }
                if ui.button("Save As...").clicked() {
                    app_state.gui_update(GUIEvent::Save);
                    ui.close_menu();
                }
            });

            if ui.button("↩").on_hover_text("Undo").clicked() {
                app_state.gui_update(GUIEvent::Undo);
            }
            if ui.button("↪").on_hover_text("Redo").clicked() {
                app_state.gui_update(GUIEvent::Redo);
            }
        });
        ui.separator();
    }
}

/// panel to select shapes and modify them. Main one
struct ShapePanel;
impl UiPanel for ShapePanel {
    fn draw(&mut self, ui: &mut egui::Ui, _ctx: &egui::Context, app_state: &mut AppState) {
        ui.horizontal(|ui| {
            ui.label("Shape:");
            egui::ComboBox::from_id_source("shape_selector")
                .selected_text(app_state.current.to_string())
                .show_ui(ui, |ui| {
                    let shapes = [
                        Shape::Line,
                        Shape::Ellipse,
                        Shape::Triangle,
                        Shape::Rectangle,
                        Shape::Bezier,
                    ];
                    for shape in shapes.iter() {
                        if ui
                            .selectable_value(&mut app_state.current, *shape, shape.to_string())
                            .changed()
                        {
                            app_state.gui_update(GUIEvent::ShapeType(*shape));
                        }
                    }
                });
        });

        if ui.button("Clear Canvas").clicked() {
            app_state.gui_update(GUIEvent::Clear);
        }
        ui.separator();
    }
}

/// panel for only color related stuf
struct ColorPanel;
impl UiPanel for ColorPanel {
    fn draw(&mut self, ui: &mut egui::Ui, _ctx: &egui::Context, app_state: &mut AppState) {
        ui.heading("COLOR");
        let colors = app_state.get_colors();
        let (mut border, mut fill, mut points, mut background) = (
            colors.0.into(),
            colors.1.into(),
            colors.2.into(),
            colors.4.into(),
        );

        egui::Grid::new("color_grid")
            .num_columns(2)
            .spacing([10.0, 8.0])
            .show(ui, |ui| {
                ui.label("Border");
                if ui
                    .color_edit_button_srgba_unmultiplied(&mut border)
                    .changed()
                {
                    app_state.gui_update(GUIEvent::BorderColor(border.into()));
                }
                ui.end_row();

                ui.label("Fill");
                if ui.color_edit_button_srgba_unmultiplied(&mut fill).changed() {
                    app_state.gui_update(GUIEvent::FillColor(fill.into()));
                }
                ui.end_row();

                ui.label("Points");
                if ui
                    .color_edit_button_srgba_unmultiplied(&mut points)
                    .changed()
                {
                    app_state.gui_update(GUIEvent::PointsColor(points.into()));
                }
                ui.end_row();

                ui.label("Background");
                if ui
                    .color_edit_button_srgba_unmultiplied(&mut background)
                    .changed()
                {
                    app_state.gui_update(GUIEvent::BackgroundColor(background.into()));
                }
                ui.end_row();
            });
        ui.separator();
    }
}

/// panel for depth stuff. only available when selecting a shape
struct DepthPanel;
impl UiPanel for DepthPanel {
    fn draw(&mut self, ui: &mut egui::Ui, ctx: &egui::Context, app_state: &mut AppState) {
        let is_shape_selected = app_state.selected.is_some();
        let depth_header = egui::CollapsingHeader::new("Depth")
            .default_open(false)
            .show(ui, |ui| {
                ui.add_enabled_ui(is_shape_selected, |ui| {
                    ui.horizontal(|ui| {
                        if ui
                            .button("Forward")
                            .on_hover_text("Bring forward by one")
                            .clicked()
                        {
                            app_state.gui_update(GUIEvent::ToFront(false));
                        }
                        if ui
                            .button("Backward")
                            .on_hover_text("Send backward by one")
                            .clicked()
                        {
                            app_state.gui_update(GUIEvent::ToBack(false));
                        }
                    });
                    ui.horizontal(|ui| {
                        if ui
                            .button("To Front")
                            .on_hover_text("Bring to front")
                            .clicked()
                        {
                            app_state.gui_update(GUIEvent::ToFront(true));
                        }
                        if ui.button("To Back").on_hover_text("Send to back").clicked() {
                            app_state.gui_update(GUIEvent::ToBack(true));
                        }
                    });
                });
            });
        if !is_shape_selected && depth_header.header_response.hovered() {
            egui::show_tooltip(ctx, egui::Id::new("depth_tooltip"), |ui| {
                ui.label("Select a shape to enable these options.");
            });
        }
    }
}

/// banel for bezier-specific settings. Only available when selecting a bezier curve
struct BezierPanel;
impl UiPanel for BezierPanel {
    fn draw(&mut self, ui: &mut egui::Ui, ctx: &egui::Context, app_state: &mut AppState) {
        let is_bezier_selected = matches!(app_state.get_selected_shape_type(), Some(Shape::Bezier));
        let bezier_header = egui::CollapsingHeader::new("Bezier Settings")
            .default_open(false)
            .show(ui, |ui| {
                ui.add_enabled_ui(is_bezier_selected, |ui| {
                    if ui.button("Degree Elevate").clicked() {
                        app_state.gui_update(GUIEvent::DegreeElevate);
                    }
                    if ui.button("Subdivide").clicked() {
                        app_state.gui_update(GUIEvent::Subdivide);
                    }
                    let mut subdivision_t = app_state.ui_subdivision_t;
                    if ui
                        .add(egui::Slider::new(&mut subdivision_t, 0.0..=1.0).text("Subdivision"))
                        .changed()
                    {
                        app_state.gui_update(GUIEvent::SubdivisionValue(subdivision_t));
                    }

                    let colors = app_state.get_colors();

                    let mut polygon_color = colors.3.into();
                    ui.label("Polygon Color");
                    if ui
                        .color_edit_button_srgba_unmultiplied(&mut polygon_color)
                        .changed()
                    {
                        app_state.gui_update(GUIEvent::ControlPolygonColor(polygon_color.into()));
                    }
                });
            });

        if !is_bezier_selected && bezier_header.header_response.hovered() {
            egui::show_tooltip(ctx, egui::Id::new("bezier_tooltip"), |ui| {
                ui.label("Select a Bezier curve to enable these options.");
            });
        }
    }
}

/// UI
pub(crate) struct TemplateApp {
    app_state: AppState,
    panels: Vec<Box<dyn UiPanel>>,
}

impl TemplateApp {
    /// called once before the first frame
    pub fn new() -> Self {
        Self {
            app_state: AppState::new(),
            panels: vec![
                Box::new(TopControlsPanel),
                Box::new(ShapePanel),
                Box::new(ColorPanel),
                Box::new(DepthPanel),
                Box::new(BezierPanel),
            ],
        }
    }

    /// called each time the UI is updated
    pub fn update(&mut self, ctx: &egui::Context) {
        egui::SidePanel::left("side_panel")
            .default_width(250.0)
            .show(ctx, |ui| {
                for panel in &mut self.panels {
                    panel.draw(ui, ctx, &mut self.app_state);
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

/// manages all state required for rendering egui with the pixels library
/// this was taken from an internet example on how to use egui with pixels
/// see https://github.com/parasyte/pixels/tree/main/examples/minimal-egui
pub(crate) struct Framework {
    egui_ctx: Context,
    egui_state: egui_winit::State,
    screen_descriptor: ScreenDescriptor,
    renderer: Renderer,
    paint_jobs: Vec<ClippedPrimitive>,
    textures: TexturesDelta,

    gui: TemplateApp,
}

impl Framework {
    pub(crate) fn new<T>(
        event_loop: &EventLoopWindowTarget<T>,
        width: u32,
        height: u32,
        scale_factor: f32,
        pixels: &pixels::Pixels,
    ) -> Self {
        let max_texture_size = pixels.device().limits().max_texture_dimension_2d as usize;

        let egui_ctx = Context::default();

        let mut style = (*egui_ctx.style()).clone();
        style.text_styles = [
            (
                egui::TextStyle::Heading,
                egui::FontId::new(22.0, egui::FontFamily::Proportional),
            ),
            (
                egui::TextStyle::Body,
                egui::FontId::new(18.0, egui::FontFamily::Proportional),
            ),
            (
                egui::TextStyle::Button,
                egui::FontId::new(18.0, egui::FontFamily::Proportional),
            ),
            (
                egui::TextStyle::Small,
                egui::FontId::new(14.0, egui::FontFamily::Proportional),
            ),
        ]
        .into();
        egui_ctx.set_style(style);

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

    /// handle input events from the window manager
    pub(crate) fn handle_event(&mut self, window: &Window, event: &winit::event::WindowEvent) {
        let _ = self.egui_state.on_window_event(window, event);
    }

    /// resize egui
    pub(crate) fn resize(&mut self, width: u32, height: u32) {
        if width > 0 && height > 0 {
            self.screen_descriptor.size_in_pixels = [width, height];
        }
    }

    /// update scaling factor
    pub(crate) fn scale_factor(&mut self, scale_factor: f64) {
        self.screen_descriptor.pixels_per_point = scale_factor as f32;
    }

    /// prepare egui
    pub(crate) fn prepare(&mut self, window: &Window) {
        let raw_input = self.egui_state.take_egui_input(window);
        let output = self.egui_ctx.run(raw_input, |egui_ctx| {
            self.gui.update(egui_ctx);
        });

        self.textures.append(output.textures_delta);
        self.egui_state
            .handle_platform_output(window, output.platform_output);
        self.paint_jobs = self
            .egui_ctx
            .tessellate(output.shapes, self.screen_descriptor.pixels_per_point);
    }

    /// render egui
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

    /// returns a mutable reference to the application state
    pub(crate) fn get_state(&mut self) -> &mut AppState {
        &mut self.gui.app_state
    }

    /// returns "true" if the GUI wants to capture pointer input..
    pub(crate) fn wants_pointer_input(&self) -> bool {
        self.egui_ctx.wants_pointer_input() || self.egui_ctx.is_pointer_over_area()
    }
}
