use crate::canvas::Canvas;

use crate::core::{Point, ShapeCore, ShapeImpl, RGBA};

const HIT_TEST_ERROR: u64 = 100;

/// line object definition
pub struct Line {
    core: ShapeCore,
}

/// line object shape implementation
impl ShapeImpl for Line {
    fn new(core: ShapeCore) -> Line {
        Line { core }
    }

    fn get_core_mut(&mut self) -> &mut ShapeCore {
        &mut self.core
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        draw_line(&self.core, canvas, true);
    }

    fn draw_with_color<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        draw_line(&self.core.copy_with_color(color), canvas, true);
    }

    fn hit_test(&self, point: Point) -> bool {
        return line_hit_test(&self.core, point);
    }
}

/// draws a line given a shape core. Used by other shapes
/// draw first is used to NOT draw the first point, used for other shapes to avoid overlapping
pub fn draw_line<'a>(core: &ShapeCore, canvas: &mut Canvas<'a>, draw_first: bool) {
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

    if draw_first {
        canvas.set_pixel(x, y, core.color);
    }

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

        x += x_inc;
        // edge case found. we want to draw a full line from a to b inclusive
        // this ensures that b is drawn, when reaching this else condition it is not drawn
        // This was tested to check if there are overlaps on the canvas
        canvas.set_pixel(x, y, core.color);
    }
}

/// for the line hit test we just check if the given point is at certain distance from the line
/// I use the vector to point formulation since it gives me the distance of a finite line
/// (vector)
/// I modified the original formula of point to vector to use ONLY INTEGER ARITHMETIC
pub fn line_hit_test(core: &ShapeCore, point: Point) -> bool {
    let p1 = core.points[0];
    let p2 = core.points[1];

    if p1 == p2 {
        // if a line is a point then is impossible to select it.
        // What we do is create a box around this point of 10 pixels and check if the click is
        // within that box
        return point.is_within_box(p1, p1, 10);
    }

    let delta = p2 - p1;
    let delta_sqr = delta.dot(delta) as u64;

    let p_to_p1 = point - p1;
    let n = p_to_p1.dot(delta);

    let closest_point = if n < 0 {
        p1
    } else if n as u64 > delta_sqr {
        p2
    } else {
        /* originally this should be:
        I changed this formula to use only integer arithmetic

        p1 + n / delta_sqr * delta
        The DISTANCE formula IS:
        distance = dist_n.dot(dist_n)
        dist_n = (point - p1 + ( n / delta_sqr ) * delta)

        SINCE:
        c*(a.dot(b)) = a.dot(c*b)
        delta_sqr * dist_n = dist_n*point - dist_n*p1 + delta * n

        THEN WITH:
        dist_n = delta_sqr * point - delta_sqr * p1 + n * delta
        distance = dist_n / (delta_sqr*delta_sqr)
        */

        (delta_sqr as i32) * p1 + n * delta
    };

    let dist_prev_x = delta_sqr * point.0 as u64 - closest_point.0 as u64;
    let dist_prev_y = delta_sqr * point.1 as u64 - closest_point.1 as u64;

    let distance = dist_prev_x * dist_prev_x + dist_prev_y * dist_prev_y;
    return distance < HIT_TEST_ERROR * delta_sqr * delta_sqr;
}
