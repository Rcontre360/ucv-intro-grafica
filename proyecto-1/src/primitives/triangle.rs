use super::core::{is_transparent, Point, ShapeCore, ShapeImpl, UpdateOp};
use super::line::{draw_line, Line};
use crate::canvas::Canvas; // To draw lines for the triangle

pub struct Triangle {
    core: ShapeCore,
}

impl ShapeImpl for Triangle {
    fn new(core: ShapeCore) -> Triangle {
        Triangle { core }
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
            UpdateOp::AddControlPoint { point } => {
                self.core.points.push(*point);
            }
            _ => {}
        }
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        if self.core.points.len() <= 2 {
            draw_line(&self.core, canvas);
        } else {
            let core = &self.core;
            let ab = core.points[0..2].to_vec();
            let bc = core.points[1..3].to_vec();
            let ca = vec![core.points[2], core.points[0]];

            draw_line(&self.core_from_points(ab), canvas);
            draw_line(&self.core_from_points(bc), canvas);
            draw_line(&self.core_from_points(ca), canvas);

            if !is_transparent(self.core.fill_color) {
                self.fill_triangle(canvas);
            }
        }
    }

    fn hit_test(&self, _point: Point) -> bool {
        false
    }
}

impl Triangle {
    fn core_from_points(&self, points: Vec<Point>) -> ShapeCore {
        let mut core = self.core.clone();
        core.points = points;
        core
    }

    fn fill_triangle<'a>(&self, canvas: &mut Canvas<'a>) {
        let mut points = self.core.points.clone();
        // sort by y
        points.sort_by(|a, b| a.1.cmp(&b.1));

        let p1 = points[0];
        let p2 = points[1];
        let p3 = points[2];

        if p1.1 == p3.1 {
            return; //flat
        }

        let p4 = (
            (p1.0 + ((p2.1 - p1.1) as f32 / (p3.1 - p1.1) as f32 * (p3.0 - p1.0) as f32) as i32),
            p2.1,
        );
        self.fill_top_triangle(canvas, p1, p2, p4);
        self.fill_bottom_triangle(canvas, p2, p4, p3);
    }

    fn fill_top_triangle<'a>(&self, canvas: &mut Canvas<'a>, p1: Point, p2: Point, p3: Point) {
        let inv_slope1 = (p2.0 - p1.0) as f32 / (p2.1 - p1.1) as f32;
        let inv_slope2 = (p3.0 - p1.0) as f32 / (p3.1 - p1.1) as f32;

        let mut cur_x1 = p1.0 as f32;
        let mut cur_x2 = p1.0 as f32;

        for y in p1.1..p2.1 {
            self.draw_horizontal(canvas, cur_x1.round() as i32, cur_x2.round() as i32, y);
            cur_x1 += inv_slope1;
            cur_x2 += inv_slope2;
        }
    }

    fn fill_bottom_triangle<'a>(&self, canvas: &mut Canvas<'a>, p1: Point, p2: Point, p3: Point) {
        let inv_slope1 = (p3.0 - p1.0) as f32 / (p3.1 - p1.1) as f32;
        let inv_slope2 = (p3.0 - p2.0) as f32 / (p3.1 - p2.1) as f32;

        let mut cur_x1 = p3.0 as f32;
        let mut cur_x2 = p3.0 as f32;

        for y in (p1.1..p3.1).rev() {
            self.draw_horizontal(canvas, cur_x1.round() as i32, cur_x2.round() as i32, y);
            cur_x1 -= inv_slope1;
            cur_x2 -= inv_slope2;
        }
    }

    fn draw_horizontal<'a>(&self, canvas: &mut Canvas<'a>, mut x1: i32, mut x2: i32, y: i32) {
        if x1 > x2 {
            std::mem::swap(&mut x1, &mut x2);
        }
        for x in x1..x2 {
            canvas.set_pixel(x, y, self.core.fill_color);
        }
    }
}
