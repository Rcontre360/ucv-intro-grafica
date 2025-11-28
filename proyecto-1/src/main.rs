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
use crate::state::MouseEvent;

mod canvas;
mod core;
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
            let is_gui = framework.wants_pointer_input();
            let state = framework.get_state();

            if input.key_pressed(KeyCode::Escape) || input.close_requested() {
                elwt.exit();
                return;
            }

            if input.key_pressed(KeyCode::Enter) {
                state.keyboard_update(KeyCode::Enter);
            }

            // mouse events on GUI dont matter
            if !is_gui {
                //mouse is moved on the ui
                if let Some((x, y)) = input.cursor() {
                    state.mouse_update(MouseEvent::Move, 0, (x, y).into());
                }

                if input.mouse_pressed(0) {
                    let (x, y) = input.cursor().unwrap();
                    state.mouse_update(MouseEvent::Click, 0, (x, y).into());
                }

                if input.mouse_pressed(1) {
                    let (x, y) = input.cursor().unwrap();
                    state.mouse_update(MouseEvent::Click, 1, (x, y).into());
                }

                if input.mouse_held(0) {
                    let (x, y) = input.cursor().unwrap();
                    state.mouse_update(MouseEvent::PressDrag, 0, (x, y).into());
                }

                if input.mouse_released(0) {
                    let (x, y) = input.cursor().unwrap();
                    state.mouse_update(MouseEvent::Release, 0, (x, y).into());
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
