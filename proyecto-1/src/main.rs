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
use winit::window::CursorIcon;
use winit::window::WindowBuilder;
use winit_input_helper::WinitInputHelper;

use crate::app_state::MouseEvent;
use crate::gui::Framework;

mod app_state;
mod canvas;
mod core;
mod draw_state;
mod gui;
mod primitives;

// initial width and height
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
            // this is the mouse cursor. We update it from the state and then load it on the window
            let mut cursor_icon = CursorIcon::Default;

            if input.key_pressed(KeyCode::Escape) || input.close_requested() {
                elwt.exit();
                return;
            }

            if input.key_pressed(KeyCode::Enter) {
                state.keyboard_update(KeyCode::Enter);
            }

            if input.key_pressed(KeyCode::Delete) {
                state.keyboard_update(KeyCode::Delete);
            }

            if input.key_pressed(KeyCode::Backspace) {
                state.keyboard_update(KeyCode::Backspace);
            }

            if input.key_pressed(KeyCode::ShiftLeft) {
                state.keyboard_update(KeyCode::ShiftLeft);
            }

            if input.key_pressed(KeyCode::ShiftRight) {
                state.keyboard_update(KeyCode::ShiftRight);
            }

            // mouse events on GUI. Avoids drawing while selecting gui buttons
            if !is_gui {
                // here we update the app state with different mouse events
                // here we also use cursor_icon, if needed the state decides if we are draggin and
                // need the cursor dragging logo
                //mouse is moved on the ui
                if let Some((x, y)) = input.cursor() {
                    cursor_icon = state.mouse_update(MouseEvent::Move, 0, (x, y).into());
                }

                if input.mouse_pressed(0) {
                    let (x, y) = input.cursor().unwrap();
                    cursor_icon = state.mouse_update(MouseEvent::Click, 0, (x, y).into());
                }

                if input.mouse_pressed(1) {
                    let (x, y) = input.cursor().unwrap();
                    cursor_icon = state.mouse_update(MouseEvent::Click, 1, (x, y).into());
                }

                if input.mouse_held(0) {
                    let (x, y) = input.cursor().unwrap();
                    cursor_icon = state.mouse_update(MouseEvent::PressDrag, 0, (x, y).into());
                }

                if input.mouse_released(0) {
                    let (x, y) = input.cursor().unwrap();
                    cursor_icon = state.mouse_update(MouseEvent::Release, 0, (x, y).into());
                }
            }
            // Update the scale factor for the window size change
            if let Some(scale_factor) = input.scale_factor() {
                framework.scale_factor(scale_factor);
            }

            // udpate the cursor on the window
            window.set_cursor_icon(cursor_icon);
            window.request_redraw();
        }

        // usually the GUI handler (input variable) above would facilitate these events for us
        // but the GUI package above doesnt work well for these events. so we do it here instead
        match event {
            Event::WindowEvent { event, .. } => {
                framework.handle_event(&window, &event);
                match event {
                    // important for resizing
                    WindowEvent::Resized(size) => {
                        if size.width > 0 && size.height > 0 {
                            // on resizing we need to resize the SURFACE AND THE BUFFER
                            // the surface is the window and the buffer is what we modify/render
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
                    // this runs every time a redraw is requested
                    // if we dont do anything the event loop runs but a redraw is not done
                    WindowEvent::RedrawRequested => {
                        // we initialize a new canvas with each run
                        // the canvas has a lifetime bounded to the buffer. So when this loop ends
                        // the canvas is removed from memory. Check the Canvas definition for more
                        let size = window.inner_size();
                        // pixels (the library we're using) gives us the buffer through
                        // "frame_mut"
                        let mut canvas = Canvas::new(pixels.frame_mut(), size.width);

                        //drawing with that buffer
                        framework.get_state().draw(&mut canvas);
                        framework.prepare(&window);

                        //rendering result
                        let render_result =
                            pixels.render_with(|encoder, render_target, context| {
                                context.scaling_renderer.render(encoder, render_target);
                                framework.render(encoder, render_target, context);

                                Ok(())
                            });

                        // simple error handling. A possible improvement would be to return a
                        // possible error on "draw" from the state and return it here if applicable
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
