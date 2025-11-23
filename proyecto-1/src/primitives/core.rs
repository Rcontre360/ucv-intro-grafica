use crate::canvas::Canvas;
use core::fmt;

#[allow(dead_code)]
pub enum Shape {
    Line,
    Ellipse,
    Triangle,
    Rectangle,
    Bezier,
}

pub type Point = (i32, i32);

pub type RGBA = [u8; 4];

pub type ControlPoints = Vec<Point>;

#[allow(dead_code)]
pub trait ShapeImpl {
    fn new(core: ShapeCore) -> Self
    where
        Self: Sized;

    fn update(&mut self, end: Point);

    fn get_core(&self) -> ShapeCore;

    fn draw<'a>(&self, buffer: &mut Canvas<'a>);

    fn hit_test(&self, point: Point) -> bool;
}

#[derive(Clone)]
pub struct ShapeCore {
    pub points: ControlPoints,
    pub color: RGBA,
}

impl fmt::Display for ShapeCore {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let points_str: Vec<String> = self
            .points
            .iter()
            .map(|(x, y)| format!("({}, {})", x, y))
            .collect();

        write!(f, "ShapeCore ([{}])", points_str.join(", "))
    }
}

pub fn rgba(r: u8, g: u8, b: u8, a: u8) -> RGBA {
    [r, g, b, a]
}
