/// Representation of the application state. In this example, a box will bounce around the screen.
pub struct State {}

impl State {
    pub fn new() -> Self {
        Self {}
    }

    pub fn update(&mut self) {}

    pub fn draw_with_mouse(&mut self, x: u32, y: u32) {}

    pub fn draw(&self, frame: &mut [u8]) {}
}
