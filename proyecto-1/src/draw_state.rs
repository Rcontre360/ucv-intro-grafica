use std::mem::discriminant;

use crate::{
    core::{ShapeCore, ShapeImpl, UpdateOp},
    primitives::new_shape_from_core,
};

#[derive(Clone)]
enum RecordType {
    IndexChange(usize, usize),
    ShapeChange(usize, UpdateOp, ShapeCore, ShapeCore),
    Deletion(usize, ShapeCore),
    Creation(ShapeCore),
}

pub struct DrawState {
    pub objects: Vec<Box<dyn ShapeImpl>>,
    history: Vec<RecordType>,
    history_idx: usize,
}

impl DrawState {
    pub fn new() -> Self {
        Self {
            objects: vec![],
            history: vec![],
            history_idx: 0,
        }
    }

    pub fn get_object(&self, idx: usize) -> &Box<dyn ShapeImpl> {
        &self.objects[idx]
    }

    pub fn get_objects(&self) -> &Vec<Box<dyn ShapeImpl>> {
        &self.objects
    }

    fn push_history(&mut self, record: &RecordType) {
        self.history.truncate(self.history_idx);

        // There's an issue since we update objects on real time.
        // so there are MANY instances of updates when we simply move an object or change its color
        // the solution is only storing the start and finish of this event
        let last_update = self.history.last();

        // this match structure runs when the last update and record match the values below
        match (last_update, record) {
            (
                Some(RecordType::ShapeChange(idx, change_last, prev, _)),
                RecordType::ShapeChange(_, change_cur, _, post),
            ) => {
                // with this we compare the enum type but not the arguments
                if discriminant(change_last) == discriminant(change_cur) {
                    // in case the last update and the record
                    let i = self.history.len() - 1;
                    self.history[i] = RecordType::ShapeChange(
                        *idx,
                        change_cur.clone(),
                        prev.clone(),
                        post.clone(),
                    );
                    return;
                }
            }
            _ => {}
        }

        self.history.push(record.clone());
        self.history_idx = self.history.len();
    }

    pub fn undo(&mut self) {
        if self.history_idx > 0 {
            self.history_idx -= 1;
            let record_opt = self.history.get(self.history_idx);

            if let Some(record) = record_opt.cloned() {
                match record {
                    RecordType::IndexChange(src, dst) => {
                        let shape = self.objects.remove(dst);
                        self.objects.insert(src, shape);
                    }
                    RecordType::ShapeChange(idx, _, prev, _) => {
                        self.objects[idx] = new_shape_from_core(prev);
                    }
                    RecordType::Deletion(_, prev) => {
                        self.objects.push(new_shape_from_core(prev));
                    }
                    RecordType::Creation(_) => {
                        self.objects.pop();
                    }
                }
            }
        }
    }

    pub fn redo(&mut self) {
        if self.history_idx < self.history.len() {
            let record = self.history[self.history_idx].clone();
            match record {
                RecordType::IndexChange(src, dst) => {
                    let shape = self.objects.remove(src);
                    self.objects.insert(dst, shape);
                }
                RecordType::ShapeChange(idx, _, _, post) => {
                    self.objects[idx] = new_shape_from_core(post);
                }
                RecordType::Deletion(idx, _) => {
                    self.objects.remove(idx);
                }
                RecordType::Creation(core) => {
                    self.objects.push(new_shape_from_core(core));
                }
            }
            self.history_idx += 1;
        }
    }

    pub fn add_shape(&mut self, shape: Box<dyn ShapeImpl>) {
        self.push_history(&RecordType::Creation(shape.get_core()));
        self.objects.push(shape);
    }

    pub fn delete_shape(&mut self, index: usize) {
        if index < self.objects.len() {
            let core = self.objects[index].get_core();
            self.push_history(&RecordType::Deletion(index, core));
            self.objects.remove(index);
        }
    }

    pub fn reorder_shape(&mut self, from: usize, to: usize) {
        if from < self.objects.len() && to < self.objects.len() {
            let shape = self.objects.remove(from);
            self.objects.insert(to, shape);
            self.push_history(&RecordType::IndexChange(from, to));
        }
    }

    // this is the only way to mutate a shape, and this allows us to record shape changes
    pub fn update_shape(&mut self, shape_idx: usize, op: UpdateOp) {
        if let Some(shape) = self.objects.get_mut(shape_idx) {
            let prev_core = shape.get_core().clone();
            shape.update(&op);
            let new_core = shape.get_core().clone();

            self.push_history(&RecordType::ShapeChange(shape_idx, op, prev_core, new_core));
        }
    }
}
