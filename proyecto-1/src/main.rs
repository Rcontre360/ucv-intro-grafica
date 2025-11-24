#![deny(clippy::all)]
#![forbid(unsafe_code)]

use canvas::Canvas;
use error_iter::ErrorIter as _;
use log::error;
use pixels::{Error, Pixels, SurfaceTexture};
use winit::dpi::LogicalSize;
use winit::event::{Event, WindowEvent};
use winit::event_loop::EventLoop;
use winit::keyboard::KeyCode;
use winit::window::WindowBuilder;
use winit_input_helper::WinitInputHelper;

use crate::gui::Framework;

mod canvas;
mod gui;
mod primitives;
mod state;

const WIDTH: u32 = 640;
const HEIGHT: u32 = 480;

fn main() -> Result<(), Error> {
    env_logger::init();
    let event_loop = EventLoop::new().unwrap();
    let mut input = WinitInputHelper::new();
    let window = {
        let size = LogicalSize::new(WIDTH as f64, HEIGHT as f64);
        WindowBuilder::new()
            .with_title("Hello Pixels + egui")
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

    let res = event_loop.run(|event, elwt| {
        if input.update(&event) {
            if input.key_pressed(KeyCode::Escape) || input.close_requested() {
                elwt.exit();
                return;
            }

            if input.key_pressed(KeyCode::Enter) {
                framework.get_state().subdivide_selected();
            }

            if input.mouse_pressed(0) {
                if !framework.wants_pointer_input() {
                    if let Some((x, y)) = input.cursor() {
                        let _x = x.round() as i32;
                        let _y = y.round() as i32;
                        if let Some(control_point_index) =
                            framework.get_state().hit_test_control_points((_x, _y))
                        {
                            framework.get_state().dragging = Some(control_point_index);
                        } else {
                            framework.get_state().handle_selection((_x, _y));
                            framework.get_state().start_current_shape((_x, _y));
                        }
                    }
                }
            }

            if input.mouse_held(0) {
                if let Some((x, y)) = input.cursor() {
                    let _x = x.round() as i32;
                    let _y = y.round() as i32;
                    if framework.get_state().dragging.is_some() {
                        framework.get_state().update_dragged_control_point((_x, _y));
                    } else {
                        framework
                            .get_state()
                            .update_current_shape((x.round() as i32, y.round() as i32));
                    }
                }
            }

            if input.mouse_released(0) {
                if let Some((x, y)) = input.cursor() {
                    if framework.get_state().dragging.is_some() {
                        framework.get_state().dragging = None;
                    } else {
                        framework
                            .get_state()
                            .end_current_shape((x.round() as i32, y.round() as i32));
                    }
                }
            }

            // Update the scale factor
            if let Some(scale_factor) = input.scale_factor() {
                framework.scale_factor(scale_factor);
            }

            window.request_redraw();
        }

        match event {
            Event::WindowEvent { event, .. } => {
                framework.handle_event(&window, &event);
                match event {
                    WindowEvent::Resized(size) => {
                        if size.width > 0 && size.height > 0 {
                            if let Err(err) = pixels.resize_surface(size.width, size.height) {
                                log_error("pixels.resize_surface", err);
                            }
                            if let Err(err) = pixels.resize_buffer(size.width, size.height) {
                                log_error("pixels.resize_buffer", err);
                            }
                            framework.resize(size.width, size.height);
                            window.request_redraw();
                        }
                    }
                    WindowEvent::RedrawRequested => {
                        let size = window.inner_size();
                        let mut canvas = Canvas::new(pixels.frame_mut(), size.width, size.height);
                        framework.get_state().draw(&mut canvas);

                        // Prepare egui
                        framework.prepare(&window);

                        let render_result =
                            pixels.render_with(|encoder, render_target, context| {
                                context.scaling_renderer.render(encoder, render_target);
                                framework.render(encoder, render_target, context);

                                Ok(())
                            });

                        if let Err(err) = render_result {
                            log_error("pixels.render", err);
                            elwt.exit();
                        }
                    }
                    _ => (),
                }
            }
            _ => (),
        }
    });
    res.map_err(|e| Error::UserDefined(Box::new(e)))
}

fn log_error<E: std::error::Error + 'static>(method_name: &str, err: E) {
    error!("{method_name}() failed: {err}");
    for source in err.sources().skip(1) {
        error!("  Caused by: {source}");
    }
}
