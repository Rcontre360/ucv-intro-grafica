use crate::{
    core::{Point, ShapeCore, ShapeImpl, UpdateOp},
    primitives::new_shape_from_core,
};

#[derive(Clone)]
enum RecordType {
    IndexChange(usize, usize),
    ShapeChange(usize, ShapeCore, ShapeCore),
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

    pub fn get_objects(&self) -> &Vec<Box<dyn ShapeImpl>> {
        &self.objects
    }

    pub fn get_objects_mut(&mut self) -> &mut Vec<Box<dyn ShapeImpl>> {
        &mut self.objects
    }

    fn push_history(&mut self, record: RecordType) {
        if self.history_idx != self.history.len() - 1 {
            self.history.truncate(self.history_idx);
        }

        self.history.push(record);
        self.history_idx = self.history.len() - 1;
    }

    pub fn undo(&mut self) {
        if self.history_idx > 0 {
            let record = self.history[self.history_idx].clone();

            match record {
                RecordType::IndexChange(src, dst) => {
                    let shape = self.objects.remove(dst);
                    self.objects.insert(src, shape);
                }
                RecordType::ShapeChange(idx, prev, _) => {
                    self.objects[idx] = new_shape_from_core(prev);
                }
                RecordType::Deletion(_, prev) => {
                    self.objects.push(new_shape_from_core(prev));
                }
                RecordType::Creation(_) => {
                    self.objects.pop();
                }
            }

            self.history_idx -= 1;
        }
    }

    pub fn redo(&mut self) {
        if self.history_idx < self.history.len() - 1 {
            let record = self.history[self.history_idx].clone();
            match record {
                RecordType::IndexChange(src, dst) => {
                    let shape = self.objects.remove(src);
                    self.objects.insert(dst, shape);
                }
                RecordType::ShapeChange(idx, _, post) => {
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
        self.push_history(RecordType::Creation(shape.get_core()));
        self.objects.push(shape);
    }

    pub fn delete_shape(&mut self, index: usize) {
        if index < self.objects.len() {
            let core = self.objects[index].get_core();
            self.push_history(RecordType::Deletion(index, core));
            self.objects.remove(index);
        }
    }

    pub fn reorder_shape(&mut self, from: usize, to: usize) {
        if from < self.objects.len() && to < self.objects.len() {
            let shape = self.objects.remove(from);
            self.objects.insert(to, shape);
            self.push_history(RecordType::IndexChange(from, to));
        }
    }

    pub fn update_shape_control_point(&mut self, shape_idx: usize, point_idx: usize, point: Point) {
        if let Some(shape) = self.objects.get_mut(shape_idx) {
            let prev_core = shape.get_core().clone();
            let op = UpdateOp::ControlPoint {
                index: point_idx,
                point,
            };
            shape.update(&op);
            let new_core = shape.get_core().clone();

            self.push_history(RecordType::ShapeChange(shape_idx, prev_core, new_core));
        }
    }

    pub fn move_shape(&mut self, shape_idx: usize, delta: Point) {
        if let Some(shape) = self.objects.get_mut(shape_idx) {
            let prev_core = shape.get_core().clone();
            let op = UpdateOp::Move(delta);
            shape.update(&op);
            let new_core = shape.get_core().clone();

            self.push_history(RecordType::ShapeChange(shape_idx, prev_core, new_core));
        }
    }
}
