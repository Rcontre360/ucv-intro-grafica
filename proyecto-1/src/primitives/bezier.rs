use super::{
    core::{Point, ShapeCore, ShapeImpl, UpdateOp},
    line::draw_line,
};
use crate::canvas::Canvas;

const DETAIL_FACTOR: f32 = 0.2;

pub struct Bezier {
    core: ShapeCore,
}

impl ShapeImpl for Bezier {
    fn new(core: ShapeCore) -> Bezier {
        Bezier { core }
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
                let core = ShapeCore {
                    points: vec![prev, p],
                    color: self.core.color,
                    fill_color: self.core.fill_color,
                };
                draw_line(&core, canvas);
            }
            prev_pts = Some(p);
            //canvas.set_pixel(p.0, p.1, self.core.color);
            t += detail;
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
}

impl Bezier {
    fn get_detail(&self) -> f32 {
        let mut distance = 0.0;
        let pts = &self.core.points;
        for i in 0..pts.len() - 1 {
            let dx = pts[i + 1].0 - pts[i].0;
            let dy = pts[i + 1].1 - pts[i].1;
            // euclidean distance
            distance += ((dx * dx + dy * dy) as f32).sqrt();
        }

        let dist = distance * DETAIL_FACTOR / 10.0;
        1.0 / dist
    }

    fn de_casteljau(&self, t: f32) -> Point {
        let mut pts_cpy = self.core.points.clone();

        for r in 1..pts_cpy.len() {
            for i in 0..(pts_cpy.len() - r) {
                let p1 = pts_cpy[i];
                let p2 = pts_cpy[i + 1];
                let x = (1.0 - t) * p1.0 as f32 + t * p2.0 as f32;
                let y = (1.0 - t) * p1.1 as f32 + t * p2.1 as f32;

                pts_cpy[i] = (x.round() as i32, y.round() as i32).into();
            }
        }

        pts_cpy[0]
    }

    pub fn degree_elevate(&mut self) {
        let n = self.core.points.len() as f32;
        let mut new_points: Vec<Point> = Vec::with_capacity(n as usize + 1);

        new_points.push(self.core.points[0]);

        for i in 1..(n as usize) {
            let b_prev = self.core.points[i - 1];
            let b = self.core.points[i];
            let j = i as f32;

            // j / n * b_j-1 + (1 - j/n) * b_j
            // formula says its n+1 but we already added one before the loop
            let x = j / n * (b_prev.0 as f32) + (1.0 - j / n) * (b.0 as f32);
            let y = j / n * (b_prev.1 as f32) + (1.0 - j / n) * (b.1 as f32);

            new_points.push((x, y).into());
        }

        new_points.push(*self.core.points.last().unwrap());

        self.core.points = new_points;
    }
}
