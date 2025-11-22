use crate::canvas::Canvas;
use super::core::{Point, ShapeCore, ShapeImpl};

pub struct Ellipse {
    core: ShapeCore,
}

impl ShapeImpl for Ellipse {
    fn new(core: ShapeCore) -> Ellipse {
        Ellipse { core }
    }

    fn update(&mut self, end: Point) {
        // For an ellipse, the two points define the bounding box.
        // The center is the midpoint, and the radii are half the width/height.
        self.core.points[1] = end;
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        let points = &self.core.points;
        let (x0, y0) = points[0];
        let (x1, y1) = points[1];

        let xc = (x0 + x1) / 2;
        let yc = (y0 + y1) / 2;
        let rx = ((x1 - x0) / 2).abs();
        let ry = ((y1 - y0) / 2).abs();

        let color = self.core.color;

        // Midpoint Ellipse Algorithm
        let mut x = 0;
        let mut y = ry;
        let mut p1 = (ry * ry) - (rx * rx * ry) + (rx * rx / 4);

        // Region 1
        while (2 * ry * ry * x) < (2 * rx * rx * y) {
            canvas.set_pixel(xc + x, yc + y, color);
            canvas.set_pixel(xc - x, yc + y, color);
            canvas.set_pixel(xc + x, yc - y, color);
            canvas.set_pixel(xc - x, yc - y, color);

            x += 1;
            if p1 < 0 {
                p1 = p1 + (2 * ry * ry * x) + (ry * ry);
            } else {
                y -= 1;
                p1 = p1 + (2 * ry * ry * x) - (2 * rx * rx * y) + (ry * ry);
            }
        }

        // Region 2
        let mut p2 = (ry * ry * (x + 1 / 2) * (x + 1 / 2)) + (rx * rx * (y - 1) * (y - 1)) - (rx * rx * ry * ry);
        while y >= 0 {
            canvas.set_pixel(xc + x, yc + y, color);
            canvas.set_pixel(xc - x, yc + y, color);
            canvas.set_pixel(xc + x, yc - y, color);
            canvas.set_pixel(xc - x, yc - y, color);

            y -= 1;
            if p2 > 0 {
                p2 = p2 - (2 * rx * rx * y) + (rx * rx);
            } else {
                x += 1;
                p2 = p2 + (2 * ry * ry * x) - (2 * rx * rx * y) + (rx * rx);
            }
        }
    }
}
