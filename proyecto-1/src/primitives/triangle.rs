use super::core::{Point, ShapeCore, ShapeImpl, UpdateOp};
use super::line::Line;
use crate::canvas::Canvas; // To draw lines for the triangle

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

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {}

    fn hit_test(&self, _point: Point) -> bool {
        false
    }
}
