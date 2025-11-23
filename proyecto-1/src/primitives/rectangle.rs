use std::cmp::{max, min};

use super::core::{Point, ShapeCore, ShapeImpl};
use crate::canvas::Canvas; // To draw lines for the rectangle

pub struct Rectangle {
    core: ShapeCore,
}

impl ShapeImpl for Rectangle {
    fn new(core: ShapeCore) -> Rectangle {
        Rectangle { core }
    }

    fn update(&mut self, end: Point) {
        self.core.points[1] = end;
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        let p1 = self.core.points[0];
        let p2 = self.core.points[1];

        let min_x = min(p1.0, p2.0);
        let max_x = max(p1.0, p2.0);

        let min_y = min(p1.1, p2.1);
        let max_y = max(p1.1, p2.1);

        for x in min_x..max_x {
            canvas.set_pixel(x, max_y, self.core.color);
            canvas.set_pixel(x, min_y, self.core.color);
        }

        for y in min_y..max_y {
            canvas.set_pixel(min_x, y, self.core.color);
            canvas.set_pixel(max_x, y, self.core.color);
        }
    }
}
