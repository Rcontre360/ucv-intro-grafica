use crate::canvas::Canvas;
use super::core::{Point, ShapeCore, ShapeImpl};
use super::line::Line; // To draw lines for the rectangle

pub struct Rectangle {
    core: ShapeCore,
}

impl ShapeImpl for Rectangle {
    fn new(core: ShapeCore) -> Rectangle {
        Rectangle { core }
    }

    fn update(&mut self, end: Point) {
        self.core.points[1] = end;
    }

    fn get_core(&self) -> ShapeCore {
        self.core.clone()
    }

    fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        let p1 = self.core.points[0];
        let p2 = self.core.points[1];

        let top_left = (p1.0.min(p2.0), p1.1.min(p2.1));
        let bottom_right = (p1.0.max(p2.0), p1.1.max(p2.1));

        let p_tl = top_left;
        let p_tr = (bottom_right.0, top_left.1);
        let p_bl = (top_left.0, bottom_right.1);
        let p_br = bottom_right;

        // Draw four lines to form the rectangle
        Line::new(ShapeCore {
            points: vec![p_tl, p_tr],
            color: self.core.color,
        }).draw(canvas);

        Line::new(ShapeCore {
            points: vec![p_tr, p_br],
            color: self.core.color,
        }).draw(canvas);

        Line::new(ShapeCore {
            points: vec![p_br, p_bl],
            color: self.core.color,
        }).draw(canvas);

        Line::new(ShapeCore {
            points: vec![p_bl, p_tl],
            color: self.core.color,
        }).draw(canvas);
    }
}
