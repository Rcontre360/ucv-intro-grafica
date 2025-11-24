use super::core::{Point, ShapeCore, ShapeImpl, UpdateOp};
use crate::canvas::Canvas;

const CURVE_DETAIL: f32 = 0.0001;

pub struct Bezier {
    core: ShapeCore,
    detail: f32,
}

impl ShapeImpl for Bezier {
    fn new(core: ShapeCore) -> Bezier {
        Bezier {
            core,
            detail: CURVE_DETAIL,
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
            UpdateOp::Subdivide => {
                self.subdivide();
            }
        }
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        let mut t = 0.0;

        while t <= 1.0 {
            let p = self.de_casteljau(t);
            canvas.set_pixel(p.0, p.1, self.core.color);
            t += self.detail;
        }
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

    fn as_bezier_mut(&mut self) -> Option<&mut Bezier> {
        Some(self)
    }
}

impl Bezier {
    pub fn add_control_point(&mut self, point: Point) {
        self.core.points.push(point);
    }

    pub fn end_control_point(&mut self, point: Point) {
        if let Some(last) = self.core.points.last_mut() {
            *last = point;
        }
    }

    fn de_casteljau(&self, t: f32) -> Point {
        let mut pts_cpy = self.core.points.clone();

        for r in 1..pts_cpy.len() {
            for i in 0..(pts_cpy.len() - r) {
                let p1 = pts_cpy[i];
                let p2 = pts_cpy[i + 1];
                let x = (1.0 - t) * p1.0 as f32 + t * p2.0 as f32;
                let y = (1.0 - t) * p1.1 as f32 + t * p2.1 as f32;

                pts_cpy[i] = (x.round() as i32, y.round() as i32);
            }
        }

        pts_cpy[0]
    }

    pub fn subdivide(&mut self) {
        let n = self.core.points.len();
        if n < 2 {
            return;
        }

        let t = 0.5;
        let mut new_points = Vec::with_capacity(n + 1);
        let mut last_points = self.core.points.clone();

        new_points.push(self.core.points[0]);

        for i in 1..n {
            let mut next_points = Vec::with_capacity(n - i);
            for j in 0..n - i {
                let p1 = last_points[j];
                let p2 = last_points[j + 1];
                let x = (1.0 - t) * p1.0 as f32 + t * p2.0 as f32;
                let y = (1.0 - t) * p1.1 as f32 + t * p2.1 as f32;
                next_points.push((x.round() as i32, y.round() as i32));
            }
            new_points.push(next_points[0]);
            last_points = next_points;
        }

        new_points.push(self.core.points[n - 1]);
        self.core.points = new_points;
    }
}
