use core::fmt;
use std::ffi::os_str::Display;

pub enum Shape {
    Line,
    Circle,
    Ellipse,
    Triangle,
    Curve,
}

pub type Point = (u32, u32);

pub type RGBA = [u8; 4];

pub type ControlPoints = Vec<Point>;

pub trait ShapeImpl {
    fn new(core: ShapeCore) -> Self
    where
        Self: Sized;

    fn update(&mut self, end: Point);

    fn get_core(&self) -> ShapeCore;

    fn draw(&self, buffer: &mut [u8]);
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
