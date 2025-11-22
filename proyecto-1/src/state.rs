use crate::{
    canvas::Canvas,
    primitives::{
        core::{rgba, Point, Shape, ShapeCore, ShapeImpl, RGBA},
        line::Line,
        ellipse::Ellipse,
        circle::Circle,
        triangle::Triangle,
        rectangle::Rectangle,
        bezier::Bezier,
    },
};

pub struct State {
    pub current: Shape,
    pub color: RGBA,
    pub cur_shape: Option<Box<dyn ShapeImpl>>,
    objects: Vec<Box<dyn ShapeImpl>>,
}

impl State {
    pub fn new() -> Self {
        Self {
            current: Shape::Line,
            color: rgba(255, 255, 255, 255),
            cur_shape: None,
            objects: vec![],
        }
    }

    pub fn start_current_shape(&mut self, start: Point) {
        match self.current {
            Shape::Line => {
                self.cur_shape = Some(Box::new(Line::new(ShapeCore {
                    points: vec![start, start],
                    color: self.color,
                })));
            }
            Shape::Ellipse => {
                self.cur_shape = Some(Box::new(Ellipse::new(ShapeCore {
                    points: vec![start, start],
                    color: self.color,
                })));
            }
            Shape::Circle => {
                self.cur_shape = Some(Box::new(Circle::new(ShapeCore {
                    points: vec![start, start],
                    color: self.color,
                })));
            }
            Shape::Triangle => {
                self.cur_shape = Some(Box::new(Triangle::new(ShapeCore {
                    points: vec![start, start, start], // Initialize with 3 points for triangle
                    color: self.color,
                })));
            }
            Shape::Rectangle => {
                self.cur_shape = Some(Box::new(Rectangle::new(ShapeCore {
                    points: vec![start, start],
                    color: self.color,
                })));
            }
            Shape::Bezier => {
                self.cur_shape = Some(Box::new(Bezier::new(ShapeCore {
                    points: vec![start, start], // Initialize with 2 points for bezier (for now)
                    color: self.color,
                })));
            }
            _ => {}
        }
    }

    pub fn update_current_shape(&mut self, end: Point) {
        if let Some(mut cur) = self.cur_shape.take() {
            cur.as_mut().update(end);
            self.cur_shape = Some(cur);
        }
    }

    pub fn end_current_shape(&mut self, end: Point) {
        if let Some(cur) = self.cur_shape.take() {
            self.update_current_shape(end);
            self.objects.push(cur);
        }
    }

    pub fn update(&mut self) {}

    pub fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        canvas.clear();

        for shape in &self.objects {
            shape.draw(canvas);
        }

        if let Some(cur) = self.cur_shape.as_ref() {
            cur.draw(canvas);
        }
    }
}
