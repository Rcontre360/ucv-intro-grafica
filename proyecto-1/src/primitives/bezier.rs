use super::core::{Point, ShapeCore, ShapeImpl};
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

    fn update(&mut self, end: Point) {
        self.core.points[1] = end;
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
}
