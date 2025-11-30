use super::line::draw_line;
use crate::canvas::Canvas;
use crate::core::{Point, RGBA, ShapeCore, ShapeImpl, UpdateOp};

const DETAIL_FACTOR: f32 = 0.3;

pub struct Bezier {
    core: ShapeCore,
    subdivide_t: f32,
}

impl ShapeImpl for Bezier {
    fn new(core: ShapeCore) -> Bezier {
        Bezier {
            core,
            subdivide_t: 0.5,
        }
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn get_core_mut(&mut self) -> &mut ShapeCore {
        &mut self.core
    }

    fn update(&mut self, op: &UpdateOp) {
        self.update_basic(op);

        match op {
            UpdateOp::DegreeElevate => {
                self.degree_elevate();
            }
            UpdateOp::UpdateSubdivide(t) => {
                self.subdivide_t = *t;
            }
            _ => {}
        }
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        println!("DRAWING BEZIER");
        draw_bezier(&self.core, canvas);
    }

    fn draw_with_color<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        draw_bezier(&self.core.copy_with_color(color), canvas);
    }

    fn draw_selection<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        self.draw_selection_basic(color, canvas);

        for i in 1..self.core.points.len() {
            let line_core = self
                .core
                .copy_with_points(vec![self.core.points[i - 1], self.core.points[i]]);
            draw_line(&line_core, canvas, true);
        }

        let p = de_casteljau(&self.core, self.subdivide_t);
        self.draw_control_point(p, color, canvas);
    }

    fn hit_test(&self, point: Point) -> bool {
        if self.core.points.is_empty() {
            return false;
        }

        let mut min_x = self.core.points[0].0;
        let mut max_x = self.core.points[0].0;
        let mut min_y = self.core.points[0].1;
        let mut max_y = self.core.points[0].1;

        for p in &self.core.points {
            min_x = min_x.min(p.0);
            max_x = max_x.max(p.0);
            min_y = min_y.min(p.1);
            max_y = max_y.max(p.1);
        }

        point.0 >= min_x && point.0 <= max_x && point.1 >= min_y && point.1 <= max_y
    }

    fn subdivide(&self) -> Option<(ShapeCore, ShapeCore)> {
        let mut pts_cpy = self.core.points.clone();
        let n = pts_cpy.len();

        let mut first = vec![self.core.points[0]];
        let mut second = vec![self.core.points[n - 1]];

        for r in 1..n {
            for i in 0..(n - r) {
                pts_cpy[i] = pts_cpy[i].interpolate(pts_cpy[i + 1], self.subdivide_t);
            }

            first.push(pts_cpy[0]);
            second.push(pts_cpy[n - r - 1]);
        }

        second.reverse();

        Some((
            self.core.copy_with_points(first),
            self.core.copy_with_points(second),
        ))
    }
}

impl Bezier {
    fn degree_elevate(&mut self) {
        let n = self.core.points.len() as f32;
        let mut new_points: Vec<Point> = Vec::with_capacity(n as usize + 1);

        new_points.push(self.core.points[0]);

        for i in 1..(n as usize) {
            let b_prev = self.core.points[i - 1];
            let b = self.core.points[i];

            new_points.push(b.interpolate(b_prev, i as f32 / n));
        }

        new_points.push(*self.core.points.last().unwrap());

        self.core.points = new_points;
    }
}

fn draw_bezier(core: &ShapeCore, canvas: &mut Canvas) {
    let mut t = 0.0;
    let detail = get_detail(core);
    let mut prev_pts: Option<Point> = None;
    let mut draw_last = false;

    while t <= 1.0 {
        let p = de_casteljau(core, t);
        if let Some(prev) = prev_pts {
            // since draw line must not draw the first point we sort the points backwards p first
            // then prev
            // that way we draw a line that is connected to the next one without overlapping
            let line_core = core.copy_with_points(vec![p, prev]);
            draw_line(&line_core, canvas, draw_last);
        }

        prev_pts = Some(p);
        t += detail;
        // our lines are drawn from a to b like this [a,b). the last point must be drawn, when this
        // is true the full line is drawn [a,b]
        draw_last = t + detail > 1.0;
    }
}

fn de_casteljau(core: &ShapeCore, t: f32) -> Point {
    let mut pts_cpy = core.points.clone();

    for r in 1..pts_cpy.len() {
        for i in 0..(pts_cpy.len() - r) {
            pts_cpy[i] = pts_cpy[i].interpolate(pts_cpy[i + 1], t);
        }
    }

    pts_cpy[0]
}

fn get_detail(core: &ShapeCore) -> f32 {
    let mut distance = 0.0;
    let pts = &core.points;
    for i in 0..pts.len() - 1 {
        distance += pts[i].distance(pts[i + 1]);
    }

    let dist = distance * DETAIL_FACTOR / 10.0;
    if dist.abs() < 1.0 {
        return 0.1;
    }
    1.0 / dist
}
