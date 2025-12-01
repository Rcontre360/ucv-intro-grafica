use crate::canvas::Canvas;

// this are rust modules. mod.rs defines a module and imports from other files inside its root
// folder. Here we have access to point and rgba but we only expose what is under the "pub" keyword
mod point;
mod rgba;

// we expose the Point and RGBA modules
pub use point::Point;
pub use rgba::RGBA;

use serde::{Deserialize, Serialize};
use std::fmt;

/// shape is an enum that specifies the shape
#[derive(Copy, Clone, Serialize, Deserialize, PartialEq)]
pub enum Shape {
    Line,
    Ellipse,
    Triangle,
    Rectangle,
    Bezier,
}

// this is for debugging and the UI, rust uses println!("Hello world") and to print objects we do
// println!("{}",object). This allows to specify how the object is displayed when printed or
// displayed
impl fmt::Display for Shape {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Shape::Line => write!(f, "Line"),
            Shape::Ellipse => write!(f, "Ellipse"),
            Shape::Triangle => write!(f, "Triangle"),
            Shape::Rectangle => write!(f, "Rectangle"),
            Shape::Bezier => write!(f, "Bezier"),
        }
    }
}

/// UpdateOp is an enum that defines which operation we are applying to a given Shape
#[derive(Clone, PartialEq)]
pub enum UpdateOp {
    /// Move specifies the movement of a shape. Receives as argument the Point(x,y) where x and y
    /// is how much we are moving on that axis
    Move(Point),
    /// ChangeColor changes the border color of a given Shape. Receives as argument a color RGBA
    ChangeColor(RGBA),
    /// ChangeColor changes the fill color of a given Shape. Receives as argument a color RGBA
    ChangeFillColor(RGBA),
    /// Adds a control point to the shape. Used for triangle on drawing and bezier
    AddControlPoint(Point),
    /// Changes a control point for another
    ControlPoint(usize, Point),
    /// Rewrites all points on the shape for another set of points
    RewritePoints(Vec<Point>),
    /// Makes a subdivide on the shape. Currently only used on bezier
    UpdateSubdivide(f32),
    /// Increases the degree of the shape. Only used on bezier
    DegreeElevate,
}

///Trait for the shape implementation. Implements some default functions and is used for rendering
///and shape management on the application
#[allow(dead_code)]
pub trait ShapeImpl {
    /// creates a new shape given a ShapeCore
    fn new(core: ShapeCore) -> Self
    where
        Self: Sized;

    /// shared update method. used by all shapes. Implements common update operations
    fn update_basic(&mut self, op: &UpdateOp) {
        // we obtain a mutable instance of the shape core
        let core = self.get_core_mut();
        match op {
            // update its color
            UpdateOp::ChangeColor(color) => {
                core.color = *color;
            }
            // update its fill color
            UpdateOp::ChangeFillColor(color) => {
                core.fill_color = *color;
            }
            // move the shape
            UpdateOp::Move(delta) => {
                for p in core.points.iter_mut() {
                    p.0 += delta.0;
                    p.1 += delta.1;
                }
            }
            // add a control point
            UpdateOp::AddControlPoint(point) => {
                core.points.push(*point);
            }
            // rewrite all points for others
            // used when doing undo and redo
            UpdateOp::RewritePoints(points) => {
                core.points = points.clone();
            }
            // change a control point for another one
            UpdateOp::ControlPoint(index, point) => {
                if *index < core.points.len() {
                    core.points[*index] = *point;
                }
            }
            // there are other modification methods that should be implemented by a concrete object
            _ => {}
        }
    }

    /// this method draws how a shape should look when is selected. For most is just drawing the
    /// control points. Receives the color to use for the control points
    fn draw_selection_basic<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>) {
        let points = self.get_core().points;
        for p in points {
            self.draw_control_point(p, color, canvas);
        }
    }

    /// this method draws the control points of a given shape
    /// it receives the point to draw and the color
    fn draw_control_point<'a>(&self, p: Point, color: RGBA, canvas: &mut Canvas<'a>) {
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

    /// update method receives an update operation and makes the update
    fn update(&mut self, op: &UpdateOp) {
        self.update_basic(op);
    }

    /// draws how a shape should look and can be modified by concrete classes
    fn draw_selection<'a>(&self, color1: RGBA, _color2: RGBA, canvas: &mut Canvas<'a>) {
        self.draw_selection_basic(color1, canvas);
    }

    /// subdivision function to create 2 more shapes from the original
    /// Currently only bezier implements this, we added it as part of the interface to keep the app
    /// state management agnostic from the shape type used
    fn subdivide(&self) -> Option<(ShapeCore, ShapeCore)> {
        None
    }

    /// returns the shape type of the current shape
    fn get_type(&self) -> Shape {
        self.get_core().shape_type
    }

    /// return the shape core mutable reference
    fn get_core_mut(&mut self) -> &mut ShapeCore;

    /// returns a copy of the shape core
    fn get_core(&self) -> ShapeCore;

    /// draws the shape into a given canvas
    fn draw<'a>(&self, canvas: &mut Canvas<'a>);

    /// draws the shape with a given border color
    fn draw_with_color<'a>(&self, color: RGBA, canvas: &mut Canvas<'a>);

    /// checks if the shape was hit or clicked on a given point
    fn hit_test(&self, point: Point) -> bool;
}

///This is the core fields of every shape. Is used for shape serialization into json
///Also helps with shape management and to avoid the use of dynamic types on some parts of the application
#[derive(Clone, Serialize, Deserialize)]
pub struct ShapeCore {
    /// The control points of the shape
    pub points: Vec<Point>,
    /// The color of the border of the shape
    pub color: RGBA,
    /// The fill color of the shape. Not all shapes use it
    pub fill_color: RGBA,
    /// The shape type, used for identification on some parts of the app
    pub shape_type: Shape,
}

// Custom methods of the shape core
impl ShapeCore {
    /// Creates a shape core by copying the actual one but changing its control points
    pub fn copy_with_color(&self, color: RGBA) -> ShapeCore {
        ShapeCore {
            color,
            // here we clone the previous control points
            ..self.clone()
        }
    }

    /// Creates a shape core by copying the actual one but changing its control points
    pub fn copy_with_points(&self, points: Vec<Point>) -> ShapeCore {
        ShapeCore {
            points,
            // here we clone the previous control points
            ..self.clone()
        }
    }
}
