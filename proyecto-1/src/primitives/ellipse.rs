use crate::canvas::Canvas;
use crate::core::{Point, RGBA, ShapeCore, ShapeImpl, UpdateOp};

use super::line::line_hit_test;

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

    /// hit test for ellipse uses ONLY integer arithmetic
    /// we just use the same formula to know if a point is within the ellipse
    /// the formula is dx*dx / (a*a) + dy * dy / (b*b) <= 1. After some simple algebra manipulation
    /// we get the formula bellow. We just multiply both sides by a^2 * b^2
    fn hit_test(&self, point: Point) -> bool {
        let (center, a, b) = get_ellipse(&self.core);

        // special case where the ellipse is completelly flat
        if a == 0 || b == 0 {
            return line_hit_test(&self.core, point);
        }

        let delta = point - center;

        let dx = delta.0 as i64;
        let dy = delta.1 as i64;

        (dx * dx) * (b * b) + (dy * dy) * (a * a) <= a * a * b * b
    }
}

/// we draw an ellipse using an integer only algorithm. Same as the one used on homework 1 with the
/// optimizations included.
fn draw_ellipse(core: &ShapeCore, canvas: &mut Canvas) {
    let (center, a, b) = get_ellipse(core);

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
    // here we added an extra condition that draws the inside of the ellipse.
    // it should only be used on each different "y"
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

/// this just draws a line from center-x+1 to center+x-1.
/// its always an horizontal line
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

/// draws 4 points symetric given the first one on the first quadrant
fn draw_symmetric(canvas: &mut Canvas, center: Point, x: i64, y: i64, color: RGBA) {
    let (_x, _y) = (x as i32, y as i32);
    let Point(cx, cy) = center;
    canvas.set_pixel(cx + _x, cy + _y, color);
    canvas.set_pixel(cx - _x, cy + _y, color);
    canvas.set_pixel(cx + _x, cy - _y, color);
    canvas.set_pixel(cx - _x, cy - _y, color);
}

/// draws the edge case when the ellipse is flat on the x axis. Done in homework 1 as well
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

/// returns the common values of an ellipse, center, a and b
fn get_ellipse(core: &ShapeCore) -> (Point, i64, i64) {
    let p1 = core.points[0];
    let p2 = core.points[1];
    let a = ((p2.0 - p1.0) >> 1).abs() as i64;
    let b = ((p2.1 - p1.1) >> 1).abs() as i64;

    (Point((p1.0 + p2.0) >> 1, (p1.1 + p2.1) >> 1), a, b)
}
