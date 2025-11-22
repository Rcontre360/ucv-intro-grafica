use crate::primitives::core::{rgba, RGBA};

// A lifetime it's about how long a reference to data is valid.
// I made Canvas generic over a lifetime to tell the compiler that
// the lifetime of Canvas cannot exceed the lifetime of the buffer
pub struct Canvas<'a> {
    buffer: &'a mut [u8],
    width: u32,
    height: u32,
}

impl<'a> Canvas<'a> {
    pub fn new(buffer: &'a mut [u8], width: u32, height: u32) -> Self {
        Canvas {
            buffer,
            width,
            height,
        }
    }

    // notice we will have left down corner as origin (0,0)
    pub fn set_pixel(&mut self, x: i32, y: i32, color: RGBA) {
        if x >= self.width as i32 || x < 0 || y >= self.height as i32 || y < 0 {
            return;
        }

        let index = (y as u32 * self.width + x as u32) as usize * 4;

        // to represent data for a single pixel we need 4 spaces (rgba)
        if index + 4 <= self.buffer.len() {
            self.buffer[index..index + 4].copy_from_slice(&color);
        }
    }

    pub fn clear(&mut self) {
        let color = rgba(0, 0, 0, 0);
        for pixel in self.buffer.chunks_exact_mut(4) {
            pixel.copy_from_slice(&color);
        }
    }
}
