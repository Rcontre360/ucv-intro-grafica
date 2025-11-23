use crate::{
    canvas::Canvas,
    primitives,
    primitives::core::{rgba, Point, Shape, ShapeCore, ShapeImpl, RGBA},
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
        self.cur_shape = match self.current {
            Shape::Line => box_new_shape::<primitives::Line>(start, self.color),
            Shape::Ellipse => box_new_shape::<primitives::Ellipse>(start, self.color),
            Shape::Triangle => box_new_shape::<primitives::Triangle>(start, self.color),
            Shape::Rectangle => box_new_shape::<primitives::Rectangle>(start, self.color),
            Shape::Bezier => box_new_shape::<primitives::Bezier>(start, self.color),
        };
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

fn box_new_shape<T>(start: Point, color: RGBA) -> Option<Box<dyn ShapeImpl>>
where
    T: ShapeImpl + Sized + 'static,
{
    let points = vec![start, start];
    let core = ShapeCore { points, color };
    Some(Box::new(T::new(core)))
}
