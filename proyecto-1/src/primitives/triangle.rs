use super::line::draw_line;
use crate::canvas::Canvas;
use crate::core::{Point, ShapeCore, ShapeImpl}; // To draw lines for the triangle

pub struct Triangle {
    core: ShapeCore,
}

impl ShapeImpl for Triangle {
    fn new(core: ShapeCore) -> Triangle {
        Triangle { core }
    }

    fn get_core_mut(&mut self) -> &mut ShapeCore {
        &mut self.core
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        if self.core.points.len() <= 2 {
            draw_line(&self.core, canvas);
        } else {
            if !self.core.fill_color.is_transparent() {
                self.fill_triangle(canvas);
            }

            let core = &self.core;
            let ab = core.points[0..2].to_vec();
            let bc = core.points[1..3].to_vec();
            let ca = vec![core.points[2], core.points[0]];

            draw_line(&self.core_from_points(ab), canvas);
            draw_line(&self.core_from_points(bc), canvas);
            draw_line(&self.core_from_points(ca), canvas);
        }
    }

    fn hit_test(&self, p: Point) -> bool {
        if self.core.points.len() < 3 {
            return false;
        }

        let a = self.core.points[0];
        let b = self.core.points[1];
        let c = self.core.points[2];

        let cp1 = self.edge_side_check(a, b, p);
        let cp2 = self.edge_side_check(b, c, p);
        let cp3 = self.edge_side_check(c, a, p);

        let is_same_side = (cp1 >= 0 && cp2 >= 0 && cp3 >= 0) || (cp1 <= 0 && cp2 <= 0 && cp3 <= 0);

        is_same_side
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
        )
            .into();
        self.fill_top_triangle(canvas, p1, p2, p4);
        self.fill_bottom_triangle(canvas, p2, p4, p3);
    }

    fn fill_top_triangle<'a>(&self, canvas: &mut Canvas<'a>, p1: Point, p2: Point, p3: Point) {
        let m_1 = (p2.0 - p1.0) as f32 / (p2.1 - p1.1) as f32;
        let m_2 = (p3.0 - p1.0) as f32 / (p3.1 - p1.1) as f32;

        let mut cur_x1 = p1.0 as f32;
        let mut cur_x2 = p1.0 as f32;

        for y in p1.1..p2.1 {
            self.draw_horizontal(canvas, cur_x1.round() as i32, cur_x2.round() as i32, y);
            cur_x1 += m_1;
            cur_x2 += m_2;
        }
    }

    fn fill_bottom_triangle<'a>(&self, canvas: &mut Canvas<'a>, p1: Point, p2: Point, p3: Point) {
        let m_1 = (p3.0 - p1.0) as f32 / (p3.1 - p1.1) as f32;
        let m_2 = (p3.0 - p2.0) as f32 / (p3.1 - p2.1) as f32;

        let mut cur_x1 = p3.0 as f32;
        let mut cur_x2 = p3.0 as f32;

        for y in (p1.1..p3.1).rev() {
            self.draw_horizontal(canvas, cur_x1.round() as i32, cur_x2.round() as i32, y);
            cur_x1 -= m_1;
            cur_x2 -= m_2;
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

    fn edge_side_check(&self, a: Point, b: Point, p: Point) -> i64 {
        let ax = a.0 as i64;
        let ay = a.1 as i64;
        let bx = b.0 as i64;
        let by = b.1 as i64;
        let px = p.0 as i64;
        let py = p.1 as i64;

        let ab_x = bx - ax;
        let ab_y = by - ay;

        let ap_x = px - ax;
        let ap_y = py - ay;

        ab_x * ap_y - ab_y * ap_x
    }
}
