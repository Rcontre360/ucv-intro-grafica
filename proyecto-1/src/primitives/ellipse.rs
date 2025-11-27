use super::core::{is_transparent, Point, ShapeCore, ShapeImpl, UpdateOp};
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

    fn get_core_mut(&mut self) -> &mut ShapeCore {
        &mut self.core
    }

    fn update(&mut self, op: &UpdateOp) {
        self.update_basic(op);

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
        let (a, b) = (self.a as i64, self.b as i64);
        let mut x: i64 = 0;
        let mut y: i64 = b;

        let mut d: i64 = (4 * b * b) - (4 * a * a * y) + (a * a);
        let mut m_x: i64 = 12 * b * b;
        let mut m_y: i64 = (8 * a * a * y) - (4 * a * a) + (4 * b * b);

        let sum_mx: i64 = 8 * b * b;
        let sum_my: i64 = 8 * a * a;
        let const_d1: i64 = (4 * b * b) + (4 * a * a);
        let draw_fill = !is_transparent(self.core.fill_color);

        self.draw_symmetric(canvas, x, y);
        if draw_fill {
            self.draw_fill_line(canvas, x as i32, y as i32);
        }

        while m_x < m_y {
            if d < 0 {
                d += m_x;
            } else {
                d += m_x - m_y + const_d1;
                y -= 1;
                m_y -= sum_my;
                if draw_fill {
                    self.draw_fill_line(canvas, (x + 1) as i32, y as i32);
                }
            }
            x += 1;
            m_x += sum_mx;
            self.draw_symmetric(canvas, x, y);
        }

        if y <= 0 {
            self.draw_edge_case(canvas, x as i32);
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
            if draw_fill {
                self.draw_fill_line(canvas, (x + 1) as i32, y as i32);
            }
            self.draw_symmetric(canvas, x, y);
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

impl Ellipse {
    pub fn draw_fill_line(&self, canvas: &mut Canvas, x: i32, y: i32) {
        let x_start = self.center.0 - x + 1;
        let x_end = self.center.0 + x - 1;

        for ix in x_start..x_end {
            canvas.set_pixel(ix, self.center.1 + y, self.core.fill_color);
            //edge case when we only use 1st part of algorithm. Avoids double draw
            if self.center.1 - y != self.center.1 + y {
                canvas.set_pixel(ix, self.center.1 - y, self.core.fill_color);
            }
        }
    }

    pub fn draw_symmetric(&self, canvas: &mut Canvas, x: i64, y: i64) {
        let (_x, _y) = (x as i32, y as i32);
        let (cx, cy) = self.center;
        canvas.set_pixel(cx + _x, cy + _y, self.core.color);
        canvas.set_pixel(cx - _x, cy + _y, self.core.color);
        canvas.set_pixel(cx + _x, cy - _y, self.core.color);
        canvas.set_pixel(cx - _x, cy - _y, self.core.color);
    }

    pub fn draw_edge_case(&self, canvas: &mut Canvas, x_drawn: i32) {
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
    }
}
