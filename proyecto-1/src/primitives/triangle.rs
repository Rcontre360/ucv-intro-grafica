use crate::canvas::Canvas;
use super::core::{Point, ShapeCore, ShapeImpl};
use super::line::Line; // To draw lines for the triangle

pub struct Triangle {
    core: ShapeCore,
}

impl ShapeImpl for Triangle {
    fn new(core: ShapeCore) -> Triangle {
        Triangle { core }
    }

    fn update(&mut self, end: Point) {
        // For a triangle, the first point is the start, the second is the current mouse position,
        // and the third point will be updated as the mouse moves.
        // We need to ensure we have at least 3 points for a triangle.
        if self.core.points.len() < 3 {
            self.core.points.push(end);
        } else {
            self.core.points[2] = end;
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
}
