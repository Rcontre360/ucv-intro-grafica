/// draw state is used to update everything that should be taken in account during a file save or
/// an undo/redo action
use std::{collections::VecDeque, fs, mem::discriminant, path::PathBuf};

// serde is a package that allows us to serialize and deserialize rust objects into json format
use serde::{Deserialize, Serialize};

use crate::{
    core::{ShapeCore, ShapeImpl, UpdateOp, RGBA},
    primitives::new_shape_from_core,
};

/// We must have a limit for how many undo operations we can perform
/// for now 100 is a LOT, but each record doesnt consume much memory
const HISTORY_SIZE_LIMIT: usize = 100;

/// This is the enum that defines what are we going to save during an app update. Rust allows us to
/// store information on a given enum.
/// For each record we store the previous object state and the changed state, this consumes memory in
/// exchange for simplicity, just like the log of a database we have the following:
///
/// For 2 state updates we have the prev and next of each change. The i is an index that shows
/// where the app is currently at. If we move "i" backwards we use the "prev" value to update the
/// object. If we move "i" forward we use the nxt value to update the object:
///
///                i
///                |
///                |
///                v
/// [prev,nxt],[prev,nxt],[prev,nxt]
///     i
///     |
///     |
///     v
/// [prev,nxt],["PREV",nxt],[prev,nxt]
///                             i
///                             |
///                             |
///                             v
/// [prev,nxt],[prev,"NXT"],[prev,nxt]
#[derive(Clone)]
enum RecordType {
    /// the shape was moved to back or to front (z index). We store the original and previous index
    IndexChange(usize, usize),
    /// the shape changed its location or any set of control points. we save the previous and post
    /// state
    ShapeChange(usize, UpdateOp, ShapeCore, ShapeCore),
    /// the shape subdivided. we store the original and their results
    Subdivision(usize, ShapeCore, (ShapeCore, ShapeCore)),
    /// the given shape was removed. We store its previous state
    Deletion(usize, ShapeCore),
    /// we create a shape, we store the created shape
    Creation(ShapeCore),
    /// background color change. We store the previous and next color
    BackgroundColor(RGBA, RGBA),
    /// CLEAR, we store all shapes since a CLEAR action removes everything
    Clear(Vec<ShapeCore>),
}

/// Serialized state. We use it with serde to store the shapes in json format
/// It derives (its similar to inheritance) Serialize and Deserialize, these two traits allow this
/// object to be serialized into json. The fields of this object must also derive Serialize and
/// Deserialize
#[derive(Serialize, Deserialize)]
struct SerializedState {
    pub objects: Vec<ShapeCore>,
    pub background_color: RGBA,
}

/// DrawState is the object used to update the shapes
pub struct DrawState {
    /// objects are the shapes stored
    objects: Vec<Box<dyn ShapeImpl>>,
    /// background color used
    background_color: RGBA,
    // this data structure removes elements from the start of the queue in O(1)
    /// The history of actions performed and stored to enable the undo and redo
    /// we use a vecDeque since its cheaper to remove elements from the start, done when we reach
    /// the maximum amount allowed of events stored
    history: VecDeque<RecordType>,
    /// the index that shows WHERE are we when we perform an undo or redo
    history_idx: usize,
}

// the draw state implementation
impl DrawState {
    /// new creates a new draw state.
    pub fn new() -> Self {
        Self {
            objects: vec![],
            history: VecDeque::new(),
            history_idx: 0,
            background_color: RGBA::default(),
        }
    }

    /// returns background color
    pub fn get_background_color(&self) -> RGBA {
        return self.background_color;
    }

    /// returns a given object reference
    pub fn get_object(&self, idx: usize) -> &Box<dyn ShapeImpl> {
        &self.objects[idx]
    }

    /// returns a reference to the array of objects
    pub fn get_objects(&self) -> &Vec<Box<dyn ShapeImpl>> {
        &self.objects
    }

    /// pushes a new action into the history. We truncate the actions above the history_idx since
    /// after we performa an update in "the past" its impossible to rebuild the future. So if a
    /// user performs "undo" and makes a change, we truncate what we saved as subsequent steps.
    /// This function also records only the start and end of a given action to avoid overloading.
    /// For example, if a user moves a shape it looks dynamic because we are performing many shape
    /// movement events, if we store that here when the user does "undo" the shape will only undo
    /// pixel by pixel. So we only store the START and END of each type of action, if a user moves
    /// a shape we only store the initial and final location, if a user changes the shape color we
    /// only store the initial and last color
    fn push_history(&mut self, record: &RecordType) {
        self.history.truncate(self.history_idx);

        // There's an issue since we update objects on real time.
        // so there are MANY instances of updates when we simply move an object or change its color
        // the solution is only storing one event with the initial and final state
        let last_update = self.history.back();

        // this match structure runs when the last update and record match the values below
        match (last_update, record) {
            // to run this the last_udpate must be Some(RecordType::ShapeChange) and the record
            // almost the same
            (
                Some(RecordType::ShapeChange(idx_last, change_last, prev, _)),
                RecordType::ShapeChange(idx_this, change_cur, _, post),
            ) => {
                // change_last is of type UpdateOp (see src/core/mod.rs)
                // The comparison operation of UpdateOp takes in account its arguments, thats why
                // we compare using discriminant, this uses the enum type but not the arguments
                if discriminant(change_last) == discriminant(change_cur) && idx_last == idx_this {
                    // only the last is updated with the result record
                    let i = self.history.len() - 1;
                    self.history[i] = RecordType::ShapeChange(
                        *idx_last,
                        change_cur.clone(),
                        prev.clone(),
                        post.clone(),
                    );
                    return;
                }
            }
            // same update with background color. but we dont have differentiation between UpdateOp
            (Some(RecordType::BackgroundColor(orig, _)), RecordType::BackgroundColor(_, post)) => {
                let i = self.history.len() - 1;
                self.history[i] = RecordType::BackgroundColor(*orig, *post);
                return;
            }
            _ => {}
        }

        //pushing this action to the history and updating the index to the last
        self.history.push_back(record.clone());
        self.history_idx = self.history.len();

        // we must limit the size of this history to not break the computer memory.
        // It shoudlnt happen since the program doesnt use much, but still good practice
        if self.history.len() > HISTORY_SIZE_LIMIT {
            //remove the element and change the history index
            if self.history.pop_front().is_some() {
                self.history_idx -= 1;
            }
        }
    }

    /// undo operations moves the index to the previous operation performed backwards. Updates the objects
    /// given the information of that event
    pub fn undo(&mut self) {
        if self.history_idx > 0 {
            self.history_idx -= 1;
            // we get the record on that position of the array if its available
            let record_opt = self.history.get(self.history_idx);

            // we clone the record to use it since we will clone its content anyway
            if let Some(record) = record_opt.cloned() {
                match record {
                    // for index change "undo" we remove the place where the shape is now
                    // then we put that shape where it was before
                    RecordType::IndexChange(src, dst) => {
                        let shape = self.objects.remove(dst);
                        self.objects.insert(src, shape);
                    }
                    // for shape change "undo" we just take its previous form update the object
                    // that exists on that index
                    RecordType::ShapeChange(idx, _, prev, _) => {
                        self.objects[idx] = new_shape_from_core(prev);
                    }
                    // for subdivision we remove one of the objects generated and change the other
                    // for the original shape
                    RecordType::Subdivision(idx, init, _) => {
                        self.objects[idx] = new_shape_from_core(init);
                        self.objects.pop();
                    }
                    // for deletion we push the shape we deleted in its previous location
                    RecordType::Deletion(idx, prev) => {
                        self.objects.insert(idx, new_shape_from_core(prev));
                    }
                    // for clear we recreate the full set of objects
                    RecordType::Clear(history) => {
                        self.objects = history
                            .iter()
                            .map(|core| new_shape_from_core(core.clone()))
                            .collect();
                    }
                    // for creation of a shape we remove the last shape. Its guaranteed its the
                    // last one since creation allways appends to the end, and the other operations
                    // always return to the inmediate previous state
                    RecordType::Creation(_) => {
                        self.objects.pop();
                    }
                    // for background color we just go back to the previous color
                    RecordType::BackgroundColor(prev, _) => {
                        self.background_color = prev;
                    }
                }
            }
        }
    }

    /// redo operation moves the index to the previous operation performed forward. Updates the objects
    /// given the information of that event
    pub fn redo(&mut self) {
        if self.history_idx < self.history.len() {
            let record = self.history[self.history_idx].clone();
            match record {
                // for index change we move the shape to its previous index
                RecordType::IndexChange(src, dst) => {
                    let shape = self.objects.remove(src);
                    self.objects.insert(dst, shape);
                }
                // for shape change we change the idx shape to its post state
                RecordType::ShapeChange(idx, _, _, post) => {
                    self.objects[idx] = new_shape_from_core(post);
                }
                // for subdivision we recreate the two shapes generated. Update the current one and
                // push the other two. Its like a modifycation and creation together
                RecordType::Subdivision(idx, _, (core1, core2)) => {
                    self.objects[idx] = new_shape_from_core(core1);
                    self.objects.push(new_shape_from_core(core2));
                }
                // we just repeat what we did before. Clear all the objects
                RecordType::Clear(_) => {
                    self.objects.clear();
                }
                // deletion redo is repeating the previous action
                RecordType::Deletion(idx, _) => {
                    self.objects.remove(idx);
                }
                //creation is just repeating the action
                RecordType::Creation(core) => {
                    self.objects.push(new_shape_from_core(core));
                }
                // background color change just changes the background color
                RecordType::BackgroundColor(_, nxt) => {
                    self.background_color = nxt;
                }
            }
            self.history_idx += 1;
        }
    }

    /// clear function. Clears the objects and pushes to history
    pub fn clear(&mut self) {
        self.push_history(&RecordType::Clear(
            self.objects.iter().map(|obj| obj.get_core()).collect(),
        ));
        self.objects.clear();
    }

    /// Changes the background and pushes the event to history
    pub fn change_background_color(&mut self, color: RGBA) {
        self.push_history(&RecordType::BackgroundColor(self.background_color, color));
        self.background_color = color;
    }

    /// Adds a shape and pushes the event to history
    pub fn add_shape(&mut self, shape: Box<dyn ShapeImpl>) {
        self.push_history(&RecordType::Creation(shape.get_core()));
        self.objects.push(shape);
    }

    /// Deletes a shape (if possible) and adds the event
    pub fn delete_shape(&mut self, index: usize) {
        if index < self.objects.len() {
            let core = self.objects[index].get_core();
            self.push_history(&RecordType::Deletion(index, core));
            self.objects.remove(index);
        }
    }

    /// moves a shape in the vector from index to index. Adds the event
    pub fn reorder_shape(&mut self, from: usize, to: usize) {
        if from < self.objects.len() && to < self.objects.len() {
            let shape = self.objects.remove(from);
            self.objects.insert(to, shape);
            self.push_history(&RecordType::IndexChange(from, to));
        }
    }

    /// this function modifies current shape AND creates another shape. Adds the event
    /// we treat the subdivide differently since most operations are focused on a single shape
    pub fn subdivide_shape(&mut self, shape_idx: usize) {
        if let Some(shape) = self.objects.get_mut(shape_idx) {
            let res = shape.subdivide();
            let prev_core = shape.get_core();

            // only those shapes that implement subdivide can reach this
            if let Some((core1, core2)) = res.as_ref() {
                shape.update(&UpdateOp::RewritePoints(core1.points.clone()));
                self.objects.push(new_shape_from_core(core2.clone()));

                self.push_history(&RecordType::Subdivision(
                    shape_idx,
                    prev_core,
                    (core1.clone(), core2.clone()),
                ));
            }
        }
    }

    /// this is the only function that mutates a shape, and this allows us to record shape changes
    pub fn update_shape(&mut self, shape_idx: usize, op: UpdateOp) {
        if let Some(shape) = self.objects.get_mut(shape_idx) {
            let prev_core = shape.get_core().clone();
            shape.update(&op);
            let new_core = shape.get_core().clone();

            self.push_history(&RecordType::ShapeChange(shape_idx, op, prev_core, new_core));
        }
    }

    /// loads the state from file and clears the modification history
    pub fn load_from_file(&mut self, file_path: PathBuf) {
        let state_str = fs::read_to_string(&file_path).unwrap();
        let loaded_state: SerializedState = serde_json::from_str(&state_str).unwrap();

        self.history.clear();
        self.objects.clear();
        self.history_idx = 0;

        for core in loaded_state.objects.iter() {
            let boxed_shape = new_shape_from_core(core.clone());
            self.objects.push(boxed_shape);
        }

        self.background_color = loaded_state.background_color;
    }

    /// saves the state to a file
    pub fn save_to_file(&self, file_path: PathBuf) {
        let mut core_arr = vec![];
        for shape in self.get_objects().iter() {
            core_arr.push(shape.get_core());
        }

        // we need to use SerializedState object to store the state
        let saved_state = SerializedState {
            objects: core_arr,
            background_color: self.background_color,
        };

        let state_str = serde_json::to_string_pretty(&saved_state).unwrap();
        fs::write(file_path, state_str).unwrap();
    }
}
