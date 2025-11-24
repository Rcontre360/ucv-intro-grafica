use crate::{
    canvas::Canvas,
    primitives,
    primitives::core::{rgba, Point, Shape, ShapeCore, ShapeImpl, UpdateOp, RGBA},
};

pub struct State {
    pub current: Shape,
    pub color: RGBA,
    pub cur_shape: Option<Box<dyn ShapeImpl>>,
    objects: Vec<Box<dyn ShapeImpl>>,
    pub selected: Option<usize>,
    pub dragging: Option<usize>,
}

impl State {
    pub fn new() -> Self {
        Self {
            current: Shape::Line,
            color: rgba(255, 255, 255, 255),
            cur_shape: None,
            objects: vec![],
            selected: None,
            dragging: None,
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

    pub fn hit_test_control_points(&self, pt: Point) -> Option<usize> {
        if let Some(selected_index) = self.selected {
            if let Some(object) = self.objects.get(selected_index) {
                for (i, p) in object.get_core().points.iter().enumerate() {
                    if (pt.0 - p.0).pow(2) + (pt.1 - p.1).pow(2) <= 5i32.pow(2) {
                        return Some(i);
                    }
                }
            }
        }
        None
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
            Shape::Bezier => box_new_shape::<primitives::Bezier>(start, self.color),
        };
    }

    pub fn update_current_shape(&mut self, end: Point) {
        if let Some(mut cur) = self.cur_shape.take() {
            let op = UpdateOp::ControlPoint {
                index: 1,
                point: end,
            };
            cur.as_mut().update(&op);
            self.cur_shape = Some(cur);
        }
    }

    pub fn end_current_shape(&mut self, end: Point) {
        if let Some(cur) = self.cur_shape.take() {
            self.update_current_shape(end);
            self.objects.push(cur);
        }
    }

    pub fn subdivide_selected(&mut self) {
        if let Some(selected_index) = self.selected {
            if let Some(object) = self.objects.get_mut(selected_index) {
                let op = UpdateOp::DegreeElevate;
                object.as_mut().update(&op);
            }
        }
    }

    pub fn update_dragged_control_point(&mut self, point: Point) {
        if let Some(dragging_index) = self.dragging {
            if let Some(selected_index) = self.selected {
                if let Some(object) = self.objects.get_mut(selected_index) {
                    let op = UpdateOp::ControlPoint {
                        index: dragging_index,
                        point,
                    };
                    object.as_mut().update(&op);
                }
            }
        }
    }

    pub fn update(&mut self) {}

    pub fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        canvas.clear();

        for (i, shape) in self.objects.iter().enumerate() {
            shape.draw(canvas);

            if self.selected == Some(i) {
                let core = shape.get_core();

                // drawing control points
                for p in core.points {
                    for x in (p.0 - 5)..(p.0 + 5) {
                        for y in (p.1 - 5)..(p.1 + 5) {
                            if (x - p.0).pow(2) + (y - p.1).pow(2) <= 5i32.pow(2) {
                                canvas.set_pixel(x, y, rgba(255, 255, 255, 255));
                            }
                        }
                    }

                    for x in (p.0 - 4)..(p.0 + 4) {
                        for y in (p.1 - 4)..(p.1 + 4) {
                            if (x - p.0).pow(2) + (y - p.1).pow(2) <= 4i32.pow(2) {
                                canvas.set_pixel(x, y, rgba(255, 0, 0, 255));
                            }
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
