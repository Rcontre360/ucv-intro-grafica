use super::line::draw_line;
use crate::canvas::Canvas;
use crate::core::{Point, ShapeCore, ShapeImpl, RGBA};

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

fn draw_triangle(core: &ShapeCore, canvas: &mut Canvas) {
    if core.points.len() <= 2 {
        draw_line(&core, canvas);
    } else {
        if !core.fill_color.is_transparent() {
            fill_triangle(core, canvas);
        }

        let ab_core = core.copy_with_points(core.points[0..2].to_vec());
        let bc_core = core.copy_with_points(core.points[1..3].to_vec());
        let ca_core = core.copy_with_points(vec![core.points[2], core.points[0]]);

        draw_line(&ab_core, canvas);
        draw_line(&bc_core, canvas);
        draw_line(&ca_core, canvas);
    }
}

fn fill_triangle(core: &ShapeCore, canvas: &mut Canvas) {
    let mut points = core.points.clone();
    points.sort_by(|a, b| a.1.cmp(&b.1));

    let p1 = points[0];
    let p2 = points[1];
    let p3 = points[2];

    if p1.1 == p3.1 {
        return; // Flat triangle
    }

    let p4 = (
        (p1.0 + ((p2.1 - p1.1) as f32 / (p3.1 - p1.1) as f32 * (p3.0 - p1.0) as f32) as i32),
        p2.1,
    )
        .into();
    fill_top_triangle(canvas, p1, p2, p4, core.fill_color);
    fill_bottom_triangle(canvas, p2, p4, p3, core.fill_color);
}

fn fill_top_triangle(canvas: &mut Canvas, p1: Point, p2: Point, p3: Point, color: RGBA) {
    let m_1 = (p2.0 - p1.0) as f32 / (p2.1 - p1.1) as f32;
    let m_2 = (p3.0 - p1.0) as f32 / (p3.1 - p1.1) as f32;

    let mut cur_x1 = p1.0 as f32;
    let mut cur_x2 = p1.0 as f32;

    for y in p1.1..p2.1 {
        draw_horizontal(canvas, cur_x1.round() as i32, cur_x2.round() as i32, y, color);
        cur_x1 += m_1;
        cur_x2 += m_2;
    }
}

fn fill_bottom_triangle(canvas: &mut Canvas, p1: Point, p2: Point, p3: Point, color: RGBA) {
    let m_1 = (p3.0 - p1.0) as f32 / (p3.1 - p1.1) as f32;
    let m_2 = (p3.0 - p2.0) as f32 / (p3.1 - p2.1) as f32;

    let mut cur_x1 = p3.0 as f32;
    let mut cur_x2 = p3.0 as f32;

    for y in (p1.1..p3.1).rev() {
        draw_horizontal(canvas, cur_x1.round() as i32, cur_x2.round() as i32, y, color);
        cur_x1 -= m_1;
        cur_x2 -= m_2;
    }
}

fn draw_horizontal(canvas: &mut Canvas, mut x1: i32, mut x2: i32, y: i32, color: RGBA) {
    if x1 > x2 {
        std::mem::swap(&mut x1, &mut x2);
    }
    for x in x1..x2 {
        canvas.set_pixel(x, y, color);
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
