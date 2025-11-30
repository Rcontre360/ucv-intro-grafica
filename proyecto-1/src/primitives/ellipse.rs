use crate::canvas::Canvas;
use crate::core::{Point, ShapeCore, ShapeImpl, UpdateOp, RGBA};

pub struct Ellipse {
    core: ShapeCore,
}

impl ShapeImpl for Ellipse {
    fn new(core: ShapeCore) -> Ellipse {
        Ellipse { core }
    }

    fn get_core_mut(&mut self) -> &mut ShapeCore {
        &mut self.core
    }

    fn update(&mut self, op: &UpdateOp) {
        self.update_basic(op);
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        draw_ellipse(&self.core, canvas);
    }

    fn draw_with_color<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        draw_ellipse(&self.core.copy_with_color(color), canvas);
    }

    fn hit_test(&self, point: Point) -> bool {
        let p1 = self.core.points[0];
        let p2 = self.core.points[1];
        let center: Point = ((p1.0 + p2.0) / 2, (p1.1 + p2.1) / 2).into();
        let a = ((p2.0 - p1.0) / 2).abs() as f32;
        let b = ((p2.1 - p1.1) / 2).abs() as f32;

        let dx = (point.0 - center.0) as f32;
        let dy = (point.1 - center.1) as f32;

        (dx * dx) / (a * a) + (dy * dy) / (b * b) <= 1.0
    }
}

fn draw_ellipse(core: &ShapeCore, canvas: &mut Canvas) {
    let p1 = core.points[0];
    let p2 = core.points[1];
    let center: Point = ((p1.0 + p2.0) / 2, (p1.1 + p2.1) / 2).into();
    let a = ((p2.0 - p1.0) / 2).abs() as i64;
    let b = ((p2.1 - p1.1) / 2).abs() as i64;

    let mut x: i64 = 0;
    let mut y: i64 = b;

    let mut d: i64 = (4 * b * b) - (4 * a * a * y) + (a * a);
    let mut m_x: i64 = 12 * b * b;
    let mut m_y: i64 = (8 * a * a * y) - (4 * a * a) + (4 * b * b);

    let sum_mx: i64 = 8 * b * b;
    let sum_my: i64 = 8 * a * a;
    let const_d1: i64 = (4 * b * b) + (4 * a * a);
    let draw_fill = !core.fill_color.is_transparent();

    draw_symmetric(canvas, center, x, y, core.color);
    if draw_fill {
        draw_fill_line(canvas, center, x as i32, y as i32, core.fill_color);
    }

    while m_x < m_y {
        if d < 0 {
            d += m_x;
        } else {
            d += m_x - m_y + const_d1;
            y -= 1;
            m_y -= sum_my;
            if draw_fill {
                draw_fill_line(canvas, center, (x + 1) as i32, y as i32, core.fill_color);
            }
        }
        x += 1;
        m_x += sum_mx;
        draw_symmetric(canvas, center, x, y, core.color);
    }

    if y <= 0 {
        draw_edge_case(canvas, center, a as i32, x as i32, core.color);
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
            draw_fill_line(canvas, center, (x + 1) as i32, y as i32, core.fill_color);
        }
        draw_symmetric(canvas, center, x, y, core.color);
    }
}

fn draw_fill_line(canvas: &mut Canvas, center: Point, x: i32, y: i32, color: RGBA) {
    let x_start = center.0 - x + 1;
    let x_end = center.0 + x - 1;

    for ix in x_start..x_end {
        canvas.set_pixel(ix, center.1 + y, color);
        //edge case when we only use 1st part of algorithm. Avoids double draw
        if center.1 - y != center.1 + y {
            canvas.set_pixel(ix, center.1 - y, color);
        }
    }
}

fn draw_symmetric(canvas: &mut Canvas, center: Point, x: i64, y: i64, color: RGBA) {
    let (_x, _y) = (x as i32, y as i32);
    let Point(cx, cy) = center;
    canvas.set_pixel(cx + _x, cy + _y, color);
    canvas.set_pixel(cx - _x, cy + _y, color);
    canvas.set_pixel(cx + _x, cy - _y, color);
    canvas.set_pixel(cx - _x, cy - _y, color);
}

fn draw_edge_case(canvas: &mut Canvas, center: Point, a: i32, x_drawn: i32, color: RGBA) {
    let mut x = center.0 - a;
    let end = center.0 - x_drawn;
    while x < end {
        canvas.set_pixel(x, center.1, color);
        x += 1;
    }

    x = center.0 + x_drawn;
    let end = center.0 + a;
    while x < end {
        canvas.set_pixel(x, center.1, color);
        x += 1;
    }
}
