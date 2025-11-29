use super::line::draw_line;
use crate::canvas::Canvas;
use crate::core::{Point, ShapeCore, ShapeImpl, UpdateOp, RGBA};

const DETAIL_FACTOR: f32 = 0.3;

pub struct Bezier {
    core: ShapeCore,
    // each bezier holds its own subdivide since its part of its drawing selection process
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
        let mut t = 0.0;
        let detail = self.get_detail();
        let mut prev_pts: Option<Point> = None;

        while t <= 1.0 {
            let p = self.de_casteljau(t);
            if let Some(prev) = prev_pts {
                let mut core = self.core.clone();
                core.points = vec![prev, p];
                draw_line(&core, canvas);
            }

            prev_pts = Some(p);
            t += detail;
        }
    }

    fn draw_selection<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        let points = &self.core.points;
        self.draw_selection_basic(color, canvas);

        for i in 1..points.len() {
            let lin = ShapeCore::new(vec![points[i - 1], points[i]], self.core.color);
            draw_line(&lin, canvas);
        }

        let p = self.de_casteljau(self.subdivide_t);
        self.draw_control_point(p, color, canvas);
    }

    // this hit tests is simple
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
    fn de_casteljau(&self, t: f32) -> Point {
        let mut pts_cpy = self.core.points.clone();

        for r in 1..pts_cpy.len() {
            for i in 0..(pts_cpy.len() - r) {
                pts_cpy[i] = pts_cpy[i].interpolate(pts_cpy[i + 1], t);
            }
        }

        pts_cpy[0]
    }

    fn degree_elevate(&mut self) {
        let n = self.core.points.len() as f32;
        let mut new_points: Vec<Point> = Vec::with_capacity(n as usize + 1);

        new_points.push(self.core.points[0]);

        for i in 1..(n as usize) {
            let b_prev = self.core.points[i - 1];
            let b = self.core.points[i];

            new_points.push(b.interpolate(b_prev, i as f32 / n));
        }

        new_points.push(self.core.points.last().unwrap().clone());

        self.core.points = new_points;
    }

    fn get_detail(&self) -> f32 {
        let mut distance = 0.0;
        let pts = &self.core.points;
        for i in 0..pts.len() - 1 {
            distance += pts[i].distance(pts[i + 1]);
        }

        let dist = distance * DETAIL_FACTOR / 10.0;
        1.0 / dist
    }
}
