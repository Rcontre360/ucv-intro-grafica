use crate::canvas::Canvas;
use super::core::{Point, ShapeCore, ShapeImpl, UpdateOp};
use super::line::Line; // To draw lines for the triangle

pub struct Triangle {
    core: ShapeCore,
}

impl ShapeImpl for Triangle {
    fn new(core: ShapeCore) -> Triangle {
        Triangle { core }
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
            _ => {}
        }
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        if self.core.points.len() >= 3 {
            let p1 = self.core.points[0];
            let p2 = self.core.points[1];
            let p3 = self.core.points[2];

            // Draw three lines to form the triangle
            Line::new(ShapeCore {
                points: vec![p1, p2],
                color: self.core.color,
            }).draw(canvas);

            Line::new(ShapeCore {
                points: vec![p2, p3],
                color: self.core.color,
            }).draw(canvas);

            Line::new(ShapeCore {
                points: vec![p3, p1],
                color: self.core.color,
            }).draw(canvas);
        }
    }

    fn hit_test(&self, _point: Point) -> bool {
        false
    }
}
