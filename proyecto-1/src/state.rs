use winit::keyboard::KeyCode;

use crate::{
    canvas::Canvas,
    primitives,
    primitives::core::{rgba, Point, Shape, ShapeCore, ShapeImpl, UpdateOp, RGBA},
};

// we define our own events to not depend on these libraries.
pub enum MouseAction {
    Click,
    PressDrag,
    Release,
}

pub enum EventType {
    Mouse(MouseAction, u8),
    Keyboard(KeyCode),
}

pub struct State {
    pub current: Shape,
    pub cur_shape: Option<Box<dyn ShapeImpl>>,

    objects: Vec<Box<dyn ShapeImpl>>,
    pub selected: Option<usize>,
    pub dragging: Option<usize>,

    pub color: RGBA,
    pub fill_color: RGBA,
    pub points_color: RGBA,
}

impl State {
    pub fn new() -> Self {
        Self {
            current: Shape::Line,
            color: rgba(255, 255, 255, 255),
            fill_color: rgba(0, 0, 0, 0),
            points_color: rgba(0, 0, 255, 255),
            cur_shape: None,
            objects: vec![],
            selected: None,
            dragging: None,
        }
    }

    pub fn update(&mut self, event: EventType, point: Point) {
        //handle subdivide with enter
        if let EventType::Keyboard(KeyCode::Enter) = event {
            self.subdivide_selected();
        }

        //checking if event is a normal figure selection
        if let EventType::Mouse(MouseAction::Click, 0) = event {
            if !self.is_building_bezier() {
                // if we have a selected figure and we're hitting its control points
                if let Some(fig) = self.selected {
                    if let Some(point_idx) = self.is_control_point_select(fig, point) {
                        self.dragging = Some(point_idx);
                        return;
                    }
                }

                // if we are selecting a figure
                if let Some(fig) = self.is_figure_selection(point) {
                    self.selected = Some(fig);
                    return;
                }
            }
        }

        // if we are dragging and control point is selected
        if let EventType::Mouse(MouseAction::PressDrag, 0) = event {
            if self.dragging.is_some() {
                self.update_selected_control_point(point);
            }
        }

        if let EventType::Mouse(MouseAction::Release, 0) = event {
            // we stopped moving control point
            if self.dragging.is_some() {
                self.dragging = None;
            }
        }

        // if none of the above are true then we are drawing something
        match self.current {
            Shape::Triangle => {} // Not implemented
            Shape::Bezier => {}

            _ => match event {
                EventType::Mouse(action, button) => {
                    // left mouse button events only
                    if button == 1 {
                        return;
                    }

                    match action {
                        MouseAction::Click => {
                            self.start_current_shape(point);
                        }
                        MouseAction::PressDrag => {
                            self.update_current_shape(point);
                        }
                        MouseAction::Release => {
                            self.end_current_shape(point);
                        }
                    }
                }
                _ => {}
            },
        }
    }

    fn is_figure_selection(&self, pt: Point) -> Option<usize> {
        for (i, object) in self.objects.iter().enumerate() {
            if object.hit_test(pt) {
                return Some(i);
            }
        }
        None
    }

    fn is_building_bezier(&self) -> bool {
        false
    }

    fn is_control_point_select(&self, fig: usize, target: Point) -> Option<usize> {
        if let Some(object) = self.objects.get(fig) {
            for (i, p) in object.get_core().points.iter().enumerate() {
                if (target.0 - p.0).pow(2) + (target.1 - p.1).pow(2) <= 5i32.pow(2) {
                    return Some(i);
                }
            }
        }
        None
    }

    fn start_current_shape(&mut self, start: Point) {
        self.cur_shape = match self.current {
            Shape::Line => box_new_shape::<primitives::Line>(start, (self.color, self.fill_color)),
            Shape::Ellipse => {
                box_new_shape::<primitives::Ellipse>(start, (self.color, self.fill_color))
            }
            Shape::Triangle => None, // Not implemented
            Shape::Rectangle => {
                box_new_shape::<primitives::Rectangle>(start, (self.color, self.fill_color))
            }
            Shape::Bezier => {
                box_new_shape::<primitives::Bezier>(start, (self.color, self.fill_color))
            }
        };
    }

    fn update_current_shape(&mut self, end: Point) {
        if let Some(mut cur) = self.cur_shape.take() {
            let op = UpdateOp::ControlPoint {
                index: 1,
                point: end,
            };
            cur.as_mut().update(&op);
            self.cur_shape = Some(cur);
        }
    }

    fn end_current_shape(&mut self, end: Point) {
        if let Some(cur) = self.cur_shape.take() {
            self.update_current_shape(end);
            self.objects.push(cur);
        }
    }

    fn subdivide_selected(&mut self) {
        if let Some(selected_index) = self.selected {
            if let Some(object) = self.objects.get_mut(selected_index) {
                let op = UpdateOp::DegreeElevate;
                object.as_mut().update(&op);
            }
        }
    }

    fn update_selected_control_point(&mut self, point: Point) {
        if let Some(selected_index) = self.selected {
            if let Some(object) = self.objects.get_mut(selected_index) {
                let op = UpdateOp::ControlPoint {
                    index: self.dragging.unwrap(),
                    point,
                };
                object.as_mut().update(&op);
            }
        }
    }

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

fn box_new_shape<T>(start: Point, colors: (RGBA, RGBA)) -> Option<Box<dyn ShapeImpl>>
where
    T: ShapeImpl + Sized + 'static,
{
    let points = vec![start, start];
    let core = ShapeCore {
        points,
        color: colors.0,
        fill_color: colors.1,
    };
    Some(Box::new(T::new(core)))
}
