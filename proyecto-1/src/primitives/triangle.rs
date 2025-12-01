use std::collections::HashMap;

use super::line::draw_line;
use crate::canvas::Canvas;
use crate::core::{Point, ShapeCore, ShapeImpl, RGBA};

type PointFloat = (f32, f32);

/// triangle object that holds the implementation for it
pub struct Triangle {
    core: ShapeCore,
}

/// triangle shape implementation
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
        draw_triangle(&self.core, canvas);
    }

    fn draw_with_color<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        draw_triangle(&self.core.copy_with_color(color), canvas);
    }

    fn hit_test(&self, p: Point) -> bool {
        if self.core.points.len() < 3 {
            return false;
        }

        let a = self.core.points[0];
        let b = self.core.points[1];
        let c = self.core.points[2];

        let cp1 = edge_side_check(a, b, p);
        let cp2 = edge_side_check(b, c, p);
        let cp3 = edge_side_check(c, a, p);

        (cp1 >= 0 && cp2 >= 0 && cp3 >= 0) || (cp1 <= 0 && cp2 <= 0 && cp3 <= 0)
    }
}

/// draws a triangle. if we dont have enough points means we are only drawing the first line
/// if we have 3 points we draw 3 lines without overlapping
fn draw_triangle(core: &ShapeCore, canvas: &mut Canvas) {
    if core.points.len() <= 2 {
        draw_line(&core, canvas, false);
    } else {
        // we dont store the pixel cache on the triangle because its data depends on the triangle
        // control points and if we modify them we end up needing another pixel cache
        let mut pixel_cache = HashMap::new();
        let ab = core.points[0..2].to_vec();
        let bc = core.points[1..3].to_vec();
        let ca = vec![core.points[2], core.points[0]];

        draw_line_for_triangle(&core.copy_with_points(ab), canvas, &mut pixel_cache);
        draw_line_for_triangle(&core.copy_with_points(bc), canvas, &mut pixel_cache);
        draw_line_for_triangle(&core.copy_with_points(ca), canvas, &mut pixel_cache);

        // if we have a fill color defined then we will fill the triangle
        if !core.fill_color.is_transparent() {
            fill_triangle(core, canvas, &pixel_cache);
        }

        // pixel cache content is released from memory here thanks to the magic of ownership and
        // borrowing in rust: https://doc.rust-lang.org/book/ch04-01-what-is-ownership.html#memory-and-allocation
    }
}

/// this function fills the triangle given the shape core.
/// to fill it we build the top triangle from the horizontal line on the corner. Then do the same
/// on the bottom triangle
fn fill_triangle(core: &ShapeCore, canvas: &mut Canvas, drawn: &HashMap<(i32, i32), bool>) {
    let mut points = core.points.clone();
    // we sort the points by "y" to get the lowest one first
    points.sort_by(|a, b| a.1.cmp(&b.1));

    let p1: (f32, f32) = points[0].into();
    let p2: (f32, f32) = points[1].into();
    let p3: (f32, f32) = points[2].into();

    if points[0].1 == points[2].1 {
        return; // Flat triangle
    }

    // the 4th point from the horizontal line intersection with the triangle side
    let p4 = (
        (p1.0 + ((p2.1 - p1.1) as f32 / (p3.1 - p1.1) as f32 * (p3.0 - p1.0) as f32)),
        p2.1 as f32,
    );
    // first fills the top triangle then the bottom one
    fill_top_triangle(p1, p2, p4, core.fill_color, canvas, drawn);
    fill_bottom_triangle(p2, p4, p3, core.fill_color, canvas, drawn);
}

/// We didnt managed to have a "PERFECT" fill for all triangles, we modified fill_top_triangle and
/// fill_bottom_triangle to produce the best triangles we could, with as many pixels of the inside
/// filled and as less pixels outside the border filled. We tested this with an array of triangles
/// with different angles on different points
fn fill_top_triangle(
    p1: PointFloat,
    p2: PointFloat,
    p3: PointFloat,
    color: RGBA,
    canvas: &mut Canvas,
    drawn: &HashMap<(i32, i32), bool>,
) {
    let m_1 = (p2.0 - p1.0) / (p2.1 - p1.1);
    let m_2 = (p3.0 - p1.0) / (p3.1 - p1.1);

    let mut cur_x1 = p1.0;
    let mut cur_x2 = p1.0;

    for y in (p1.1.ceil() as i32)..(p2.1.ceil() as i32) {
        draw_horizontal(cur_x1.ceil(), cur_x2.ceil(), y, color, canvas, drawn);
        cur_x1 += m_1;
        cur_x2 += m_2;
    }
}

/// We didnt managed to have a "PERFECT" fill for all triangles, we modified fill_top_triangle and
/// fill_bottom_triangle to produce the best triangles we could, with as many pixels of the inside
/// filled and as less pixels outside the border filled. We tested this with an array of triangles
/// with different angles on different points
fn fill_bottom_triangle(
    p1: PointFloat,
    p2: PointFloat,
    p3: PointFloat,
    color: RGBA,
    canvas: &mut Canvas,
    drawn: &HashMap<(i32, i32), bool>,
) {
    let m_1 = (p3.0 - p1.0) / (p3.1 - p1.1);
    let m_2 = (p3.0 - p2.0) / (p3.1 - p2.1);

    let mut cur_x1 = p3.0;
    let mut cur_x2 = p3.0;

    for y in ((p1.1.round() as i32)..(p3.1.round() as i32)).rev() {
        draw_horizontal(
            // for different slopes (angles) this configuration gave us the best filling possible
            if m_1 > 0.0 {
                cur_x1.floor()
            } else {
                cur_x1.ceil() + 1.0
            },
            if m_2 > 0.0 {
                cur_x2.floor()
            } else {
                cur_x2.ceil()
            },
            y,
            color,
            canvas,
            drawn,
        );
        cur_x1 -= m_1;
        cur_x2 -= m_2;
    }
}

/// draws horizontal lines to fill the triangle. Recieves the height (y), the start and end (x1,x2)
/// checks if we are in a border before filling
fn draw_horizontal(
    mut x1: f32,
    mut x2: f32,
    y: i32,
    color: RGBA,
    canvas: &mut Canvas,
    drawn: &HashMap<(i32, i32), bool>,
) {
    if x1 > x2 {
        std::mem::swap(&mut x1, &mut x2);
    }
    for x in (x1.round() as i32)..(x2.round() as i32) {
        if drawn.get(&(x, y)).is_none() {
            canvas.set_pixel(x, y, color);
        }
    }
}

fn edge_side_check(a: Point, b: Point, p: Point) -> i64 {
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

/// same algorithm used in draw_line with the DIFFERENCE that this one tracks every pixel of the
/// borders. since there's not an effective way to avoid drawing the same pixel twice with
/// triangles with very narrow angles (45 degrees or less) we do something different for this shape
/// we will track all border pixels, this way we wont have issues with drawing the same
/// pixel twice
/// we copied the algorithm instead of making the modification on the other one BECAUSE we dont
/// want the performance of others to be hit by this one, also only the triangle suffers from this
/// error.
pub fn draw_line_for_triangle<'a>(
    core: &ShapeCore,
    canvas: &mut Canvas<'a>,
    drawn: &mut HashMap<(i32, i32), bool>,
) {
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

    let mut draw_if_available = |x, y| {
        if drawn.get(&(x, y)).is_none() {
            drawn.insert((x, y), true);
            canvas.set_pixel(x, y, core.color);
        }
    };

    draw_if_available(x, y);

    if run_on_x {
        while x > b.0 || x < b.0 {
            if d <= 0 {
                d += inc_ne;
                y += y_inc;
            } else {
                d += inc_e;
            }

            x += x_inc;

            draw_if_available(x, y);
        }
    } else {
        while (y > b.1 || y < b.1) {
            if d <= 0 {
                d += inc_ne;
                x += x_inc;
            } else {
                d += inc_e;
            }

            y += y_inc;

            draw_if_available(x, y);
        }

        x += x_inc;
        // edge case found. we want to draw a full line from a to b inclusive
        // this ensures that b is drawn, when reaching this else condition it is not drawn
        // This was tested to check if there are overlaps on the canvas
        draw_if_available(x, y);
    }
}
