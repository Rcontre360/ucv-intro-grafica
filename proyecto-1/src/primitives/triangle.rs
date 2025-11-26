use super::core::{Point, ShapeCore, ShapeImpl, UpdateOp};
use super::line::{draw_line, Line};
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
            UpdateOp::AddControlPoint { point } => {
                self.core.points.push(*point);
            }
            _ => {}
        }
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        if self.core.points.len() <= 2 {
            draw_line(&self.core, canvas);
        } else {
            let core = &self.core;
            let ab = core.points[0..2].to_vec();
            let bc = core.points[1..3].to_vec();
            let ca = vec![core.points[2], core.points[0]];

            draw_line(&self.core_from_points(ab), canvas);
            draw_line(&self.core_from_points(bc), canvas);
            draw_line(&self.core_from_points(ca), canvas);
        }
    }

    fn hit_test(&self, _point: Point) -> bool {
        false
    }
}

impl Triangle {
    fn core_from_points(&self, points: Vec<Point>) -> ShapeCore {
        let mut core = self.core.clone();
        core.points = points;
        core
    }
}
