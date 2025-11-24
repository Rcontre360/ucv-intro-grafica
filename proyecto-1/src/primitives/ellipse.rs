use super::core::{Point, ShapeCore, ShapeImpl, UpdateOp};
use crate::canvas::Canvas;

pub struct Ellipse {
    core: ShapeCore,
    center: Point,
    a: i32,
    b: i32,
}

impl ShapeImpl for Ellipse {
    fn new(core: ShapeCore) -> Ellipse {
        let (x0, y0) = core.points[0];
        let (x1, y1) = core.points[1];

        Ellipse {
            core,
            center: ((x0 + x1) / 2, (y0 + y1) / 2),
            a: ((x1 - x0) / 2).abs() as i32,
            b: ((y1 - y0) / 2).abs() as i32,
        }
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

        let (x0, y0) = self.core.points[0];
        let (x1, y1) = self.core.points[1];

        self.center = ((x0 + x1) / 2, (y0 + y1) / 2);
        self.a = ((x1 - x0) / 2).abs() as i32;
        self.b = ((y1 - y0) / 2).abs() as i32;
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        let draw_symmetric = |canvas: &mut Canvas, x, y| {
            //simple casting
            let (_x, _y) = (x as i32, y as i32);
            let (cx, cy) = self.center;
            canvas.set_pixel(cx + _x, cy + _y, self.core.color);
            canvas.set_pixel(cx - _x, cy + _y, self.core.color);
            canvas.set_pixel(cx + _x, cy - _y, self.core.color);
            canvas.set_pixel(cx - _x, cy - _y, self.core.color);
        };

        let draw_edge_case = |canvas: &mut Canvas, x_drawn| {
            let mut x = self.center.0 - self.a;
            let end = self.center.0 - x_drawn;
            while x < end {
                canvas.set_pixel(x, self.center.1, self.core.color);
                x += 1;
            }

            x = self.center.0 + x_drawn;
            let end = self.center.0 + self.a;
            while x < end {
                canvas.set_pixel(x, self.center.1, self.core.color);
                x += 1;
            }
        };

        let (a, b) = (self.a as i64, self.b as i64);
        let mut x: i64 = 0;
        let mut y: i64 = b;

        let mut d: i64 = (4 * b * b) - (4 * a * a * y) + (a * a);
        let mut m_x: i64 = 12 * b * b;
        let mut m_y: i64 = (8 * a * a * y) - (4 * a * a) + (4 * b * b);

        let sum_mx: i64 = 8 * b * b;
        let sum_my: i64 = 8 * a * a;
        let const_d1: i64 = (4 * b * b) + (4 * a * a);

        draw_symmetric(canvas, x, y);

        while m_x < m_y {
            if d < 0 {
                d += m_x;
            } else {
                d += m_x - m_y + const_d1;
                y -= 1;
                m_y -= sum_my;
            }
            x += 1;
            m_x += sum_mx;
            draw_symmetric(canvas, x, y);
        }

        if y <= 0 {
            draw_edge_case(canvas, x as i32);
        }

        let aux2 = (8 * a * a) + (4 * b * b);
        let const_d2 = 8 * a * a;
        d = b * b * (4 * x * x + 4 * x + 1) + a * a * (4 * y * y - 8 * y + 4) - 4 * a * a * b * b;

        m_y -= aux2;
        m_x -= aux2;

        while y > 0 {
            if d < 0 {
                d += m_x - m_y + const_d2;
                x += 1;
                m_x += sum_mx;
            } else {
                d -= m_y;
            }

            y -= 1;
            m_y -= sum_my;
            draw_symmetric(canvas, x, y);
        }
    }

    fn hit_test(&self, point: Point) -> bool {
        let (px, py) = point;
        let (cx, cy) = self.center;
        let (a, b) = (self.a as f32, self.b as f32);

        let dx = (px - cx) as f32;
        let dy = (py - cy) as f32;

        (dx * dx) / (a * a) + (dy * dy) / (b * b) <= 1.0
    }
}
