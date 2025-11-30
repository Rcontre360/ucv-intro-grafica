use std::cmp::{max, min};

use crate::canvas::Canvas;
use crate::core::{Point, RGBA, ShapeCore, ShapeImpl}; // To draw lines for the rectangle

pub struct Rectangle {
    core: ShapeCore,
}

impl ShapeImpl for Rectangle {
    fn new(core: ShapeCore) -> Rectangle {
        Rectangle { core }
    }

    fn get_core_mut(&mut self) -> &mut ShapeCore {
        &mut self.core
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        draw_rectangle(&self.core, canvas);
    }

    fn draw_with_color<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        draw_rectangle(&self.core.copy_with_color(color), canvas);
    }

    fn hit_test(&self, point: Point) -> bool {
        let p1 = self.core.points[0];
        let p2 = self.core.points[1];

        let min_x = min(p1.0, p2.0);
        let max_x = max(p1.0, p2.0);

        let min_y = min(p1.1, p2.1);
        let max_y = max(p1.1, p2.1);

        point.0 >= min_x && point.0 <= max_x && point.1 >= min_y && point.1 <= max_y
    }
}

fn draw_rectangle<'a>(core: &ShapeCore, canvas: &mut Canvas<'a>) {
    let p1 = core.points[0];
    let p2 = core.points[1];

    let min_x = min(p1.0, p2.0);
    let max_x = max(p1.0, p2.0);

    let min_y = min(p1.1, p2.1);
    let max_y = max(p1.1, p2.1);

    for x in min_x..max_x {
        canvas.set_pixel(x, max_y, core.color);
        canvas.set_pixel(x, min_y, core.color);
    }

    for y in min_y..max_y {
        canvas.set_pixel(min_x, y, core.color);
        canvas.set_pixel(max_x, y, core.color);
    }

    if !core.fill_color.is_transparent() {
        for x in (min_x + 1)..(max_x - 1) {
            for y in (min_y + 1)..(max_y - 1) {
                canvas.set_pixel(x, y, core.fill_color);
            }
        }
    }
}
