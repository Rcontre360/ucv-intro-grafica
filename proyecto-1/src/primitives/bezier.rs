use crate::canvas::Canvas;
use super::core::{Point, ShapeCore, ShapeImpl};

pub struct Bezier {
    core: ShapeCore,
}

impl ShapeImpl for Bezier {
    fn new(core: ShapeCore) -> Bezier {
        Bezier { core }
    }

    fn update(&mut self, end: Point) {
        // For now, just update the last point.
        // A proper Bezier implementation would need multiple control points.
        if self.core.points.len() < 2 {
            self.core.points.push(end);
        } else {
            self.core.points[1] = end;
        }
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, _canvas: &mut Canvas<'a>) {
        // TODO: Implement Bezier curve drawing
        // For now, do nothing.
    }

    fn hit_test(&self, _point: Point) -> bool {
        false
    }
}
