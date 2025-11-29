use crate::core::{Shape, ShapeCore, ShapeImpl};

mod bezier;
mod ellipse;
mod line;
mod rectangle;
mod triangle;

pub use bezier::Bezier;
pub use ellipse::Ellipse;
pub use line::Line;
pub use rectangle::Rectangle;
pub use triangle::Triangle;

pub fn new_shape_from_core(core: ShapeCore) -> Box<dyn ShapeImpl> {
    match core.shape_type {
        Shape::Line => Box::new(Line::new(core)),
        Shape::Ellipse => Box::new(Ellipse::new(core)),
        Shape::Triangle => Box::new(Triangle::new(core)),
        Shape::Rectangle => Box::new(Rectangle::new(core)),
        Shape::Bezier => Box::new(Bezier::new(core)),
    }
}
