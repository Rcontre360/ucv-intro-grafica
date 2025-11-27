use crate::canvas::Canvas;

use super::core::{Point, ShapeCore, ShapeImpl, UpdateOp};

pub struct Line {
    core: ShapeCore,
}

impl ShapeImpl for Line {
    fn new(core: ShapeCore) -> Line {
        Line { core }
    }

    fn update(&mut self, op: &UpdateOp) {
        match op {
            UpdateOp::Move { delta } => {
                for p in self.core.points.iter_mut() {
                    p.0 += delta.0;
                    p.1 += delta.1;
                }
            }
            UpdateOp::ControlPoint { index, point } => {
                if *index < self.core.points.len() {
                    self.core.points[*index] = *point;
                }
            }
            _ => {}
        }
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    //TODO fix drawing error saw in first homework
    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        draw_line(&self.core, canvas);
    }

    fn hit_test(&self, point: Point) -> bool {
        let p1 = self.core.points[0];
        let p2 = self.core.points[1];
        let p = point;

        let dx = (p2.0 - p1.0) as f32;
        let dy = (p2.1 - p1.1) as f32;

        if dx == 0.0 && dy == 0.0 {
            let dist = ((p.0 - p1.0).pow(2) + (p.1 - p1.1).pow(2)) as f32;
            return dist.sqrt() < 5.0;
        }

        let t = ((p.0 - p1.0) as f32 * dx + (p.1 - p1.1) as f32 * dy) / (dx * dx + dy * dy);

        let closest_point = if t < 0.0 {
            p1
        } else if t > 1.0 {
            p2
        } else {
            (p1.0 + (t * dx) as i32, p1.1 + (t * dy) as i32)
        };

        let dist = ((p.0 - closest_point.0).pow(2) + (p.1 - closest_point.1).pow(2)) as f32;
        dist.sqrt() < 5.0
    }
}

// exposed for usage on triangles
pub fn draw_line<'a>(core: &ShapeCore, canvas: &mut Canvas<'a>) {
    let points = &core.points;
    let a = points[0];
    let b = points[1];

    let mut dx = (b.0 - a.0) as i32;
    let mut dy = (b.1 - a.1) as i32;
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

    let mut d = (dx - 2 * dy) * if run_on_x { 1 } else { -1 };
    let mut x = a.0 as i32;
    let mut y = a.1 as i32;
    canvas.set_pixel(x, y, core.color);

    if run_on_x {
        while x > b.0 || x < b.0 {
            if d <= 0 {
                d += inc_ne;
                y += y_inc;
            } else {
                d += inc_e;
            }

            x += x_inc;
            canvas.set_pixel(x, y, core.color);
        }
    } else {
        while y > b.1 || y < b.1 {
            if d <= 0 {
                d += inc_ne;
                x += x_inc;
            } else {
                d += inc_e;
            }

            y += y_inc;
            canvas.set_pixel(x, y, core.color);
        }
    }
}
