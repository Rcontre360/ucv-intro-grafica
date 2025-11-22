use crate::primitives::{
    core::{rgba, Point, Shape, ShapeCore, ShapeImpl, RGBA},
    line::Line,
};

/// Representation of the application state. In this example, a box will bounce around the screen.
pub struct State {
    current: Shape,
    color: RGBA,
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
            println!("LINE! {}", cur.get_core());
            self.update_current_shape(end);
            self.objects.push(cur);
        }
    }

    pub fn update(&mut self) {}

    pub fn draw(&self, buffer: &mut [u8]) {
        let bg_color = self.background_color;
        for pixel in buffer.chunks_exact_mut(4) {
            pixel.copy_from_slice(&bg_color);
        }

        for shape in &self.objects {
            shape.draw(buffer);
        }

        if let Some(cur) = self.cur_shape.as_ref() {
            cur.draw(buffer);
        }
    }
}
