use crate::canvas::Canvas;
use core::fmt;

mod point;
mod rgba;

pub use point::Point;
pub use rgba::RGBA;

#[derive(Copy, Clone)]
pub enum Shape {
    Line,
    Ellipse,
    Triangle,
    Rectangle,
    Bezier,
}

pub enum UpdateOp {
    Move { delta: Point },
    ChangeColor { color: RGBA },
    ChangeFillColor { color: RGBA },
    ControlPoint { index: usize, point: Point },
    AddControlPoint { point: Point },
    DegreeElevate,
}

pub type ControlPoints = Vec<Point>;

#[allow(dead_code)]
pub trait ShapeImpl {
    fn new(core: ShapeCore) -> Self
    where
        Self: Sized;

    //shared update method
    fn update_basic(&mut self, op: &UpdateOp) {
        let core = self.get_core_mut();
        match op {
            UpdateOp::ChangeColor { color } => {
                core.color = *color;
            }
            UpdateOp::ChangeFillColor { color } => {
                core.fill_color = *color;
            }
            UpdateOp::Move { delta } => {
                for p in core.points.iter_mut() {
                    p.0 += delta.0;
                    p.1 += delta.1;
                }
            }
            UpdateOp::ControlPoint { index, point } => {
                if *index < core.points.len() {
                    core.points[*index] = *point;
                }
            }
            UpdateOp::AddControlPoint { point } => {
                core.points.push(*point);
            }
            _ => {}
        }
    }

    fn update(&mut self, op: &UpdateOp) {
        self.update_basic(op);
    }

    fn get_core_mut(&mut self) -> &mut ShapeCore;

    fn get_core(&self) -> ShapeCore;

    fn draw<'a>(&self, canvas: &mut Canvas<'a>);

    fn draw_selection_basic<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        let points = self.get_core().points;
        for p in points {
            for x in (p.0 - 5)..(p.0 + 5) {
                for y in (p.1 - 5)..(p.1 + 5) {
                    if (x - p.0).pow(2) + (y - p.1).pow(2) <= 5i32.pow(2) {
                        canvas.set_pixel(x, y, RGBA::new(255, 255, 255, 255));
                    }
                }
            }

            for x in (p.0 - 4)..(p.0 + 4) {
                for y in (p.1 - 4)..(p.1 + 4) {
                    if (x - p.0).pow(2) + (y - p.1).pow(2) <= 4i32.pow(2) {
                        canvas.set_pixel(x, y, color);
                    }
                }
            }
        }
    }

    fn draw_selection<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        self.draw_selection_basic(color, canvas);
    }

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
            .map(|Point(x, y)| format!("({}, {})", x, y))
            .collect();

        write!(f, "ShapeCore ([{}])", points_str.join(", "))
    }
}
