use std::cmp::{max, min};

use rfd::FileDialog;
use winit::keyboard::KeyCode;
use winit::window::CursorIcon;

/// We try to avoid using PIXELS package to have this state management library agnostic.
/// The only dependency is winit and rfd, winit can be easily replaced by enums since we only use
/// some enums definitions from it
use crate::{
    canvas::Canvas,
    core::{Point, Shape, ShapeCore, ShapeImpl, UpdateOp, RGBA},
    draw_state::DrawState,
    primitives::new_shape_from_core,
};

/// here we dont use winit mouse events. We use our own. If this was a real app, this would make
/// the app library agnostic, which is better for third party integrations
#[derive(Copy, Clone, PartialEq)]
pub enum MouseEvent {
    Click,
    Move,
    PressDrag,
    Release,
}

/// These are user interface events definitions. They trigger actions on the app state
#[derive(Copy, Clone)]
pub enum GUIEvent {
    /// change of shape type
    ShapeType(Shape),
    /// change of border color
    BorderColor(RGBA),
    /// change of fill color
    FillColor(RGBA),
    /// change of control points color. Is global
    PointsColor(RGBA),
    /// change on background color
    BackgroundColor(RGBA),
    /// The toFront button was clicked. If its argument is true we move the shape all the way to
    /// the front
    ToFront(bool),
    /// The toBack button was clicked. If its argument is true we move the shape all the way to
    /// the back
    ToBack(bool),
    /// save button clicked
    Save,
    /// load button clicked
    Load,
    /// degree elevate button clicked
    DegreeElevate,
    /// subdivide button clicked
    Subdivide,
    /// change of control polygon color
    ControlPolygonColor(RGBA),
    /// subdivision value changed
    SubdivisionValue(f32),
    /// clear button clicked
    Clear,
    /// undo button clicked
    Undo,
    /// redo button clicked
    Redo,
}

/// aggregates the different events that update the app state
#[derive(Copy, Clone)]
pub enum EventType {
    /// mouse event definition. has as argument the type, the mouse button (u8) and the point in
    /// screen where it happened
    Mouse(MouseEvent, u8, Point),
    /// keyboard event, bolean to know if its pressed or released
    Keyboard(KeyCode, bool),
    /// gui event
    GUI(GUIEvent),
}

/// current shape selected. Since we might as well be selecting a control point we encapsulate that
/// as well. We might also be moving the shape, on which case we need to store the point where we
/// clicked and we are moving from. Encapsulated for ease of use
/// we only store the index of the shape and control point
pub struct ShapeSelected {
    /// The index of the selected shape in the `objects` vector of `DrawState`.
    pub index: usize,
    /// The index of the selected control point, if any.
    pub control_point_selected: Option<usize>,
    /// The coordinate where the user clicked to start moving the shape.
    pub coord_clicked: Option<Point>,
}

impl ShapeSelected {
    /// Creates a new `ShapeSelected` with just the index of the shape.
    pub fn new(index: usize) -> Self {
        ShapeSelected {
            index,
            control_point_selected: None,
            coord_clicked: None,
        }
    }

    /// creates a new shape with a selected point
    pub fn new_with_point(index: usize, click: Point) -> Self {
        ShapeSelected {
            index,
            control_point_selected: None,
            coord_clicked: Some(click),
        }
    }

    /// we set a control point selection
    pub fn set_control_point(&mut self, ptn: usize) {
        self.control_point_selected = Some(ptn);
    }
}

/// app state. holds and coordinates every update on the app
pub struct AppState {
    /// current is the current TYPE of shape we are drawing
    pub current: Shape,
    /// cur_shape is the current shape we are drawing and not releasing yet
    pub cur_shape: Option<Box<dyn ShapeImpl>>,
    /// selected is the current selected shape. see above
    pub selected: Option<ShapeSelected>,
    /// ui_subdivision_t is the current "t" UI value for subdivision
    pub ui_subdivision_t: f32,

    /// checks if shift is being pressed
    shift_pressed: bool,
    /// draw state holds the shapes and what is rendered. Every object that should be saved to a
    /// file or that should be stored on the event queue is here
    draw_state: DrawState,
    /// color of border
    color: RGBA,
    /// color of filling
    fill_color: RGBA,
    /// control points global color
    points_color: RGBA,
    /// color of a selected shape. Cannot be changed
    selection_color: RGBA,
    /// bezier control polygon color
    bezier_control_polygon_color: RGBA,
}

impl AppState {
    /// initializes app state
    pub fn new() -> Self {
        Self {
            current: Shape::Line,
            color: RGBA::new(255, 255, 255, 200),
            fill_color: RGBA::new(100, 50, 10, 0),
            points_color: RGBA::new(255, 80, 80, 255),
            bezier_control_polygon_color: RGBA::new(255, 80, 80, 255),
            selection_color: RGBA::new(80, 80, 250, 255),
            draw_state: DrawState::new(),
            ui_subdivision_t: 0.5,
            shift_pressed: false,
            cur_shape: None,
            selected: None,
        }
    }

    /// returns the shape type of the selected shape. useful for the UI
    pub fn get_selected_shape_type(&self) -> Option<Shape> {
        self.selected
            .as_ref()
            .map(|s| self.draw_state.get_object(s.index).get_core().shape_type)
    }

    /// returns all the color related fields on the UI
    pub fn get_colors(&self) -> (RGBA, RGBA, RGBA, RGBA, RGBA) {
        (
            self.color,
            self.fill_color,
            self.points_color,
            self.bezier_control_polygon_color,
            self.draw_state.get_background_color(),
        )
    }

    /// handles a GUI update. Used to avoid bloating the code with event definition wrapping.
    /// Same reason for "keyboard_update" and "mouse_update"
    pub fn gui_update(&mut self, e: GUIEvent) {
        self.update(EventType::GUI(e));
    }

    /// handles keyboard update
    pub fn keyboard_update(&mut self, key: KeyCode, is_pressed: bool) {
        self.update(EventType::Keyboard(key, is_pressed));
    }

    /// handles mouse update
    pub fn mouse_update(&mut self, e: MouseEvent, btn: u8, point: Point) -> CursorIcon {
        return self.update(EventType::Mouse(e, btn, point));
    }

    /// Handles a generic update. Every update on the state goes through here, no other public
    /// function can update the internal state. This design decision
    /// came because I needed a single place where everything is happening, this way its easier to
    /// debug. Also the same event can trigger multiple updates, so we need to handle them in the
    /// same place.
    /// This function also returns how the cursor should look like
    pub fn update(&mut self, event: EventType) -> CursorIcon {
        // this is a match. Is similar to a switch but handles more detail over the data
        // comparison
        match event {
            // gui events only
            EventType::GUI(gui_ev) => {
                self.handle_gui_event(gui_ev);
            }
            // mouse events only
            EventType::Mouse(mouse_ev, btn, point) => {
                if MouseEvent::Click == mouse_ev && btn == 0 {
                    if !self.is_building_bezier() {
                        if let Some(fig) = self.selected.as_ref() {
                            // if we fall on this condition, it means we are selecting a control point
                            if let Some(point_idx) = self.is_control_point_select(fig.index, point)
                            {
                                self.selected.as_mut().unwrap().set_control_point(point_idx);
                                return CursorIcon::Grab;
                            }
                        }

                        // if we fall on this condition, it means we are selecting a shape
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
                        // if we fall on this condition it means we are moving a control point
                        if selected.control_point_selected.is_some() {
                            self.update_selected_control_point(point);
                            return CursorIcon::Grabbing;
                        }

                        // if we fall on this condition we are moving a shape
                        if orig.is_some() {
                            self.handle_move_selected_shape(orig.unwrap(), point);
                            return CursorIcon::Grabbing;
                        }
                    }
                }

                // if we didnt return before it means that we are drawing a shape
                self.handle_figure_draw(event);
            }
            // only keyboard events
            EventType::Keyboard(key_ev, is_pressed) => {
                self.handle_keyboard_event(key_ev, is_pressed);
            }
        }

        // here we should only care about updating the cursor type
        return self.handle_mouse_change(event);
    }

    /// decides which mouse type to use at the end of update if we didnt returned before
    fn handle_mouse_change(&self, event: EventType) -> CursorIcon {
        if let EventType::Mouse(MouseEvent::Move, 0, point) = event {
            match (
                self.is_building_bezier(),
                self.is_figure_selection(point),
                self.selected.as_ref(),
            ) {
                // we fall here if:
                // "is_building_bezier" == false
                // we are NOT hovering over a shape
                // self.selected is not null (a shape is selected). Since control points dont
                // appear unless a shape is selected.
                // so if we are not building a bezier curve and we hover over a shape, we have a
                // pointer cursor
                (false, None, Some(selected)) => {
                    // if we are over a control point of the selected shape, then we change the cursor
                    if self
                        .is_control_point_select(selected.index, point)
                        .is_some()
                    {
                        return CursorIcon::Pointer;
                    }
                }
                // we fall here if:
                // "is_building_bezier" == false
                // the point we are hovering has a shape
                (false, Some(_), _) => {
                    return CursorIcon::Pointer;
                }
                _ => {}
            }
        }

        CursorIcon::Default
    }

    ///handles keyboard events given the key pressed
    fn handle_keyboard_event(&mut self, event: KeyCode, is_pressed: bool) {
        match (event, is_pressed) {
            // degree elevate for bezier. YES! you can elevate its degree with only enter
            (KeyCode::Enter, true) => self.handle_degree_elevate(),
            // delete a shape with delete and backspace
            (KeyCode::Delete | KeyCode::Backspace, true) => self.handle_delete_figure(),
            // we do that weird requirement where we transform ellipses and rectangles on shift
            (KeyCode::ShiftLeft | KeyCode::ShiftRight, is_pressed) => {
                if self.shift_pressed != is_pressed {
                    self.shift_pressed = is_pressed;
                }
            }
            _ => {}
        }
    }

    /// handles a GUI event
    fn handle_gui_event(&mut self, event: GUIEvent) {
        match event {
            GUIEvent::ShapeType(shape) => self.current = shape,
            GUIEvent::PointsColor(c) => self.points_color = c,
            GUIEvent::ControlPolygonColor(c) => self.bezier_control_polygon_color = c,
            GUIEvent::BackgroundColor(c) => self.draw_state.change_background_color(c),
            GUIEvent::DegreeElevate => self.handle_degree_elevate(),
            GUIEvent::Subdivide => self.handle_subdivide(),
            GUIEvent::SubdivisionValue(t) => {
                // updates subdivide value if a shape is selected
                if let Some(selected) = self.selected.as_ref() {
                    self.draw_state
                        .update_shape(selected.index, UpdateOp::UpdateSubdivide(t));
                    self.ui_subdivision_t = t;
                }
            }
            GUIEvent::Save => self.save_state(),
            GUIEvent::Load => self.load_state(),
            // updates the border color if a shape is selected
            GUIEvent::BorderColor(c) => {
                self.color = c;
                if let Some(selected) = self.selected.as_ref() {
                    self.draw_state
                        .update_shape(selected.index, UpdateOp::ChangeColor(c));
                }
            }
            // updates the fill color if a shape is selected
            GUIEvent::FillColor(c) => {
                self.fill_color = c;
                if let Some(selected) = self.selected.as_ref() {
                    self.draw_state
                        .update_shape(selected.index, UpdateOp::ChangeFillColor(c));
                }
            }
            // moves a shape if selected
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
            // moves a shape if selected
            GUIEvent::ToBack(all) => {
                if let Some(i) = self.selected.as_ref() {
                    let target_index = if all { 0 } else { i.index.saturating_sub(1) };
                    self.reorder_selected(target_index);
                }
            }
            GUIEvent::Clear => {
                self.draw_state.clear();
                self.selected = None;
                self.cur_shape = None;
            }
            GUIEvent::Undo => {
                // must do since we might have selected a shape that will
                // disappear
                self.selected = None;
                self.draw_state.undo();
            }
            GUIEvent::Redo => {
                // edge case happens when you delete a shape, create another one
                // and then undo, select the deleted and redo. This triggers the same undo error
                self.selected = None;
                self.draw_state.redo()
            }
        };
    }

    /// Handles the drawing of a shape based on the current shape type and mouse events.
    /// This function is responsible for starting, updating, and ending the shape creation process.
    /// Since Triangles and Bezier curves are created with a different set of events (2 clicks, n
    /// clicks) we have to check which shape is being created before reacting to events
    fn handle_figure_draw(&mut self, event: EventType) {
        match self.current {
            Shape::Triangle => match event {
                EventType::Mouse(action, 0, point) => match action {
                    // triangle drawing reacts to 2 clicks
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
                    // bezier drawing reacts to n clicks untl right click is done
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
            // the other shapes behave all the same
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

    /// Draws all the shapes on the canvas.
    /// It clears the canvas, then draws all the shapes from the `draw_state`.
    /// If a shape is selected, it's drawn with a selection highlight.
    /// Finally, it draws the shape currently being created, if any.
    pub fn draw<'a>(&self, canvas: &mut Canvas<'a>) {
        canvas.clear(self.draw_state.get_background_color());

        for (id, shape) in self.draw_state.get_objects().iter().enumerate() {
            if self.selected.as_ref().is_some() && self.selected.as_ref().unwrap().index == id {
                shape.draw_with_color(self.selection_color, canvas);
                shape.draw_selection(self.points_color, self.bezier_control_polygon_color, canvas);
            } else {
                shape.draw(canvas);
            }
        }

        if let Some(cur) = self.cur_shape.as_ref() {
            cur.draw(canvas);
        }
    }

    /// Deletes the currently selected figure.
    fn handle_delete_figure(&mut self) {
        if let Some(selected) = self.selected.take() {
            self.draw_state.delete_shape(selected.index);
        }
    }

    /// Elevates the degree of the currently selected Bezier curve.
    fn handle_degree_elevate(&mut self) {
        if let Some(selected) = self.selected.as_ref() {
            self.draw_state
                .update_shape(selected.index, UpdateOp::DegreeElevate);
        }
    }

    /// Subdivides the currently selected shape.
    fn handle_subdivide(&mut self) {
        if let Some(selected) = self.selected.take() {
            self.draw_state.subdivide_shape(selected.index);
        }
    }

    /// Reorders the selected shape to a new index in the `objects` vector.
    fn reorder_selected(&mut self, new_index: usize) {
        if let Some(selected) = self.selected.as_ref() {
            if selected.index != new_index {
                self.draw_state.reorder_shape(selected.index, new_index);
                self.selected = Some(ShapeSelected::new(new_index));
            }
        }
    }

    /// Moves the selected shape based on the drag origin and current mouse position.
    fn handle_move_selected_shape(&mut self, origin: Point, end: Point) {
        if let Some(selected) = self.selected.as_ref() {
            // how far we moved
            let delta = end - origin;

            // this condition avoids having a drag event after selection (normal click)
            if delta == Point(0, 0) {
                return;
            }

            // move operation must take the delta
            let op = UpdateOp::Move(delta);
            self.draw_state.update_shape(selected.index, op);

            if let Some(selected) = self.selected.as_mut() {
                selected.coord_clicked = Some(end);
            }
        }
    }

    /// Checks if a figure is selected at a given point.
    fn is_figure_selection(&self, pt: Point) -> Option<usize> {
        for (i, object) in self.draw_state.get_objects().iter().enumerate().rev() {
            if object.hit_test(pt) {
                return Some(i);
            }
        }
        None
    }

    /// Checks if a Bezier curve is currently being built.
    fn is_building_bezier(&self) -> bool {
        if let Some(shape) = self.cur_shape.as_ref() {
            return shape.get_type() == Shape::Bezier;
        }
        return false;
    }

    /// Checks if a control point of a figure is selected.
    fn is_control_point_select(&self, fig: usize, target: Point) -> Option<usize> {
        let object = self.draw_state.get_object(fig);
        for (i, p) in object.get_core().points.iter().enumerate() {
            let delta = target - *p;
            if delta.0 * delta.0 + delta.1 * delta.1 <= 100 {
                return Some(i);
            }
        }
        None
    }

    /// Starts the creation of a new shape.
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

    /// Adds a control point to the shape currently being created.
    fn shape_add_control_point(&mut self, nxt: Point) {
        if let Some(cur) = self.cur_shape.as_mut() {
            cur.update(&UpdateOp::AddControlPoint(nxt));
        }
    }

    /// Updates the last control point of the shape currently being created.
    fn shape_update_last_point(&mut self, nxt: Point) {
        if let Some(cur) = self.cur_shape.as_mut() {
            // next point will be given shift pressed and shape type or just the next one given by
            // the cursor
            let next_point = match (self.shift_pressed, cur.get_core().shape_type) {
                (true, Shape::Rectangle | Shape::Ellipse) => {
                    let core = cur.get_core();

                    // if shape created is one of these
                    // delta.x != delta.y happens, its not square
                    let delta = nxt - core.points[0];
                    // we do abs here to always pick the longest distance
                    let mx = max(delta.0.abs(), delta.1.abs());
                    let (sum_x, sum_y) = match (delta.0 > 0, delta.1 > 0) {
                        (true, true) => (mx, mx),
                        (true, false) => (mx, -mx),
                        (false, false) => (-mx, -mx),
                        (false, true) => (-mx, mx),
                    };

                    Point(core.points[0].0 + sum_x, core.points[0].1 + sum_y)
                }
                _ => nxt,
            };

            let last_point = cur.get_core().points.len() - 1;
            cur.update(&UpdateOp::ControlPoint(last_point, next_point));
        }
    }

    /// Finishes the creation of a shape.
    fn shape_end(&mut self, end: Point) {
        if let Some(mut cur) = self.cur_shape.take() {
            let last_point = cur.get_core().points.len() - 1;
            cur.update(&UpdateOp::ControlPoint(last_point, end));
            self.draw_state.add_shape(cur);
        }
    }

    /// Updates the position of a selected control point.
    fn update_selected_control_point(&mut self, point: Point) {
        if let Some(selected) = self.selected.as_ref() {
            if let Some(pnt_idx) = selected.control_point_selected {
                let op = UpdateOp::ControlPoint(pnt_idx, point);
                self.draw_state.update_shape(selected.index, op);
            }
        }
    }

    /// Saves the current drawing state to a file.
    fn save_state(&self) {
        if let Some(path) = FileDialog::new()
            .set_title("Save drawing")
            .set_file_name("drawing.json")
            .add_filter("JSON Files", &["json"])
            .save_file()
        {
            self.draw_state.save_to_file(path);
        }
    }

    /// Loads a drawing state from a file.
    fn load_state(&mut self) {
        if let Some(path) = FileDialog::new()
            .set_title("Open draw")
            .add_filter("JSON Files", &["json"])
            .pick_file()
        {
            self.draw_state.load_from_file(path);
        }
    }
}
