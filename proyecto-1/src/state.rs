use crate::{
    canvas::Canvas,
    primitives,
    primitives::core::{Point, RGBA, Shape, ShapeCore, ShapeImpl, rgba},
};

pub struct State {
    pub current: Shape,
    pub color: RGBA,
    pub cur_shape: Option<Box<dyn ShapeImpl>>,
    objects: Vec<Box<dyn ShapeImpl>>,
    pub selected: Option<usize>,
}

impl State {
    pub fn new() -> Self {
        Self {
            current: Shape::Line,
            color: rgba(255, 255, 255, 255),
            cur_shape: None,
            objects: vec![],
            selected: None,
        }
    }

    pub fn handle_selection(&mut self, pt: Point) {
        for (i, object) in self.objects.iter().enumerate() {
            if object.hit_test(pt) {
                self.selected = Some(i);
                return;
            }
        }
        self.selected = None;
    }

    pub fn start_current_shape(&mut self, start: Point) {
        if self.selected.is_some() {
            return;
        }

        self.cur_shape = match self.current {
            Shape::Line => box_new_shape::<primitives::Line>(start, self.color),
            Shape::Ellipse => box_new_shape::<primitives::Ellipse>(start, self.color),
            Shape::Triangle => None, // Not implemented
            Shape::Rectangle => box_new_shape::<primitives::Rectangle>(start, self.color),
            Shape::Bezier => None, // Not implemented
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

    pub fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        canvas.clear();

        for (i, shape) in self.objects.iter().enumerate() {
            shape.draw(canvas);
            if self.selected == Some(i) {
                let core = shape.get_core();
                for p in core.points {
                    for x in (p.0 - 3)..(p.0 + 3) {
                        for y in (p.1 - 3)..(p.1 + 3) {
                            canvas.set_pixel(x, y, rgba(255, 0, 0, 255));
                        }
                    }
                }
            }
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
