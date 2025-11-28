use winit::keyboard::KeyCode;

use crate::{
    canvas::Canvas,
    core::{Point, Shape, ShapeCore, ShapeImpl, UpdateOp, RGBA},
    primitives,
};

// we define our own events to not depend on these libraries.
#[derive(Copy, Clone)]
pub enum MouseEvent {
    Click,
    Move,
    PressDrag,
    Release,
}

#[derive(Copy, Clone)]
pub enum GUIEvent {
    ShapeType(Shape),
    BorderColor(RGBA),
    FillColor(RGBA),
    PointsColor(RGBA),
    Subdivide,
    ToFront(bool),
    ToBack(bool),
}

#[derive(Copy, Clone)]
pub enum EventType {
    Mouse(MouseEvent, u8, Point),
    Keyboard(KeyCode),
    GUI(GUIEvent),
}

pub struct ShapeSelected {
    pub index: usize,
    pub control_point_selected: Option<usize>,
    pub coord_clicked: Option<Point>,
}

impl ShapeSelected {
    pub fn new(index: usize) -> Self {
        ShapeSelected {
            index,
            control_point_selected: None,
            coord_clicked: None,
        }
    }

    pub fn new_with_point(index: usize, click: Point) -> Self {
        ShapeSelected {
            index,
            control_point_selected: None,
            coord_clicked: Some(click),
        }
    }

    pub fn set_control_point(&mut self, ptn: usize) {
        self.control_point_selected = Some(ptn);
    }
}

pub struct State {
    pub current: Shape,
    pub cur_shape: Option<Box<dyn ShapeImpl>>,

    pub selected: Option<ShapeSelected>,

    objects: Vec<Box<dyn ShapeImpl>>,
    color: RGBA,
    fill_color: RGBA,
    points_color: RGBA,
}

impl State {
    pub fn new() -> Self {
        Self {
            current: Shape::Triangle,
            color: RGBA::new(255, 255, 255, 255),
            fill_color: RGBA::new(100, 50, 10, 150),
            points_color: RGBA::new(0, 0, 255, 255),
            cur_shape: None,
            objects: vec![],
            selected: None,
        }
    }

    pub fn get_colors(&self) -> (RGBA, RGBA, RGBA) {
        (self.color, self.fill_color, self.points_color)
    }

    pub fn gui_update(&mut self, e: GUIEvent) {
        self.update(EventType::GUI(e));
    }

    pub fn mouse_update(&mut self, e: MouseEvent, btn: u8, point: Point) {
        self.update(EventType::Mouse(e, btn, point));
    }

    pub fn keyboard_update(&mut self, key: KeyCode) {
        self.update(EventType::Keyboard(key));
    }

    pub fn update(&mut self, event: EventType) {
        //handle subdivide with enter
        if let EventType::Keyboard(KeyCode::Enter) = event {
            self.handle_bezier_subdivide();
        }

        //checking if event is a normal figure selection
        if let EventType::Mouse(MouseEvent::Click, 0, point) = event {
            if !self.is_building_bezier() {
                // if we have a selected figure and we're hitting its control points
                // also if we are dragging that figure
                if let Some(fig) = self.selected.as_ref() {
                    if let Some(point_idx) = self.is_control_point_select(fig.index, point) {
                        // this needs to be done here and not on the "fig" variable because
                        // above we are borrowing an inmutable reference and we can read as we want
                        // from it but not mutate it. Here we mutate so we need to "borrow" again
                        self.selected.as_mut().unwrap().set_control_point(point_idx);
                        return;
                    }
                }

                // if we are selecting a figure
                if let Some(fig) = self.is_figure_selection(point) {
                    self.selected = Some(ShapeSelected::new_with_point(fig, point));
                    return;
                } else {
                    self.selected = None;
                }
            }
        }

        // if we are dragging and control point is selected
        if let EventType::Mouse(MouseEvent::PressDrag, 0, point) = event {
            if let Some(selected) = self.selected.as_mut() {
                let orig = selected.coord_clicked;
                if selected.control_point_selected.is_some() {
                    self.update_selected_control_point(point);
                    return;
                }

                if orig.is_some() {
                    self.handle_move_selected_shape(orig.unwrap(), point);
                }
            }
        }

        if let EventType::Mouse(MouseEvent::Release, 0, _) = event {
            // we stopped moving control point
            if let Some(selected) = self.selected.as_mut() {
                selected.control_point_selected = None;
                selected.coord_clicked = None;
            }
        }

        // if none of the above are true then we are drawing something
        self.handle_figure_draw(event);
        self.handle_gui_event(event);
    }

    fn handle_gui_event(&mut self, event: EventType) {
        match event {
            EventType::GUI(gui_ev) => match gui_ev {
                GUIEvent::ShapeType(shape) => self.current = shape,
                GUIEvent::BorderColor(c) => self.color = c,
                GUIEvent::FillColor(c) => self.fill_color = c,
                GUIEvent::PointsColor(c) => self.points_color = c,
                GUIEvent::Subdivide => self.handle_bezier_subdivide(),
                GUIEvent::ToFront(all) => {
                    if let Some(i) = self.selected.as_ref() {
                        let len = self.objects.len();
                        let target_index = if all {
                            len - 1
                        } else {
                            (i.index + 1).min(len - 1)
                        };
                        self.reorder_selected(target_index);
                    }
                }
                GUIEvent::ToBack(all) => {
                    if let Some(i) = self.selected.as_ref() {
                        let target_index = if all { 0 } else { i.index.saturating_sub(1) };
                        self.reorder_selected(target_index);
                    }
                }
            },
            _ => {}
        };
    }

    fn handle_figure_draw(&mut self, event: EventType) {
        match self.current {
            Shape::Triangle => match event {
                EventType::Mouse(action, 0, point) => match action {
                    MouseEvent::Click => {
                        if self.cur_shape.is_some() {
                            self.shape_end(point);
                        } else {
                            self.shape_start(point);
                        }
                    }
                    MouseEvent::PressDrag => {
                        self.shape_update_last_point(point);
                    }
                    MouseEvent::Release => {
                        self.shape_add_control_point(point);
                    }
                    MouseEvent::Move => {
                        self.shape_update_last_point(point);
                    }
                    _ => {}
                },
                _ => {}
            }, // Not implemented
            Shape::Bezier => match event {
                EventType::Mouse(action, button, point) => match action {
                    MouseEvent::Click => {
                        // left button == 0
                        if button == 0 {
                            // we add control points with each click
                            if self.cur_shape.is_some() {
                                self.shape_add_control_point(point);
                            } else {
                                self.shape_start(point);
                            }
                        } else if self.cur_shape.is_some() {
                            self.shape_end(point);
                        }
                    }
                    MouseEvent::Move => {
                        self.shape_update_last_point(point);
                    }
                    _ => {}
                },
                _ => {}
            },

            _ => match event {
                EventType::Mouse(action, 0, point) => match action {
                    MouseEvent::Click => {
                        self.shape_start(point);
                    }
                    MouseEvent::PressDrag => {
                        self.shape_update_last_point(point);
                    }
                    MouseEvent::Release => {
                        self.shape_end(point);
                    }
                    _ => {}
                },
                _ => {}
            },
        }
    }

    pub fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        canvas.clear();

        // objects are drawn in order, so the last one is on top (front).
        for shape in self.objects.iter() {
            shape.draw(canvas);
        }

        if let Some(selected) = self.selected.as_ref() {
            let shape = self.objects.get(selected.index).unwrap();
            shape.draw_selection(self.points_color, canvas);
        }

        if let Some(cur) = self.cur_shape.as_ref() {
            cur.draw(canvas);
        }
    }

    fn handle_bezier_subdivide(&mut self) {
        if let Some(selected) = self.selected.as_ref() {
            if let Some(object) = self.objects.get_mut(selected.index) {
                let op = UpdateOp::DegreeElevate;
                object.as_mut().update(&op);
            }
        }
    }

    fn reorder_selected(&mut self, new_index: usize) {
        if let Some(selected) = self.selected.as_ref() {
            if selected.index != new_index && new_index < self.objects.len() {
                let object = self.objects.remove(selected.index);
                self.objects.insert(new_index, object);
                self.selected = Some(ShapeSelected::new(new_index));
            }
        }
    }

    fn handle_move_selected_shape(&mut self, origin: Point, end: Point) {
        if let Some(selected) = self.selected.as_mut() {
            if let Some(shape) = self.objects.get_mut(selected.index) {
                let delta = end - origin;
                let op = UpdateOp::Move { delta };
                shape.as_mut().update(&op);
                selected.coord_clicked = Some(end);
            }
        }
    }

    fn is_figure_selection(&self, pt: Point) -> Option<usize> {
        // Iterate backwards to select the topmost (last drawn) figure first
        for (i, object) in self.objects.iter().enumerate().rev() {
            if object.hit_test(pt) {
                return Some(i);
            }
        }
        None
    }

    fn is_building_bezier(&self) -> bool {
        // This function should probably check if self.current is Shape::Bezier AND self.cur_shape is Some
        // However, based on the original code, I will keep it returning false to match the provided logic.
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

    fn shape_start(&mut self, start: Point) {
        self.cur_shape = match self.current {
            Shape::Line => box_new_shape::<primitives::Line>(start, (self.color, self.fill_color)),
            Shape::Ellipse => {
                box_new_shape::<primitives::Ellipse>(start, (self.color, self.fill_color))
            }
            Shape::Triangle => {
                box_new_shape::<primitives::Triangle>(start, (self.color, self.fill_color))
            }
            Shape::Rectangle => {
                box_new_shape::<primitives::Rectangle>(start, (self.color, self.fill_color))
            }
            Shape::Bezier => {
                box_new_shape::<primitives::Bezier>(start, (self.color, self.fill_color))
            }
        };
    }

    fn shape_add_control_point(&mut self, nxt: Point) {
        if let Some(cur) = self.cur_shape.as_mut() {
            let op = UpdateOp::AddControlPoint { point: nxt };
            cur.as_mut().update(&op);
        }
    }

    fn shape_update_last_point(&mut self, nxt: Point) {
        if let Some(cur) = self.cur_shape.as_mut() {
            let last_point = cur.get_core().points.len() - 1;
            let op = UpdateOp::ControlPoint {
                index: last_point,
                point: nxt,
            };
            cur.as_mut().update(&op);
        }
    }

    fn shape_end(&mut self, end: Point) {
        // "take" function already does self.cur_shape = None
        if let Some(cur) = self.cur_shape.take() {
            self.shape_update_last_point(end);
            self.objects.push(cur);
        }
    }

    fn update_selected_control_point(&mut self, point: Point) {
        if let Some(selected) = self.selected.as_ref() {
            if let Some(object) = self.objects.get_mut(selected.index) {
                let op = UpdateOp::ControlPoint {
                    index: selected.control_point_selected.unwrap(),
                    point,
                };
                object.as_mut().update(&op);
            }
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

fn draw_control_points(points: Vec<Point>, color: RGBA, canvas: &mut Canvas) {
    // drawing control points
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
