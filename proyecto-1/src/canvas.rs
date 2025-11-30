use crate::core::RGBA;

// See Canvas<'a>. "'a'" its not a type, its a lifetime. A lifetime it's about how long a reference to data is valid.
// I made Canvas generic over a lifetime to tell the compiler that
// the lifetime of Canvas cannot exceed the lifetime of the buffer. Once the buffer gets out of the
// program scope is removed from memory with the canvas
// see more about lifetimes on https://doc.rust-lang.org/rust-by-example/scope/lifetime.html
// https://doc.rust-lang.org/rust-by-example/scope/lifetime/explicit.html

/// Canvas is an object that facilitates the drawing of the buffer. Holds the app buffer and allows
/// its modification by exposing methods
pub struct Canvas<'a> {
    /// the buffer that is drawn
    buffer: &'a mut [u8],
    /// the length of the buffer
    length: u32,
}

// implementation of methods for canvas
impl<'a> Canvas<'a> {
    /// initializes a new Canvas, receives the buffer and its length
    pub fn new(buffer: &'a mut [u8], length: u32) -> Self {
        Canvas { buffer, length }
    }

    /// sets a pixel on the buffer. The left upper corner is the origin, x and y are checked to be
    /// inside the buffer boundaries.
    pub fn set_pixel(&mut self, x: i32, y: i32, color: RGBA) {
        let index = (y as u32 * self.length + x as u32) as usize * 4;

        if index >= self.buffer.len() {
            return;
        }

        // we only perform the alpha calculation if the current color alpha is bellow max
        if color[3] < 255 {
            // here we get the previous color on that position
            let raw_prev = &self.buffer[index..index + 4];
            let prev_color = RGBA::new(raw_prev[0], raw_prev[1], raw_prev[2], raw_prev[3]);
            if prev_color[3] > 0 {
                println!("OVERLAPPING");
            }
            // the + operation between colors is defined at ./src/core/rgba.rs
            let new_color = color + prev_color;

            // we paste the new_color which is has 4 u8 entries
            self.buffer[index..index + 4].copy_from_slice(&new_color);
        } else {
            self.buffer[index..index + 4].copy_from_slice(&color);
        }
    }

    /// draws the specified color on the whole buffer
    pub fn clear(&mut self, color: RGBA) {
        for pixel in self.buffer.chunks_exact_mut(4) {
            pixel.copy_from_slice(&color);
        }
    }
}
