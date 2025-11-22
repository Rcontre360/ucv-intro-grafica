use crate::canvas::Canvas;

use super::core::{Point, ShapeCore, ShapeImpl};

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

    //TODO fix drawing error saw in first homework
    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        let points = &self.core.points;
        let a = points[0];
        let b = points[1];

        let mut dx = (b.0 - a.0) as i32;
        let mut dy = (b.1 - a.1) as i32;
        let mut d = dx - 2 * dy;
        let x_inc = if dx < 0 { -1 as i32 } else { 1 };
        let y_inc = if dy < 0 { -1 as i32 } else { 1 };

        if dx < 0 {
            dx *= -1;
        }
        if dy < 0 {
            dy *= -1;
        }

        let run_on_x = dx >= dy;
        let inc_e = -2 * (if run_on_x { dy } else { dx });
        let inc_ne = 2 * (dx - dy) * (if run_on_x { 1 } else { -1 });

        let mut x = a.0 as i32;
        let mut y = a.1 as i32;
        canvas.set_pixel(x, y, self.core.color);

        if run_on_x {
            while x > b.0 || x < b.0 {
                if d <= 0 {
                    d += inc_ne;
                    y += y_inc;
                } else {
                    d += inc_e;
                }

                // we increase x or y depending on the case (dx >= dy)
                x += x_inc;
                canvas.set_pixel(x, y, self.core.color);
            }
        } else {
            while x > b.0 || x < b.0 {
                if d <= 0 {
                    d += inc_ne;
                    x += x_inc;
                } else {
                    d += inc_e;
                }

                // we increase x or y depending on the case (dx >= dy)
                y += y_inc;
                canvas.set_pixel(x, y, self.core.color);
            }
        }
    }
}
