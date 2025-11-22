use super::core::{Point, ShapeCore, ShapeImpl, RGBA};

pub struct Line {
    core: ShapeCore,
}

impl ShapeImpl for Line {
    fn new(core: ShapeCore) -> Line {
        Line { core }
    }

    fn update(&mut self, end: Point) {
        self.core.points[1] = end;
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw(&self, buffer: &mut [u8]) {
        // Ensure we have at least two control points (start and end)
        if self.core.points.len() < 2 {
            return;
        }

        let (x0, y0) = self.core.points[0];
        let (x1, y1) = self.core.points[1];
        let color = self.core.color;

        let dx = x1 as f32 - x0 as f32;
        let dy = y1 as f32 - y0 as f32;

        // The number of steps is the largest absolute difference in coordinates.
        // This ensures the line moves one pixel at a time along the major axis.
        let steps = dx.abs().max(dy.abs()).round() as u32;

        if steps == 0 {
            // If start and end are the same, just plot the single point
            set_pixel(buffer, x0, y0, color);
            return;
        }

        // Calculate the step increment for each axis for one major step
        let x_increment = dx / steps as f32;
        let y_increment = dy / steps as f32;

        let mut x_current = x0 as f32;
        let mut y_current = y0 as f32;

        // Iterate through all steps, plotting the rounded (x, y) coordinates
        for _ in 0..=steps {
            // Plot the current point, rounding to the nearest integer pixel
            let plot_x = x_current.round() as u32;
            let plot_y = y_current.round() as u32;

            set_pixel(buffer, plot_x, plot_y, color);

            // Move to the next point using floating-point accumulation
            x_current += x_increment;
            y_current += y_increment;
        }
    }
}

/// Helper function to safely set a single pixel in the frame buffer.
fn set_pixel(frame: &mut [u8], x: u32, y: u32, color: RGBA) {
    // Calculate the index in the 1D buffer: (y * width + x) * 4 bytes per pixel
    let index = (y * 640 + x) as usize * 4;

    // Safety check for the end of the buffer
    if index + 4 <= frame.len() {
        frame[index..index + 4].copy_from_slice(&color);
    }
}
