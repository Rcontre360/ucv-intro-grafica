use crate::canvas::Canvas;
use super::core::{Point, ShapeCore, ShapeImpl};

pub struct Circle {
    core: ShapeCore,
}

impl ShapeImpl for Circle {
    fn new(core: ShapeCore) -> Circle {
        Circle { core }
    }

    fn update(&mut self, end: Point) {
        // For a circle, the first point is the center, and the second point defines the radius.
        self.core.points[1] = end;
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        let points = &self.core.points;
        let (x0, y0) = points[0];
        let (x1, y1) = points[1];

        let center_x = x0;
        let center_y = y0;
        let radius = (((x1 - x0).pow(2) + (y1 - y0).pow(2)) as f32).sqrt() as i32;

        let color = self.core.color;

        // Midpoint Circle Algorithm
        let mut x = radius;
        let mut y = 0;
        let mut p = 1 - radius;

        while x >= y {
            canvas.set_pixel(center_x + x, center_y + y, color);
            canvas.set_pixel(center_x + y, center_y + x, color);
            canvas.set_pixel(center_x - y, center_y + x, color);
            canvas.set_pixel(center_x - x, center_y + y, color);
            canvas.set_pixel(center_x - x, center_y - y, color);
            canvas.set_pixel(center_x - y, center_y - x, color);
            canvas.set_pixel(center_x + y, center_y - x, color);
            canvas.set_pixel(center_x + x, center_y - y, color);

            y += 1;
            if p < 0 {
                p = p + 2 * y + 1;
            } else {
                x -= 1;
                p = p + 2 * y - 2 * x + 1;
            }
        }
    }
}
