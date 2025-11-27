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

pub enum UpdateOp {
    Move { delta: (i32, i32) },
    ControlPoint { index: usize, point: Point },
    AddControlPoint { point: Point },
    DegreeElevate,
}

pub type Point = (i32, i32);

pub type RGBA = [u8; 4];

pub type ControlPoints = Vec<Point>;

#[allow(dead_code)]
pub trait ShapeImpl {
    fn new(core: ShapeCore) -> Self
    where
        Self: Sized;

    fn update(&mut self, op: &UpdateOp);

    fn get_core(&self) -> ShapeCore;

    fn draw<'a>(&self, buffer: &mut Canvas<'a>);

    fn hit_test(&self, point: Point) -> bool;
}

#[derive(Clone)]
pub struct ShapeCore {
    pub points: ControlPoints,
    pub color: RGBA,
    pub fill_color: RGBA,
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

pub fn is_transparent(color: RGBA) -> bool {
    color[3] == 0
}

pub fn mix_colors(new: RGBA, old: RGBA) -> RGBA {
    let alpha = new[3] as f32 / 255.0;
    let new_r = (new[0] as f32 * alpha + old[0] as f32 * (1.0 - alpha)) as u8;
    let new_g = (new[1] as f32 * alpha + old[1] as f32 * (1.0 - alpha)) as u8;
    let new_b = (new[2] as f32 * alpha + old[2] as f32 * (1.0 - alpha)) as u8;

    [new_r, new_g, new_b, 255]
}
