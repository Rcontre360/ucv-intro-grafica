use crate::core::RGBA;

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
        let index = (y as u32 * self.width + x as u32) as usize * 4;

        if index >= self.buffer.len() {
            return;
        }

        if color[3] < 255 {
            let raw_prev = &self.buffer[index..index + 4];
            let prev_color = RGBA::new(raw_prev[0], raw_prev[1], raw_prev[2], raw_prev[3]);
            let new_color = color + prev_color;

            self.buffer[index..index + 4].copy_from_slice(&new_color);
        } else {
            self.buffer[index..index + 4].copy_from_slice(&color);
        }
    }

    pub fn clear(&mut self) {
        let color = RGBA::new(0, 0, 0, 0);
        for pixel in self.buffer.chunks_exact_mut(4) {
            pixel.copy_from_slice(&color);
        }
    }
}
