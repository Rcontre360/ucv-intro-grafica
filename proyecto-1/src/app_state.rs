use std::{cmp::min, fs};

use rfd::FileDialog;
use serde::{Deserialize, Serialize};
use winit::keyboard::KeyCode;
use winit::window::CursorIcon;

use crate::{
    canvas::Canvas,
    core::{Point, Shape, ShapeCore, ShapeImpl, UpdateOp, RGBA},
    draw_state::DrawState,
    primitives::bezier::Bezier,
    primitives::new_shape_from_core,
};

// we define our own events to not depend on these libraries.
#[derive(Copy, Clone, PartialEq)]
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
    ToFront(bool),
    ToBack(bool),
    Save,
    Load,
    DegreeElevate,
    Subdivide,
    SubdivisionValue(f32),
    Clear,
    Undo,
    Redo,
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

#[derive(Serialize, Deserialize)]
struct SerializedState {
    objects: Vec<ShapeCore>,
    background_color: RGBA,
}

pub struct AppState {
    pub current: Shape,
    pub cur_shape: Option<Box<dyn ShapeImpl>>,
    pub selected: Option<ShapeSelected>,
    pub ui_subdivision_t: f32,

    draw_state: DrawState,
    color: RGBA,
    fill_color: RGBA,
    points_color: RGBA,
    background_color: RGBA,
}

impl AppState {
    pub fn new() -> Self {
        Self {
            current: Shape::Line,
            color: RGBA::new(255, 255, 255, 255),
            fill_color: RGBA::new(100, 50, 10, 150),
            points_color: RGBA::new(0, 0, 255, 255),
            background_color: RGBA::default(),
            cur_shape: None,
            selected: None,
            draw_state: DrawState::new(),
            ui_subdivision_t: 0.5,
        }
    }

    pub fn get_colors(&self) -> (RGBA, RGBA, RGBA, RGBA) {
        (
            self.color,
            self.fill_color,
            self.points_color,
            self.background_color,
        )
    }

    pub fn gui_update(&mut self, e: GUIEvent) {
        self.update(EventType::GUI(e));
    }

    pub fn keyboard_update(&mut self, key: KeyCode) {
        self.update(EventType::Keyboard(key));
    }

    pub fn mouse_update(&mut self, e: MouseEvent, btn: u8, point: Point) -> CursorIcon {
        return self.update(EventType::Mouse(e, btn, point));
    }

    pub fn update(&mut self, event: EventType) -> CursorIcon {
        match event {
            EventType::GUI(gui_ev) => {
                self.handle_gui_event(gui_ev);
            }
            EventType::Mouse(mouse_ev, btn, point) => {
                if MouseEvent::Click == mouse_ev && btn == 0 {
                    if !self.is_building_bezier() {
                        if let Some(fig) = self.selected.as_ref() {
                            if let Some(point_idx) = self.is_control_point_select(fig.index, point)
                            {
                                self.selected.as_mut().unwrap().set_control_point(point_idx);
                                return CursorIcon::Grab;
                            }
                        }

                        if let Some(fig) = self.is_figure_selection(point) {
                            self.selected = Some(ShapeSelected::new_with_point(fig, point));
                            return CursorIcon::Grab;
                        } else {
                            self.selected = None;
                        }
                    }
                }

                if MouseEvent::PressDrag == mouse_ev && btn == 0 {
                    if let Some(selected) = self.selected.as_mut() {
                        let orig = selected.coord_clicked;
                        if selected.control_point_selected.is_some() {
                            self.update_selected_control_point(point);
                            return CursorIcon::Grabbing;
                        }

                        if orig.is_some() {
                            self.handle_move_selected_shape(orig.unwrap(), point);
                            return CursorIcon::Grabbing;
                        }
                    }
                }

                if MouseEvent::Release == mouse_ev && btn == 0 {
                    if let Some(selected) = self.selected.as_mut() {
                        selected.control_point_selected = None;
                        selected.coord_clicked = None;
                    }
                }
                self.handle_figure_draw(event);
            }
            EventType::Keyboard(key_ev) => {
                self.handle_keyboard_event(key_ev);
            }
        }

        // returns cursor state
        return self.handle_mouse_change(event);
    }

    fn handle_mouse_change(&self, event: EventType) -> CursorIcon {
        if let EventType::Mouse(MouseEvent::Move, 0, point) = event {
            match (
                self.is_building_bezier(),
                self.is_figure_selection(point),
                self.selected.as_ref(),
            ) {
                (false, Some(fig_index), Some(fig_selected)) => {
                    if fig_index == fig_selected.index {
                        return CursorIcon::Pointer;
                    }

                    if self.is_control_point_select(fig_index, point).is_some() {
                        return CursorIcon::Grab;
                    }
                }
                _ => {}
            }
        }

        CursorIcon::Default
    }

    fn handle_keyboard_event(&mut self, event: KeyCode) {
        match event {
            KeyCode::Enter => self.handle_degree_elevate(),
            KeyCode::Delete | KeyCode::Backspace => self.handle_delete_figure(),
            KeyCode::ShiftLeft | KeyCode::ShiftRight => self.handle_shift_drag(),
            _ => {}
        }
    }

    fn handle_gui_event(&mut self, event: GUIEvent) {
        match event {
            GUIEvent::ShapeType(shape) => self.current = shape,
            GUIEvent::PointsColor(c) => self.points_color = c,
            GUIEvent::DegreeElevate => self.handle_degree_elevate(),
            GUIEvent::Subdivide => self.handle_subdivide(),
            GUIEvent::SubdivisionValue(t) => {
                if let Some(selected) = self.selected.as_ref() {
                    self.draw_state
                        .update_shape(selected.index, UpdateOp::UpdateSubdivide(t));
                    self.ui_subdivision_t = t;
                }
            }
            GUIEvent::Save => self.save_state(),
            GUIEvent::Load => self.load_state(),
            GUIEvent::BorderColor(c) => {
                self.color = c;
                if let Some(selected) = self.selected.as_ref() {
                    self.draw_state
                        .update_shape(selected.index, UpdateOp::ChangeColor(c));
                }
            }
            GUIEvent::FillColor(c) => {
                self.fill_color = c;
                if let Some(selected) = self.selected.as_ref() {
                    self.draw_state
                        .update_shape(selected.index, UpdateOp::ChangeFillColor(c));
                }
            }
            GUIEvent::ToFront(all) => {
                if let Some(i) = self.selected.as_ref() {
                    let len = self.draw_state.get_objects().len();
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
            GUIEvent::Clear => {
                // self.objects.clear();
                self.selected = None;
                self.cur_shape = None;
            }
            GUIEvent::Undo => self.draw_state.undo(),
            GUIEvent::Redo => self.draw_state.redo(),
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
                    MouseEvent::PressDrag => self.shape_update_last_point(point),
                    MouseEvent::Release => self.shape_add_control_point(point),
                    MouseEvent::Move => self.shape_update_last_point(point),
                },
                _ => {}
            },
            Shape::Bezier => match event {
                EventType::Mouse(action, button, point) => match action {
                    MouseEvent::Click => {
                        if button == 0 {
                            if self.cur_shape.is_some() {
                                self.shape_add_control_point(point);
                            } else {
                                self.shape_start(point);
                            }
                        } else if self.cur_shape.is_some() {
                            self.shape_end(point);
                        }
                    }
                    MouseEvent::Move => self.shape_update_last_point(point),
                    _ => {}
                },
                _ => {}
            },
            _ => match event {
                EventType::Mouse(action, 0, point) => match action {
                    MouseEvent::Click => self.shape_start(point),
                    MouseEvent::PressDrag => self.shape_update_last_point(point),
                    MouseEvent::Release => self.shape_end(point),
                    _ => {}
                },
                _ => {}
            },
        }
    }

    pub fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        canvas.clear();

        for shape in self.draw_state.get_objects().iter() {
            shape.draw(canvas);
        }

        if let Some(selected) = self.selected.as_ref() {
            let shape = self.draw_state.get_object(selected.index);
            shape.draw_selection(self.points_color, canvas);
        }

        if let Some(cur) = self.cur_shape.as_ref() {
            cur.draw(canvas);
        }
    }

    fn handle_delete_figure(&mut self) {
        if let Some(selected) = self.selected.take() {
            self.draw_state.delete_shape(selected.index);
        }
    }

    fn handle_shift_drag(&mut self) {
        if let Some(selected) = self.selected.as_ref() {
            let obj = self.draw_state.get_object(selected.index);
            let core = obj.get_core();

            if let Shape::Rectangle | Shape::Ellipse = core.shape_type {
                // delta.x != delta.y happens, its not square
                let delta = core.points[1] - core.points[0];
                let min = min(delta.0, delta.1);

                let new_pnt = Point(core.points[0].0 + min, core.points[0].1 + min);

                let op = UpdateOp::ControlPoint(1, new_pnt);
                self.draw_state.update_shape(selected.index, op);
            }
        }
    }

    fn handle_degree_elevate(&mut self) {
        if let Some(selected) = self.selected.as_ref() {
            self.draw_state
                .update_shape(selected.index, UpdateOp::DegreeElevate);
        }
    }

    fn handle_subdivide(&mut self) {
        if let Some(selected) = self.selected.take() {
            self.draw_state.subdivide_shape(selected.index);
        }
    }

    fn reorder_selected(&mut self, new_index: usize) {
        if let Some(selected) = self.selected.as_ref() {
            if selected.index != new_index {
                self.draw_state.reorder_shape(selected.index, new_index);
                self.selected = Some(ShapeSelected::new(new_index));
            }
        }
    }

    fn handle_move_selected_shape(&mut self, origin: Point, end: Point) {
        if let Some(selected) = self.selected.as_ref() {
            let delta = end - origin;

            let op = UpdateOp::Move(delta);
            self.draw_state.update_shape(selected.index, op);

            if let Some(selected) = self.selected.as_mut() {
                selected.coord_clicked = Some(end);
            }
        }
    }

    fn is_figure_selection(&self, pt: Point) -> Option<usize> {
        for (i, object) in self.draw_state.get_objects().iter().enumerate().rev() {
            if object.hit_test(pt) {
                return Some(i);
            }
        }
        None
    }

    //TODO finish this
    fn is_building_bezier(&self) -> bool {
        false
    }

    fn is_control_point_select(&self, fig: usize, target: Point) -> Option<usize> {
        let object = self.draw_state.get_object(fig);
        for (i, p) in object.get_core().points.iter().enumerate() {
            if (target.0 - p.0).pow(2) + (target.1 - p.1).pow(2) <= 5i32.pow(2) {
                return Some(i);
            }
        }
        None
    }

    fn shape_start(&mut self, start: Point) {
        let points = vec![start, start];
        let core = ShapeCore {
            points,
            color: self.color,
            fill_color: self.fill_color,
            shape_type: self.current,
        };
        self.cur_shape = Some(new_shape_from_core(core));
    }

    fn shape_add_control_point(&mut self, nxt: Point) {
        if let Some(cur) = self.cur_shape.as_mut() {
            cur.update(&UpdateOp::AddControlPoint(nxt));
        }
    }

    fn shape_update_last_point(&mut self, nxt: Point) {
        if let Some(cur) = self.cur_shape.as_mut() {
            let last_point = cur.get_core().points.len() - 1;
            cur.update(&UpdateOp::ControlPoint(last_point, nxt));
        }
    }

    fn shape_end(&mut self, end: Point) {
        if let Some(mut cur) = self.cur_shape.take() {
            let last_point = cur.get_core().points.len() - 1;
            cur.update(&UpdateOp::ControlPoint(last_point, end));
            self.draw_state.add_shape(cur);
        }
    }

    fn update_selected_control_point(&mut self, point: Point) {
        if let Some(selected) = self.selected.as_ref() {
            if let Some(pnt_idx) = selected.control_point_selected {
                let op = UpdateOp::ControlPoint(pnt_idx, point);
                self.draw_state.update_shape(selected.index, op);
            }
        }
    }

    fn save_state(&self) {
        let mut core_arr = vec![];
        for shape in self.draw_state.get_objects().iter() {
            core_arr.push(shape.get_core());
        }

        let saved_state = SerializedState {
            objects: core_arr,
            background_color: self.background_color,
        };

        let state_str = serde_json::to_string_pretty(&saved_state).unwrap();
        if let Some(path) = FileDialog::new()
            .set_title("Save drawing")
            .set_file_name("drawing.json")
            .add_filter("JSON Files", &["json"])
            .save_file()
        {
            fs::write(path, state_str).unwrap();
        }
    }

    fn load_state(&mut self) {
        if let Some(path) = FileDialog::new()
            .set_title("Open draw")
            .add_filter("JSON Files", &["json"])
            .pick_file()
        {
            let state_str = fs::read_to_string(&path).unwrap();
            let loaded_state: SerializedState = serde_json::from_str(&state_str).unwrap();

            for core in loaded_state.objects {
                let boxed_shape = new_shape_from_core(core);
                self.draw_state.add_shape(boxed_shape);
            }
            self.background_color = loaded_state.background_color;
        }
    }
}
